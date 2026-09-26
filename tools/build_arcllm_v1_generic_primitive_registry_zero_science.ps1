param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $PSScriptRoot
py -3 (Join-Path $Root "tests\test_arcllm_v1_generic_primitive_registry_static.py")
if($LASTEXITCODE-ne0){throw "generic registry static QA failed"}
$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Exe=Join-Path $Root "arcllm_v1_generic_primitive_registry_zero_science.exe"
$Test=Join-Path $Root "tests\arcllm_v1_generic_primitive_registry_zero_science.cpp"
$Core=Join-Path $Root "src\arcllm_v1_primitive_registry.cpp"
$Ref=Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration.cpp"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $Root 'include')+'" /I"'+(Join-Path $Root 'src')+'" /Fe:"'+$Exe+'" "'+$Test+'" "'+$Core+'" "'+$Ref+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "generic registry native build failed"}
& $Exe
if($LASTEXITCODE-ne0){throw "generic registry zero-science QA failed"}
Write-Host "PHASE2_GENERIC_PRIMITIVE_REGISTRY_BUILD_AND_QA=PASS"
Write-Host "NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO PLACEMENT BENCHMARK."
