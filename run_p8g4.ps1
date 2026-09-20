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
if($f.Length -ne $ExpectedSize){throw "P8-G4 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G4 target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8g3_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g3_arithmetic_precision_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g3_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8"){throw "P8-G4 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835"){throw "P8-G4 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D"){throw "P8-G4 parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g4_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G4 static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g4_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G4 shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g4.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G4 native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g4_fresh_compositional_results.json"
& (Join-Path $Here "arcllm_p8g4.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G4 result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Classification=if($Obj.cohort){[string]$Obj.cohort.classification}else{"UNAVAILABLE"}
$PassCount=if($Obj.cohort){[int]$Obj.cohort.seed_pass_count}else{-1}
$Summary=[ordered]@{
  schema="arcllm.p8g4.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  diagnostic_valid=([bool]$Obj.diagnostic_valid)
  classification=$Classification
  seed_pass_count=$PassCount
  fresh_input_ids=@(17,29,43,61)
  p8g_verdict_frozen="FAIL"
  p8g1_classification_frozen="H-AMPLIFICATION"
  p8g2_classification_frozen="H-NONLINEAR/UNEXPLAINED"
  p8g3_classification_frozen="H-FP32-ACCUMULATION"
  replacement_gate_defined=$false
  historical_total_gate_decision_role="descriptive_only"
  p8h_permitted=$false
  full_inference_permitted=$false
  next_step=if($Obj.status -eq "COMPLETE"){"adjudicate P8-G4 fresh-cohort compositional decomposition"}else{"repair P8-G4 package/runtime before adjudication"}
}
[IO.File]::WriteAllText((Join-Path $Results "p8g4_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g4_shader_provenance.json"
Write-Host "  results\p8g4_fresh_compositional_results.json"
Write-Host "  results\p8g4_summary.json"
exit $Code
