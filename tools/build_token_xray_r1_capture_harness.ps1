$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

py -3 (Join-Path $Root "tests\test_token_xray_r1_representation_capture_static.py")
if($LASTEXITCODE-ne0){throw "R1 static hook audit failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_full_runtime_q4_v4.ps1")
if($LASTEXITCODE-ne0){throw "R1 runtime shader build failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$Exe=Join-Path $Root "token_xray_r1_capture_harness.exe"
$Sources=@(
 "src\arcllm_v1_runtime.cpp",
 "src\token_xray_r1_capture_harness.cpp",
 "src\gguf.cpp",
 "src\tensor_store.cpp",
 "src\arcllm_v1_primitive_registry_v2.cpp",
 "src\arcllm_v1_generic_policy_engine_v4.cpp",
 "src\arcllm_v1_generic_backend_binding_v4.cpp",
 "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp"
)|ForEach-Object{'"'+(Join-Path $Root $_)+'"'}
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj /Fe:"'+$Exe+'" '+($Sources -join ' ')
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "R1 capture harness native build failed"}

$Build=[ordered]@{
 schema="arcllm.token_xray_r1.capture_harness_build.v0.1"
 status="PASS_STATIC_SHADER_NATIVE_BUILD"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 executable="token_xray_r1_capture_harness.exe"
 exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
 exe_bytes=(Get-Item $Exe).Length
 model_executed=$false
 science_executed=$false
 performance_adjudicated=$false
}
[IO.File]::WriteAllText((Join-Path $Results "TOKEN_XRAY_R1_CAPTURE_HARNESS_BUILD.json"),($Build|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "TOKEN_XRAY_R1_CAPTURE_HARNESS_BUILD=PASS"
Write-Host "EXE_SHA256=$($Build.exe_sha256)"
