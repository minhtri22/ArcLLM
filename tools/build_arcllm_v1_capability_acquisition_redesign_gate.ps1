param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $PSScriptRoot
py -3 (Join-Path $Root "tests\test_arcllm_v1_capability_acquisition_semantics_redesign_gate_static.py")
if($LASTEXITCODE-ne0){throw "redesign static QA failed"}
$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Exe=Join-Path $Root "arcllm_v1_capability_acquisition_redesign_gate.exe"
$Test=Join-Path $Root "tests\arcllm_v1_capability_acquisition_semantics_redesign_gate.cpp"
$Sources=@(
  (Join-Path $Root "src\arcllm_v1_generic_policy_engine.cpp"),
  (Join-Path $Root "src\arcllm_v1_primitive_registry.cpp"),
  (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration.cpp"),
  (Join-Path $Root "src\arcllm_v1_generic_policy_engine_v2.cpp"),
  (Join-Path $Root "src\arcllm_v1_primitive_registry_v2.cpp"),
  (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp"),
  (Join-Path $Root "src\registrations\arcllm_v1_p8_segmented_reference_registration_v2.cpp")
)
$Quoted=($Sources|ForEach-Object{'"'+$_+'"'}) -join ' '
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $Root 'include')+'" /I"'+(Join-Path $Root 'src')+'" /Fe:"'+$Exe+'" "'+$Test+'" '+$Quoted
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "redesign native build failed"}
& $Exe
if($LASTEXITCODE-ne0){throw "redesign gate failed"}
Write-Host "PHASE2_CAPABILITY_ACQUISITION_REDESIGN_BUILD_AND_QA=PASS"
Write-Host "NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO NEW PLACEMENT STUDY."
