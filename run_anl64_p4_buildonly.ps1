$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath=Join-Path $Root "config\anl64_p4_implementation_authorization_v0.1.json"
$LockPath=Join-Path $Root "config\anl64_p4_implementation_lock_v0.2.json"
$BuildDir=Join-Path $Root "artifacts\ANL64\P4"
$ResultsDir=Join-Path $Root "results\anl64_p4_buildonly"
$Bundle=Join-Path $Root "results\anl64_p4_buildonly_return_to_chatgpt.zip"

function Fail([string]$Message){throw "ANL64_P4_BUILDONLY_FAIL_CLOSED: $Message"}
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

Write-Host "ANL64 P4 bounded BuildOnly runner"
Write-Host "No executable launch, model load, GPU dispatch, or performance measurement is authorized."

Require (Test-Path $AuthPath -PathType Leaf) "authorization missing"
Require (Test-Path $LockPath -PathType Leaf) "implementation lock missing"

$Tracked=(& git -C $Root status --porcelain --untracked-files=no | Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json

Require ($Auth.decision -eq "P4_BOUNDED_IMPLEMENTATION_AUTHORIZED") "wrong P4 authorization decision"
Require ([bool]$Auth.authorization.implementation) "implementation not authorized"
Require (-not[bool]$Auth.authorization.target_model_inference) "target inference must remain forbidden"
Require (-not[bool]$Auth.authorization.scientific_performance_measurement) "performance measurement must remain forbidden"
Require ($Lock.status -eq "P4_IMPLEMENTATION_LOCKED_BUILDONLY_PENDING") "implementation lock status mismatch"

foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("Git blob mismatch: "+$P.Name)
}
Require ((GitBlob "run_anl64_p4_buildonly.ps1") -eq [string]$Lock.runner_git_blob) "runner self blob mismatch"

# Historical evidence sources remain immutable.
Require ((GitBlob "src/q2_benchmark.cpp") -eq "ea1e986e22f6921e7f6c52a4fa5935121cfec663") "legacy Q2 source drift"
Require ((GitBlob "shaders/p7_q4k_gemm_2d.comp") -eq "fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a") "legacy Q4 safe shader drift"
Require ((GitBlob "shaders/sa1_q4k_subgroup_splitk.comp") -eq "56999d88dc1bef6486e7e1908982f6de4b0f9f6a") "inherited Q4_FAST shader drift"

# Generated BuildOnly outputs are non-scientific and may be cleanly regenerated.
if(Test-Path $BuildDir){Remove-Item -Recurse -Force $BuildDir}
if(Test-Path $ResultsDir){Remove-Item -Recurse -Force $ResultsDir}
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
New-Item -ItemType Directory -Force -Path $ResultsDir|Out-Null

Write-Host "1/3 static package QA"
py -3 (Join-Path $Root "tests\test_anl64_p4_package.py")
Require ($LASTEXITCODE -eq 0) "static package QA failed"

Write-Host "2/3 shader BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_anl64_p4_shaders.ps1")
Require ($LASTEXITCODE -eq 0) "shader BuildOnly failed"

Write-Host "3/3 native BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_anl64_p4.ps1")
Require ($LASTEXITCODE -eq 0) "native BuildOnly failed"

$ProvPath=Join-Path $BuildDir "shader_provenance.json"
$NativePath=Join-Path $BuildDir "native_build.json"
$Exe=Join-Path $BuildDir "anl64_p4.exe"
Require (Test-Path $ProvPath -PathType Leaf) "shader provenance missing"
Require (Test-Path $NativePath -PathType Leaf) "native build manifest missing"
Require (Test-Path $Exe -PathType Leaf) "native executable missing"

$Prov=Get-Content $ProvPath -Raw -Encoding UTF8|ConvertFrom-Json
$Native=Get-Content $NativePath -Raw -Encoding UTF8|ConvertFrom-Json
Require ($Prov.schema -eq "arcllm.anl64.p4.shader_provenance.v0.1") "shader provenance schema mismatch"
Require ([int]$Prov.shader_count -eq 17) "shader count mismatch"
$Fast=@($Prov.compiled|Where-Object {$_.source -eq "sa1_q4k_subgroup_splitk.comp" -and $_.spv -eq "anl64_q4_fast.spv"})
Require ($Fast.Count -eq 1) "Q4_FAST provenance entry mismatch"
Require (([string]$Fast[0].spv_sha256).ToUpperInvariant() -eq "B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569") "Q4_FAST SPIR-V does not reproduce closed SA1 build"
$Safe=@($Prov.compiled|Where-Object {$_.source -eq "p7_q4k_gemm_2d.comp"})
Require ($Safe.Count -eq 1) "Q4 safe provenance entry mismatch"
Require (([string]$Safe[0].spv_sha256).ToUpperInvariant() -eq "2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A") "Q4 safe SPIR-V drift"

Require (-not[bool]$Native.executable_launched) "native build claims executable launch"
Require (-not[bool]$Native.model_loaded) "native build claims model load"
Require (-not[bool]$Native.gpu_dispatch) "native build claims GPU dispatch"
Require (-not[bool]$Native.performance_measurement) "native build claims performance measurement"
Require ((Sha256 $Exe) -eq ([string]$Native.executable_sha256).ToUpperInvariant()) "executable SHA mismatch"

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)

$Result=[ordered]@{
 schema="arcllm.anl64.p4.buildonly_result.v0.1"
 status="P4_BUILDONLY_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 authorization_blob=(GitBlob "config/anl64_p4_implementation_authorization_v0.1.json")
 implementation_lock_blob=(GitBlob "config/anl64_p4_implementation_lock_v0.2.json")
 runner_blob=(GitBlob "run_anl64_p4_buildonly.ps1")
 static_test="PASS"
 shader_build="PASS"
 native_build="PASS"
 shader_count=17
 q4_fast_spv_sha256=([string]$Fast[0].spv_sha256).ToUpperInvariant()
 q4_safe_spv_sha256=([string]$Safe[0].spv_sha256).ToUpperInvariant()
 executable_sha256=(Sha256 $Exe)
 executable_bytes=(Get-Item $Exe).Length
 environment=[ordered]@{
   os_caption=$OS.Caption
   os_version=$OS.Version
   os_build=$OS.BuildNumber
   cpu=$CPU
   gpu=$GPU
 }
 zero_science=[ordered]@{
   executable_launched=$false
   model_loaded=$false
   gpu_dispatch=$false
   performance_measurement=$false
   scientific_outcome=$false
 }
 p5_authorized=$false
}
$ResultPath=Join-Path $ResultsDir "P4_BUILDONLY_RESULT.json"
[IO.File]::WriteAllText($ResultPath,($Result|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $ProvPath (Join-Path $ResultsDir "shader_provenance.json")
Copy-Item $NativePath (Join-Path $ResultsDir "native_build.json")
Copy-Item $AuthPath (Join-Path $ResultsDir "anl64_p4_implementation_authorization_v0.1.json")
Copy-Item $LockPath (Join-Path $ResultsDir "anl64_p4_implementation_lock_v0.2.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P4_STATIC_QA_v0.2.json") (Join-Path $ResultsDir "ANL64_P4_STATIC_QA_v0.2.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P4_BUILDONLY_PREFLIGHT_ADJUDICATION_v0.1.json") (Join-Path $ResultsDir "ANL64_P4_BUILDONLY_PREFLIGHT_ADJUDICATION_v0.1.json")

Compress-Archive -Path (Join-Path $ResultsDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash=Sha256 $Bundle

Write-Host ""
Write-Host "ANL64_P4_BUILDONLY_COMPLETE"
Write-Host "EXECUTABLE_NOT_LAUNCHED"
Write-Host "MODEL_NOT_LOADED"
Write-Host "GPU_DISPATCH_NOT_RUN"
Write-Host "PERFORMANCE_NOT_MEASURED"
Write-Host "P5_NOT_AUTHORIZED"
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
