param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\qualify_arcllm_v1_q4_down_4arm_buildonly.ps1")
if($LASTEXITCODE-ne0){throw "Q4-down build-only qualification failed"}

$Results=Join-Path $Root "results"
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Results ("q4_down_4arm_buildonly_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

$Required=@(
 "ARCLLM_V1_Q4_DOWN_4ARM_BUILDONLY_QA.json",
 "q4_down_4arm_buildonly.json",
 "q4_down_4arm_shader_provenance.json"
)
foreach($N in $Required){
 $P=Join-Path $Results $N
 if(-not(Test-Path $P)){throw "Missing build-only evidence: $P"}
 Copy-Item $P (Join-Path $Dir $N) -Force
}
Copy-Item (Join-Path $Root "config\arcllm_v1_q4_down_4arm_correctness_lock_v0.1.json") $Dir -Force

$Meta=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.dev_host_buildonly_return.v0.1"
 status="PASS_STATIC_SYNTHETIC_BUILDONLY"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 target_model_loaded=$false
 gpu_dispatch_executed=$false
 performance_authorized=$false
 hardware_counters_authorized=$false
 timing_executed=$false
 next="RETURN_BUNDLE_FOR_INDEPENDENT_REVIEW_BEFORE_REAL_MODEL_CORRECTNESS"
}
$MetaPath=Join-Path $Dir "Q4_DOWN_4ARM_DEV_HOST_BUILDONLY_RETURN.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Dir "Q4_DOWN_4ARM_BUILDONLY_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force
Write-Host "Q4_DOWN_4ARM_BUILDONLY=PASS"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO TIMING. PERFORMANCE REMAINS FORBIDDEN."
