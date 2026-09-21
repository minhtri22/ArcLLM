param([string]$VulkanSdkRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
$Candidates=@();if($VulkanSdkRoot){$Candidates+=$VulkanSdkRoot};$Candidates+=(Join-Path $Root ".q2_toolchains\VulkanSDK\1.4.357.0");if($env:VULKAN_SDK){$Candidates+=$env:VULKAN_SDK}
$Sdk=$null;foreach($C in $Candidates){if($C -and(Test-Path(Join-Path $C "Include\vulkan\vulkan.h"))-and(Test-Path(Join-Path $C "Lib\vulkan-1.lib"))){$Sdk=(Resolve-Path $C).Path;break}}
if(-not$Sdk){throw "SA1 Vulkan SDK not found"}
$PF86=[Environment]::GetFolderPath("ProgramFilesX86");$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe";if(-not(Test-Path $VsWhere)){throw "vswhere.exe missing"}
$VS=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;if(-not$VS){throw "MSVC Build Tools missing"}
$Dev=Join-Path $VS "VC\Auxiliary\Build\vcvars64.bat";$OutDir=Join-Path $Root "artifacts\SA1_K1\build";New-Item -ItemType Directory -Force $OutDir|Out-Null
$Src=Join-Path $Root "src\sa1_component_benchmark.cpp";$Exe=Join-Path $OutDir "sa1_component_benchmark.exe"
$Cmd='call "'+$Dev+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $Sdk "Include")+'" /Fe:"'+$Exe+'" "'+$Src+'" /link /LIBPATH:"'+(Join-Path $Sdk "Lib")+'" vulkan-1.lib'
& cmd.exe /d /s /c $Cmd;if($LASTEXITCODE -ne 0){throw "SA1 native build failed"}
$M=[ordered]@{schema="arcllm.sa1.k1.native_build.v0.1";git_head=((& git -C $Root rev-parse HEAD).Trim());source_git_blob=((& git -C $Root rev-parse "HEAD:src/sa1_component_benchmark.cpp").Trim());source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant();executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant();sdk_root=$Sdk;sdk_header_sha256=(Get-FileHash (Join-Path $Sdk "Include\vulkan\vulkan.h") -Algorithm SHA256).Hash.ToUpperInvariant();model_loaded=$false}
[IO.File]::WriteAllText((Join-Path $OutDir "native_build.json"),($M|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "SA1-K1 native BuildOnly PASS"
