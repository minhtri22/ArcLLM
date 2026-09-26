param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

$PrevEap=$ErrorActionPreference
$ErrorActionPreference="Continue"
$StaticOut=(py -3 (Join-Path $Root "tests\test_arcllm_v1_b1_2_performance_lock.py") 2>&1|Out-String).Trim()
$StaticCode=$LASTEXITCODE
$ErrorActionPreference=$PrevEap
if($StaticCode-ne0){throw "B1.2 performance-lock static QA failed (`$StaticCode=$StaticCode): `n$StaticOut"}
Write-Host $StaticOut

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_b1_2_zero_science.ps1")
if($LASTEXITCODE-ne0){throw "B1.2 performance inherited shader compile failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){throw "ProgramFiles(x86) not available"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$Src=Join-Path $Root "src\arcllm_v1_b1_2_performance.cpp"
$Exe=Join-Path $Root "arcllm_v1_b1_2_performance.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Src+'" "'+(Join-Path $Root 'src\gguf.cpp')+'" "'+(Join-Path $Root 'src\tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "B1.2 performance native build failed"}

$Prov=Get-Content (Join-Path $Results "B1_2_ZERO_SCIENCE_SHADER_PROVENANCE.json") -Raw|ConvertFrom-Json
$Evidence=[ordered]@{
 schema="arcllm.v1.b1_2.performance.buildonly.v0.1"
 status="PASS_PERFORMANCE_WRAPPER_BUILDONLY"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 execution_authorized=$false
 model_loaded=$false
 gpu_dispatch=$false
 sidecar_io=$false
 performance_execution=$false
 blobs=[ordered]@{
  performance_source=((& git -C $Root rev-parse "HEAD:src/arcllm_v1_b1_2_performance.cpp").Trim())
  zero_science_source=((& git -C $Root rev-parse "HEAD:src/arcllm_v1_b1_2_zero_science.cpp").Trim())
  transitive_runtime=((& git -C $Root rev-parse "HEAD:src/arcllm_v1_q4_down_4arm_timing_runtime.cpp").Trim())
  p1_shader_source=((& git -C $Root rev-parse "HEAD:shaders/b1_2_exec148_gpu_materialize.comp").Trim())
 }
 compiled=[ordered]@{
  exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  exe_bytes=(Get-Item $Exe).Length
  p1_spv_sha256=[string]$Prov.p1.spv_sha256
  p1_spv_bytes=[int64]$Prov.p1.spv_bytes
  q4_reference_spv_sha256=[string]$Prov.inherited.base_q4_spv_sha256
  b_serial_spv_sha256=[string]$Prov.inherited.b_exec148_serial_spv_sha256
 }
}
[IO.File]::WriteAllText((Join-Path $Results "B1_2_PERFORMANCE_BUILDONLY.json"),($Evidence|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
Write-Host "B1_2_PERFORMANCE_BUILDONLY=PASS"
Write-Host "EXE_SHA256=$($Evidence.compiled.exe_sha256)"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO SIDECAR IO. NO PERFORMANCE EXECUTION."
