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
if($f.Length -ne $ExpectedSize){throw "P8-G1 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G1 target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8g_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g_four_layer_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9"){throw "P8-G1 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129"){throw "P8-G1 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE"){throw "P8-G1 parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g1_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G1 static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g1_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G1 shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g1.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G1 native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g1_causal_decomposition_results.json"
& (Join-Path $Here "arcllm_p8g1.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G1 result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Classification=if($Obj.adjudication){[string]$Obj.adjudication.classification}else{"UNAVAILABLE"}
$Summary=[ordered]@{
  schema="arcllm.p8g1.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  diagnostic_valid=([bool]$Obj.diagnostic_valid)
  classification=$Classification
  p8g_verdict_frozen="FAIL"
  p8g_verdict_changed=$false
  p8h_permitted=$false
  full_inference_permitted=$false
  next_step=if($Obj.status -eq "COMPLETE"){"adjudicate P8-G1 causal decomposition"}else{"repair P8-G1 reproduction/package/runtime before adjudication"}
}
[IO.File]::WriteAllText((Join-Path $Results "p8g1_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g1_shader_provenance.json"
Write-Host "  results\p8g1_causal_decomposition_results.json"
Write-Host "  results\p8g1_summary.json"
exit $Code
