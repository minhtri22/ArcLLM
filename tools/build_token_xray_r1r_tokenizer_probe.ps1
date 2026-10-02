param(
  [string]$LlamaDir,
  [string]$BuildDir
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$PinnedCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$RepoUrl="https://github.com/ggml-org/llama.cpp.git"
if(-not $LlamaDir){$LlamaDir=Join-Path $Root "third_party\llama.cpp-r1r-tokenizer"}
if(-not $BuildDir){$BuildDir=Join-Path $Root "build\token_xray_r1r_tokenizer"}

$CMake=Get-Command cmake.exe -ErrorAction SilentlyContinue
if($CMake){$CMakeExe=$CMake.Source}else{
  $VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if(-not(Test-Path $VsWhere)){throw "CMake/Visual Studio unavailable"}
  $VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if(-not $VSInstall){throw "Visual Studio unavailable"}
  $CMakeExe=Join-Path $VSInstall "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}
if(-not(Test-Path (Join-Path $LlamaDir ".git"))){
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LlamaDir)|Out-Null
  git clone --filter=blob:none --no-checkout $RepoUrl $LlamaDir
  if($LASTEXITCODE-ne0){throw "llama.cpp clone failed"}
}
git -C $LlamaDir fetch --force --depth 1 origin $PinnedCommit
if($LASTEXITCODE-ne0){throw "llama.cpp pinned commit fetch failed"}
git -C $LlamaDir checkout --detach $PinnedCommit
if($LASTEXITCODE-ne0){throw "llama.cpp pinned checkout failed"}
$Head=(git -C $LlamaDir rev-parse HEAD).Trim()
if($Head-ne$PinnedCommit){throw "llama.cpp tokenizer probe commit mismatch"}

if(Test-Path $BuildDir){Remove-Item -Recurse -Force $BuildDir}
& $CMakeExe -S (Join-Path $Root "baseline\r1r_tokenizer") -B $BuildDir -G "Visual Studio 17 2022" -A x64 "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE-ne0){throw "R1R tokenizer probe CMake configure failed"}
& $CMakeExe --build $BuildDir --config Release --target token_xray_r1r_tokenizer_probe -- /m
if($LASTEXITCODE-ne0){throw "R1R tokenizer probe build failed"}
$Exe=Get-ChildItem $BuildDir -Recurse -Filter token_xray_r1r_tokenizer_probe.exe|Where-Object {$_.FullName -match "\\Release\\"}|Select-Object -First 1 -ExpandProperty FullName
if(-not $Exe){$Exe=Get-ChildItem $BuildDir -Recurse -Filter token_xray_r1r_tokenizer_probe.exe|Select-Object -First 1 -ExpandProperty FullName}
if(-not $Exe){throw "R1R tokenizer probe executable missing"}
$Out=Join-Path $Root "token_xray_r1r_tokenizer_probe.exe"
Copy-Item -Force $Exe $Out
Write-Host "TOKEN_XRAY_R1R_TOKENIZER_PROBE_BUILD=PASS"
Write-Host "LLAMA_CPP_HEAD=$Head"
Write-Host "EXE=$Out"
