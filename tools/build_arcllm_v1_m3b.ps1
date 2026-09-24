param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
if(-not $env:VULKAN_SDK){throw "VULKAN_SDK is not set"}
$Inc=Join-Path $env:VULKAN_SDK "Include"
$Lib=Join-Path $env:VULKAN_SDK "Lib"
$PF86=[Environment]::GetFolderPath("ProgramFilesX86")
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$SrcDir=Join-Path $Root "src"
$Exe=Join-Path $Root "arcllm_v1_m3b_counter_collection.exe"
$Sources=@(
  (Join-Path $SrcDir "arcllm_v1_m3b_counter_collection.cpp"),
  (Join-Path $SrcDir "arcllm_v1_m3_counter_shim.cpp"),
  (Join-Path $SrcDir "gguf.cpp"),
  (Join-Path $SrcDir "tensor_store.cpp")
)
$SrcArgs=($Sources | ForEach-Object {'"'+$_+'"'}) -join " "
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+$Inc+'" /Fe:"'+$Exe+'" '+$SrcArgs+' /link /LIBPATH:"'+$Lib+'" vulkan-1.lib'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
Write-Host "Built: $Exe"
