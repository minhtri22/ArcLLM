param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
py -3 (Join-Path $Root "tests\test_arcllm_v1_phase2_closed_surface_v4_static.py")
if($LASTEXITCODE-ne0){throw "v4 static QA failed"}
$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Exe=Join-Path $Root "arcllm_v1_phase2_closed_surface_v4_zero_science.exe"
$Sources=@(
 "tests\arcllm_v1_phase2_closed_surface_v4_zero_science.cpp",
 "src\arcllm_v1_primitive_registry_v2.cpp",
 "src\arcllm_v1_generic_policy_engine_v4.cpp",
 "src\arcllm_v1_generic_backend_binding_v4.cpp",
 "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp",
 "src\registrations\arcllm_v1_p8_segmented_reference_registration_v2.cpp"
)|ForEach-Object{'"'+(Join-Path $Root $_)+'"'}
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" '+($Sources -join ' ')
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "v4 native build failed"}
& $Exe
if($LASTEXITCODE-ne0){throw "v4 zero-science runtime QA failed"}
Write-Host "PHASE2_CLOSED_SURFACE_V4_BUILD_AND_QA=PASS"
Write-Host "EXE_SHA256=$((Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant())"
