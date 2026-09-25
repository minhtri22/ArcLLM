param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_native_counter_execution_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "STOP: Q4 native-counter candidate lock missing"}
$L=Get-Content $LockPath -Raw|ConvertFrom-Json
if(-not[bool]$L.authorization.buildonly_authorized -or [bool]$L.authorization.counter_execution_authorized){throw "STOP: invalid candidate authorization"}
if([bool]$L.authorization.primary_timing -or [bool]$L.authorization.token_xray -or [bool]$L.authorization.child_c){throw "STOP: forbidden scope opened"}
git -C $Root fetch origin | Out-Null
$Head=(& git -C $Root rev-parse HEAD).Trim();$Remote=(& git -C $Root rev-parse origin/research/arcllm-v1).Trim()
if($Head-ne$Remote){throw "STOP: local HEAD != remote research/arcllm-v1"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){$Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim();if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: critical blob mismatch: $($P.Name)"}}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\qualify_arcllm_v1_q4_down_4arm_native_counter_buildonly.ps1")
if($LASTEXITCODE-ne0){throw "STOP: Q4 native-counter zero-science QA failed"}
$BPath=Join-Path $Root "results\Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY.json";$B=Get-Content $BPath -Raw|ConvertFrom-Json
if([string]$B.status-ne"PASS_NATIVE_COUNTER_BUILDONLY"){throw "STOP: build-only status not PASS"}
if($B.model_loaded-or$B.gpu_dispatch_executed-or$B.counter_probe_executed-or$B.primary_timing_executed){throw "STOP: zero-science boundary violated"}
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ");$Dir=Join-Path $Root ("results\q4_down_4arm_native_counter_buildonly_"+$Stamp);New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Files=@($BPath,(Join-Path $Root "results\q4_down_4arm_shader_provenance.json"),$LockPath,(Join-Path $Root "arcllm_v1_q4_down_4arm_native_counter.exe"))
foreach($F in $Files){if(-not(Test-Path $F)){throw "STOP: build-only evidence missing: $F"};Copy-Item $F $Dir -Force}
$R=[ordered]@{schema="arcllm.v1.q4_down.4arm.native_counter.dev_host_buildonly_return.v0.1";status="PASS_DEV_HOST_NATIVE_COUNTER_ZERO_SCIENCE_BUILDONLY";head=$Head;exe_sha256=[string]$B.exe_sha256;exe_bytes=[int64]$B.exe_bytes;model_loaded=$false;gpu_dispatch_executed=$false;counter_probe_executed=$false;primary_timing_executed=$false;counter_execution_authorized=$false;next="INDEPENDENT_REVIEW_THEN_FINAL_COUNTER_EXECUTION_LOCK_V0_2"}
$RP=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_DEV_HOST_BUILDONLY_RETURN.json";[IO.File]::WriteAllText($RP,($R|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$Zip=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY_RETURN_TO_CHATGPT.zip";Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_4ARM_NATIVE_COUNTER_BUILDONLY=PASS"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO COUNTER PROBE. NO PRIMARY TIMING."
