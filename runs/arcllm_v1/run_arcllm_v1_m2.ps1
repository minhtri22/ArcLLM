param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$LockPath=Join-Path $Root "config\arcllm_v1_m2_lock_v0.1.2.json"
$Lock=Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json
if((git -C $Root branch --show-current).Trim() -ne [string]$Lock.branch){throw "M2 wrong branch"}
foreach($Entry in $Lock.critical_git_blobs.PSObject.Properties){
  $Got=(git -C $Root hash-object -- $Entry.Name).Trim()
  if($Got -ne [string]$Entry.Value){throw "M2 critical blob mismatch: $($Entry.Name)"}
}
py -3 (Join-Path $Root "tests\test_arcllm_v1_m2_package.py")
if($LASTEXITCODE -ne 0){throw "M2 static QA failed"}

$Exe=Join-Path $Root "artifacts\i003_baseline\i003_llama_adapter.exe"
$NeedBuild=$true
if(Test-Path $Exe){
  $H=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  if($H -eq ([string]$Lock.baseline_adapter_sha256).ToUpperInvariant()){$NeedBuild=$false}
}
if($NeedBuild){
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\qualify_arcllm_v1_i003_baseline.ps1")
  if($LASTEXITCODE -ne 0){throw "M2 pinned baseline build failed"}
}
if(-not(Test-Path $Exe)){throw "M2 baseline adapter missing"}
$ExeHash=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
if($ExeHash -ne ([string]$Lock.baseline_adapter_sha256).ToUpperInvariant()){throw "M2 baseline adapter hash mismatch"}

$Cfg=Get-Content (Join-Path $Root "config\p8_target.json") -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "M2 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$MF=Get-Item $ModelPath
$MH=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($MF.Length -ne [int64]$Lock.target_model.bytes -or $MH -ne ([string]$Lock.target_model.sha256).ToUpperInvariant()){throw "M2 model mismatch"}

$Dir=Join-Path $Root "results\m2_llama_same_semantic_map"
if(Test-Path $Dir){Remove-Item $Dir -Recurse -Force}
New-Item -ItemType Directory -Force -Path $Dir | Out-Null

function Invoke-NativeCaptured(
  [string]$FilePath,
  [string[]]$ArgumentList,
  [string]$StdoutPath,
  [string]$StderrPath
){
  $P=Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -NoNewWindow -Wait -PassThru
  if($null -eq $P){throw "M2 native process launch returned null"}
  return [int]$P.ExitCode
}

# Windows/PowerShell transport probe: native stderr with exit 0 must remain captured data, not a terminating PowerShell error.
$ProbeOut=Join-Path $Dir "_native_stderr_probe_stdout.txt"
$ProbeErr=Join-Path $Dir "_native_stderr_probe_stderr.txt"
$ProbeArgs=@("/d","/s","/c",'"echo M2_STDOUT_PROBE&&echo M2_STDERR_PROBE 1>&2&&exit /b 0"')
$ProbeCode=Invoke-NativeCaptured -FilePath $env:ComSpec -ArgumentList $ProbeArgs -StdoutPath $ProbeOut -StderrPath $ProbeErr
if($ProbeCode -ne 0){throw "M2 native stderr transport probe exit=$ProbeCode"}
if(-not((Get-Content $ProbeOut -Raw -ErrorAction Stop) -match "M2_STDOUT_PROBE")){throw "M2 native stdout transport probe missing marker"}
if(-not((Get-Content $ProbeErr -Raw -ErrorAction Stop) -match "M2_STDERR_PROBE")){throw "M2 native stderr transport probe missing marker"}
Remove-Item $ProbeOut,$ProbeErr -Force

function Run-M2([string]$Workload,[string]$Stem){
  $Result=Join-Path $Dir ($Stem+"_result.json")
  $Stdout=Join-Path $Dir ($Stem+"_stdout.txt")
  $Perf=Join-Path $Dir ($Stem+"_vulkan_perf.txt")
  $OldPerf=$env:GGML_VK_PERF_LOGGER; $OldFreq=$env:GGML_VK_PERF_LOGGER_FREQUENCY; $OldConcurrent=$env:GGML_VK_PERF_LOGGER_CONCURRENT
  try{
    $env:GGML_VK_PERF_LOGGER="1"
    $env:GGML_VK_PERF_LOGGER_FREQUENCY="1"
    Remove-Item Env:GGML_VK_PERF_LOGGER_CONCURRENT -ErrorAction SilentlyContinue
    $Args=@("--model",('"{0}"' -f $ModelPath),"--workload",$Workload,"--warmups","1","--measured","1","--out",('"{0}"' -f $Result))
    $Code=Invoke-NativeCaptured -FilePath $Exe -ArgumentList $Args -StdoutPath $Stdout -StderrPath $Perf
    if($Code -ne 0){throw "M2 llama $Workload failed exit=$Code"}
  } finally {
    if($null -eq $OldPerf){Remove-Item Env:GGML_VK_PERF_LOGGER -ErrorAction SilentlyContinue}else{$env:GGML_VK_PERF_LOGGER=$OldPerf}
    if($null -eq $OldFreq){Remove-Item Env:GGML_VK_PERF_LOGGER_FREQUENCY -ErrorAction SilentlyContinue}else{$env:GGML_VK_PERF_LOGGER_FREQUENCY=$OldFreq}
    if($null -eq $OldConcurrent){Remove-Item Env:GGML_VK_PERF_LOGGER_CONCURRENT -ErrorAction SilentlyContinue}else{$env:GGML_VK_PERF_LOGGER_CONCURRENT=$OldConcurrent}
  }
  if(-not(Test-Path $Result)){throw "M2 llama $Workload result JSON missing"}
  if(-not(Test-Path $Perf)){throw "M2 llama $Workload Vulkan perf log missing"}
  $PerfText=Get-Content $Perf -Raw -Encoding UTF8
  if($PerfText -notmatch "Vulkan Timings:"){throw "M2 llama $Workload perf logger emitted no Vulkan Timings blocks"}
  $Obj=Get-Content $Result -Raw -Encoding UTF8 | ConvertFrom-Json
  if(-not $Obj.attempts[0].success){throw "M2 llama $Workload semantic attempt failed"}
  $Expected=if($Workload -eq "W-S"){"1a31529a4fa40742"}else{"d9b495aa8e8764e4"}
  if([string]$Obj.attempts[0].generated_hash_fnv1a64 -ne $Expected){throw "M2 llama $Workload generated-hash drift"}
}
Run-M2 "W-S" "m2_W_S"
Run-M2 "W-C" "m2_W_C"

$Summary=Join-Path $Dir "ARCLLM_V1_M2_LLAMA_MAP.json"
py -3 (Join-Path $Root "tools\parse_arcllm_v1_m2_llama_perf.py") --ws-log (Join-Path $Dir "m2_W_S_vulkan_perf.txt") --wc-log (Join-Path $Dir "m2_W_C_vulkan_perf.txt") --out $Summary
if($LASTEXITCODE -ne 0){throw "M2 perf parser failed"}
$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.status -ne "PASS"){throw "M2 map rejected"}

$Meta=[ordered]@{schema="arcllm.v1.m2.run_meta.v0.1";status="PASS";head=(git -C $Root rev-parse HEAD).Trim();baseline_commit=$Lock.baseline_commit;baseline_adapter_sha256=$ExeHash;model_sha256=$MH;quiet_host_required=$false;machine_profile_repeated=$false;hardware_counters_used=$false;logger="GGML_VK_PERF_LOGGER=1";workloads=@("W-S","W-C");probe_decode_indices=@(0,15,30)}
$MetaPath=Join-Path $Dir "M2_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Dir "arcllm_v1_m2_return_to_chatgpt.zip"
Compress-Archive -Path (Join-Path $Dir "m2_W_S_result.json"),(Join-Path $Dir "m2_W_C_result.json"),(Join-Path $Dir "m2_W_S_vulkan_perf.txt"),(Join-Path $Dir "m2_W_C_vulkan_perf.txt"),$Summary,$MetaPath,$LockPath -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host ""
Write-Host "M2_RESULT=PASS"
Write-Host "BASELINE_COMMIT=$($Lock.baseline_commit)"
Write-Host "QUIET_HOST_REQUIRED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
