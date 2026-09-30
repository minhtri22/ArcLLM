param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Here "runs\_relocation_compat.ps1")
$Cfg=Get-Content (Join-Path $Here "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
$ExpectedSize=[int64]$Cfg.size_bytes
$ExpectedHash=([string]$Cfg.sha256).ToUpperInvariant()
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "P8 Ollama target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
if(-not(Test-Path $ModelPath)){throw "Frozen P8 target not found: $ModelPath"}
$f=Get-Item $ModelPath
if($f.Length -ne $ExpectedSize){throw "Frozen P8 target size mismatch: $($f.Length) expected $ExpectedSize"}
Write-Host "Verifying frozen P8 target SHA256..."
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "Frozen P8 target SHA256 mismatch: $Hash expected $ExpectedHash"}
Write-Host "Frozen P8 target verified: $ModelPath"
Write-Host "size=$($f.Length)"
Write-Host "sha256=$Hash"
py -3 (Join-Path $Here "tests\test_p8a_package.py");if($LASTEXITCODE -ne 0){throw "P8-A static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8a.ps1");if($LASTEXITCODE -ne 0){throw "P8-A build failed"}
$Results=Join-Path $Here "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
$Out=Join-Path $Results "p8a_memory_plan.json"
& (Join-Path $Here "arcllm_p8a.exe") --model $ModelPath --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-A did not produce result JSON"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-B residency allocation bring-up"}elseif($Obj.status -eq "FAIL"){"adjudicate memory-plan obstruction before any full inference"}else{"repair harness/package before scientific adjudication"}
$Summary=[ordered]@{schema="arcllm.p8a.summary.v1";target_source="ollama-local-auto";target_path=$ModelPath;target_sha256=$Hash;target_size_bytes=$f.Length;process_exit_code=$Code;status=$Obj.status;p8a_pass=($Obj.status -eq "PASS");full_inference_permitted=($Obj.status -eq "PASS");next_step=$Next}
$SummaryPath=Join-Path $Results "p8a_summary.json";[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8a_memory_plan.json"
Write-Host "  results\p8a_summary.json"
exit $Code
