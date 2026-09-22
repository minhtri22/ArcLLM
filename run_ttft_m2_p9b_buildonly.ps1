$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath=Join-Path $Root "config\arcllm_ttft_m2_p9b_buildonly_authorization_v0.2.json"
$LockPath=Join-Path $Root "config\arcllm_ttft_m2_p9b_implementation_lock_v0.2.json"
$PayloadDir=Join-Path $Root "artifacts\TTFT_M2\P9B\runtime_payload"
$ResultsDir=Join-Path $Root "results\ttft_m2_p9b_buildonly"
$Bundle=Join-Path $Root "results\ttft_m2_p9b_buildonly_return_to_chatgpt.zip"

function Fail([string]$m){throw "TTFT_M2_P9B_BUILDONLY_FAIL_CLOSED: $m"}
function Require([bool]$c,[string]$m){if(-not$c){Fail $m}}
function GitBlob([string]$p){$v=(& git -C $Root rev-parse ("HEAD:"+($p -replace '\\','/')) 2>$null);if($LASTEXITCODE-ne 0){Fail "cannot resolve $p"};return $v.Trim()}
function Sha256([string]$p){Require (Test-Path $p -PathType Leaf) ("missing file: "+$p);return (Get-FileHash $p -Algorithm SHA256).Hash.ToUpperInvariant()}

Write-Host "ARCLLM TTFT M2 P9B canonical scientific harness BuildOnly"
Write-Host "ZERO-SCIENCE: executable build only; no executable launch, model load, GPU dispatch, timing, TTFT observation or H-ART mechanism adjudication."

Require (Test-Path $AuthPath -PathType Leaf) "P9B BuildOnly authorization missing"
Require (Test-Path $LockPath -PathType Leaf) "P9B implementation lock missing"
$Tracked=(& git -C $Root status --porcelain --untracked-files=no|Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
Require ($Auth.decision -eq "M2_P9B_CANONICAL_HARNESS_BUILDONLY_AUTHORIZED") "wrong P9B authorization"
Require ([bool]$Auth.authorization.buildonly) "P9B BuildOnly not authorized"
Require (-not[bool]$Auth.authorization.diagnostic_executable_launch) "executable launch must remain forbidden"
Require (-not[bool]$Auth.authorization.target_model_execution) "target model execution must remain forbidden"
Require (-not[bool]$Auth.authorization.model_load) "model load must remain forbidden"
Require (-not[bool]$Auth.authorization.gpu_dispatch) "GPU dispatch must remain forbidden"
Require (-not[bool]$Auth.authorization.performance_measurement) "timing must remain forbidden"
Require (-not[bool]$Auth.authorization.fresh_ttft_observation) "fresh TTFT must remain forbidden"
Require (-not[bool]$Auth.authorization.h_art_mechanism_adjudication) "H-ART mechanism adjudication must remain forbidden"

Require ((GitBlob "config/arcllm_ttft_m2_p9b_implementation_lock_v0.2.json") -eq [string]$Auth.implementation_lock_blob) "P9B lock blob mismatch"
Require ((GitBlob "run_ttft_m2_p9b_buildonly.ps1") -eq [string]$Auth.buildonly_runner_blob) "P9B runner self blob mismatch"
foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("exact Git blob mismatch: "+$P.Name)
}

if(Test-Path $PayloadDir){Remove-Item -Recurse -Force $PayloadDir}
if(Test-Path $ResultsDir){Remove-Item -Recurse -Force $ResultsDir}
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
New-Item -ItemType Directory -Force -Path $ResultsDir|Out-Null

Write-Host "1/3 static M2 package QA"
py -3 (Join-Path $Root "tests\test_ttft_m2_p9b_package.py")
Require ($LASTEXITCODE-eq 0) "P9B static package QA failed"

Write-Host "2/3 shader BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_ttft_m2_p9b_shaders.ps1")
Require ($LASTEXITCODE-eq 0) "P9B shader BuildOnly failed"

Write-Host "3/3 native diagnostic BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_ttft_m2_p9b.ps1")
Require ($LASTEXITCODE-eq 0) "P9B native BuildOnly failed"

$ShaderManifest=Join-Path $PayloadDir "SHADER_BUILD.json"
$NativeManifest=Join-Path $PayloadDir "NATIVE_BUILD.json"
$Exe=Join-Path $PayloadDir "ttft_m2_diagnostic.exe"
Require (Test-Path $ShaderManifest -PathType Leaf) "shader manifest missing"
Require (Test-Path $NativeManifest -PathType Leaf) "native build manifest missing"
Require (Test-Path $Exe -PathType Leaf) "diagnostic executable missing"

$SM=Get-Content $ShaderManifest -Raw -Encoding UTF8|ConvertFrom-Json
$NM=Get-Content $NativeManifest -Raw -Encoding UTF8|ConvertFrom-Json
Require ([int]$SM.common_shader_count -eq 16) "common shader census mismatch"
Require (-not[bool]$SM.h_art_mechanism_adjudication_performed) "shader BuildOnly performed H-ART adjudication"
Require (-not[bool]$SM.target_model_loaded) "shader BuildOnly claims model load"
Require (-not[bool]$SM.executable_launched) "shader BuildOnly claims executable launch"
Require (-not[bool]$SM.gpu_dispatch) "shader BuildOnly claims GPU dispatch"
Require (-not[bool]$SM.performance_measurement) "shader BuildOnly claims performance measurement"
Require ([int]$SM.fresh_ttft_observations -eq 0) "shader BuildOnly claims TTFT observations"
Require (-not[bool]$NM.executable_launched) "native BuildOnly claims executable launch"
Require (-not[bool]$NM.model_loaded) "native BuildOnly claims model load"
Require (-not[bool]$NM.gpu_dispatch) "native BuildOnly claims GPU dispatch"
Require (-not[bool]$NM.performance_measurement) "native BuildOnly claims timing"
Require ([int]$NM.fresh_ttft_observations -eq 0) "native BuildOnly claims TTFT observations"
$ExeHash=Sha256 $Exe
Require ($ExeHash -eq ([string]$NM.executable_sha256).ToUpperInvariant()) "executable SHA mismatch"

$Result=[ordered]@{
 schema="arcllm.ttft_m2.p9b.buildonly_result.v0.1"
 status="M2_P9B_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 authorization_blob=(GitBlob "config/arcllm_ttft_m2_p9b_buildonly_authorization_v0.2.json")
 implementation_lock_blob=(GitBlob "config/arcllm_ttft_m2_p9b_implementation_lock_v0.2.json")
 buildonly_runner_blob=(GitBlob "run_ttft_m2_p9b_buildonly.ps1")
 science_runner_blob=(GitBlob "scripts/ttft_m2/run_p9_science.ps1")
 canonical_design_manifest_blob=(GitBlob "config/arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json")
 executable_sha256=$ExeHash
 executable_bytes=(Get-Item $Exe).Length
 shader_manifest_sha256=Sha256 $ShaderManifest
 native_build_manifest_sha256=Sha256 $NativeManifest
 common_shader_count=16
 h_art_mechanism_adjudication_performed=$false
 zero_science=[ordered]@{
   diagnostic_executable_launched=$false
   target_model_loaded=$false
   gpu_dispatch=$false
   performance_measurement=$false
   fresh_ttft_observations=0
   mechanism_adjudication_performed=$false
   scientific_result="NONE"
 }
 p9_scientific_execution_authorized=$false
 next="INDEPENDENT_P9B_ADJUDICATION"
}
$ResultPath=Join-Path $ResultsDir "TTFT_M2_P9B_BUILDONLY_RESULT.json"
[IO.File]::WriteAllText($ResultPath,($Result|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))

Copy-Item $AuthPath (Join-Path $ResultsDir "arcllm_ttft_m2_p9b_buildonly_authorization_v0.2.json")
Copy-Item $LockPath (Join-Path $ResultsDir "arcllm_ttft_m2_p9b_implementation_lock_v0.2.json")
Copy-Item (Join-Path $Root "config\arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json") (Join-Path $ResultsDir "arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json")
Copy-Item $PayloadDir (Join-Path $ResultsDir "runtime_payload") -Recurse

Compress-Archive -Path (Join-Path $ResultsDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash=Sha256 $Bundle
Write-Host ""
Write-Host "TTFT_M2_P9B_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION"
Write-Host "EXECUTABLE_NOT_LAUNCHED"
Write-Host "MODEL_NOT_LOADED"
Write-Host "GPU_DISPATCH_NOT_RUN"
Write-Host "PERFORMANCE_NOT_MEASURED"
Write-Host "FRESH_TTFT_OBSERVATIONS=0"
Write-Host "H_ART_MECHANISM_ADJUDICATION_NOT_PERFORMED"
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
