param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$Dir=Join-Path $Root "results\m3b_hardware_counters"
$LockPath=Join-Path $Root "config\arcllm_v1_m3b_lock_v0.1.json"
$Lock=Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json

# Recovery is packaging-only. It must never execute the M3-B native collector.
$Exe=Join-Path $Root "arcllm_v1_m3b_counter_collection.exe"
$RawNames=@(
  "M3B_W_S_memory_cache.json",
  "M3B_W_S_execution_occupancy.json",
  "M3B_W_S_stall_cause.json",
  "M3B_W_C_memory_cache.json",
  "M3B_W_C_execution_occupancy.json",
  "M3B_W_C_stall_cause.json"
)
if(-not(Test-Path $Dir)){throw "M3-B recovery results directory missing: $Dir"}
foreach($Name in $RawNames){if(-not(Test-Path (Join-Path $Dir $Name))){throw "M3-B recovery missing raw run: $Name"}}

$Heads=@()
foreach($Name in $RawNames){
  $Obj=Get-Content (Join-Path $Dir $Name) -Raw -Encoding UTF8 | ConvertFrom-Json
  if($Obj.schema -ne "arcllm.v1.m3b.command_scope_counter_collection.v0.1" -or $Obj.status -ne "PASS"){throw "M3-B recovery invalid raw run: $Name"}
  if(-not $Obj.implementation_commit){throw "M3-B recovery raw run missing implementation_commit: $Name"}
  $Heads += [string]$Obj.implementation_commit
}
$UniqueHeads=@($Heads | Sort-Object -Unique)
if($UniqueHeads.Count -ne 1){throw "M3-B recovery raw runs do not share one collection head: $($UniqueHeads -join ',')"}
$CollectionHead=$UniqueHeads[0]
if($CollectionHead -ne "818c522c8c31d845c0ae6c358a769ebe0da5fc7b"){throw "M3-B recovery unexpected collection head=$CollectionHead"}

$Obs=Join-Path $Dir "HARDWARE_OBSERVATIONS.jsonl"
$Summary=Join-Path $Dir "M3B_COLLECTION_SUMMARY.json"
py -3 (Join-Path $Root "tools\parse_arcllm_v1_m3b.py") --results-dir $Dir --out-observations $Obs --out-summary $Summary
if($LASTEXITCODE -ne 0){throw "M3-B recovery parser failed"}
$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.status -ne "PASS" -or $S.raw_runs -ne 6 -or $S.dispatch_observations -ne 2814){throw "M3-B recovery summary rejected"}

$PackagingHead=(git -C $Root rev-parse HEAD).Trim()
$Branch=(git -C $Root branch --show-current).Trim()
if($Branch -ne "research/arcllm-v1"){throw "M3-B recovery wrong branch=$Branch"}

$Meta=[ordered]@{
  schema="arcllm.v1.m3b.run_meta.v0.2"; status="PASS";
  branch=$Branch; collection_head=$CollectionHead; packaging_head=$PackagingHead;
  quiet_host_confirmed=$true; machine_profile_repeated=$false;
  model_sha256=[string]$Lock.target_model.sha256; model_size_bytes=[int64]$Lock.target_model.bytes;
  provider="VULKAN_KHR_PERFORMANCE_QUERY"; scope="COMMAND"; decode_index=15;
  raw_runs=6; dispatch_observations=2814;
  external_contract_reference_commit=[string]$Lock.token_xray_contract.contract_commit;
  timing_use="INSTRUMENTED_DIAGNOSTIC_ONLY"; benchmark_timing_substitution_forbidden=$true;
  recovery=[ordered]@{
    packaging_only=$true; collector_reexecuted=$false; counter_collection_reexecuted=$false;
    reason="Original collection and parser completed; initial runner failed only while reading a renamed metadata property after parser PASS."
  }
}
$MetaPath=Join-Path $Dir "M3B_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

# Preserve the exact collection-time lock separately from the current packaging lock.
$CollectionLockPath=Join-Path $Dir "M3B_COLLECTION_LOCK_v0.1.5.json"
$CollectionLockText=(git -C $Root show "818c522c8c31d845c0ae6c358a769ebe0da5fc7b:config/arcllm_v1_m3b_lock_v0.1.json") -join "`n"
if(-not $CollectionLockText){throw "M3-B recovery could not materialize collection-time lock"}
[IO.File]::WriteAllText($CollectionLockPath,$CollectionLockText+"`n",(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Dir "arcllm_v1_m3b_return_to_chatgpt.zip"
if(Test-Path $Zip){Remove-Item $Zip -Force}
$Files=@()
foreach($Name in $RawNames){$Files += (Join-Path $Dir $Name)}
$Files += $Obs,$Summary,$MetaPath,$CollectionLockPath,$LockPath
Compress-Archive -Path $Files -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()

Write-Host ""
Write-Host "M3_B_RECOVERY=PASS"
Write-Host "COLLECTOR_REEXECUTED=false"
Write-Host "COUNTER_COLLECTION_REEXECUTED=false"
Write-Host "COLLECTION_HEAD=$CollectionHead"
Write-Host "PACKAGING_HEAD=$PackagingHead"
Write-Host "RAW_RUNS=$($S.raw_runs)"
Write-Host "DISPATCH_OBSERVATIONS=$($S.dispatch_observations)"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
