param(
  [switch]$QuietHostConfirmed,
  [string]$ModelPath,
  [string]$OllamaModelsRoot
)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$LockPath=Join-Path $Root "config\arcllm_v1_m3b_lock_v0.1.json"
if(-not(Test-Path $LockPath)){throw "M3-B lock missing"}
$Lock=Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $QuietHostConfirmed){
  throw "M3-B requires quiet-host confirmation. Close/stop avoidable GPU/CPU-heavy workloads, then rerun with -QuietHostConfirmed."
}
$Branch=(git -C $Root branch --show-current).Trim()
if($Branch -ne [string]$Lock.branch){throw "M3-B wrong branch: $Branch"}
foreach($Entry in $Lock.critical_git_blobs.PSObject.Properties){
  $Path=Resolve-RunPhysicalPath -RepoRoot $Root -Path $Entry.Name
  if(-not(Test-Path $Path)){throw "M3-B critical file missing: $($Entry.Name)"}
  $Got=(Get-RunLogicalGitBlob -RepoRoot $Root -Path $Entry.Name)
  if($Got -ne [string]$Entry.Value){throw "M3-B critical blob mismatch: $($Entry.Name) got=$Got expected=$($Entry.Value)"}
}
py -3 (Join-Path $Root "tests\test_arcllm_v1_m3b_package.py")
if($LASTEXITCODE -ne 0){throw "M3-B static/synthetic QA failed"}

# Build the unchanged post-I002 shader set and verify the causal carry-through shader identity.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE -ne 0){throw "M3-B shader compile failed"}
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
$SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
if($SpvHash -ne ([string]$Lock.candidate_spv_sha256).ToUpperInvariant()){throw "M3-B candidate SPIR-V mismatch"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_m3b.ps1")
if($LASTEXITCODE -ne 0){throw "M3-B native build failed"}
$Exe=Join-Path $Root "arcllm_v1_m3b_counter_collection.exe"
if(-not(Test-Path $Exe)){throw "M3-B executable missing"}

$Cfg=Get-Content (Join-Path $Root "config\p8_target.json") -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "M3-B target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
if(-not(Test-Path $ModelPath)){throw "M3-B target model missing"}
$MF=Get-Item $ModelPath
$MH=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($MF.Length -ne [int64]$Lock.target_model.bytes -or $MH -ne ([string]$Lock.target_model.sha256).ToUpperInvariant()){throw "M3-B target model mismatch"}

$Dir=Join-Path $Root "results\m3b_hardware_counters"
if(Test-Path $Dir){Remove-Item $Dir -Recurse -Force}
New-Item -ItemType Directory -Force -Path $Dir | Out-Null
$Head=(git -C $Root rev-parse HEAD).Trim()
$ShaderDir=Join-Path $Root "compiled_shaders"

function Invoke-NativeCaptured([string]$FilePath,[string]$ArgumentLine,[string]$StdoutPath,[string]$StderrPath){
  $P=Start-Process -FilePath $FilePath -ArgumentList $ArgumentLine -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -NoNewWindow -Wait -PassThru
  if($null -eq $P){throw "M3-B native process launch returned null"}
  return [int]$P.ExitCode
}

function Run-M3B([string]$Workload,[string]$Group,[string]$Stem){
  $Out=Join-Path $Dir ($Stem+".json")
  $Stdout=Join-Path $Dir ($Stem+"_stdout.txt")
  $Stderr=Join-Path $Dir ($Stem+"_stderr.txt")
  $ArgLine='--model "'+$ModelPath+'" --shader-dir "'+$ShaderDir+'" --implementation-commit '+$Head+' --workload '+$Workload+' --counter-group '+$Group+' --warmups 1 --measured 1 --out "'+$Out+'"'
  Write-Host "M3-B BEGIN workload=$Workload group=$Group"
  $Code=Invoke-NativeCaptured -FilePath $Exe -ArgumentLine $ArgLine -StdoutPath $Stdout -StderrPath $Stderr
  if($Code -ne 0){
    $ErrText=if(Test-Path $Stderr){Get-Content $Stderr -Raw -ErrorAction SilentlyContinue}else{""}
    throw "M3-B workload=$Workload group=$Group failed exit=$Code stderr=$ErrText"
  }
  if(-not(Test-Path $Out)){throw "M3-B result missing: $Stem"}
  $Obj=Get-Content $Out -Raw -Encoding UTF8 | ConvertFrom-Json
  if($Obj.status -ne "PASS"){throw "M3-B raw result rejected: $Stem"}
  Write-Host "M3-B END workload=$Workload group=$Group pass_count=$($Obj.counter_probe.pass_count)"
}

Run-M3B "W-S" "memory_cache"        "M3B_W_S_memory_cache"
Run-M3B "W-S" "execution_occupancy" "M3B_W_S_execution_occupancy"
Run-M3B "W-S" "stall_cause"         "M3B_W_S_stall_cause"
Run-M3B "W-C" "memory_cache"        "M3B_W_C_memory_cache"
Run-M3B "W-C" "execution_occupancy" "M3B_W_C_execution_occupancy"
Run-M3B "W-C" "stall_cause"         "M3B_W_C_stall_cause"

$Obs=Join-Path $Dir "HARDWARE_OBSERVATIONS.jsonl"
$Summary=Join-Path $Dir "M3B_COLLECTION_SUMMARY.json"
py -3 (Join-Path $Root "tools\parse_arcllm_v1_m3b.py") --results-dir $Dir --out-observations $Obs --out-summary $Summary
if($LASTEXITCODE -ne 0){throw "M3-B parser failed"}
$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.status -ne "PASS" -or $S.dispatch_observations -ne 2814){throw "M3-B collection summary rejected"}

$Meta=[ordered]@{
  schema="arcllm.v1.m3b.run_meta.v0.1"; status="PASS"; branch=$Branch; head=$Head;
  quiet_host_confirmed=$true; machine_profile_repeated=$false; model_sha256=$MH; model_size_bytes=$MF.Length;
  provider="VULKAN_KHR_PERFORMANCE_QUERY"; scope="COMMAND"; decode_index=15;
  raw_runs=6; dispatch_observations=2814; token_xray_contract_commit=$Lock.token_xray_contract.contract_commit;
  timing_use="INSTRUMENTED_DIAGNOSTIC_ONLY"; benchmark_timing_substitution_forbidden=$true
}
$MetaPath=Join-Path $Dir "M3B_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Dir "arcllm_v1_m3b_return_to_chatgpt.zip"
if(Test-Path $Zip){Remove-Item $Zip -Force}
$Files=@(
  (Join-Path $Dir "M3B_W_S_memory_cache.json"),
  (Join-Path $Dir "M3B_W_S_execution_occupancy.json"),
  (Join-Path $Dir "M3B_W_S_stall_cause.json"),
  (Join-Path $Dir "M3B_W_C_memory_cache.json"),
  (Join-Path $Dir "M3B_W_C_execution_occupancy.json"),
  (Join-Path $Dir "M3B_W_C_stall_cause.json"),
  $Obs,$Summary,$MetaPath,$LockPath
)
Compress-Archive -Path $Files -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host ""
Write-Host "M3_B_RESULT=PASS"
Write-Host "QUIET_HOST_CONFIRMED=true"
Write-Host "RAW_RUNS=6"
Write-Host "DISPATCH_OBSERVATIONS=2814"
Write-Host "TIMING_USE=INSTRUMENTED_DIAGNOSTIC_ONLY"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
