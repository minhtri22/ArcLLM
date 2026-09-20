param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
$VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat";$SrcDir=Join-Path $Root "src";$Exe=Join-Path $Root "arcllm_p8g5.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+(Join-Path $SrcDir 'p8g5_production_semantic_attribution.cpp')+'" "'+(Join-Path $SrcDir 'gguf.cpp')+'" "'+(Join-Path $SrcDir 'tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
Write-Host "Built: $Exe"
