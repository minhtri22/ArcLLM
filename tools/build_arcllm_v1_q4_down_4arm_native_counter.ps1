param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here;$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null
if(-not $env:VULKAN_SDK){throw "VULKAN_SDK is not set"}
$Inc=Join-Path $env:VULKAN_SDK "Include";$Lib=Join-Path $env:VULKAN_SDK "Lib"
$PF86=[Environment]::GetFolderPath("ProgramFilesX86");$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"
$SrcDir=Join-Path $Root "src";$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm_native_counter.exe"
$Sources=@((Join-Path $SrcDir "arcllm_v1_q4_down_4arm_counter_collection.cpp"),(Join-Path $SrcDir "arcllm_v1_m3_counter_shim.cpp"),(Join-Path $SrcDir "gguf.cpp"),(Join-Path $SrcDir "tensor_store.cpp"))
$SrcArgs=($Sources|ForEach-Object{'"'+$_+'"'}) -join " "
$Cmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /I"'+$Inc+'" /Fe:"'+$Exe+'" '+$SrcArgs+' /link /LIBPATH:"'+$Lib+'" vulkan-1.lib'
cmd.exe /d /s /c $Cmd
if($LASTEXITCODE-ne0-or-not(Test-Path $Exe)){throw "Q4 native counter build failed"}
$E=[ordered]@{schema="arcllm.v1.q4_down.4arm.native_counter.buildonly.v0.1";status="PASS_NATIVE_COUNTER_BUILDONLY";git_head=(& git -C $Root rev-parse HEAD).Trim();model_loaded=$false;gpu_dispatch_executed=$false;counter_probe_executed=$false;primary_timing_executed=$false;exe_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant();exe_bytes=(Get-Item $Exe).Length}
[IO.File]::WriteAllText((Join-Path $Results "Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY.json"),($E|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY=PASS"
Write-Host "EXE_SHA256=$($E.exe_sha256)"
