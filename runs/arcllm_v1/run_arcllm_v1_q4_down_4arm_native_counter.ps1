param([switch]$QuietHostConfirmed,[string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_native_counter_execution_lock_v0.2.json"
if(-not(Test-Path $LockPath)){throw "STOP: final native-counter execution lock v0.2 missing"}
$L=Get-Content $LockPath -Raw|ConvertFrom-Json
if(-not[bool]$L.authorization.counter_execution_authorized){throw "STOP: counter execution not authorized"}
if([bool]$L.authorization.primary_timing -or [bool]$L.authorization.token_xray -or [bool]$L.authorization.child_c){throw "STOP: forbidden scope opened"}
if(-not $QuietHostConfirmed){throw "STOP: exact native-counter campaign requires -QuietHostConfirmed"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String);if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){$Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim();if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: critical blob mismatch: $($P.Name)"}}
py -3 (Join-Path $Root "tests\test_arcllm_v1_q4_down_4arm_native_counter.py");if($LASTEXITCODE-ne0){throw "STOP: static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1");if($LASTEXITCODE-ne0){throw "STOP: shader provenance failed"}
$Exe=Join-Path $Root "arcllm_v1_q4_down_4arm_native_counter.exe"
if(-not(Test-Path $Exe)){throw "STOP: qualified counter executable missing"}
if((Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$L.qualified_build.exe_sha256){throw "STOP: counter executable hash mismatch"}
if((Get-Item $Exe).Length-ne[int64]$L.qualified_build.exe_bytes){throw "STOP: counter executable size mismatch"}
$OS=Get-CimInstance Win32_OperatingSystem;$CPU=@(Get-CimInstance Win32_Processor);$GPU=@(Get-CimInstance Win32_VideoController)
if([string]$OS.BuildNumber-ne[string]$L.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object{$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object{$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$L.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}
if(-not $ModelPath){$Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot);if($Resolved.Count-lt1){throw "STOP: model resolver failed"};$ModelPath=[string]$Resolved[-1]}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$L.target_model.sha256 -or (Get-Item $ModelPath).Length-ne[int64]$L.target_model.bytes){throw "STOP: target model mismatch"}
$Marker=Join-Path $Root ".local\Q4_DOWN_4ARM_NATIVE_COUNTER_SCIENCE_STARTED.json"
if(Test-Path $Marker){throw "STOP: counter science-start marker already exists; no rerun/selective rerun allowed"}
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ");$Dir=Join-Path $Root ("results\q4_down_4arm_native_counter_"+$Stamp);New-Item -ItemType Directory -Force -Path $Dir|Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Marker)|Out-Null
$Mark=[ordered]@{schema="arcllm.v1.q4_down.4arm.native_counter.science_start.v0.1";started_utc=(Get-Date).ToUniversalTime().ToString("o");result_dir=$Dir;lock_sha256=(Get-FileHash $LockPath -Algorithm SHA256).Hash.ToUpperInvariant()}
[IO.File]::WriteAllText($Marker,($Mark|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
function Invoke-Captured([string]$W,[string]$A){
 $Stem="Q4_COUNTER_"+($W-replace'-','_')+"_"+$A;$Out=Join-Path $Dir ($Stem+".json");$Stdout=Join-Path $Dir ($Stem+"_stdout.txt");$Stderr=Join-Path $Dir ($Stem+"_stderr.txt")
 $Args='--model "'+$ModelPath+'" --shader-dir "'+(Join-Path $Root "compiled_shaders")+'" --implementation-commit '+[string]$L.qualified_build.source_head+' --workload '+$W+' --arm '+$A+' --out "'+$Out+'"'
 Write-Host "Q4 COUNTER BEGIN workload=$W arm=$A"
 $P=Start-Process -FilePath $Exe -ArgumentList $Args -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -NoNewWindow -Wait -PassThru
 if($P.ExitCode-ne0){throw "STOP: counter probe failed workload=$W arm=$A; no rerun"}
 $R=Get-Content $Out -Raw|ConvertFrom-Json;if([string]$R.status-ne"PASS_NATIVE_COUNTER_PROBE"){throw "STOP: invalid counter result workload=$W arm=$A"}
 Write-Host "Q4 COUNTER END workload=$W arm=$A passes=$($R.counter_probe.pass_count)"
}
foreach($W in @("W-S","W-C")){foreach($A in @("0","A","B","AB")){Invoke-Captured $W $A}}
$Adj=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_ADJUDICATION.json"
py -3 (Join-Path $Root "tools\adjudicate_arcllm_v1_q4_down_4arm_native_counters.py") --results-dir $Dir --timing-canonical (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_CANONICAL_v0.1.json") --out $Adj
if($LASTEXITCODE-ne0){throw "STOP: counter recompute failed; no rerun"}
$Meta=[ordered]@{schema="arcllm.v1.q4_down.4arm.native_counter.run_meta.v0.1";status="PASS";quiet_host_confirmed=$true;probes=8;decode_index=15;counter_indices=@(56,66,64,233);primary_timing_rerun=$false;token_xray=$false;child_c=$false}
$MP=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_RUN_META.json";[IO.File]::WriteAllText($MP,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Copy-Item $LockPath $Dir -Force;Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_CANONICAL_v0.1.json") $Dir -Force
$Zip=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_RETURN_TO_CHATGPT.zip";Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_4ARM_NATIVE_COUNTER=PASS_COLLECTION"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "PRIMARY_TIMING_NOT_RERUN; TOKEN_XRAY_CHILD_C_CLOSED"
