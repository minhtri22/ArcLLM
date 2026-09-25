param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_primary_timing_execution_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "STOP: candidate timing execution lock missing"}
$L=Get-Content $LockPath -Raw|ConvertFrom-Json
if(-not[bool]$L.authorization.buildonly_authorized){throw "STOP: build-only not authorized"}
if([bool]$L.authorization.execution_authorized){throw "STOP: candidate lock must not authorize timing execution"}
if([bool]$L.authorization.hardware_counters -or [bool]$L.authorization.token_xray -or [bool]$L.authorization.child_c){throw "STOP: forbidden scope opened"}
git -C $Root fetch origin | Out-Null
$Head=(& git -C $Root rev-parse HEAD).Trim()
$Remote=(& git -C $Root rev-parse origin/research/arcllm-v1).Trim()
if($Head-ne$Remote){throw "STOP: worktree HEAD does not match remote research/arcllm-v1"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){$Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim();if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: critical package blob mismatch: $($P.Name)"}}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\qualify_arcllm_v1_q4_down_4arm_primary_timing_buildonly.ps1")
if($LASTEXITCODE-ne0){throw "STOP: primary timing zero-science QA failed"}
$BPath=Join-Path $Root "results\Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY.json"
$B=Get-Content $BPath -Raw|ConvertFrom-Json
if([string]$B.status-ne"PASS_PRIMARY_TIMING_BUILDONLY"){throw "STOP: native build-only status not PASS"}
if($B.target_model_loaded-or$B.gpu_dispatch_executed-or$B.timing_executed-or$B.hardware_counters_executed){throw "STOP: zero-science boundary violated"}
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\q4_down_4arm_primary_timing_buildonly_"+$Stamp);New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Files=@($BPath,(Join-Path $Root "results\q4_down_4arm_shader_provenance.json"),$LockPath,(Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_STATIC_AUDIT_v0.1.json"),(Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_GITHUB_CI_INFRA_BLOCKER_v0.1.json"),(Join-Path $Root "arcllm_v1_q4_down_4arm_primary_timing.exe"))
foreach($F in $Files){if(-not(Test-Path $F)){throw "STOP: build-only evidence missing: $F"};Copy-Item $F $Dir -Force}
$Return=[ordered]@{schema="arcllm.v1.q4_down.4arm.primary_timing.dev_host_buildonly_return.v0.1";status="PASS_DEV_HOST_ZERO_SCIENCE_BUILDONLY";head=$Head;exe_sha256=[string]$B.exe_sha256;exe_bytes=[int64]$B.exe_bytes;target_model_loaded=$false;gpu_dispatch_executed=$false;timing_executed=$false;hardware_counters_executed=$false;execution_authorized=$false;next="RETURN_BUNDLE_FOR_INDEPENDENT_REVIEW_AND_FINAL_EXECUTION_LOCK_V0_2"}
$ReturnPath=Join-Path $Dir "Q4_DOWN_4ARM_PRIMARY_TIMING_DEV_HOST_BUILDONLY_RETURN.json"
[IO.File]::WriteAllText($ReturnPath,($Return|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$Zip=Join-Path $Dir "Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_4ARM_PRIMARY_TIMING_BUILDONLY=PASS"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO PRIMARY TIMING. COUNTERS/TOKEN-XRAY/CHILD-C CLOSED."
