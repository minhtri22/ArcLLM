$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Results=Join-Path $Root "results\anl64_crt_p1"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

py -3 (Join-Path $Root "tests\test_anl64_crt_p1_static.py")
if($LASTEXITCODE-ne0){throw "ANL64 CRT P1 static QA failed"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_full_runtime_q4_v4.ps1")
if($LASTEXITCODE-ne0){throw "ANL64 CRT P1 inherited shader build failed"}

$Fast=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
if(-not(Test-Path $Fast)){throw "missing residual Q4_FAST SPIR-V"}
$FastHash=(Get-FileHash $Fast -Algorithm SHA256).Hash.ToUpperInvariant()
if($FastHash-ne"B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){
  throw "ANL64 CRT P1 Q4_FAST byte identity mismatch"
}

$PF86=${env:ProgramFiles(x86)}
if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Exe=Join-Path $Root "anl64_crt_p1_runtime.exe"
$Sources=@(
 "src\anl64_crt_p1_runtime.cpp",
 "src\gguf.cpp",
 "src\tensor_store.cpp",
 "src\arcllm_v1_primitive_registry_v2.cpp",
 "src\arcllm_v1_generic_policy_engine_v4.cpp",
 "src\arcllm_v1_generic_backend_binding_v4.cpp",
 "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp"
)|ForEach-Object{'"'+(Join-Path $Root $_)+'"'}
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj /Fe:"'+$Exe+'" '+($Sources -join ' ')
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "ANL64 CRT P1 native build failed"}

$Obj=[ordered]@{
 schema="arcllm.anl64_crt.p1.build.v0.1"
 status="PASS_STATIC_SHADER_NATIVE_BUILD"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 source_blob=((& git -C $Root rev-parse "HEAD:src/anl64_crt_p1_runtime.cpp").Trim())
 exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
 exe_bytes=(Get-Item $Exe).Length
 q4_fast_spv_sha256=$FastHash
 target_execution_started=$false
 performance_observed=$false
}
[IO.File]::WriteAllText((Join-Path $Results "BUILD.json"),($Obj|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "ANL64_CRT_P1_BUILD=PASS"
Write-Host "EXE_SHA256=$($Obj.exe_sha256)"
