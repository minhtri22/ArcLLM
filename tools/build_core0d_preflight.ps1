param(
  [string]$TokenXrayRoot = "D:\WORK\RESEARCH\_token_xray_core0a_freeze",
  [string]$LlamaDir = "D:\WORK\_llama_core0b",
  [string]$GeneratedDir = "D:\WORK\_core0d_generated",
  [string]$LlamaBuildDir = "D:\WORK\_core0d_llama_build",
  [switch]$ReuseExistingLlamaBuild
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Artifacts=Join-Path $Root "artifacts\core0d"
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Artifacts,$Results|Out-Null

$PinnedLlama="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$PinnedTx="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
if(-not(Test-Path (Join-Path $LlamaDir ".git"))){throw "CORE0D exact llama checkout missing"}
if(-not(Test-Path (Join-Path $TokenXrayRoot ".git"))){throw "CORE0D Token-XRay checkout missing"}
$LlamaHead=(git -C $LlamaDir rev-parse HEAD).Trim()
$TxHead=(git -C $TokenXrayRoot rev-parse HEAD).Trim()
if($LlamaHead-ne$PinnedLlama){throw "CORE0D llama HEAD mismatch"}
if($TxHead-ne$PinnedTx){throw "CORE0D Token-XRay HEAD mismatch"}
if(((git -C $LlamaDir status --porcelain)|Out-String).Trim()){throw "CORE0D llama checkout dirty"}
if(((git -C $TokenXrayRoot status --porcelain)|Out-String).Trim()){throw "CORE0D Token-XRay checkout dirty"}

if(Test-Path $GeneratedDir){Remove-Item -Recurse -Force $GeneratedDir}
py -3 (Join-Path $Root "tools\materialize_core0d_runtime.py") --out-dir $GeneratedDir
if($LASTEXITCODE-ne0){throw "CORE0D materializer failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "CORE0D vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "CORE0D Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$VulkanVersion="1.4.357.0"
$Candidates=@($env:VULKAN_SDK,"C:\VulkanSDK\$VulkanVersion",(Join-Path $Root ".q2_toolchains\VulkanSDK\$VulkanVersion"))|Where-Object {$_}
$VulkanHome=$null
foreach($c in $Candidates){
  if((Test-Path (Join-Path $c "Include\vulkan\vulkan.h")) -and (Test-Path (Join-Path $c "Lib\vulkan-1.lib"))){$VulkanHome=$c;break}
}
if(-not $VulkanHome){throw "CORE0D Vulkan SDK $VulkanVersion not found"}
$env:VULKAN_SDK=$VulkanHome
$env:VK_SDK_PATH=$VulkanHome
$env:PATH=(Join-Path $VulkanHome "Bin")+";"+$env:PATH

$CMakeCmd=Get-Command cmake.exe -ErrorAction SilentlyContinue
if($CMakeCmd){$CMakeExe=$CMakeCmd.Source}else{
  $CMakeExe=Join-Path $VSInstall "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if(-not(Test-Path $CMakeExe)){throw "CORE0D cmake.exe not found"}
}

if((Test-Path $LlamaBuildDir) -and (-not $ReuseExistingLlamaBuild)){Remove-Item -Recurse -Force $LlamaBuildDir}
& $CMakeExe -S (Join-Path $Root "baseline") -B $LlamaBuildDir -G "Visual Studio 17 2022" -A x64 "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE-ne0){throw "CORE0D llama configure failed"}
& $CMakeExe --build $LlamaBuildDir --config Release --target core0d_llama_teacher_forced_adapter -- /m
if($LASTEXITCODE-ne0){throw "CORE0D llama build failed"}
$LlamaExe=Get-ChildItem $LlamaBuildDir -Recurse -Filter core0d_llama_teacher_forced_adapter.exe|Where-Object {$_.FullName-match"\Release\"}|Select-Object -First 1 -ExpandProperty FullName
if(-not $LlamaExe){throw "CORE0D llama executable missing"}
$LlamaOut=Join-Path $Artifacts "core0d_llama_teacher_forced_adapter.exe"
Copy-Item -Force $LlamaExe $LlamaOut

$TxSdk=Join-Path $TokenXrayRoot "sdk"
$TxVk=Join-Path $TxSdk "vulkan"
$CommonSources=@(
  (Join-Path $Root "src\core0d_arcllm_teacher_forced_cli.cpp"),
  (Join-Path $Root "src\gguf.cpp"),
  (Join-Path $Root "src\tensor_store.cpp"),
  (Join-Path $Root "src\arcllm_v1_primitive_registry_v2.cpp"),
  (Join-Path $Root "src\arcllm_v1_generic_policy_engine_v4.cpp"),
  (Join-Path $Root "src\arcllm_v1_generic_backend_binding_v4.cpp"),
  (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp")
)
function Build-Arc([string]$RuntimeSource,[string]$OutExe,[bool]$NeedTx){
  $srcs=@($RuntimeSource)+$CommonSources
  $srcArg=($srcs|ForEach-Object {'"'+$_+'"'}) -join ' '
  $incs='/I"'+$GeneratedDir+'" /I"'+(Join-Path $Root "src")+'" /I"'+(Join-Path $Root "include")+'"'
  if($NeedTx){$incs+=' /I"'+$TxVk+'" /I"'+$TxSdk+'"'}
  $cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj '+$incs+' /Fe:"'+$OutExe+'" '+$srcArg
  cmd.exe /d /s /c $cmd
  if($LASTEXITCODE-ne0-or-not(Test-Path $OutExe)){throw "CORE0D Arc build failed: $OutExe"}
}
$ArcControl=Join-Path $Artifacts "core0d_arcllm_control_phase.exe"
$ArcTrace=Join-Path $Artifacts "core0d_arcllm_trace.exe"
Build-Arc (Join-Path $GeneratedDir "core0d_arcllm_control_phase_runtime.cpp") $ArcControl $false
Build-Arc (Join-Path $GeneratedDir "core0d_arcllm_trace_runtime.cpp") $ArcTrace $true

& $ArcControl --self-test
if($LASTEXITCODE-ne0){throw "CORE0D Arc CLI self-test failed"}
& $LlamaOut --self-test
if($LASTEXITCODE-ne0){throw "CORE0D llama adapter self-test failed"}
py -3 (Join-Path $Root "tools\parse_core0d_llama_vk_perf.py") --self-test
if($LASTEXITCODE-ne0){throw "CORE0D llama perf parser self-test failed"}

$Manifest=Get-Content (Join-Path $GeneratedDir "CORE0D_GENERATED_RUNTIME_MANIFEST.json") -Raw|ConvertFrom-Json
$Q=[ordered]@{
  schema="arcllm.core0d.build_qualification.v0.1"
  status="PASS_CORE0D_BUILD_AND_ZERO_SCIENCE_SELF_TESTS"
  classification="ZERO_SCIENCE_IMPLEMENTATION_PREFLIGHT"
  git_head=((git -C $Root rev-parse HEAD).Trim())
  llama_head=$LlamaHead
  llama_clean=$true
  token_xray_head=$TxHead
  token_xray_clean=$true
  vulkan_sdk=$VulkanHome
  generated_runtime_manifest=$Manifest
  arc_control_phase_exe_sha256=(Get-FileHash $ArcControl -Algorithm SHA256).Hash.ToUpperInvariant()
  arc_trace_exe_sha256=(Get-FileHash $ArcTrace -Algorithm SHA256).Hash.ToUpperInvariant()
  llama_adapter_exe_sha256=(Get-FileHash $LlamaOut -Algorithm SHA256).Hash.ToUpperInvariant()
  model_inference_executed=$false
  measured_requests_executed=0
  canonical_runtime_modified=$false
  kernel_source_modified=$false
}
[IO.File]::WriteAllText((Join-Path $Results "CORE0D_BUILD_QUALIFICATION.json"),($Q|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
Write-Host "CORE0D_BUILD=PASS"
Write-Host "ARC_CONTROL_SHA=$($Q.arc_control_phase_exe_sha256)"
Write-Host "ARC_TRACE_SHA=$($Q.arc_trace_exe_sha256)"
Write-Host "LLAMA_SHA=$($Q.llama_adapter_exe_sha256)"
