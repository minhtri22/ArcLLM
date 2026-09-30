param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_correctness_lock_v0.1.json"
$L=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json

if([string]$L.schema-ne"arcllm.v1.q4_down.4arm.correctness_lock.v0.1"){throw "STOP: correctness lock schema mismatch"}
if([bool]$L.performance_authorized-or[bool]$L.hardware_counters_authorized-or[bool]$L.timing_authorized){throw "STOP: correctness lock illegally authorizes performance"}

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}

foreach($P in $L.critical_git_blobs.PSObject.Properties){
 $Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim()
 if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: critical blob mismatch: $($P.Name)"}
}

py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_4arm_package.py")
if($LASTEXITCODE-ne0){throw "STOP: static QA failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "STOP: shader build failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "STOP: native/synthetic build failed"}

if(-not $ModelPath){
 $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
 if($Resolved.Count-lt1){throw "STOP: model resolver failed"}
 $ModelPath=[string]$Resolved[-1]
}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$L.target_model.sha256){throw "STOP: exact model SHA256 mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$L.target_model.bytes){throw "STOP: exact model byte size mismatch"}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor)
$GPU=@(Get-CimInstance Win32_VideoController)
if([string]$OS.BuildNumber-ne[string]$L.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object{$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object{$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$L.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}

# Qualify the frozen Child-A 32-lane subgroup requirement on the exact DEV_HOST.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "runs/sa/run_sa1_component_preflight.ps1") -Quant Q4
if($LASTEXITCODE-ne0){throw "STOP: exact subgroup32 capability/correctness preflight failed"}

$Head=(& git -C $Root rev-parse HEAD).Trim()
$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm.exe"
$ShaderDir=Join-Path $Root "compiled_shaders"
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\q4_down_4arm_correctness_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

$Rows=@()
foreach($W in @("W-S","W-C")){
 $Out=Join-Path $Dir ("correctness_"+($W-replace'-','_')+".json")
 & $Exe --model $ModelPath --shader-dir $ShaderDir --implementation-commit $Head --workload $W --out $Out
 if($LASTEXITCODE-ne0){throw "STOP: real-model correctness harness failed for $W"}

 $R=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
 if([string]$R.status-ne"PASS_Q4_DOWN_4ARM_CORRECTNESS_QUALIFICATION"){throw "STOP: correctness status failed for $W"}
 if([bool]$R.performance.authorized-or[bool]$R.performance.latency_fields_emitted-or[bool]$R.performance.hardware_counters_executed){throw "STOP: performance leakage in correctness result"}
 if($null-ne$R.performance.materialization_time_ms-or$null-ne$R.performance.validation_time_ms){throw "STOP: correctness result contains timing"}
 if([string]$R.exec148.family_source_sha256-ne[string]$R.exec148.family_exec_sha256){throw "STOP: canonical family hash mismatch"}
 if(@($R.arms).Count-ne4-or@($R.arms|Where-Object{-not[bool]$_.success}).Count-ne0){throw "STOP: not all four arms passed"}

 $Expected=[string]$L.correctness.workloads.($W).generated_hash_fnv1a64
 if(@($R.arms|Where-Object{[string]$_.generated_hash_fnv1a64-ne$Expected}).Count-ne0){throw "STOP: frozen generated hash mismatch for $W"}

 $Rows+=[ordered]@{
  workload=$W
  result=(Split-Path -Leaf $Out)
  result_sha256=(Get-FileHash $Out -Algorithm SHA256).Hash.ToUpperInvariant()
  generated_hashes=@($R.arms|ForEach-Object{[string]$_.generated_hash_fnv1a64})
  family_canonical_sha256=[string]$R.exec148.family_source_sha256
 }
}

$Final=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.correctness_qualification.v0.1"
 status="PASS_Q4_DOWN_4ARM_CORRECTNESS_QUALIFICATION"
 implementation_commit=$Head
 model_sha256=[string]$L.target_model.sha256
 performance_authorized=$false
 hardware_counters_authorized=$false
 timing_executed=$false
 materialization_time_ms=$null
 validation_time_ms=$null
 workloads=$Rows
 next="FREEZE_CORRECTNESS_PACKAGE_THEN_SEPARATELY_AUTHORIZE_PERFORMANCE"
}
$FinalPath=Join-Path $Dir "Q4_DOWN_4ARM_CORRECTNESS_QUALIFICATION.json"
[IO.File]::WriteAllText($FinalPath,($Final|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $LockPath (Join-Path $Dir (Split-Path -Leaf $LockPath)) -Force
Copy-Item (Join-Path $Root "results\ARCLLM_V1_Q4_DOWN_4ARM_BUILDONLY_QA.json") $Dir -Force
Copy-Item (Join-Path $Root "results\q4_down_4arm_shader_provenance.json") $Dir -Force

$Zip=Join-Path $Dir "Q4_DOWN_4ARM_CORRECTNESS_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force

Write-Host "Q4_DOWN_4ARM_CORRECTNESS=PASS"
Write-Host "FINAL=$FinalPath"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "PERFORMANCE_REMAINS_FORBIDDEN"
