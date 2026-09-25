param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here;$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_4arm_package.py")
if($LASTEXITCODE-ne0){throw "Q4-down static package QA failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "Q4-down shader build-only failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "Q4-down native build-only failed"}

$Build=Get-Content (Join-Path $Results "q4_down_4arm_buildonly.json") -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Build.status-ne"PASS_BUILDONLY_SYNTHETIC"-or[bool]$Build.target_model_loaded-or[bool]$Build.gpu_dispatch_executed-or[bool]$Build.timing_executed){throw "Q4-down build-only evidence invariant failed"}

$E=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.buildonly_qa.v0.1"
 status="PASS_STATIC_SYNTHETIC_BUILDONLY"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 static_qa="PASS"
 shader_build="PASS"
 synthetic_exec148="PASS"
 native_build="PASS"
 target_model_loaded=$false
 gpu_dispatch_executed=$false
 performance_authorized=$false
 hardware_counters_authorized=$false
 timing_executed=$false
 next="DEV_HOST_REAL_MODEL_CORRECTNESS_ONLY"
}
[IO.File]::WriteAllText((Join-Path $Results "ARCLLM_V1_Q4_DOWN_4ARM_BUILDONLY_QA.json"),($E|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_DOWN_4ARM_BUILDONLY_QA=PASS"
Write-Host "PERFORMANCE_REMAINS_FORBIDDEN"
