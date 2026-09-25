param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_splitk_causal_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "STOP: Q4-down causal lock missing"}
$L=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
if(-not[bool]$L.authorization.buildonly){throw "STOP: build-only not authorized"}

git -C $Root fetch origin | Out-Null
$Head=(git -C $Root rev-parse HEAD).Trim()
$Remote=(git -C $Root rev-parse ("origin/"+[string]$L.branch)).Trim()
if($Head-ne$Remote){throw "STOP: causal worktree HEAD does not match remote"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){
  $Got=(git -C $Root hash-object -- $P.Name).Trim()
  if($Got-ne[string]$P.Value){throw "STOP: critical blob mismatch: $($P.Name)"}
}

py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_splitk_causal.py")
if($LASTEXITCODE-ne0){throw "STOP: Q4-down causal static/synthetic QA failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE-ne0){throw "STOP: shader compile failed"}
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
$SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
if($SpvHash-ne([string]$L.candidate_spv_sha256).ToUpperInvariant()){throw "STOP: split-K SPIR-V mismatch"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_q4_down_splitk_causal.ps1")
if($LASTEXITCODE-ne0){throw "STOP: causal native build failed"}
$Exe=Join-Path $Root "arcllm_v1_q4_down_splitk_causal.exe"
if(-not(Test-Path $Exe)){throw "STOP: causal executable missing"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\q4_down_splitk_causal_buildonly_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Evidence=[ordered]@{
  schema="arcllm.v1.q4_down_splitk_causal.buildonly.v0.1"
  status="PASS_BUILDONLY"
  head=$Head
  source_blob=[string]$L.critical_git_blobs."src/arcllm_v1_q4_down_splitk_causal.cpp"
  executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  splitk_spv_sha256=$SpvHash
  target_model_loaded=$false
  gpu_dispatch_executed=$false
  wall_timing_executed=$false
  timestamp_queries_executed=$false
  hardware_counters_executed=$false
  correctness_authorized=[bool]$L.authorization.correctness
  performance_authorized=[bool]$L.authorization.performance
  next="RETURN_BUILDONLY_BUNDLE_FOR_INDEPENDENT_REVIEW"
}
$EvidencePath=Join-Path $Dir "Q4_DOWN_SPLITK_CAUSAL_BUILDONLY.json"
[IO.File]::WriteAllText($EvidencePath,($Evidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Copy-Item $LockPath $Dir -Force
Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\Q4_DOWN_SPLITK_CAUSAL_PRELOCK_v0.1.json") $Dir -Force
$Zip=Join-Path $Dir "Q4_DOWN_SPLITK_CAUSAL_BUILDONLY_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_SPLITK_CAUSAL_BUILDONLY=PASS"
Write-Host "MODEL_LOADED=false"
Write-Host "GPU_DISPATCH=false"
Write-Host "PERFORMANCE_AUTHORIZED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
