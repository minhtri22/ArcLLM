param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}

$LockPath=Join-Path $Root "config\arcllm_v1_b1_2_performance_execution_lock_v0.1.json"
$FreezePath=Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_MEASUREMENT_WRAPPER_FREEZE_v0.1.json"
$ReviewPath=Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_INDEPENDENT_IMPLEMENTATION_PACKAGE_REVIEW_v0.1.json"
$ZeroPath=Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_P1_P3_ZERO_SCIENCE_QUALIFICATION_CANONICAL_v0.1.json"
foreach($P in @($LockPath,$FreezePath,$ReviewPath,$ZeroPath)){
 if(-not(Test-Path $P)){throw "STOP: required frozen artifact missing: $P"}
}

$Lock=Get-Content $LockPath -Raw|ConvertFrom-Json
if([string]$Lock.status-ne"FROZEN_IMMUTABLE_B1_2_PERFORMANCE_EXECUTION_LOCK"){throw "STOP: invalid lock status"}
if(-not[bool]$Lock.execution_authorization.buildonly_authorized){throw "STOP: build-only is not authorized"}
if([bool]$Lock.execution_authorization.performance_execution){throw "STOP: performance execution unexpectedly open"}
if([bool]$Lock.execution_authorization.execution_gate_present){throw "STOP: execution gate must not be present during build-only qualification"}

$GatePath=Join-Path $Root "config\arcllm_v1_b1_2_performance_execution_gate_v0.1.json"
if(Test-Path $GatePath){throw "STOP: performance execution gate already exists; build-only binding stage expected gate absence"}

$Critical=[ordered]@{
 "src/arcllm_v1_b1_2_performance.cpp"=[string]$Lock.implementation.performance_source_blob
 "src/arcllm_v1_b1_2_zero_science.cpp"=[string]$Lock.implementation.zero_science_source_blob
 "src/arcllm_v1_q4_down_4arm_timing_runtime.cpp"=[string]$Lock.implementation.transitive_runtime_blob
 "shaders/b1_2_exec148_gpu_materialize.comp"=[string]$Lock.implementation.p1_shader_source_blob
 "tests/test_arcllm_v1_b1_2_performance_lock.py"=[string]$Lock.implementation.static_test_blob
 "tools/build_arcllm_v1_b1_2_performance_buildonly.ps1"=[string]$Lock.implementation.buildonly_script_blob
 "run_arcllm_v1_b1_2_performance_one_shot.ps1"=[string]$Lock.implementation.one_shot_runner_blob
}
foreach($P in $Critical.Keys){
 $Got=(& git -C $Root rev-parse ("HEAD:"+$P)).Trim()
 if($LASTEXITCODE-ne0-or$Got-ne[string]$Critical[$P]){throw "STOP: locked blob mismatch: $P"}
}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_b1_2_performance_buildonly.ps1")
if($LASTEXITCODE-ne0){throw "STOP: B1.2 performance wrapper build-only qualification failed"}

$EvidencePath=Join-Path $Root "results\B1_2_PERFORMANCE_BUILDONLY.json"
if(-not(Test-Path $EvidencePath)){throw "STOP: build-only evidence missing"}
$E=Get-Content $EvidencePath -Raw|ConvertFrom-Json
if([string]$E.status-ne"PASS_PERFORMANCE_WRAPPER_BUILDONLY"){throw "STOP: invalid build-only status"}
if([bool]$E.execution_authorized -or [bool]$E.model_loaded -or [bool]$E.gpu_dispatch -or [bool]$E.sidecar_io -or [bool]$E.performance_execution){
 throw "STOP: build-only scope violation"
}

if([string]$E.blobs.performance_source-ne[string]$Lock.implementation.performance_source_blob){throw "STOP: performance source evidence mismatch"}
if([string]$E.blobs.zero_science_source-ne[string]$Lock.implementation.zero_science_source_blob){throw "STOP: zero-science source evidence mismatch"}
if([string]$E.blobs.transitive_runtime-ne[string]$Lock.implementation.transitive_runtime_blob){throw "STOP: transitive runtime evidence mismatch"}
if([string]$E.blobs.p1_shader_source-ne[string]$Lock.implementation.p1_shader_source_blob){throw "STOP: P1 shader source evidence mismatch"}
if([string]$E.compiled.p1_spv_sha256-ne[string]$Lock.implementation.p1_spv_sha256){throw "STOP: P1 SPV hash mismatch"}
if([int64]$E.compiled.p1_spv_bytes-ne[int64]$Lock.implementation.p1_spv_bytes){throw "STOP: P1 SPV bytes mismatch"}
if([string]$E.compiled.q4_reference_spv_sha256-ne[string]$Lock.implementation.q4_reference_spv_sha256){throw "STOP: Q4 reference SPV mismatch"}
if([string]$E.compiled.b_serial_spv_sha256-ne[string]$Lock.implementation.b_serial_spv_sha256){throw "STOP: B serial SPV mismatch"}

$Exe=Join-Path $Root "arcllm_v1_b1_2_performance.exe"
if(-not(Test-Path $Exe)){throw "STOP: performance executable missing"}
$ExeSha=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
$ExeBytes=(Get-Item $Exe).Length
if($ExeSha-ne[string]$E.compiled.exe_sha256-or$ExeBytes-ne[int64]$E.compiled.exe_bytes){
 throw "STOP: EXE does not match build-only evidence"
}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\b1_2_performance_buildonly_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

$Return=[ordered]@{
 schema="arcllm.v1.b1_2.performance.buildonly.dev_host_return.v0.1"
 status="PASS_DEV_HOST_PERFORMANCE_WRAPPER_BUILDONLY"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 scope=[ordered]@{
  model_loaded=$false
  gpu_dispatch=$false
  sidecar_io=$false
  performance_execution=$false
  science_marker_created=$false
 }
 lock=[ordered]@{
  path="config/arcllm_v1_b1_2_performance_execution_lock_v0.1.json"
  blob=((& git -C $Root rev-parse "HEAD:config/arcllm_v1_b1_2_performance_execution_lock_v0.1.json").Trim())
 }
 executable=[ordered]@{
  filename="arcllm_v1_b1_2_performance.exe"
  sha256=$ExeSha
  bytes=$ExeBytes
 }
 compiled=[ordered]@{
  p1_spv_sha256=[string]$E.compiled.p1_spv_sha256
  p1_spv_bytes=[int64]$E.compiled.p1_spv_bytes
  q4_reference_spv_sha256=[string]$E.compiled.q4_reference_spv_sha256
  b_serial_spv_sha256=[string]$E.compiled.b_serial_spv_sha256
 }
 next="RETURN_BUNDLE_FOR_EXACT_EXE_BINDING_AND_INDEPENDENT_B1_2_PERFORMANCE_LOCK_REVIEW"
}
$ReturnPath=Join-Path $Dir "B1_2_PERFORMANCE_BUILDONLY_DEV_HOST_RETURN.json"
[IO.File]::WriteAllText($ReturnPath,($Return|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $EvidencePath (Join-Path $Dir "B1_2_PERFORMANCE_BUILDONLY.json") -Force
Copy-Item $Exe (Join-Path $Dir "arcllm_v1_b1_2_performance.exe") -Force
Copy-Item $LockPath $Dir -Force
Copy-Item $FreezePath $Dir -Force
Copy-Item $ReviewPath $Dir -Force
Copy-Item $ZeroPath $Dir -Force
Copy-Item (Join-Path $Root "results\B1_2_ZERO_SCIENCE_SHADER_PROVENANCE.json") $Dir -Force

$Zip=Join-Path $Dir "B1_2_PERFORMANCE_BUILDONLY_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force

Write-Host "B1_2_PERFORMANCE_WRAPPER_BUILDONLY_QUALIFICATION=PASS"
Write-Host "EXE_SHA256=$ExeSha"
Write-Host "EXE_BYTES=$ExeBytes"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO SIDECAR IO. NO PERFORMANCE EXECUTION. NO SCIENCE MARKER."
