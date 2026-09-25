param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
py -3 (Join-Path (Split-Path -Parent $Root) "tests\test_arcllm_v1_q4_down_4arm_native_counter.py")
if($LASTEXITCODE-ne0){throw "Q4 native counter static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path (Split-Path -Parent $Root) "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "Q4 native counter frozen shader compile failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path (Split-Path -Parent $Root) "tools\build_arcllm_v1_q4_down_4arm_native_counter.ps1")
if($LASTEXITCODE-ne0){throw "Q4 native counter native build failed"}
$B=Get-Content (Join-Path (Split-Path -Parent $Root) "results\Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY.json") -Raw|ConvertFrom-Json
if($B.model_loaded-or$B.gpu_dispatch_executed-or$B.counter_probe_executed-or$B.primary_timing_executed){throw "zero-science boundary violated"}
Write-Host "Q4_DOWN_4ARM_NATIVE_COUNTER_ZERO_SCIENCE_QA=PASS"
