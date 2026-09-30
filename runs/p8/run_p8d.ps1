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
$f=Get-Item $ModelPath
if($f.Length -ne $ExpectedSize){throw "P8-D target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "P8-D target SHA mismatch"}

$Plan=Join-Path $Here "inputs\p8a2_segment_plan.authoritative.json"
$Access=Join-Path $Here "inputs\p8c_access_results.authoritative.json"
$ParentSummary=Join-Path $Here "inputs\p8c_summary.authoritative.json"
if((Get-FileHash $Plan -Algorithm SHA256).Hash.ToUpperInvariant() -ne "7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481"){throw "P8-D P8-A2 parent SHA mismatch"}
if((Get-FileHash $Access -Algorithm SHA256).Hash.ToUpperInvariant() -ne "9773C48A7D895EB3D22B993132854E58BC0668288725E5186E80D3462D4D5340"){throw "P8-D P8-C access SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "8620A9089CF9066088F5E30543864D7A17B87F4DC55CEFE1CD10320C01574CD4"){throw "P8-D P8-C summary SHA mismatch"}
$FrozenSums=Get-Content (Join-Path $Here "P8C_SHA256SUMS.txt") -Raw -Encoding UTF8
if($FrozenSums -notmatch "25D7E7D01F033B85692E83C102D269A37E21988D229191BCA5FF51DBC0E118E3"){throw "P8-D frozen P8-C shader provenance hash record missing"}

py -3 (Join-Path $Here "tests\test_p8d_package.py")
if($LASTEXITCODE -ne 0){throw "P8-D static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_p8d_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "P8-D shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_p8d.ps1")
if($LASTEXITCODE -ne 0){throw "P8-D native build failed"}

$Results=Join-Path $Here "results";$Out=Join-Path $Results "p8d_graph_binding_results.json"
& (Join-Path $Here "arcllm_p8d.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --plan $Plan --parent-access $Access --parent-summary $ParentSummary --out $Out
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "P8-D result JSON missing"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$Next=if($Obj.status -eq "PASS"){"P8-E bounded single-layer 7B graph correctness bring-up"}elseif($Obj.status -eq "FAIL"){"adjudicate exact P8-D graph-binding obstruction"}else{"repair package/shader/runtime before scientific adjudication"}
$Summary=[ordered]@{
  schema="arcllm.p8d.summary.v1"
  target_sha256=$Hash
  target_size_bytes=$f.Length
  process_exit_code=$Code
  status=$Obj.status
  p8d_pass=($Obj.status -eq "PASS")
  full_inference_permitted=$false
  single_layer_bringup_permitted=($Obj.status -eq "PASS")
  next_step=$Next
}
$SummaryPath=Join-Path $Results "p8d_summary.json"
[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host ""
Write-Host "Result:"
Write-Host "  results\p8d_shader_provenance.json"
Write-Host "  results\p8d_graph_binding_results.json"
Write-Host "  results\p8d_summary.json"
exit $Code
