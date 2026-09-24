param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_m1_lock_v0.1.json"
$Lock=Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json
if((git -C $Root branch --show-current).Trim() -ne [string]$Lock.branch){throw "M1 wrong branch"}
foreach($Entry in $Lock.critical_git_blobs.PSObject.Properties){
  $Got=(git -C $Root hash-object -- $Entry.Name).Trim()
  if($Got -ne [string]$Entry.Value){throw "M1 critical blob mismatch: $($Entry.Name)"}
}
py -3 (Join-Path $Root "tests\test_arcllm_v1_m1_package.py")
if($LASTEXITCODE -ne 0){throw "M1 static QA failed"}

# Compile the already-frozen post-I002 shader set; this is build infrastructure, not a measurement.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE -ne 0){throw "M1 shader compile failed"}
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
$SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
if($SpvHash -ne [string]$Lock.candidate_spv_sha256){throw "M1 candidate SPIR-V mismatch"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_m1.ps1")
if($LASTEXITCODE -ne 0){throw "M1 native build failed"}
$Exe=Join-Path $Root "arcllm_v1_m1_profile.exe"
if(-not(Test-Path $Exe)){throw "M1 executable missing"}

$Cfg=Get-Content (Join-Path $Root "config\p8_target.json") -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "M1 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
if(-not(Test-Path $ModelPath)){throw "M1 target model missing"}
$MF=Get-Item $ModelPath
$MH=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($MF.Length -ne [int64]$Lock.target_model.bytes -or $MH -ne ([string]$Lock.target_model.sha256).ToUpperInvariant()){throw "M1 target model mismatch"}

$Results=Join-Path $Root "results\m1_post_i002_node_timing"
if(Test-Path $Results){Remove-Item $Results -Recurse -Force}
New-Item -ItemType Directory -Force -Path $Results | Out-Null
$Head=(git -C $Root rev-parse HEAD).Trim()

function Run-M1([string]$Workload,[string]$Name){
  $Out=Join-Path $Results $Name
  & $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --implementation-commit $Head --workload $Workload --warmups 1 --measured 2 --out $Out
  if($LASTEXITCODE -ne 0){throw "M1 $Workload execution failed"}
}
Run-M1 "W-S" "m1_W_S.json"
Run-M1 "W-C" "m1_W_C.json"

$Summary=Join-Path $Results "ARCLLM_V1_M1_SUMMARY.json"
py -3 (Join-Path $Root "tools\summarize_arcllm_v1_m1.py") --results-dir $Results --hardware-model (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL_v0.2.json") --out $Summary
if($LASTEXITCODE -ne 0){throw "M1 summarizer failed"}
$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.status -ne "PASS"){throw "M1 summary rejected collection"}

$Meta=[ordered]@{
 schema="arcllm.v1.m1.run_meta.v0.1"; status="PASS"; branch=$Lock.branch; head=$Head;
 model_sha256=$MH; model_size_bytes=$MF.Length; candidate_spv_sha256=$SpvHash;
 quiet_host_required=$false; machine_profile_repeated=$false; hardware_counters_used=$false;
 measured_attempts=4; profiled_decode_steps=12; op_timestamp_values=5628;
 workloads=@("W-S","W-C"); probe_decode_indices=@(0,15,30)
}
$MetaPath=Join-Path $Results "M1_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Zip=Join-Path $Results "arcllm_v1_m1_return_to_chatgpt.zip"
Compress-Archive -Path (Join-Path $Results "m1_W_S.json"),(Join-Path $Results "m1_W_C.json"),$Summary,$MetaPath,$LockPath -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host ""
Write-Host "M1_RESULT=PASS"
Write-Host "MEASURED_ATTEMPTS=4"
Write-Host "PROFILED_DECODE_STEPS=12"
Write-Host "QUIET_HOST_REQUIRED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
