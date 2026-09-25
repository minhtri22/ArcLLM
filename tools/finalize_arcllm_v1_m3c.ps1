param(
  [string]$ResultsDir
)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$LockPath=Join-Path $Root "config\arcllm_v1_m3c_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "M3-C lock missing"}
$Lock=Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $ResultsDir){$ResultsDir=Join-Path $Root "results\m3c_targeted_mechanism"}
if(-not(Test-Path $ResultsDir)){throw "M3-C results directory missing: $ResultsDir"}

$RawNames=@(
  "M3C_W_S_memory_cache.json",
  "M3C_W_S_execution_occupancy.json",
  "M3C_W_S_stall_cause.json",
  "M3C_W_C_memory_cache.json",
  "M3C_W_C_execution_occupancy.json",
  "M3C_W_C_stall_cause.json"
)
$CollectionHeads=@()
foreach($Name in $RawNames){
  $Path=Join-Path $ResultsDir $Name
  if(-not(Test-Path $Path)){throw "M3-C raw result missing: $Name"}
  $Obj=Get-Content $Path -Raw -Encoding UTF8 | ConvertFrom-Json
  if($Obj.status -ne "PASS"){throw "M3-C raw result not PASS: $Name"}
  if($Obj.counter_probe.logical_dispatches -ne 469 -or $Obj.counter_probe.queried_dispatches -ne 103){
    throw "M3-C raw census mismatch: $Name"
  }
  if(@($Obj.dispatches).Count -ne 103){throw "M3-C raw queried dispatch count mismatch: $Name"}
  $CollectionHeads += [string]$Obj.implementation_commit
}
$UniqueHeads=@($CollectionHeads | Sort-Object -Unique)
if($UniqueHeads.Count -ne 1){throw "M3-C raw results disagree on collection implementation commit"}
$CollectionHead=[string]$UniqueHeads[0]
if($CollectionHead -ne "9ace722cea2e51001d47834ce09a65aa7c1ca7a1"){
  throw "M3-C rescue expected collection HEAD 9ace722..., got $CollectionHead"
}

$Summary=Join-Path $ResultsDir "M3C_COLLECTION_SUMMARY.json"
$Mechanism=Join-Path $ResultsDir "M3C_MECHANISM_DISCRIMINATION.json"
$Obs=Join-Path $ResultsDir "HARDWARE_OBSERVATIONS.jsonl"
foreach($P in @($Summary,$Mechanism,$Obs)){if(-not(Test-Path $P)){throw "M3-C finalized evidence missing: $P"}}

$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.status -ne "PASS" -or $S.raw_runs -ne 6 -or $S.dispatch_observations -ne 618 -or $S.queried_dispatches_per_run -ne 103){
  throw "M3-C collection summary rejected during finalize-only recovery"
}
$M=Get-Content $Mechanism -Raw -Encoding UTF8 | ConvertFrom-Json
if($M.status -ne "PASS"){throw "M3-C mechanism discrimination is not PASS"}

$ObsCount=(Get-Content $Obs | Measure-Object -Line).Lines
if($ObsCount -ne 618){throw "M3-C HARDWARE_OBSERVATIONS expected 618 lines; got $ObsCount"}

$FinalizationHead=(git -C $Root rev-parse HEAD).Trim()
$Meta=[ordered]@{
  schema="arcllm.v1.m3c.run_meta.v0.1"
  status="PASS"
  recovery_class="POST_COLLECTION_PACKAGING_ONLY"
  collection_head=$CollectionHead
  finalization_head=$FinalizationHead
  quiet_host_confirmed=$true
  machine_profile_repeated=$false
  model_sha256=[string]$Lock.target_model.sha256
  model_size_bytes=[int64]$Lock.target_model.bytes
  provider="VULKAN_KHR_PERFORMANCE_QUERY"
  scope="COMMAND"
  decode_index=15
  raw_runs=6
  dispatch_observations=618
  queried_dispatches_per_run=103
  target_family_counts=[ordered]@{lm_head_q6=19;ffn_down_q4=14;ffn_down_q6=14;split_k_q4_control=56}
  phase2_basis="LM-head 63.91%, FFN-down 16.27%, split-K gate/up positive control"
  token_xray_contract_commit=[string]$Lock.token_xray_contract.contract_commit
  timing_use="INSTRUMENTED_DIAGNOSTIC_ONLY"
  benchmark_timing_substitution_forbidden=$true
  counter_collection_rerun=$false
}
$MetaPath=Join-Path $ResultsDir "M3C_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $ResultsDir "arcllm_v1_m3c_return_to_chatgpt.zip"
if(Test-Path $Zip){Remove-Item $Zip -Force}
$Files=@()
foreach($Name in $RawNames){$Files += (Join-Path $ResultsDir $Name)}
$Files += @($Obs,$Summary,$Mechanism,$MetaPath,$LockPath)
Compress-Archive -Path $Files -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()

Write-Host ""
Write-Host "M3_C_FINALIZE_ONLY=PASS"
Write-Host "COUNTER_COLLECTION_RERUN=false"
Write-Host "COLLECTION_HEAD=$CollectionHead"
Write-Host "FINALIZATION_HEAD=$FinalizationHead"
Write-Host "RAW_RUNS=6"
Write-Host "DISPATCH_OBSERVATIONS=618"
Write-Host "QUERIED_DISPATCHES_PER_RUN=103"
Write-Host "MECHANISM_DISCRIMINATION=$Mechanism"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
