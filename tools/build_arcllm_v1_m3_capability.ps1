param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if(-not $env:VULKAN_SDK){throw "VULKAN_SDK is not set; M3-A needs Vulkan SDK headers/libs for capability enumeration"}
$Inc=Join-Path $env:VULKAN_SDK "Include"
$Lib=Join-Path $env:VULKAN_SDK "Lib"
$Src=Join-Path $Root "src\arcllm_v1_m3_vulkan_counter_capability.cpp"
$Exe=Join-Path $Root "arcllm_v1_m3_counter_capability.exe"
$PF86=[Environment]::GetFolderPath("ProgramFilesX86")
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+$Inc+'" /Fe:"'+$Exe+'" "'+$Src+'" /link /LIBPATH:"'+$Lib+'" vulkan-1.lib'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
Write-Host "Built: $Exe"
