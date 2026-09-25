param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here;$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null
$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)");if(-not $PF86){throw "ProgramFiles(x86) not available"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe";if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$Src=Join-Path $Root "src\arcllm_v1_q4_down_4arm_primary_timing.cpp";$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm_primary_timing.exe"
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Fe:"'+$Exe+'" "'+$Src+'" "'+(Join-Path $Root 'src\gguf.cpp')+'" "'+(Join-Path $Root 'src\tensor_store.cpp')+'"'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "Q4-down primary timing native build failed"}
$Evidence=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.primary_timing.buildonly.v0.1"
 status="PASS_PRIMARY_TIMING_BUILDONLY"
 git_head=(& git -C $Root rev-parse HEAD).Trim()
 target_model_loaded=$false;gpu_dispatch_executed=$false;timing_executed=$false;hardware_counters_executed=$false
 exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
 exe_bytes=(Get-Item $Exe).Length
}
[IO.File]::WriteAllText((Join-Path $Results "Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY.json"),($Evidence|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY=PASS"
Write-Host "EXE_SHA256=$($Evidence.exe_sha256)"
