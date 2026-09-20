param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Cfg=Get-Content (Join-Path $Here "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
$ExpectedSize=[int64]$Cfg.size_bytes;$ExpectedHash=([string]$Cfg.sha256).ToUpperInvariant()

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "P8 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$f=Get-Item $ModelPath
if($f.Length -ne $ExpectedSize){throw "P8-G5 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G5 target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8g4_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g4_fresh_compositional_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g4_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C"){throw "P8-G5 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757"){throw "P8-G5 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0"){throw "P8-G5 parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g5_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G5 static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g5_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G5 shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g5.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G5 native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g5_production_semantic_attribution_results.json"
& (Join-Path $Here "arcllm_p8g5.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G5 result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Classification=if($Obj.adjudication){[string]$Obj.adjudication.classification}else{"UNAVAILABLE"}
$Summary=[ordered]@{
  schema="arcllm.p8g5.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  diagnostic_valid=([bool]$Obj.diagnostic_valid)
  classification=$Classification
  cohort_ids=@(17,29,43,61)
  retrospective_attribution_only=$true
  p8g_verdict_frozen="FAIL"
  p8g1_classification_frozen="H-AMPLIFICATION"
  p8g2_classification_frozen="H-NONLINEAR/UNEXPLAINED"
  p8g3_classification_frozen="H-FP32-ACCUMULATION"
  p8g4_classification_frozen="H-LOCAL-ERROR-NONNEGLIGIBLE"
  replacement_gate_defined=$false
  p8h_permitted=$false
  full_inference_permitted=$false
  next_step=if($Obj.status -eq "COMPLETE"){"adjudicate P8-G5 production-semantic local-error attribution"}else{"repair P8-G5 package/runtime before adjudication"}
}
[IO.File]::WriteAllText((Join-Path $Results "p8g5_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g5_shader_provenance.json"
Write-Host "  results\p8g5_production_semantic_attribution_results.json"
Write-Host "  results\p8g5_summary.json"
exit $Code
