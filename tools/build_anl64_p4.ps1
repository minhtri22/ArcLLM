param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$SrcDir=Join-Path $Root "src"
$OutDir=Join-Path $Root "artifacts\ANL64\P4"
$Exe=Join-Path $OutDir "anl64_p4.exe"
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+(Join-Path $SrcDir 'anl64_runtime.cpp')+'" "'+(Join-Path $SrcDir 'gguf.cpp')+'" "'+(Join-Path $SrcDir 'tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$Manifest=[ordered]@{
 schema="arcllm.anl64.p4.native_build.v0.1"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 executable=(Resolve-Path $Exe).Path
 executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
 executable_bytes=(Get-Item $Exe).Length
 source_blobs=[ordered]@{
   runtime=((& git -C $Root rev-parse "HEAD:src/anl64_runtime.cpp").Trim())
   plan=((& git -C $Root rev-parse "HEAD:src/anl64_plan.hpp").Trim())
   gguf=((& git -C $Root rev-parse "HEAD:src/gguf.cpp").Trim())
   tensor_store=((& git -C $Root rev-parse "HEAD:src/tensor_store.cpp").Trim())
 }
 executable_launched=$false
 model_loaded=$false
 gpu_dispatch=$false
 performance_measurement=$false
}
$Path=Join-Path $OutDir "native_build.json"
[IO.File]::WriteAllText($Path,($Manifest|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ("ANL64 P4 native BuildOnly PASS: "+$Exe)
