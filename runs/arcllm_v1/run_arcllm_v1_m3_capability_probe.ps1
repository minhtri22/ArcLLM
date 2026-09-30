param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$Lock=Get-Content (Join-Path $Root "config\arcllm_v1_m3a_lock_v0.1.json") -Raw -Encoding UTF8 | ConvertFrom-Json
if((git -C $Root branch --show-current).Trim() -ne [string]$Lock.branch){throw "M3-A wrong branch"}
foreach($Entry in $Lock.critical_git_blobs.PSObject.Properties){
  $Got=(git -C $Root hash-object -- $Entry.Name).Trim()
  if($Got -ne [string]$Entry.Value){throw "M3-A critical blob mismatch: $($Entry.Name)"}
}
py -3 (Join-Path $Root "tests\test_arcllm_v1_m3a_package.py")
if($LASTEXITCODE -ne 0){throw "M3-A static QA failed"}
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_m3_capability.ps1")
if($LASTEXITCODE -ne 0){throw "M3-A build failed"}
$Dir=Join-Path $Root "results\m3a_counter_capability"
if(Test-Path $Dir){Remove-Item $Dir -Recurse -Force}
New-Item -ItemType Directory -Force -Path $Dir | Out-Null
$Raw=Join-Path $Dir "M3A_VULKAN_COUNTER_CAPABILITY_RAW.json"
& (Join-Path $Root "arcllm_v1_m3_counter_capability.exe") --out $Raw
if($LASTEXITCODE -ne 0){throw "M3-A Vulkan capability probe failed"}
if(-not(Test-Path $Raw)){throw "M3-A raw capability missing"}
$VtuneCmd=Get-Command vtune.exe -ErrorAction SilentlyContinue
if(-not $VtuneCmd){$VtuneCmd=Get-Command vtune -ErrorAction SilentlyContinue}
$Vtune=[ordered]@{available=$false;path=$null;version=$null}
if($VtuneCmd){
  $Vtune.available=$true; $Vtune.path=$VtuneCmd.Source
  try {$Vtune.version=(Get-Item $VtuneCmd.Source).VersionInfo.FileVersion} catch {}
}
$VtunePath=Join-Path $Dir "M3A_VTUNE_CAPABILITY.json"
[IO.File]::WriteAllText($VtunePath,($Vtune|ConvertTo-Json -Depth 4),(New-Object Text.UTF8Encoding($false)))
$Summary=Join-Path $Dir "COUNTER_CAPABILITY.json"
py -3 (Join-Path $Root "tools\summarize_arcllm_v1_m3_capability.py") --raw $Raw --vtune-json $VtunePath --out $Summary
if($LASTEXITCODE -ne 0){throw "M3-A summarizer failed"}
$S=Get-Content $Summary -Raw -Encoding UTF8 | ConvertFrom-Json
if($S.artifact_type -ne "COUNTER_CAPABILITY"){throw "M3-A bad summary artifact"}
$Meta=[ordered]@{schema="arcllm.v1.m3a.run_meta.v0.1";status="PASS";head=(git -C $Root rev-parse HEAD).Trim();quiet_host_required=$false;model_loaded=$false;inference_executed=$false;counter_collection_executed=$false;token_xray_contract_commit=$Lock.token_xray_contract.commit}
$MetaPath=Join-Path $Dir "M3A_RUN_META.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
$Zip=Join-Path $Dir "arcllm_v1_m3a_return_to_chatgpt.zip"
Compress-Archive -Path $Raw,$VtunePath,$Summary,$MetaPath,(Join-Path $Root "config\arcllm_v1_m3a_lock_v0.1.json") -DestinationPath $Zip -CompressionLevel Optimal
$ZH=(Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host ""
Write-Host "M3_A_RESULT=PASS"
Write-Host "PREFERRED_PROVIDER=$($S.selection.preferred_provider)"
Write-Host "QUIET_HOST_REQUIRED=false"
Write-Host "MODEL_LOADED=false"
Write-Host "INFERENCE_EXECUTED=false"
Write-Host "COUNTER_COLLECTION_EXECUTED=false"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$ZH"
