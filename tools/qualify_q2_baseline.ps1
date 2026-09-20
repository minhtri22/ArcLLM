param(
  [string]$LlamaDir,
  [string]$BuildDir
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$PinnedCommit="391fac16460f15233a7740550d858ac96df3419d"
$PinnedRelease="v0.4.1"
$RepoUrl="https://github.com/ggml-org/llama.cpp.git"
if(-not $LlamaDir){$LlamaDir=Join-Path $Root "third_party\llama.cpp-q2"}
if(-not $BuildDir){$BuildDir=Join-Path $Root "build\q2_baseline"}

if(-not(Test-Path (Join-Path $LlamaDir ".git"))){
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LlamaDir)|Out-Null
  git clone --filter=blob:none --no-checkout $RepoUrl $LlamaDir
  if($LASTEXITCODE -ne 0){throw "Q2 baseline clone failed"}
}
git -C $LlamaDir fetch --force --depth 1 origin $PinnedCommit
if($LASTEXITCODE -ne 0){throw "Q2 baseline commit fetch failed"}
git -C $LlamaDir fetch --force --depth 1 origin "refs/tags/$PinnedRelease:refs/tags/$PinnedRelease"
if($LASTEXITCODE -ne 0){throw "Q2 baseline tag fetch failed"}
git -C $LlamaDir checkout --detach $PinnedCommit
if($LASTEXITCODE -ne 0){throw "Q2 baseline checkout failed"}
$Head=(git -C $LlamaDir rev-parse HEAD).Trim()
$TagCommit=(git -C $LlamaDir rev-list -n 1 $PinnedRelease).Trim()
if($Head -ne $PinnedCommit -or $TagCommit -ne $PinnedCommit){throw "Q2 baseline pin mismatch"}
$Dirty=((git -C $LlamaDir status --porcelain)|Out-String)
if($Dirty.Trim()){throw "Q2 baseline source tree is dirty"}

if(-not $env:VULKAN_SDK){throw "Q2 baseline qualification requires VULKAN_SDK"}
$glslc=Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
if(-not(Test-Path $glslc)){$glslc=Join-Path $env:VULKAN_SDK "bin\glslc.exe"}
if(-not(Test-Path $glslc)){throw "Q2 baseline Vulkan SDK missing glslc"}

if(Test-Path $BuildDir){Remove-Item -Recurse -Force $BuildDir}
cmake -S (Join-Path $Root "baseline") -B $BuildDir -G "Ninja Multi-Config" "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE -ne 0){throw "Q2 baseline CMake configure failed"}
cmake --build $BuildDir --config Release --target q2_llama_adapter
if($LASTEXITCODE -ne 0){throw "Q2 baseline native/Vulkan build failed"}

$Candidates=@(
  (Join-Path $BuildDir "Release\q2_llama_adapter.exe"),
  (Join-Path $BuildDir "q2_llama_adapter.exe")
)
$Exe=$Candidates|Where-Object {Test-Path $_}|Select-Object -First 1
if(-not $Exe){throw "Q2 baseline adapter executable missing"}

$OutDir=Join-Path $Root "artifacts\q2_baseline"
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$OutExe=Join-Path $OutDir "q2_llama_adapter.exe"
Copy-Item -Force $Exe $OutExe
$ExeHash=(Get-FileHash $OutExe -Algorithm SHA256).Hash.ToUpperInvariant()

$Results=Join-Path $Root "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
$Q=[ordered]@{
  schema="arcllm.q2.baseline_qualification.v1"
  repository="ggml-org/llama.cpp"
  release=$PinnedRelease
  commit=$PinnedCommit
  source_head=$Head
  source_clean=$true
  build_backend="Vulkan"
  vulkan_sdk=$env:VULKAN_SDK
  build_shared_libs=$false
  ggml_native=$false
  ggml_backend_dl=$false
  adapter_path=(Resolve-Path $OutExe).Path
  adapter_sha256=$ExeHash
  raw_token_adapter=$true
  tokenizer_path_used=$false
  target_model_executed=$false
  qualification="BUILD_API_QUALIFIED"
}
$Path=Join-Path $Results "q2_baseline_qualification.json"
[IO.File]::WriteAllText($Path,($Q|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q2 baseline BUILD_API_QUALIFIED"
Write-Host "  commit=$Head"
Write-Host "  adapter=$OutExe"
Write-Host "  sha256=$ExeHash"
