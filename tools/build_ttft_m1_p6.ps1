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
$OutDir=Join-Path $Root "artifacts\TTFT_M1\P6"
$Exe=Join-Path $OutDir "ttft_m1_diagnostic.exe"
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+(Join-Path $SrcDir 'ttft_m1_diagnostic.cpp')+'" "'+(Join-Path $SrcDir 'gguf.cpp')+'" "'+(Join-Path $SrcDir 'tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$M=[ordered]@{
 schema="arcllm.ttft_m1.p6.native_build.v0.1"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 executable=(Resolve-Path $Exe).Path
 executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
 executable_bytes=(Get-Item $Exe).Length
 source_blobs=[ordered]@{
   diagnostic=((& git -C $Root rev-parse "HEAD:src/ttft_m1_diagnostic.cpp").Trim())
   gguf=((& git -C $Root rev-parse "HEAD:src/gguf.cpp").Trim())
   tensor_store=((& git -C $Root rev-parse "HEAD:src/tensor_store.cpp").Trim())
 }
 executable_launched=$false
 model_loaded=$false
 gpu_dispatch=$false
 performance_measurement=$false
}
$Path=Join-Path $OutDir "NATIVE_BUILD.json"
[IO.File]::WriteAllText($Path,($M|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ("TTFT M1 diagnostic BuildOnly PASS: "+$Exe)
