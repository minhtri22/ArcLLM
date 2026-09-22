$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath=Join-Path $Root "config\arcllm_ttft_m1_p6_implementation_authorization_v0.1.json"
$LockPath=Join-Path $Root "config\arcllm_ttft_m1_p6_implementation_lock_v0.2.json"
$Base=Join-Path $Root "artifacts\TTFT_M1\P6"
$ResultsDir=Join-Path $Root "results\ttft_m1_p6_buildonly"
$Bundle=Join-Path $Root "results\ttft_m1_p6_buildonly_return_to_chatgpt.zip"

function Fail([string]$Message){throw "TTFT_M1_P6_BUILDONLY_FAIL_CLOSED: $Message"}
function Require([bool]$Condition,[string]$Message){if(-not $Condition){Fail $Message}}
function GitBlob([string]$Path){
  $p=$Path -replace '\\','/'
  $v=(& git -C $Root rev-parse ("HEAD:"+$p) 2>$null)
  if($LASTEXITCODE -ne 0){Fail "cannot resolve Git blob for $Path"}
  return $v.Trim()
}
function Sha256([string]$Path){
  Require (Test-Path $Path -PathType Leaf) "missing file: $Path"
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

Write-Host "ARCLLM_TTFT_M1 P6 diagnostic BuildOnly runner"
Write-Host "No model load, executable launch, GPU dispatch, or timing science is authorized."

Require (Test-Path $AuthPath -PathType Leaf) "P6 implementation authorization missing"
Require (Test-Path $LockPath -PathType Leaf) "P6 implementation lock missing"
$Tracked=(& git -C $Root status --porcelain --untracked-files=no | Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
Require ($Auth.decision -eq "P6_BOUNDED_DIAGNOSTIC_IMPLEMENTATION_AND_BUILDONLY_AUTHORIZED") "wrong P6 authorization"
Require ([bool]$Auth.authorization.buildonly) "BuildOnly not authorized"
Require (-not[bool]$Auth.authorization.target_model_execution) "target model execution must remain forbidden"
Require (-not[bool]$Auth.authorization.gpu_dispatch) "GPU dispatch must remain forbidden"
Require (-not[bool]$Auth.authorization.performance_measurement) "performance timing must remain forbidden"
Require ($Lock.status -eq "P6_IMPLEMENTATION_LOCKED_BUILDONLY_PENDING") "implementation lock status mismatch"

foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("Git blob mismatch: "+$P.Name)
}
Require ((GitBlob "run_ttft_m1_p6_buildonly.ps1") -eq [string]$Lock.runner_git_blob) "runner self blob mismatch"

# Critical parent/production identities remain immutable.
Require ((GitBlob "src/q2_benchmark.cpp") -eq "ea1e986e22f6921e7f6c52a4fa5935121cfec663") "safe Q2 source drift"
Require ((GitBlob "src/anl64_runtime.cpp") -eq "dbcb7afed5a08e7aff3ca02a1bd95bd985076f70") "ANL64 source drift"
Require ((GitBlob "src/p8c_segmented_access_correctness.cpp") -eq "8432ca554b36a2167429b640c5ec6779cf2b3e6b") "Vulkan runtime source drift"
Require ((GitBlob "shaders/sa1_q4k_subgroup_splitk.comp") -eq "56999d88dc1bef6486e7e1908982f6de4b0f9f6a") "Q4FAST shader source drift"

# BuildOnly outputs are non-scientific and may be regenerated.
if(Test-Path $Base){Remove-Item -Recurse -Force $Base}
if(Test-Path $ResultsDir){Remove-Item -Recurse -Force $ResultsDir}
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
New-Item -ItemType Directory -Force -Path $ResultsDir|Out-Null

Write-Host "1/3 static package QA"
py -3 (Join-Path $Root "tests\test_ttft_m1_p6_package.py")
Require ($LASTEXITCODE -eq 0) "static package QA failed"

Write-Host "2/3 H-ART dual artifact BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_ttft_m1_p6_shaders.ps1")
$CompileCode=$LASTEXITCODE
$HArtPath=Join-Path $Base "H_ART_BUILDONLY.json"
Require (Test-Path $HArtPath -PathType Leaf) "H-ART evidence missing"
$HArt=Get-Content $HArtPath -Raw -Encoding UTF8|ConvertFrom-Json
Require ([int]$HArt.common_shader_count -eq 16) "H-ART common shader census mismatch"
Require (-not[bool]$HArt.target_model_loaded) "H-ART claims model load"
Require (-not[bool]$HArt.gpu_dispatch) "H-ART claims GPU dispatch"
Require (-not[bool]$HArt.performance_measurement) "H-ART claims performance measurement"

$NativeStatus="NOT_RUN"
$ExeHash=$null
$ExeBytes=$null

if($HArt.classification -eq "H_ART_SUPPORTED_STATIC_STOP_TIMING"){
  Require ($CompileCode -ne 0) "H-ART mismatch should fail the compiler wrapper"
  Write-Host "H-ART supported statically; native diagnostic build is unnecessary and future timing must STOP."
}elseif($HArt.classification -eq "H_ART_FALSIFIED_STATIC"){
  Require ($CompileCode -eq 0) "H-ART compiler returned nonzero despite falsification"
  Require ([bool]$HArt.common_prefill_artifacts_exact_match) "H-ART exact common match false"
  Write-Host "3/3 native diagnostic BuildOnly"
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_ttft_m1_p6.ps1")
  Require ($LASTEXITCODE -eq 0) "native diagnostic BuildOnly failed"
  $NativePath=Join-Path $Base "NATIVE_BUILD.json"
  $Exe=Join-Path $Base "ttft_m1_diagnostic.exe"
  Require (Test-Path $NativePath -PathType Leaf) "native build manifest missing"
  Require (Test-Path $Exe -PathType Leaf) "diagnostic executable missing"
  $Native=Get-Content $NativePath -Raw -Encoding UTF8|ConvertFrom-Json
  Require (-not[bool]$Native.executable_launched) "native build claims executable launch"
  Require (-not[bool]$Native.model_loaded) "native build claims model load"
  Require (-not[bool]$Native.gpu_dispatch) "native build claims GPU dispatch"
  Require (-not[bool]$Native.performance_measurement) "native build claims performance measurement"
  $ExeHash=Sha256 $Exe
  Require ($ExeHash -eq ([string]$Native.executable_sha256).ToUpperInvariant()) "native executable SHA mismatch"
  $ExeBytes=(Get-Item $Exe).Length
  $NativeStatus="PASS"
}else{
  Fail ("unknown H-ART classification: "+[string]$HArt.classification)
}

$Result=[ordered]@{
  schema="arcllm.ttft_m1.p6.buildonly_result.v0.1"
  status="P6_BUILDONLY_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION"
  git_head=((& git -C $Root rev-parse HEAD).Trim())
  authorization_blob=(GitBlob "config/arcllm_ttft_m1_p6_implementation_authorization_v0.1.json")
  implementation_lock_blob=(GitBlob "config/arcllm_ttft_m1_p6_implementation_lock_v0.1.json")
  runner_blob=(GitBlob "run_ttft_m1_p6_buildonly.ps1")
  static_test="PASS"
  h_art_classification=[string]$HArt.classification
  h_art_common_exact_match=[bool]$HArt.common_prefill_artifacts_exact_match
  q4fast_spv_sha256=[string]$HArt.q4fast_spv_sha256
  native_build=$NativeStatus
  executable_sha256=$ExeHash
  executable_bytes=$ExeBytes
  zero_science=[ordered]@{
    executable_launched=$false
    model_loaded=$false
    gpu_dispatch=$false
    performance_measurement=$false
    parent_p6_timing_read=$false
  }
  p7_execution_authorized=$false
}
$ResultPath=Join-Path $ResultsDir "TTFT_M1_P6_BUILDONLY_RESULT.json"
[IO.File]::WriteAllText($ResultPath,($Result|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $HArtPath (Join-Path $ResultsDir "H_ART_BUILDONLY.json")
if(Test-Path (Join-Path $Base "NATIVE_BUILD.json")){Copy-Item (Join-Path $Base "NATIVE_BUILD.json") (Join-Path $ResultsDir "NATIVE_BUILD.json")}
Copy-Item $AuthPath (Join-Path $ResultsDir "arcllm_ttft_m1_p6_implementation_authorization_v0.1.json")
Copy-Item $LockPath (Join-Path $ResultsDir "arcllm_ttft_m1_p6_implementation_lock_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\TTFT_M1\TTFT_M1_P6_STATIC_QA_v0.2.json") (Join-Path $ResultsDir "TTFT_M1_P6_STATIC_QA_v0.2.json")
Copy-Item (Join-Path $Root "artifacts\TTFT_M1\TTFT_M1_DUPLICATE_SPEC_RECONCILIATION_v0.1.json") (Join-Path $ResultsDir "TTFT_M1_DUPLICATE_SPEC_RECONCILIATION_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\TTFT_M1\TTFT_M1_P6_PRERUN_H_ART_ADJUDICATION_v0.1.json") (Join-Path $ResultsDir "TTFT_M1_P6_PRERUN_H_ART_ADJUDICATION_v0.1.json")

Compress-Archive -Path (Join-Path $ResultsDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash=Sha256 $Bundle

Write-Host ""
Write-Host "TTFT_M1_P6_BUILDONLY_COMPLETE"
Write-Host ("H_ART_CLASSIFICATION="+[string]$HArt.classification)
Write-Host ("NATIVE_BUILD="+$NativeStatus)
Write-Host "EXECUTABLE_NOT_LAUNCHED"
Write-Host "MODEL_NOT_LOADED"
Write-Host "GPU_DISPATCH_NOT_RUN"
Write-Host "PERFORMANCE_NOT_MEASURED"
Write-Host "P7_NOT_AUTHORIZED"
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
