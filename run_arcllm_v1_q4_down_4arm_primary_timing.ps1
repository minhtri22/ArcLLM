param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_primary_timing_execution_lock_v0.2.json"
if(-not(Test-Path $LockPath)){throw "STOP: frozen primary timing execution lock missing"}
$L=Get-Content $LockPath -Raw|ConvertFrom-Json
if(-not[bool]$L.authorization.execution_authorized){throw "STOP: primary timing execution package not authorized"}
if([bool]$L.authorization.hardware_counters -or [bool]$L.authorization.token_xray -or [bool]$L.authorization.child_c){throw "STOP: forbidden campaign opened"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String);if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
$Head=(& git -C $Root rev-parse HEAD).Trim()
foreach($P in $L.critical_git_blobs.PSObject.Properties){$Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim();if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: package blob mismatch: $($P.Name)"}}
# Zero-science rechecks only; never rebuild the qualified native executable here.
py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_4arm_primary_timing.py")
if($LASTEXITCODE-ne0){throw "STOP: primary timing static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "STOP: frozen shader compile/provenance failed"}
$SP=Get-Content (Join-Path $Root "results\q4_down_4arm_shader_provenance.json") -Raw|ConvertFrom-Json
if([string]$SP.child_a.spv_sha256-ne[string]$L.qualified_build.target_spv_sha256.child_a){throw "STOP: Child-A SPIR-V drift"}
$Serial=@($SP.exec148_shaders|Where-Object{$_.spv-eq"q4_down_exec148_serial.spv"})
$Split=@($SP.exec148_shaders|Where-Object{$_.spv-eq"q4_down_exec148_splitk32.spv"})
if($Serial.Count-ne1-or[string]$Serial[0].spv_sha256-ne[string]$L.qualified_build.target_spv_sha256.B){throw "STOP: B SPIR-V drift"}
if($Split.Count-ne1-or[string]$Split[0].spv_sha256-ne[string]$L.qualified_build.target_spv_sha256.AB){throw "STOP: AB SPIR-V drift"}

$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm_primary_timing.exe"
if(-not(Test-Path $Exe)){throw "STOP: qualified timing executable missing"}
if((Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$L.qualified_build.exe_sha256){throw "STOP: qualified timing executable hash mismatch"}
if((Get-Item $Exe).Length-ne[int64]$L.qualified_build.exe_bytes){throw "STOP: qualified timing executable size mismatch"}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor)
$GPU=@(Get-CimInstance Win32_VideoController)
if([string]$OS.BuildNumber-ne[string]$L.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object{$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object{$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$L.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}
if(-not $ModelPath){$Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot);if($Resolved.Count-lt1){throw "STOP: model resolver failed"};$ModelPath=[string]$Resolved[-1]}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$L.target_model.sha256){throw "STOP: model hash mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$L.target_model.bytes){throw "STOP: model size mismatch"}
$ShaderDir=Join-Path $Root "compiled_shaders"
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ");$Dir=Join-Path $Root ("results\q4_down_4arm_primary_timing_"+$Stamp);New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Paths=@{}
foreach($W in @("W-S","W-C")){
  $Out=Join-Path $Dir ("primary_timing_"+($W-replace'-','_')+".json")
  & $Exe --model $ModelPath --shader-dir $ShaderDir --implementation-commit ([string]$L.qualified_build.source_head) --workload $W --out $Out
  if($LASTEXITCODE-ne0){throw "STOP: primary timing collection failed: $W"}
  $R=Get-Content $Out -Raw|ConvertFrom-Json
  if([string]$R.status-ne"PASS_PRIMARY_TIMING_COLLECTION"){throw "STOP: invalid timing collection status: $W"}
  if([bool]$R.measurement.hardware_counters_executed -or [bool]$R.measurement.token_xray_executed){throw "STOP: forbidden instrumentation leakage"}
  $Paths[$W]=$Out
}
$Adj=Join-Path $Dir "Q4_DOWN_4ARM_PRIMARY_TIMING_ADJUDICATION.json"
py -3 (Join-Path $Root "tools\adjudicate_arcllm_v1_q4_down_4arm_primary_timing.py") --w-s $Paths["W-S"] --w-c $Paths["W-C"] --out $Adj
if($LASTEXITCODE-ne0){throw "STOP: timing recompute/adjudication failed"}
Copy-Item $LockPath $Dir -Force
Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY_CANONICAL_v0.1.json") $Dir -Force
$Zip=Join-Path $Dir "Q4_DOWN_4ARM_PRIMARY_TIMING_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_4ARM_PRIMARY_TIMING=PASS_COLLECTION"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "COUNTERS_TOKEN_XRAY_CHILD_C_REMAIN_CLOSED"
