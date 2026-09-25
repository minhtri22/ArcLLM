param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here;$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){throw "ProgramFiles(x86) not available"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

$SynSrc=Join-Path $Root "tests\q4_down_exec148_synthetic.cpp";$SynExe=Join-Path $Root "q4_down_exec148_synthetic.exe"
$SynCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$SynExe+'" "'+$SynSrc+'"'
cmd.exe /d /s /c $SynCmd
if($LASTEXITCODE-ne0-or-not(Test-Path $SynExe)){throw "EXEC148 synthetic native build failed"}
$SynOut=(& $SynExe 2>&1|Out-String).Trim()
if($LASTEXITCODE-ne0-or$SynOut-notlike"EXEC148_SYNTHETIC=PASS*"){throw "EXEC148 synthetic validation failed: $SynOut"}

$Src=Join-Path $Root "src\arcllm_v1_q4_down_arch_4arm.cpp";$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Src+'" "'+(Join-Path $Root 'src\gguf.cpp')+'" "'+(Join-Path $Root 'src\tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "Q4-down 4-arm native build failed"}

$Head=(& git -C $Root rev-parse HEAD).Trim()
$Evidence=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.buildonly.v0.1"
 status="PASS_BUILDONLY_SYNTHETIC"
 git_head=$Head
 target_model_loaded=$false
 gpu_dispatch_executed=$false
 performance_authorized=$false
 hardware_counters_authorized=$false
 timing_executed=$false
 synthetic_output=$SynOut
 hashes=[ordered]@{
  harness_exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  synthetic_exe_sha256=(Get-FileHash $SynExe -Algorithm SHA256).Hash.ToUpperInvariant()
 }
}
[IO.File]::WriteAllText((Join-Path $Results "q4_down_4arm_buildonly.json"),($Evidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_DOWN_4ARM_NATIVE_BUILD=PASS"
Write-Host $SynOut
