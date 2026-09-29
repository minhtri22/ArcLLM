param(
  [string]$LlamaDir,
  [string]$BuildDir,
  [bool]$BootstrapVulkanSdk=$true
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$PinnedCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$PinnedRelease="v0.4.1"
$RepoUrl="https://github.com/ggml-org/llama.cpp.git"
$VulkanVersion="1.4.357.0"
$ExpectedVulkanInstallerSha="81F474711E9042F4CD22B31B2F7A8870DB2E428B21586FB43DD80150BE97310D"
$SystemVulkanHome="C:\VulkanSDK\$VulkanVersion"
$PortableVulkanHome=Join-Path $Root (".q2_toolchains\VulkanSDK\"+$VulkanVersion)
if(-not $LlamaDir){$LlamaDir=Join-Path $Root "third_party\llama.cpp-core0b"}
if(-not $BuildDir){$BuildDir=Join-Path $Root "build\core0b_baseline"}

function Test-VulkanSdk([string]$Path){
  if(-not $Path){return $false}
  return (Test-Path (Join-Path $Path "Bin\glslc.exe")) -and
         (Test-Path (Join-Path $Path "Include\vulkan\vulkan.h")) -and
         (Test-Path (Join-Path $Path "Lib\vulkan-1.lib"))
}

$SelectedVulkanHome=$null
$BootstrapMode=$null
$InstallerSha=$null
if($env:VULKAN_SDK -and (Split-Path -Leaf $env:VULKAN_SDK) -eq $VulkanVersion -and (Test-VulkanSdk $env:VULKAN_SDK)){
  $SelectedVulkanHome=$env:VULKAN_SDK;$BootstrapMode="EXISTING_ENV"
}elseif(Test-VulkanSdk $SystemVulkanHome){
  $SelectedVulkanHome=$SystemVulkanHome;$BootstrapMode="EXISTING_SYSTEM"
}elseif(Test-VulkanSdk $PortableVulkanHome){
  $SelectedVulkanHome=$PortableVulkanHome;$BootstrapMode="PORTABLE_CACHE"
}elseif($BootstrapVulkanSdk){
  $Installer=Join-Path $env:TEMP ("VulkanSDK-"+$VulkanVersion+".exe")
  $Url="https://sdk.lunarg.com/sdk/download/$VulkanVersion/windows/vulkansdk-windows-X64-$VulkanVersion.exe"
  if(Test-Path $Installer){
    $InstallerSha=(Get-FileHash $Installer -Algorithm SHA256).Hash.ToUpperInvariant()
    if($InstallerSha-ne$ExpectedVulkanInstallerSha){Remove-Item -Force $Installer;$InstallerSha=$null}
  }
  if(-not(Test-Path $Installer)){
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $Installer
    $InstallerSha=(Get-FileHash $Installer -Algorithm SHA256).Hash.ToUpperInvariant()
  }
  if($InstallerSha-ne$ExpectedVulkanInstallerSha){throw "CORE0B Vulkan SDK installer SHA mismatch"}
  if(Test-Path $PortableVulkanHome){Remove-Item -Recurse -Force $PortableVulkanHome}
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $PortableVulkanHome)|Out-Null
  & $Installer --root $PortableVulkanHome --accept-licenses --default-answer --confirm-command install copy_only=1
  if($LASTEXITCODE-ne0){throw "CORE0B portable Vulkan SDK install failed"}
  if(-not(Test-VulkanSdk $PortableVulkanHome)){throw "CORE0B portable Vulkan SDK incomplete"}
  $SelectedVulkanHome=$PortableVulkanHome;$BootstrapMode="PORTABLE_COPY_ONLY"
}else{
  throw "CORE0B requires Vulkan SDK 1.4.357.0"
}

$env:VULKAN_SDK=$SelectedVulkanHome
$env:VK_SDK_PATH=$SelectedVulkanHome
$env:PATH=(Join-Path $SelectedVulkanHome "Bin")+";"+$env:PATH

$CMakeCmd=Get-Command cmake.exe -ErrorAction SilentlyContinue
if($CMakeCmd){$CMakeExe=$CMakeCmd.Source}else{
  $VsWhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if(-not(Test-Path $VsWhere)){throw "CORE0B requires CMake or Visual Studio CMake"}
  $VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if(-not $VSInstall){throw "CORE0B cannot locate Visual Studio"}
  $CMakeExe=Join-Path $VSInstall "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if(-not(Test-Path $CMakeExe)){throw "CORE0B cannot locate Visual Studio CMake"}
}

if(-not(Test-Path (Join-Path $LlamaDir ".git"))){
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LlamaDir)|Out-Null
  git clone --filter=blob:none --no-checkout $RepoUrl $LlamaDir
  if($LASTEXITCODE-ne0){throw "CORE0B llama clone failed"}
}
git -C $LlamaDir fetch --force --depth 1 origin $PinnedCommit
if($LASTEXITCODE-ne0){throw "CORE0B llama commit fetch failed"}
git -C $LlamaDir fetch --force --depth 1 origin "refs/tags/${PinnedRelease}:refs/tags/${PinnedRelease}"
if($LASTEXITCODE-ne0){throw "CORE0B llama tag fetch failed"}
git -C $LlamaDir checkout --detach $PinnedCommit
if($LASTEXITCODE-ne0){throw "CORE0B llama checkout failed"}
$Head=(git -C $LlamaDir rev-parse HEAD).Trim()
$TagCommit=(git -C $LlamaDir rev-list -n 1 $PinnedRelease).Trim()
if($Head-ne$PinnedCommit-or$TagCommit-ne$PinnedCommit){throw "CORE0B llama release/commit mismatch"}
$Dirty=((git -C $LlamaDir status --porcelain)|Out-String)
if($Dirty.Trim()){throw "CORE0B llama source tree dirty"}

if(Test-Path $BuildDir){Remove-Item -Recurse -Force $BuildDir}
& $CMakeExe -S (Join-Path $Root "baseline") -B $BuildDir -G "Visual Studio 17 2022" -A x64 "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE-ne0){throw "CORE0B llama CMake configure failed"}
& $CMakeExe --build $BuildDir --config Release --target core0b_llama_cold_adapter -- /m
if($LASTEXITCODE-ne0){throw "CORE0B llama native/Vulkan build failed"}

$Exe=Get-ChildItem $BuildDir -Recurse -Filter core0b_llama_cold_adapter.exe |
  Where-Object {$_.FullName -match "\\Release\\"} |
  Select-Object -First 1 -ExpandProperty FullName
if(-not $Exe){$Exe=Get-ChildItem $BuildDir -Recurse -Filter core0b_llama_cold_adapter.exe|Select-Object -First 1 -ExpandProperty FullName}
if(-not $Exe){throw "CORE0B llama adapter executable missing"}

$OutDir=Join-Path $Root "artifacts\core0b_baseline"
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$OutExe=Join-Path $OutDir "core0b_llama_cold_adapter.exe"
Copy-Item -Force $Exe $OutExe
$ExeHash=(Get-FileHash $OutExe -Algorithm SHA256).Hash.ToUpperInvariant()
$SrcBlob=(& git -C $Root rev-parse "HEAD:baseline/core0b_llama_cold_adapter.cpp").Trim()
$CMakeBlob=(& git -C $Root rev-parse "HEAD:baseline/CMakeLists.txt").Trim()
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

$Q=[ordered]@{
  schema="arcllm.core0b.llama_build_qualification.v0.1"
  status="PASS_BUILD_EXACT_LLAMA_BASELINE"
  repository="ggml-org/llama.cpp"
  release=$PinnedRelease
  commit=$PinnedCommit
  source_head=$Head
  source_clean=$true
  backend="Vulkan"
  vulkan_sdk=$SelectedVulkanHome
  vulkan_sdk_version=$VulkanVersion
  vulkan_bootstrap_mode=$BootstrapMode
  adapter_source_blob=$SrcBlob
  baseline_cmake_blob=$CMakeBlob
  adapter_path=(Resolve-Path $OutExe).Path
  adapter_sha256=$ExeHash
  target_model_executed=$false
  science_execution=$false
}
[IO.File]::WriteAllText((Join-Path $Results "CORE0B_LLAMA_BUILD_QUALIFICATION.json"),($Q|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "CORE0B_LLAMA_BUILD=PASS"
Write-Host "LLAMA_COMMIT=$Head"
Write-Host "ADAPTER=$OutExe"
Write-Host "ADAPTER_SHA256=$ExeHash"
exit 0
