param(
  [string]$TokenXrayRoot = "D:\WORK\RESEARCH\_token_xray_core0a_freeze",
  [string]$GeneratedDir = "D:\WORK\_core0c_generated"
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Results=Join-Path $Root "results"
$Artifacts=Join-Path $Root "artifacts\core0c_trace"
$FixtureDir=Join-Path $Results "core0c_fixture"
New-Item -ItemType Directory -Force -Path $Results,$Artifacts,$FixtureDir|Out-Null

$ExpectedTx="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
if(-not(Test-Path (Join-Path $TokenXrayRoot ".git"))){throw "CORE0C Token-XRay checkout missing"}
$TxHead=(git -C $TokenXrayRoot rev-parse HEAD).Trim()
if($TxHead-ne$ExpectedTx){throw "CORE0C Token-XRay HEAD mismatch: $TxHead"}
$TxDirty=((git -C $TokenXrayRoot status --porcelain)|Out-String).Trim()
if($TxDirty){throw "CORE0C Token-XRay checkout must be clean"}

if(Test-Path $GeneratedDir){Remove-Item -Recurse -Force $GeneratedDir}
py -3 (Join-Path $Root "tools\materialize_core0c_trace_runtime.py") --out-dir $GeneratedDir
if($LASTEXITCODE-ne0){throw "CORE0C materializer failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "CORE0C vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "CORE0C Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$VulkanVersion="1.4.357.0"
$Candidates=@(
  $env:VULKAN_SDK,
  "C:\VulkanSDK\$VulkanVersion",
  (Join-Path $Root ".q2_toolchains\VulkanSDK\$VulkanVersion")
) | Where-Object {$_}
$VulkanHome=$null
foreach($c in $Candidates){
  if((Test-Path (Join-Path $c "Include\vulkan\vulkan.h")) -and
     (Test-Path (Join-Path $c "Lib\vulkan-1.lib"))){
    $VulkanHome=$c;break
  }
}
if(-not $VulkanHome){throw "CORE0C Vulkan SDK $VulkanVersion not found"}

$ProbeExe=Join-Path $Artifacts "core0c_vulkan_timestamp_probe.exe"
$ProbeSrc=Join-Path $Root "tools\core0c_vulkan_timestamp_probe.cpp"
$ProbeCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $VulkanHome "Include")+'" /Fe:"'+$ProbeExe+'" "'+$ProbeSrc+'" /link /LIBPATH:"'+(Join-Path $VulkanHome "Lib")+'" vulkan-1.lib'
cmd.exe /d /s /c $ProbeCmd
if($LASTEXITCODE-ne0-or-not(Test-Path $ProbeExe)){throw "CORE0C timestamp probe build failed"}
$ProbeOut=Join-Path $Results "CORE0C_VULKAN_TIMESTAMP_PROBE.json"
& $ProbeExe | Set-Content -Encoding utf8 $ProbeOut
if($LASTEXITCODE-ne0){throw "CORE0C timestamp probe execution failed"}
$Probe=Get-Content $ProbeOut -Raw | ConvertFrom-Json
if($Probe.status-ne"PASS"-or[double]$Probe.timestamp_period_ns-le0-or[int]$Probe.timestamp_valid_bits-le0){
  throw "CORE0C timestamp probe invalid"
}

$TxSdk=Join-Path $TokenXrayRoot "sdk"
$TxVk=Join-Path $TxSdk "vulkan"
$FixtureExe=Join-Path $Artifacts "core0c_token_xray_fixture_smoke.exe"
$FixtureSrc=Join-Path $Root "tests\core0c_token_xray_fixture_smoke.cpp"
$FixtureCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+(Join-Path $Root "src")+'" /I"'+$TxVk+'" /I"'+$TxSdk+'" /Fe:"'+$FixtureExe+'" "'+$FixtureSrc+'"'
cmd.exe /d /s /c $FixtureCmd
if($LASTEXITCODE-ne0-or-not(Test-Path $FixtureExe)){throw "CORE0C fixture smoke build failed"}

Remove-Item -Recurse -Force $FixtureDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $FixtureDir|Out-Null
$env:ARCLLM_CORE0C_TRACE_DIR=$FixtureDir
$env:ARCLLM_CORE0C_RUN_ID="core0c-fixture"
$env:ARCLLM_CORE0C_REQUEST_ID="core0c-fixture-request"
$env:ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS=([string]$Probe.timestamp_period_ns)
$env:ARCLLM_CORE0C_TIMESTAMP_VALID_BITS=([string]$Probe.timestamp_valid_bits)
& $FixtureExe
if($LASTEXITCODE-ne0){throw "CORE0C fixture smoke execution failed"}
Remove-Item Env:ARCLLM_CORE0C_TRACE_DIR,Env:ARCLLM_CORE0C_RUN_ID,Env:ARCLLM_CORE0C_REQUEST_ID,Env:ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS,Env:ARCLLM_CORE0C_TIMESTAMP_VALID_BITS -ErrorAction SilentlyContinue

$TraceExe=Join-Path $Artifacts "arcllm_v1_core0c_trace.exe"
$Sources=@(
  (Join-Path $GeneratedDir "core0c_arcllm_v1_runtime.cpp"),
  (Join-Path $Root "src\arcllm_v1_runtime_cli.cpp"),
  (Join-Path $Root "src\gguf.cpp"),
  (Join-Path $Root "src\tensor_store.cpp"),
  (Join-Path $Root "src\arcllm_v1_primitive_registry_v2.cpp"),
  (Join-Path $Root "src\arcllm_v1_generic_policy_engine_v4.cpp"),
  (Join-Path $Root "src\arcllm_v1_generic_backend_binding_v4.cpp"),
  (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp")
) | ForEach-Object {'"'+$_+'"'}
$Includes=@(
  '/I"'+$GeneratedDir+'"',
  '/I"'+(Join-Path $Root "src")+'"',
  '/I"'+(Join-Path $Root "include")+'"',
  '/I"'+$TxVk+'"',
  '/I"'+$TxSdk+'"'
) -join ' '
$TraceCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj '+$Includes+' /Fe:"'+$TraceExe+'" '+($Sources -join ' ')
cmd.exe /d /s /c $TraceCmd
if($LASTEXITCODE-ne0-or-not(Test-Path $TraceExe)){throw "CORE0C diagnostic runtime build failed"}

$ManifestPath=Join-Path $GeneratedDir "CORE0C_GENERATED_RUNTIME_MANIFEST.json"
$Manifest=Get-Content $ManifestPath -Raw | ConvertFrom-Json
$Build=[ordered]@{
  schema="arcllm.core0c.build_qualification.v0.1"
  status="PASS_CORE0C_BUILD_AND_SYNTHETIC_FIXTURE"
  classification="ZERO_SCIENCE_IMPLEMENTATION_PREFLIGHT"
  git_head=((git -C $Root rev-parse HEAD).Trim())
  token_xray_head=$TxHead
  token_xray_clean=$true
  vulkan_sdk=$VulkanHome
  vulkan_device=$Probe.device_name
  timestamp_period_ns=[double]$Probe.timestamp_period_ns
  timestamp_valid_bits=[int]$Probe.timestamp_valid_bits
  timestamp_probe_sha256=(Get-FileHash $ProbeExe -Algorithm SHA256).Hash.ToUpperInvariant()
  fixture_exe_sha256=(Get-FileHash $FixtureExe -Algorithm SHA256).Hash.ToUpperInvariant()
  trace_exe_sha256=(Get-FileHash $TraceExe -Algorithm SHA256).Hash.ToUpperInvariant()
  generated_runtime_manifest=$Manifest
  synthetic_fixture_files=(Get-ChildItem $FixtureDir -File | Sort-Object Name | Select-Object -ExpandProperty Name)
  model_inference_executed=$false
  measured_requests_executed=0
  canonical_runtime_modified=$false
  kernel_source_modified=$false
}
$BuildPath=Join-Path $Results "CORE0C_BUILD_QUALIFICATION.json"
[IO.File]::WriteAllText($BuildPath,($Build|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
Write-Host "CORE0C_BUILD=PASS"
Write-Host "TIMESTAMP_PERIOD_NS=$($Build.timestamp_period_ns)"
Write-Host "TIMESTAMP_VALID_BITS=$($Build.timestamp_valid_bits)"
Write-Host "TRACE_EXE_SHA256=$($Build.trace_exe_sha256)"
Write-Host "FIXTURE_FILES=$($Build.synthetic_fixture_files.Count)"
