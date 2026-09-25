param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_splitk_causal_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "STOP: Q4-down causal lock missing"}
$L=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
if(-not[bool]$L.authorization.correctness){throw "STOP: real-model correctness not authorized"}
if([bool]$L.authorization.performance){throw "STOP: correctness stage must precede performance authorization"}

git -C $Root fetch origin | Out-Null
$Head=(git -C $Root rev-parse HEAD).Trim()
$Remote=(git -C $Root rev-parse ("origin/"+[string]$L.branch)).Trim()
if($Head-ne$Remote){throw "STOP: causal worktree HEAD does not match remote"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){
  $Got=(git -C $Root hash-object -- $P.Name).Trim()
  if($Got-ne[string]$P.Value){throw "STOP: critical blob mismatch: $($P.Name)"}
}

py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_splitk_causal.py")
if($LASTEXITCODE-ne0){throw "STOP: static/synthetic QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE-ne0){throw "STOP: shader compile failed"}
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
if((Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()-ne([string]$L.candidate_spv_sha256).ToUpperInvariant()){throw "STOP: split-K SPIR-V mismatch"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_q4_down_splitk_causal.ps1")
if($LASTEXITCODE-ne0){throw "STOP: causal native build failed"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count-lt1){throw "STOP: model resolver failed"}
  $ModelPath=[string]$Resolved[-1]
}
if(-not(Test-Path $ModelPath)){throw "STOP: target model missing"}
$MF=Get-Item $ModelPath
$MH=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($MF.Length-ne[int64]$L.target_model.bytes-or$MH-ne([string]$L.target_model.sha256).ToUpperInvariant()){throw "STOP: exact model mismatch"}

$Exe=Join-Path $Root "arcllm_v1_q4_down_splitk_causal.exe"
$ShaderDir=Join-Path $Root "compiled_shaders"
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\q4_down_splitk_causal_correctness_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

$Rows=@()
foreach($W in @("W-S","W-C")){
  $Out=Join-Path $Dir ("correctness_"+($W-replace'-','_')+".json")
  & $Exe --model $ModelPath --shader-dir $ShaderDir --implementation-commit $Head --workload $W --mode correctness --out $Out
  if($LASTEXITCODE-ne0){throw "STOP: correctness execution failed: $W"}
  $R=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
  if([string]$R.status-ne"PASS_CORRECTNESS"){throw "STOP: correctness status failed: $W"}
  if(-not[bool]$R.component_correctness.pass-or[double]$R.component_correctness.max_abs-gt0.02-or[double]$R.component_correctness.rmse-gt0.005){throw "STOP: component oracle failed: $W"}
  if([bool]$R.performance.authorized-or[bool]$R.performance.wall_timing_executed-or[bool]$R.performance.timestamp_queries_executed-or[bool]$R.performance.hardware_counters_executed){throw "STOP: performance leakage in correctness stage"}
  if(@($R.arms).Count-ne2-or@($R.arms|Where-Object{-not[bool]$_.success}).Count-ne0){throw "STOP: 0/A semantic correctness failed"}
  $Expected=[string]$L.correctness.generated_hashes.($W)
  if(@($R.arms|Where-Object{[string]$_.generated_hash_fnv1a64-ne$Expected}).Count-ne0){throw "STOP: generated hash mismatch: $W"}
  $Rows+=[ordered]@{workload=$W;result=(Split-Path -Leaf $Out);sha256=(Get-FileHash $Out -Algorithm SHA256).Hash.ToUpperInvariant()}
}
$Final=[ordered]@{
  schema="arcllm.v1.q4_down_splitk_causal.correctness_qualification.v0.1"
  status="PASS_CORRECTNESS_QUALIFICATION"
  implementation_commit=$Head
  model_sha256=$MH
  workloads=$Rows
  performance_authorized=$false
  timing_executed=$false
  hardware_counters_executed=$false
  next="RETURN_CORRECTNESS_BUNDLE_FOR_INDEPENDENT_REVIEW_BEFORE_PERFORMANCE"
}
$FinalPath=Join-Path $Dir "Q4_DOWN_SPLITK_CAUSAL_CORRECTNESS_QUALIFICATION.json"
[IO.File]::WriteAllText($FinalPath,($Final|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Copy-Item $LockPath $Dir -Force
$Zip=Join-Path $Dir "Q4_DOWN_SPLITK_CAUSAL_CORRECTNESS_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_SPLITK_CAUSAL_CORRECTNESS=PASS"
Write-Host "PERFORMANCE_AUTHORIZED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
