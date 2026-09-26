param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $PSScriptRoot

py -3 (Join-Path $Root "tests\test_arcllm_v1_b1_policy_static.py")
if($LASTEXITCODE-ne0){throw "B1 policy static QA failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){throw "ProgramFiles(x86) unavailable"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$Src=Join-Path $Root "tests\arcllm_v1_b1_policy_zero_science.cpp"
$Exe=Join-Path $Root "arcllm_v1_b1_policy_zero_science.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Src+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "B1 policy zero-science native build failed"}

& $Exe
if($LASTEXITCODE-ne0){throw "B1 policy zero-science executable failed"}

Write-Host "B1_POLICY_ZERO_SCIENCE_BUILD_AND_QA=PASS"
Write-Host "NO MODEL LOAD. NO GPU. NO VULKAN. NO PERFORMANCE TIMING. NO PLACEMENT BENCHMARK."
