param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$Cfg=Get-Content (Join-Path $Root "config\p8_target.json") -Raw -Encoding UTF8 | ConvertFrom-Json
$Lock=Get-Content (Join-Path $Root "config\arcllm_v1_m0_tensor_census_lock_v0.1.json") -Raw -Encoding UTF8 | ConvertFrom-Json
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "M0 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
if(-not(Test-Path $ModelPath)){throw "Frozen target not found: $ModelPath"}
$F=Get-Item $ModelPath
if($F.Length -ne [int64]$Cfg.size_bytes){throw "M0 size mismatch"}
Write-Host "Verifying exact model SHA256..."
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne ([string]$Cfg.sha256).ToUpperInvariant()){throw "M0 SHA256 mismatch: $Hash"}
$Branch=(git -C $Root branch --show-current).Trim()
if($Branch -ne [string]$Lock.branch){throw "M0 wrong branch: $Branch"}
foreach($Entry in $Lock.critical_git_blobs.PSObject.Properties){
  $Path=Resolve-RunPhysicalPath -RepoRoot $Root -Path $Entry.Name
  if(-not(Test-Path $Path)){throw "M0 critical file missing: $($Entry.Name)"}
  $Got=(Get-RunLogicalGitBlob -RepoRoot $Root -Path $Entry.Name)
  if($Got -ne [string]$Entry.Value){throw "M0 critical blob mismatch: $($Entry.Name) got=$Got expected=$($Entry.Value)"}
}
py -3 (Join-Path $Root "tests\test_arcllm_v1_m0_package.py")
if($LASTEXITCODE -ne 0){throw "M0 static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_m0.ps1")
if($LASTEXITCODE -ne 0){throw "M0 build failed"}
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $Results | Out-Null
$Out=Join-Path $Results "arcllm_v1_m0_tensor_census.json"
& (Join-Path $Root "arcllm_v1_m0_tensor_census.exe") --model $ModelPath --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "M0 result missing"}
$Obj=Get-Content $Out -Raw -Encoding UTF8 | ConvertFrom-Json
if($Obj.execution_class -ne "METADATA_ONLY_NO_INFERENCE_NO_PERFORMANCE"){throw "M0 execution class mismatch"}
$Summary=[ordered]@{
  schema="arcllm.v1.m0.summary.v0.1"
  status=$Obj.status
  branch=$Branch
  head=(git -C $Root rev-parse HEAD).Trim()
  model_sha256=$Hash
  model_size_bytes=$F.Length
  target_tensor_count=$Obj.target_summary.target_tensor_count
  target_q4_k=$Obj.target_summary.Q4_K
  target_q6_k=$Obj.target_summary.Q6_K
  no_inference=$true
  no_performance_measurement=$true
  next_step=if($Obj.status -eq "PASS"){"INDEPENDENT_M0_ADJUDICATION_AND_PATCH_ONE_TOKEN_HARDWARE_MODEL"}else{"STOP_REPAIR_OR_ADJUDICATE_M0"}
}
$SummaryPath=Join-Path $Results "arcllm_v1_m0_summary.json"
[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$Zip=Join-Path $Results "arcllm_v1_m0_return_to_chatgpt.zip"
if(Test-Path $Zip){Remove-Item $Zip -Force}
Compress-Archive -Path $Out,$SummaryPath,(Join-Path $Root "config\arcllm_v1_m0_tensor_census_lock_v0.1.json") -DestinationPath $Zip -CompressionLevel Optimal
$ZipHash=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host ""
Write-Host "M0_RESULT=$($Obj.status)"
Write-Host "TARGET_Q4_K=$($Obj.target_summary.Q4_K)"
Write-Host "TARGET_Q6_K=$($Obj.target_summary.Q6_K)"
Write-Host "INFERENCE_EXECUTED=false"
Write-Host "PERFORMANCE_MEASURED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZipHash"
exit $Code
