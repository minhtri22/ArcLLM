param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

$PrevEap=$ErrorActionPreference
$ErrorActionPreference="Continue"
$StaticOut=(py -3 (Join-Path $Root "tests\test_arcllm_v1_b1_2_zero_science.py") 2>&1|Out-String).Trim()
$StaticCode=$LASTEXITCODE
$ErrorActionPreference=$PrevEap
if($StaticCode-ne0){throw "B1.2 static QA failed (`$StaticCode=$StaticCode): `n$StaticOut"}
Write-Host $StaticOut

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_b1_2_zero_science.ps1")
if($LASTEXITCODE-ne0){throw "B1.2 shader compile failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){throw "ProgramFiles(x86) not available"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$Src=Join-Path $Root "src\arcllm_v1_b1_2_zero_science.cpp"
$Exe=Join-Path $Root "arcllm_v1_b1_2_zero_science.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Src+'" "'+(Join-Path $Root 'src\gguf.cpp')+'" "'+(Join-Path $Root 'src\tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "B1.2 native build failed"}

$Evidence=[ordered]@{
 schema="arcllm.v1.b1_2.zero_science.build.v0.1"
 status="PASS_STATIC_SHADER_NATIVE_BUILD"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 source_git_blob=((& git -C $Root rev-parse "HEAD:src/arcllm_v1_b1_2_zero_science.cpp").Trim())
 performance_authorized=$false
 acquisition_timing_authorized=$false
 target_model_loaded=$false
 gpu_dispatch_executed=$false
 sidecar_io_executed=$false
 hashes=[ordered]@{
  exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  exe_bytes=(Get-Item $Exe).Length
 }
}
[IO.File]::WriteAllText((Join-Path $Results "B1_2_ZERO_SCIENCE_BUILD.json"),($Evidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "B1_2_ZERO_SCIENCE_BUILD=PASS"
Write-Host "EXE_SHA256=$($Evidence.hashes.exe_sha256)"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO SIDECAR IO. NO ACQUISITION TIMING."
