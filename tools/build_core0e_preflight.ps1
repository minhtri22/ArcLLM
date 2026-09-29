param(
  [string]$TokenXrayRoot="D:\WORK\RESEARCH\_token_xray_core0a_freeze",
  [string]$LlamaDir="D:\WORK\_llama_core0b",
  [string]$GeneratedDir="D:\WORK\_core0e_generated",
  [string]$LlamaBuildDir="D:\WORK\_core0d_llama_build"
)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Artifacts=Join-Path $Root "artifacts\core0e"; $Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Artifacts,$Results|Out-Null
$PinnedLlama="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"; $PinnedTx="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
if((git -C $LlamaDir rev-parse HEAD).Trim()-ne$PinnedLlama){throw "CORE0E llama HEAD mismatch"}
if((git -C $TokenXrayRoot rev-parse HEAD).Trim()-ne$PinnedTx){throw "CORE0E Token-XRay HEAD mismatch"}
if(((git -C $LlamaDir status --porcelain)|Out-String).Trim()){throw "CORE0E llama checkout dirty"}
if(((git -C $TokenXrayRoot status --porcelain)|Out-String).Trim()){throw "CORE0E Token-XRay checkout dirty"}

if(Test-Path $GeneratedDir){Remove-Item -Recurse -Force $GeneratedDir}
py -3 (Join-Path $Root "tools\materialize_core0e_runtime.py") --out-dir $GeneratedDir
if($LASTEXITCODE-ne0){throw "CORE0E materializer failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)"); if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "CORE0E VS Build Tools missing"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$VulkanVersion="1.4.357.0"; $VulkanHome=$null
foreach($c in @($env:VULKAN_SDK,"C:\VulkanSDK\$VulkanVersion",(Join-Path $Root ".q2_toolchains\VulkanSDK\$VulkanVersion"))|Where-Object {$_}){
 if((Test-Path (Join-Path $c "Include\vulkan\vulkan.h"))-and(Test-Path (Join-Path $c "Lib\vulkan-1.lib"))){$VulkanHome=$c;break}
}
if(-not $VulkanHome){throw "CORE0E Vulkan SDK missing"}
$env:VULKAN_SDK=$VulkanHome;$env:VK_SDK_PATH=$VulkanHome;$env:PATH=(Join-Path $VulkanHome "Bin")+";"+$env:PATH
$CMake=(Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
if(-not $CMake){$CMake=Join-Path $VSInstall "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"}

& $CMake -S (Join-Path $Root "baseline") -B $LlamaBuildDir -G "Visual Studio 17 2022" -A x64 "-DLLAMA_CPP_DIR=$LlamaDir"
if($LASTEXITCODE-ne0){throw "CORE0E llama configure failed"}
& $CMake --build $LlamaBuildDir --config Release --target core0e_llama_combined_phase_trace_adapter -- /m:1 /nodeReuse:false
if($LASTEXITCODE-ne0){throw "CORE0E llama build failed"}
$LlamaExe=Get-ChildItem $LlamaBuildDir -Recurse -Filter core0e_llama_combined_phase_trace_adapter.exe|Select-Object -First 1 -ExpandProperty FullName
$LlamaOut=Join-Path $Artifacts "core0e_llama_combined_phase_trace_adapter.exe"; Copy-Item -Force $LlamaExe $LlamaOut

$Common=@(
 (Join-Path $Root "src\core0e_arcllm_teacher_forced_cli.cpp"),
 (Join-Path $Root "src\gguf.cpp"),(Join-Path $Root "src\tensor_store.cpp"),
 (Join-Path $Root "src\arcllm_v1_primitive_registry_v2.cpp"),
 (Join-Path $Root "src\arcllm_v1_generic_policy_engine_v4.cpp"),
 (Join-Path $Root "src\arcllm_v1_generic_backend_binding_v4.cpp"),
 (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp")
)
$TxSdk=Join-Path $TokenXrayRoot "sdk";$TxVk=Join-Path $TxSdk "vulkan"
$ArcOut=Join-Path $Artifacts "core0e_arcllm_combined.exe"
$srcs=@((Join-Path $GeneratedDir "core0e_arcllm_combined_runtime.cpp"))+$Common
$srcArg=($srcs|ForEach-Object {'"'+$_+'"'}) -join ' '
$incs='/I"'+$GeneratedDir+'" /I"'+(Join-Path $Root "src")+'" /I"'+(Join-Path $Root "include")+'" /I"'+$TxVk+'" /I"'+$TxSdk+'"'
$cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj '+$incs+' /Fe:"'+$ArcOut+'" '+$srcArg
cmd.exe /d /s /c $cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $ArcOut)){throw "CORE0E Arc combined build failed"}

& $ArcOut --self-test; if($LASTEXITCODE-ne0){throw "CORE0E Arc self-test failed"}
& $LlamaOut --self-test; if($LASTEXITCODE-ne0){throw "CORE0E llama self-test failed"}
py -3 (Join-Path $Root "tools\parse_core0d_llama_vk_perf.py") --self-test; if($LASTEXITCODE-ne0){throw "CORE0E vk parser self-test failed"}
py -3 (Join-Path $Root "tools\core0e_phase_contract.py") --self-test; if($LASTEXITCODE-ne0){throw "CORE0E phase self-test failed"}

$Manifest=Get-Content (Join-Path $GeneratedDir "CORE0E_GENERATED_RUNTIME_MANIFEST.json") -Raw|ConvertFrom-Json
$q=[ordered]@{
 schema="arcllm.core0e.build_qualification.v0.1";status="PASS_CORE0E_BUILD_AND_ZERO_SCIENCE_SELF_TESTS";
 classification="ZERO_SCIENCE_IMPLEMENTATION_PREFLIGHT";git_head=(git -C $Root rev-parse HEAD).Trim();
 llama_head=$PinnedLlama;token_xray_head=$PinnedTx;generated_runtime_manifest=$Manifest;
 arc_combined_exe_sha256=(Get-FileHash $ArcOut -Algorithm SHA256).Hash.ToUpperInvariant();
 llama_combined_exe_sha256=(Get-FileHash $LlamaOut -Algorithm SHA256).Hash.ToUpperInvariant();
 model_inference_executed=$false;measured_requests_executed=0;canonical_runtime_modified=$false;kernel_shader_modified=$false
}
[IO.File]::WriteAllText((Join-Path $Results "CORE0E_BUILD_QUALIFICATION.json"),($q|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
Write-Host "CORE0E_BUILD=PASS";Write-Host ("ARC_COMBINED_SHA="+$q.arc_combined_exe_sha256);Write-Host ("LLAMA_COMBINED_SHA="+$q.llama_combined_exe_sha256)
