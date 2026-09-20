param([string]$VulkanSdkRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
function Sha([string]$P){(Get-FileHash -Algorithm SHA256 -LiteralPath $P).Hash.ToUpperInvariant()}
$Candidates=@()
if($VulkanSdkRoot){$Candidates+=$VulkanSdkRoot}
$Candidates+=(Join-Path $Root ".q2_toolchains\VulkanSDK\1.4.357.0")
if($env:VULKAN_SDK){$Candidates+=$env:VULKAN_SDK}
$Sdk=$null
foreach($C in $Candidates){
  if($C -and (Test-Path (Join-Path $C "Include\vulkan\vulkan.h")) -and (Test-Path (Join-Path $C "Lib\vulkan-1.lib"))){$Sdk=(Resolve-Path $C).Path;break}
}
if(-not $Sdk){throw "SA0-CAP requires a Vulkan SDK containing Include\vulkan\vulkan.h and Lib\vulkan-1.lib. Reuse .q2_toolchains\VulkanSDK\1.4.357.0 or pass -VulkanSdkRoot."}
$ProgramFilesX86=[Environment]::GetFolderPath("ProgramFilesX86")
$VsWhere=Join-Path $ProgramFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VS=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VS){throw "Visual Studio C++ Build Tools not found"}
$Dev=Join-Path $VS "VC\Auxiliary\Build\vcvars64.bat"
$OutDir=Join-Path $Root "artifacts\SA0_CAP";New-Item -ItemType Directory -Force $OutDir|Out-Null
$Exe=Join-Path $OutDir "arcllm_sa0_cap.exe"
$Src=Join-Path $Root "src\sa0_capability_probe.cpp"
$Cmd='call "'+$Dev+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $Sdk "Include")+'" /Fe:"'+$Exe+'" "'+$Src+'" /link /LIBPATH:"'+(Join-Path $Sdk "Lib")+'" vulkan-1.lib'
& cmd.exe /d /s /c $Cmd
if($LASTEXITCODE -ne 0){throw "SA0-CAP native build failed (exit=$LASTEXITCODE)"}
$Head=(& git -C $Root rev-parse HEAD).Trim()
$Critical=@(
 "src/sa0_capability_probe.cpp",
 "tools/build_sa0_cap.ps1",
 "run_sa0_capability_preflight.ps1",
 "tests/test_sa0_capability_package.py",
 "config/sa0_capability_preflight_v0.1.json"
)
$Blobs=[ordered]@{};foreach($Rel in $Critical){$Blobs[$Rel]=((& git -C $Root rev-parse ("HEAD:"+$Rel)).Trim())}
$Manifest=[ordered]@{
 schema="arcllm.sa0.capability.build.v0.1"
 git_head=$Head
 source_sha256=(Sha $Src)
 executable_sha256=(Sha $Exe)
 sdk_root=$Sdk
 sdk_header_sha256=(Sha (Join-Path $Sdk "Include\vulkan\vulkan.h"))
 critical_git_blobs=$Blobs
 scientific_workload="NOT_RUN"
 model_load="NOT_PERMITTED"
 shader_compile="NOT_PERFORMED"
 shader_modules_created=0
 compute_pipelines_created=0
 dispatches_submitted=0
 successor_kernel_implemented=$false
}
[IO.File]::WriteAllText((Join-Path $OutDir "build_manifest.json"),($Manifest|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "SA0-CAP BuildOnly PASS"
Write-Host "  exe=$Exe"
Write-Host "  scientific workload=NOT RUN"
