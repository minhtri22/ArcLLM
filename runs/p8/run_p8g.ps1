param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Here "runs\_relocation_compat.ps1")
$Cfg=Get-Content (Join-Path $Here "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
$ExpectedSize=[int64]$Cfg.size_bytes;$ExpectedHash=([string]$Cfg.sha256).ToUpperInvariant()

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "P8 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$f=Get-Item $ModelPath
if($f.Length -ne $ExpectedSize){throw "P8-G target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8f_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8f_two_layer_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8f_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "9686F747B6A4D7380F4621B1A3EEC09B82DE7832461B1A9C80548D21D7D70A41"){throw "P8-G parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "F172E1B0B00558BDE53EB6994BDC1E5BFFC417F96A33FA1C53B0983BB494AE98"){throw "P8-G parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "1F3EF56BC723FE2D8D8E567ADCB89BC23AE212CB0BBBB51CBC5B83E81DF9FC70"){throw "P8-G parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g_four_layer_results.json"
& (Join-Path $Here "arcllm_p8g.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-H bounded eight-layer prefix correctness design"}elseif($Obj.status -eq "FAIL"){"adjudicate first failing P8-G checkpoint"}else{"repair package/build/runtime before scientific adjudication"}
$Summary=[ordered]@{
  schema="arcllm.p8g.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  p8g_pass=($Obj.status -eq "PASS")
  full_inference_permitted=$false
  larger_prefix_design_permitted=($Obj.status -eq "PASS")
  next_step=$Next
}
[IO.File]::WriteAllText((Join-Path $Results "p8g_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g_shader_provenance.json"
Write-Host "  results\p8g_four_layer_results.json"
Write-Host "  results\p8g_summary.json"
exit $Code
