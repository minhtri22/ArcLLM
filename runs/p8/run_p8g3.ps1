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
if($f.Length -ne $ExpectedSize){throw "P8-G3 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-G3 target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8g2_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g2_amplification_geometry_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g2_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4"){throw "P8-G3 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168"){throw "P8-G3 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6"){throw "P8-G3 parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8g3_package.py")
if($LASTEXITCODE -ne 0){throw "P8-G3 static contract failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8g3_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G3 shader compile failed"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8g3.ps1")
if($LASTEXITCODE -ne 0){throw "P8-G3 native build failed"}

$Results=Join-Path $Here "results"
$Out=Join-Path $Results "p8g3_arithmetic_precision_results.json"
& (Join-Path $Here "arcllm_p8g3.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-G3 result JSON missing"}

$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Classification=if($Obj.attribution){[string]$Obj.attribution.classification}else{"UNAVAILABLE"}
$Summary=[ordered]@{
  schema="arcllm.p8g3.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  diagnostic_valid=([bool]$Obj.diagnostic_valid)
  classification=$Classification
  closure_earliest_pass=if($Obj.attribution){[string]$Obj.attribution.closure_earliest_pass}else{"UNAVAILABLE"}
  alpha_earliest_pass=if($Obj.attribution){[string]$Obj.attribution.alpha_earliest_pass}else{"UNAVAILABLE"}
  p8g_verdict_frozen="FAIL"
  p8g1_classification_frozen="H-AMPLIFICATION"
  p8g2_classification_frozen="H-NONLINEAR/UNEXPLAINED"
  replacement_gate_defined=$false
  p8h_permitted=$false
  full_inference_permitted=$false
  next_step=if($Obj.status -eq "COMPLETE"){"adjudicate P8-G3 arithmetic-precision attribution"}else{"repair P8-G3 reproduction/package/runtime before adjudication"}
}
[IO.File]::WriteAllText((Join-Path $Results "p8g3_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8g3_shader_provenance.json"
Write-Host "  results\p8g3_arithmetic_precision_results.json"
Write-Host "  results\p8g3_summary.json"
exit $Code
