param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_4arm_primary_timing.py")
if($LASTEXITCODE-ne0){throw "primary timing static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "frozen shader compile/provenance failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_q4_down_4arm_primary_timing.ps1")
if($LASTEXITCODE-ne0){throw "primary timing native build failed"}
$B=Get-Content (Join-Path $Root "results\Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY.json") -Raw|ConvertFrom-Json
if($B.target_model_loaded-or$B.gpu_dispatch_executed-or$B.timing_executed-or$B.hardware_counters_executed){throw "zero-science boundary violated"}
Write-Host "Q4_DOWN_4ARM_PRIMARY_TIMING_ZERO_SCIENCE_QA=PASS"
