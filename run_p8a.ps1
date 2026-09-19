param([string]$ModelPath)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$ExpectedSize=4683073536
$ExpectedHash="509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C"
if(-not $ModelPath){$ModelPath=Join-Path $Here ".models\qwen2.5-coder-7b-instruct-q4_k_m.gguf"}
if(-not(Test-Path $ModelPath)){throw "Frozen P8 target not found: $ModelPath. Run .\tools\fetch_p8_target.ps1 first or pass -ModelPath."}
$f=Get-Item $ModelPath;if($f.Length -ne $ExpectedSize){throw "Frozen P8 target size mismatch: $($f.Length)"}
Write-Host "Verifying frozen P8 target SHA256..."
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant();if($Hash -ne $ExpectedHash){throw "Frozen P8 target SHA256 mismatch: $Hash"}
py -3 (Join-Path $Here "tests\test_p8a_package.py");if($LASTEXITCODE -ne 0){throw "P8-A static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8a.ps1");if($LASTEXITCODE -ne 0){throw "P8-A build failed"}
$Results=Join-Path $Here "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
$Out=Join-Path $Results "p8a_memory_plan.json"
& (Join-Path $Here "arcllm_p8a.exe") --model $ModelPath --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-A did not produce result JSON"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-B residency allocation bring-up"}elseif($Obj.status -eq "FAIL"){"adjudicate memory-plan obstruction before any full inference"}else{"repair harness/package before scientific adjudication"}
$Summary=[ordered]@{schema="arcllm.p8a.summary.v1";target_sha256=$Hash;target_size_bytes=$f.Length;process_exit_code=$Code;status=$Obj.status;p8a_pass=($Obj.status -eq "PASS");full_inference_permitted=($Obj.status -eq "PASS");next_step=$Next}
$SummaryPath=Join-Path $Results "p8a_summary.json";[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8a_memory_plan.json"
Write-Host "  results\p8a_summary.json"
exit $Code
