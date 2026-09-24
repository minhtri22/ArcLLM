param(
  [string]$LlamaDir,
  [string]$BuildDir,
  [bool]$BootstrapVulkanSdk=$true
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$PinnedCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$PinnedRelease="v0.4.1"
$RepoUrl="https://github.com/ggml-org/llama.cpp.git"
$VulkanVersion="1.4.357.0"
$ExpectedVulkanInstallerSha="81F474711E9042F4CD22B31B2F7A8870DB2E428B21586FB43DD80150BE97310D"
$SystemVulkanHome="C:\VulkanSDK\$VulkanVersion"
$PortableVulkanHome=Join-Path $Root (".q2_toolchains\VulkanSDK\"+$VulkanVersion)
$InstallerSha=$null
$VulkanBootstrapMode=$null
if(-not $LlamaDir){$LlamaDir=Join-Path $Root "third_party\llama.cpp-q2"}
if(-not $BuildDir){$BuildDir=Join-Path $Root "build\i003_baseline"}

function Test-Q2VulkanSdk([string]$Path){
  if(-not $Path){return $false}
  return (Test-Path (Join-Path $Path "Bin\glslc.exe")) -and
         (Test-Path (Join-Path $Path "Include\vulkan\vulkan.h")) -and
         (Test-Path (Join-Path $Path "Lib\vulkan-1.lib"))
}

$SelectedVulkanHome=$null
if($env:VULKAN_SDK -and (Split-Path -Leaf $env:VULKAN_SDK) -eq $VulkanVersion -and (Test-Q2VulkanSdk $env:VULKAN_SDK)){
  $SelectedVulkanHome=$env:VULKAN_SDK
  $VulkanBootstrapMode="EXISTING_ENV"
}elseif(Test-Q2VulkanSdk $SystemVulkanHome){
  $SelectedVulkanHome=$SystemVulkanHome
  $VulkanBootstrapMode="EXISTING_SYSTEM"
}elseif(Test-Q2VulkanSdk $PortableVulkanHome){
  $SelectedVulkanHome=$PortableVulkanHome
  $VulkanBootstrapMode="PORTABLE_CACHE"
}elseif($BootstrapVulkanSdk){
  $Installer=Join-Path $env:TEMP ("VulkanSDK-"+$VulkanVersion+".exe")
  $Url="https://sdk.lunarg.com/sdk/download/$VulkanVersion/windows/vulkansdk-windows-X64-$VulkanVersion.exe"
  if(Test-Path $Installer){
    $InstallerSha=(Get-FileHash $Installer -Algorithm SHA256).Hash.ToUpperInvariant()
    if($InstallerSha -ne $ExpectedVulkanInstallerSha){Remove-Item -Force $Installer;$InstallerSha=$null}
  }
  if(-not(Test-Path $Installer)){
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $Installer
    $InstallerSha=(Get-FileHash $Installer -Algorithm SHA256).Hash.ToUpperInvariant()
  }
  if($InstallerSha -ne $ExpectedVulkanInstallerSha){throw "Q2 Vulkan SDK installer SHA mismatch"}

  if(Test-Path $PortableVulkanHome){Remove-Item -Recurse -Force $PortableVulkanHome}
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $PortableVulkanHome)|Out-Null

  # LunarG-supported non-admin mode: copy SDK files only; no registry, shortcuts, or system PATH changes.
  & $Installer --root $PortableVulkanHome --accept-licenses --default-answer --confirm-command install copy_only=1
  if($LASTEXITCODE -ne 0){throw "Q2 Vulkan SDK portable copy-only install failed"}
  if(-not(Test-Q2VulkanSdk $PortableVulkanHome)){throw "Q2 portable Vulkan SDK incomplete after copy-only install"}

  $SelectedVulkanHome=$PortableVulkanHome
  $VulkanBootstrapMode="PORTABLE_COPY_ONLY"
}else{
  throw "I003 baseline qualification requires Vulkan SDK 1.4.357.0"
}

$env:VULKAN_SDK=$SelectedVulkanHome
$env:VK_SDK_PATH=$SelectedVulkanHome
$env:PATH=(Join-Path $SelectedVulkanHome "Bin")+";"+$env:PATH

$glslc=Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
if(-not(Test-Q2VulkanSdk $env:VULKAN_SDK)){throw "I003 baseline Vulkan SDK validation failed"}

$CMakeCmd=Get-Command cmake.exe -ErrorAction SilentlyContinue
if($CMakeCmd){$CMakeExe=$CMakeCmd.Source}else{
  $VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if(-not(Test-Path $VsWhere)){throw "I003 baseline qualification requires CMake or Visual Studio CMake"}
  $VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if(-not $VSInstall){throw "I003 baseline qualification cannot locate Visual Studio"}
  $CMakeExe=Join-Path $VSInstall "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if(-not(Test-Path $CMakeExe)){throw "I003 baseline qualification cannot locate Visual Studio CMake"}
}
if(-not(Test-Path (Join-Path $LlamaDir ".git"))){
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LlamaDir)|Out-Null
  git clone --filter=blob:none --no-checkout $RepoUrl $LlamaDir
  if($LASTEXITCODE -ne 0){throw "I003 baseline clone failed"}
}
git -C $LlamaDir fetch --force --depth 1 origin $PinnedCommit
if($LASTEXITCODE -ne 0){throw "I003 baseline commit fetch failed"}
git -C $LlamaDir fetch --force --depth 1 origin "refs/tags/${PinnedRelease}:refs/tags/${PinnedRelease}"
if($LASTEXITCODE -ne 0){throw "I003 baseline tag fetch failed"}
git -C $LlamaDir checkout --detach $PinnedCommit
if($LASTEXITCODE -ne 0){throw "I003 baseline checkout failed"}
$Head=(git -C $LlamaDir rev-parse HEAD).Trim()
$TagObject=(git -C $LlamaDir rev-parse $PinnedRelease).Trim()
$TagCommit=(git -C $LlamaDir rev-list -n 1 $PinnedRelease).Trim()
$ExpectedTagObject="29aaf1c27faa48292357cea2120d94114a545006"
if($Head -ne $PinnedCommit -or $TagCommit -ne $PinnedCommit -or $TagObject -ne $ExpectedTagObject){throw "I003 baseline release tag/commit identity mismatch"}
$Dirty=((git -C $LlamaDir status --porcelain)|Out-String)
if($Dirty.Trim()){throw "I003 baseline source tree is dirty"}

if(Test-Path $BuildDir){Remove-Item -Recurse -Force $BuildDir}
& $CMakeExe -S (Join-Path $Root "baseline") -B $BuildDir -G "Visual Studio 17 2022" -A x64 "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE -ne 0){throw "I003 baseline CMake configure failed"}
& $CMakeExe --build $BuildDir --config Release --target i003_llama_adapter -- /m
if($LASTEXITCODE -ne 0){throw "I003 baseline native/Vulkan build failed"}

$Exe=Get-ChildItem $BuildDir -Recurse -Filter i003_llama_adapter.exe|Where-Object {$_.FullName -match "\\Release\\"}|Select-Object -First 1 -ExpandProperty FullName
if(-not $Exe){$Exe=Get-ChildItem $BuildDir -Recurse -Filter i003_llama_adapter.exe|Select-Object -First 1 -ExpandProperty FullName}
if(-not $Exe){throw "I003 baseline adapter executable missing"}

$OutDir=Join-Path $Root "artifacts\i003_baseline"
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$OutExe=Join-Path $OutDir "i003_llama_adapter.exe"
Copy-Item -Force $Exe $OutExe
$ExeHash=(Get-FileHash $OutExe -Algorithm SHA256).Hash.ToUpperInvariant()
$CMakeVersionLines=@(& $CMakeExe --version)
if($LASTEXITCODE-ne0-or$CMakeVersionLines.Count-lt1){throw "I003 baseline CMake version query failed"}
$CMakeVersion=([string]$CMakeVersionLines[0]).Trim()
$Results=Join-Path $Root "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
$Q=[ordered]@{
  schema="arcllm.v1.i003.baseline_qualification.v0.1"
  repository="ggml-org/llama.cpp"
  release=$PinnedRelease
  commit=$PinnedCommit
  annotated_tag_object_sha=$TagObject
  annotated_tag_commit_sha=$TagCommit
  release_build_is_dev=$false
  source_head=$Head
  source_clean=$true
  build_backend="Vulkan"
  vulkan_sdk=$env:VULKAN_SDK
  vulkan_sdk_version=$VulkanVersion
  vulkan_sdk_bootstrap_mode=$VulkanBootstrapMode
  vulkan_sdk_portable_copy_only=($VulkanBootstrapMode -eq "PORTABLE_COPY_ONLY" -or $VulkanBootstrapMode -eq "PORTABLE_CACHE")
  vulkan_sdk_admin_required=$false
  vulkan_installer_sha256_expected=$ExpectedVulkanInstallerSha
  vulkan_installer_sha256_observed=$InstallerSha
  cmake=$CMakeVersion
  generator="Visual Studio 17 2022 x64"
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
$Path=Join-Path $Results "i003_baseline_qualification.json"
[IO.File]::WriteAllText($Path,($Q|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "I003 baseline BUILD_API_QUALIFIED"
Write-Host "  commit=$Head"
Write-Host "  adapter=$OutExe"
Write-Host "  sha256=$ExeHash"
exit 0
