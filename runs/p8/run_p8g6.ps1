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
if($f.Length -ne $ExpectedSize){throw "P8-G6 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G6 target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8g5_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g5_production_semantic_attribution_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g5_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A"){throw "P8-G6 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096"){throw "P8-G6 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751"){throw "P8-G6 parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g6_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G6 static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g6_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G6 shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g6.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G6 native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g6_fresh_production_semantic_results.json"
& (Join-Path $Here "arcllm_p8g6.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G6 result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Classification=if($Obj.adjudication){[string]$Obj.adjudication.classification}else{"UNAVAILABLE"}
$Summary=[ordered]@{
  schema="arcllm.p8g6.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  diagnostic_valid=([bool]$Obj.diagnostic_valid)
  classification=$Classification
  fresh_cohort_ids=@(73,89,107,131)
  previous_cohort_ids=@(17,29,43,61)
  fresh_confirmation=$true
  primary_oracle="R0"
  r1_decision_role="descriptive_only"
  p8g_verdict_frozen="FAIL"
  p8g1_classification_frozen="H-AMPLIFICATION"
  p8g2_classification_frozen="H-NONLINEAR/UNEXPLAINED"
  p8g3_classification_frozen="H-FP32-ACCUMULATION"
  p8g4_classification_frozen="H-LOCAL-ERROR-NONNEGLIGIBLE"
  p8g5_classification_frozen="H-R1-ORACLE-SEMANTIC-MISMATCH"
  replacement_gate_defined=$false
  p8h_permitted=$false
  full_inference_permitted=$false
  next_step=if($Obj.status -eq "COMPLETE"){"adjudicate P8-G6 fresh production-semantic confirmation"}else{"repair P8-G6 package/runtime before adjudication"}
}
[IO.File]::WriteAllText((Join-Path $Results "p8g6_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g6_shader_provenance.json"
Write-Host "  results\p8g6_fresh_production_semantic_results.json"
Write-Host "  results\p8g6_summary.json"
exit $Code
