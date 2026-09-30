param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$Cfg=Get-Content (Join-Path $Here "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
$ExpectedSize=[int64]$Cfg.size_bytes;$ExpectedHash=([string]$Cfg.sha256).ToUpperInvariant()
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "P8 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$f=Get-Item $ModelPath;if($f.Length -ne $ExpectedSize){throw "P8-E target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant();if($Hash -ne $ExpectedHash){throw "P8-E target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8d_shader_provenance.authoritative.json"
$ParentGraph=Join-Path $Here "inputs\p8d_graph_binding_results.authoritative.json"
$ParentSummary=Join-Path $Here "inputs\p8d_summary.authoritative.json"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "FABD3DAE027DB5AA69E037FE179BCD0C4F5A16C5D3AAFF0E08A938101AC8F454"){throw "P8-E parent shader SHA mismatch"}
if((Get-FileHash $ParentGraph -Algorithm SHA256).Hash.ToUpperInvariant() -ne "53C373BD3BA1A9BD31B45702CED08E2EB39C7F2B7B099E052EC8C5153A824ABD"){throw "P8-E parent graph SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "74624BFAC44F4E5B9A6F08AC972508A353DAB96E0B6D4B92E8C658ABB1651D27"){throw "P8-E parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8e_package.py")
if($LASTEXITCODE -ne 0){throw "P8-E static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8e_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-E shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8e.ps1")
if($LASTEXITCODE -ne 0){throw "P8-E native build failed"}

$Results=Join-Path $Here "results";$Out=Join-Path $Results "p8e_single_layer_results.json"
& (Join-Path $Here "arcllm_p8e.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-graph $ParentGraph --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-E result JSON missing"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-F bounded multi-layer-prefix correctness design"}elseif($Obj.status -eq "FAIL"){"adjudicate first failing P8-E checkpoint"}else{"repair package/build/runtime before scientific adjudication"}
$Summary=[ordered]@{
 schema="arcllm.p8e.summary.v1";target_sha256=$Hash;target_size_bytes=$f.Length;process_exit_code=$Code;status=$Obj.status;p8e_pass=($Obj.status -eq "PASS");
 full_inference_permitted=$false;multi_layer_prefix_design_permitted=($Obj.status -eq "PASS");next_step=$Next
}
[IO.File]::WriteAllText((Join-Path $Results "p8e_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8e_shader_provenance.json"
Write-Host "  results\p8e_single_layer_results.json"
Write-Host "  results\p8e_summary.json"
exit $Code
