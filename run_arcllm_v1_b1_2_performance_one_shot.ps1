param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot,
  [switch]$QuietHostConfirmed
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path

if(-not $QuietHostConfirmed){throw "STOP: B1.2 performance requires explicit -QuietHostConfirmed"}

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}

$LockPath=Join-Path $Root "config\arcllm_v1_b1_2_performance_execution_lock_v0.4.json"
$GatePath=Join-Path $Root "config\arcllm_v1_b1_2_performance_execution_gate_v0.4.json"
if(-not(Test-Path $LockPath)){throw "STOP: immutable B1.2 performance lock missing"}
if(-not(Test-Path $GatePath)){throw "STOP: B1.2 performance execution gate missing; execution remains CLOSED"}

$Lock=Get-Content $LockPath -Raw|ConvertFrom-Json
$Gate=Get-Content $GatePath -Raw|ConvertFrom-Json
if([string]$Lock.status-ne"FROZEN_IMMUTABLE_B1_2_PERFORMANCE_EXECUTION_LOCK_V0_4"){throw "STOP: invalid B1.2 lock status"}
if([string]$Gate.status-ne"PASS_OPEN_B1_2_ONE_SHOT_PERFORMANCE_EXECUTION" -or -not[bool]$Gate.execution_authorized){
  throw "STOP: B1.2 performance execution not authorized"
}

$Critical=[ordered]@{
 "src/arcllm_v1_b1_2_performance.cpp"=[string]$Lock.implementation.performance_source_blob
 "src/arcllm_v1_b1_2_zero_science.cpp"=[string]$Lock.implementation.zero_science_source_blob
 "src/arcllm_v1_q4_down_4arm_timing_runtime.cpp"=[string]$Lock.implementation.transitive_runtime_blob
 "shaders/b1_2_exec148_gpu_materialize.comp"=[string]$Lock.implementation.p1_shader_source_blob
 "run_arcllm_v1_b1_2_performance_one_shot.ps1"=[string]$Lock.implementation.one_shot_runner_blob
 "config/arcllm_v1_b1_2_performance_execution_lock_v0.4.json"=[string]$Gate.lock.blob
}
foreach($P in $Critical.Keys){
  $Got=(& git -C $Root rev-parse ("HEAD:"+$P)).Trim()
  if($LASTEXITCODE-ne0-or$Got-ne[string]$Critical[$P]){throw "STOP: critical locked blob mismatch: $P"}
}

$Exe=Join-Path $Root "arcllm_v1_b1_2_performance.exe"
if(-not(Test-Path $Exe)){throw "STOP: performance executable missing; run build-only qualification first"}
$ExeHash=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
if($ExeHash-ne[string]$Gate.executable.sha256){throw "STOP: performance executable SHA256 mismatch"}
if((Get-Item $Exe).Length-ne[int64]$Gate.executable.bytes){throw "STOP: performance executable byte count mismatch"}

$P1Spv=Join-Path $Root "compiled_shaders\b1_2_exec148_gpu_materialize.comp.spv"
$Q4Spv=Join-Path $Root "compiled_shaders\p7_q4k_gemm_2d.spv"
$BSpv=Join-Path $Root "compiled_shaders\q4_down_exec148_serial.spv"
foreach($Pair in @(
  @($P1Spv,[string]$Lock.implementation.p1_spv_sha256),
  @($Q4Spv,[string]$Lock.implementation.q4_reference_spv_sha256),
  @($BSpv,[string]$Lock.implementation.b_serial_spv_sha256)
)){
  if(-not(Test-Path $Pair[0])){throw "STOP: locked SPIR-V missing: $($Pair[0])"}
  if((Get-FileHash $Pair[0] -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$Pair[1]){throw "STOP: locked SPIR-V hash mismatch: $($Pair[0])"}
}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count-lt1){throw "STOP: model resolver failed"}
  $ModelPath=[string]$Resolved[-1]
}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$Lock.inputs.model_sha256){throw "STOP: model SHA256 mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$Lock.inputs.model_bytes){throw "STOP: model byte count mismatch"}

$Sidecar=Join-Path $Root ".local\b1_2_zero_science\Q4K_SERIAL_K_EXEC148_V0_1_60e05f2100071479.bin"
$SideManifest=$Sidecar+".manifest.json"
if(-not(Test-Path $Sidecar)-or-not(Test-Path $SideManifest)){throw "STOP: qualified sidecar/manifest missing"}
if((Get-Item $Sidecar).Length-ne[int64]$Lock.inputs.sidecar_bytes){throw "STOP: sidecar bytes mismatch"}
$SideHash=(Get-FileHash $Sidecar -Algorithm SHA256).Hash.ToLowerInvariant()
if($SideHash-ne[string]$Lock.inputs.sidecar_raw_sha256){throw "STOP: sidecar raw SHA256 mismatch"}
$Manifest=Get-Content $SideManifest -Raw|ConvertFrom-Json
if([string]$Manifest.payload_raw_sha256-ne[string]$Lock.inputs.sidecar_raw_sha256){throw "STOP: sidecar manifest raw SHA mismatch"}
if([string]$Manifest.canonical_family_hash-ne[string]$Lock.inputs.canonical_family_sha256){throw "STOP: sidecar canonical family hash mismatch"}

$Marker=Join-Path $Root ".local\B1_2_PERFORMANCE_SCIENCE_STARTED.json"
if(Test-Path $Marker){throw "STOP: B1.2 one-shot science marker already exists; no rerun allowed without governance transition"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\b1_2_performance_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Result=Join-Path $Dir "B1_2_PERFORMANCE_RESULT.json"

$MarkerObj=[ordered]@{
 schema="arcllm.v1.b1_2.performance.science_start.v0.4"
 status="SCIENCE_ATTEMPT_CONSUMED"
 timestamp_utc=$Stamp
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 lock_blob=[string]$Gate.lock.blob
 gate_blob=((& git -C $Root rev-parse "HEAD:config/arcllm_v1_b1_2_performance_execution_gate_v0.4.json").Trim())
 executable_sha256=$ExeHash
 model_sha256=[string]$Lock.inputs.model_sha256
 sidecar_raw_sha256=$SideHash
 quiet_host_confirmed=$true
 note="Any failure after creation of this marker consumes the one-shot attempt. Do not selectively rerun a candidate/state."
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Marker)|Out-Null
[IO.File]::WriteAllText($Marker,($MarkerObj|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

& $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --sidecar $Sidecar --out $Result
if($LASTEXITCODE-ne0){throw "STOP: B1.2 one-shot performance execution failed after science marker; do not rerun"}
if(-not(Test-Path $Result)){throw "STOP: B1.2 result missing after consumed science attempt"}
$R=Get-Content $Result -Raw|ConvertFrom-Json
if([string]$R.status-ne"PASS_COMPLETE_24_ATTEMPT_COLLECTION"){throw "STOP: B1.2 result incomplete after consumed science attempt"}
if($R.P1.samples_ms.Count-ne8-or$R.P3_WARM.samples_ms.Count-ne8-or$R.P3_COLD.samples_ms.Count-ne8){throw "STOP: B1.2 exact 8-attempt contract violated"}

Copy-Item $Marker (Join-Path $Dir "B1_2_PERFORMANCE_SCIENCE_STARTED.json") -Force
Copy-Item $LockPath $Dir -Force
Copy-Item $GatePath $Dir -Force
Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_INDEPENDENT_IMPLEMENTATION_PACKAGE_REVIEW_v0.1.json") $Dir -Force
Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_P1_P3_ZERO_SCIENCE_QUALIFICATION_CANONICAL_v0.1.json") $Dir -Force

$Return=[ordered]@{
 schema="arcllm.v1.b1_2.performance.dev_host_return.v0.4"
 status="PASS_COMPLETE_24_ATTEMPT_COLLECTION"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 executable_sha256=$ExeHash
 result_sha256=(Get-FileHash $Result -Algorithm SHA256).Hash.ToUpperInvariant()
 science_marker_sha256=(Get-FileHash $Marker -Algorithm SHA256).Hash.ToUpperInvariant()
 performance=[ordered]@{
  P1_median_ms=[double]$R.P1.summary_ms.median
  P3_WARM_median_ms=[double]$R.P3_WARM.summary_ms.median
  P3_COLD_median_ms=[double]$R.P3_COLD.summary_ms.median
 }
}
$ReturnPath=Join-Path $Dir "B1_2_PERFORMANCE_DEV_HOST_RETURN.json"
[IO.File]::WriteAllText($ReturnPath,($Return|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Dir "B1_2_PERFORMANCE_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "B1_2_PERFORMANCE_COLLECTION=PASS_COMPLETE_24_ATTEMPT_COLLECTION"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
