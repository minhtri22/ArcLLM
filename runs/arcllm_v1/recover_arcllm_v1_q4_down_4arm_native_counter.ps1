param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$Marker=Join-Path $Root ".local\Q4_DOWN_4ARM_NATIVE_COUNTER_SCIENCE_STARTED.json"
if(-not(Test-Path $Marker)){throw "STOP: science-start marker missing; no completed collection to recover"}
$M=Get-Content $Marker -Raw|ConvertFrom-Json
$Dir=[string]$M.result_dir
if(-not(Test-Path $Dir)){throw "STOP: recorded result_dir missing: $Dir"}

$LockPath=Join-Path $Root "config\arcllm_v1_q4_down_4arm_native_counter_execution_lock_v0.2.json"
$TimingPath=Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_CANONICAL_v0.1.json"
$Parser=Join-Path $Root "tools\adjudicate_arcllm_v1_q4_down_4arm_native_counters_recovery_v0_1_2.py"
foreach($F in @($LockPath,$TimingPath,$Parser)){if(-not(Test-Path $F)){throw "STOP: recovery prerequisite missing: $F"}}
$LockHash=(Get-FileHash $LockPath -Algorithm SHA256).Hash.ToUpperInvariant()
if([string]$M.lock_sha256-ne$LockHash){throw "STOP: science-start marker lock hash mismatch"}

$Required=@()
foreach($W in @("W-S","W-C")){foreach($A in @("0","A","B","AB")){$Required+=("Q4_COUNTER_"+($W-replace'-','_')+"_"+$A+".json")}}
$RawHashes=[ordered]@{}
foreach($N in $Required){
  $P=Join-Path $Dir $N
  if(-not(Test-Path $P)){throw "STOP: required raw probe missing: $N"}
  $O=Get-Content $P -Raw|ConvertFrom-Json
  if([string]$O.status-ne"PASS_NATIVE_COUNTER_PROBE"){throw "STOP: raw probe is not PASS: $N"}
  $RawHashes[$N]=(Get-FileHash $P -Algorithm SHA256).Hash.ToUpperInvariant()
}
$Adj=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_ADJUDICATION_RECOVERY_v0.1.2.json"
py -3 $Parser --results-dir $Dir --timing-canonical $TimingPath --out $Adj
if($LASTEXITCODE-ne0){throw "STOP: recovery adjudication failed; raw counter probes remain immutable"}
$A=Get-Content $Adj -Raw|ConvertFrom-Json
if([string]$A.status-ne"PASS_NATIVE_4_COUNTER_RECOMPUTE_RECOVERY"){throw "STOP: recovery adjudication status rejected"}

foreach($N in $Required){
  $P=Join-Path $Dir $N
  $After=(Get-FileHash $P -Algorithm SHA256).Hash.ToUpperInvariant()
  if($After-ne[string]$RawHashes[$N]){throw "STOP: raw probe mutated during recovery: $N"}
}

$Meta=[ordered]@{
  schema="arcllm.v1.q4_down.4arm.native_counter.post_collection_recovery.v0.1"
  status="PASS_POST_COLLECTION_RECOVERY_ONLY"
  recovery_reason="Original adjudicator compared emitted full shader paths against basenames."
  original_collection_preserved=$true
  collector_rerun=$false
  counter_probe_rerun=$false
  primary_timing_rerun=$false
  science_start_marker_preserved=$true
  result_dir=$Dir
  recovery_head=(& git -C $Root rev-parse HEAD).Trim()
  execution_lock_sha256=$LockHash
  raw_probe_sha256=$RawHashes
  adjudication_file=[IO.Path]::GetFileName($Adj)
  token_xray=$false
  child_c=$false
}
$MetaPath=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_RECOVERY_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))

$MarkerCopy=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_SCIENCE_STARTED.json";Copy-Item $Marker $MarkerCopy -Force
$LockCopy=Join-Path $Dir "arcllm_v1_q4_down_4arm_native_counter_execution_lock_v0.2.json";Copy-Item $LockPath $LockCopy -Force
$TimingCopy=Join-Path $Dir "ARCLLM_V1_Q4_DOWN_4ARM_PRIMARY_TIMING_CANONICAL_v0.1.json";Copy-Item $TimingPath $TimingCopy -Force

$Zip=Join-Path $Dir "Q4_DOWN_4ARM_NATIVE_COUNTER_RETURN_TO_CHATGPT_RECOVERED.zip"
if(Test-Path $Zip){Remove-Item $Zip -Force}
$Files=@()
foreach($N in $Required){$Files+=(Join-Path $Dir $N)}
foreach($W in @("W_S","W_C")){foreach($A in @("0","A","B","AB")){
  foreach($Suf in @("_stdout.txt","_stderr.txt")){$P=Join-Path $Dir ("Q4_COUNTER_"+$W+"_"+$A+$Suf);if(Test-Path $P){$Files+=$P}}
}}
$Files+=@($Adj,$MetaPath,$MarkerCopy,$LockCopy,$TimingCopy)
Compress-Archive -Path $Files -DestinationPath $Zip -CompressionLevel Optimal
Write-Host "Q4_DOWN_4ARM_NATIVE_COUNTER_RECOVERY=PASS"
Write-Host "COLLECTOR_RERUN=false"
Write-Host "COUNTER_PROBE_RERUN=false"
Write-Host "PRIMARY_TIMING_RERUN=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
