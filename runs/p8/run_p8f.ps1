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
$f=Get-Item $ModelPath;if($f.Length -ne $ExpectedSize){throw "P8-F target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant();if($Hash -ne $ExpectedHash){throw "P8-F target SHA mismatch"}

$ParentShader=Join-Path $Here "inputs\p8e_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8e_single_layer_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8e_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861"){throw "P8-F parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B"){throw "P8-F parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D"){throw "P8-F parent summary SHA mismatch"}

py -3 (Join-Path $Here "tests\test_p8f_package.py")
if($LASTEXITCODE -ne 0){throw "P8-F static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8f_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-F shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8f.ps1")
if($LASTEXITCODE -ne 0){throw "P8-F native build failed"}

$Results=Join-Path $Here "results";$Out=Join-Path $Results "p8f_two_layer_results.json"
& (Join-Path $Here "arcllm_p8f.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-F result JSON missing"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-G larger bounded-prefix correctness design"}elseif($Obj.status -eq "FAIL"){"adjudicate first failing P8-F checkpoint"}else{"repair package/build/runtime before scientific adjudication"}
$Summary=[ordered]@{
 schema="arcllm.p8f.summary.v1";target_sha256=$Hash;target_size_bytes=$f.Length;process_exit_code=$Code;status=$Obj.status;p8f_pass=($Obj.status -eq "PASS");
 full_inference_permitted=$false;larger_prefix_design_permitted=($Obj.status -eq "PASS");next_step=$Next
}
[IO.File]::WriteAllText((Join-Path $Results "p8f_summary.json"),($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8f_shader_provenance.json"
Write-Host "  results\p8f_two_layer_results.json"
Write-Host "  results\p8f_summary.json"
exit $Code
