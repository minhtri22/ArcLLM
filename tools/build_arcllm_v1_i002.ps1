param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Src=Join-Path $Root "src"

$Targets=@(
 @("arcllm_v1_i002_t1.exe","arcllm_v1_i002_t1_transfer.cpp"),
 @("arcllm_v1_i002_t3.exe","arcllm_v1_i002_t3_paired.cpp")
)
foreach($T in $Targets){
 $Exe=Join-Path $Root $T[0]
 $Main=Join-Path $Src $T[1]
 $Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Main+'" "'+(Join-Path $Src 'gguf.cpp')+'" "'+(Join-Path $Src 'tensor_store.cpp')+'"'
 cmd.exe /d /s /c $Cmd
 if($LASTEXITCODE -ne 0){throw "I002 native build failed: $($T[1])"}
 if(-not(Test-Path $Exe)){throw "I002 executable missing: $Exe"}
 Write-Host "Built: $Exe"
}
