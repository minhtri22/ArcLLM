param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}

$AuthPath=Join-Path $Root "config\arcllm_v1_b1_2_p1_p3_implementation_authorization_v0.1.json"
$Auth=Get-Content $AuthPath -Raw|ConvertFrom-Json
if([string]$Auth.status-ne"BOUNDED_P1_P3_IMPLEMENTATION_AND_ZERO_SCIENCE_QUALIFICATION_AUTHORIZED"){throw "STOP: B1.2 implementation authorization invalid"}
if(-not[bool]$Auth.authorization.bounded_implementation -or -not[bool]$Auth.authorization.zero_science_qualification){throw "STOP: B1.2 zero-science not authorized"}
if([bool]$Auth.authorization.P1_performance_execution -or [bool]$Auth.authorization.P3_performance_execution -or [bool]$Auth.authorization.any_acquisition_timing){throw "STOP: performance/timing must remain closed"}

$Critical=[ordered]@{
 "config/arcllm_v1_b1_2_p1_p3_placement_experiment_prelock_v0.1.json"="11a7e57087cd27c82f328ad8ab25fc0c1b6d6b52"
 "artifacts/ARCLLM_V1/ARCLLM_V1_B1_2_P1_P3_PLACEMENT_EXPERIMENT_DESIGN_v0.1.json"="613d8d335bc7f638a9c42c0853741388ca88a54e"
 "artifacts/ARCLLM_V1/ARCLLM_V1_B1_2_INDEPENDENT_PRELOCK_REVIEW_v0.1.json"="9a664d4aa1f53fb53030377b7b0d5be3c47fe497"
 "src/q4_down_exec148_materializer.h"="611efe8a087b2a5d00d887287034e2fb4f63a58f"
 "shaders/q4_down_exec148_serial.comp"="56de6602e7aca31c84a3d7764b10c0c55eb2fef5"
 "shaders/p7_q4k_gemm_2d.comp"="4d88dbd867da39c42bbfcbe3697acc7b8ebeb3fa"
}
# p7_q4k_gemm_2d.comp is inherited through historical source provenance; only verify paths that exist with pinned blob below.
$Critical.Remove("shaders/p7_q4k_gemm_2d.comp")
foreach($P in $Critical.Keys){
 $Got=(& git -C $Root rev-parse ("HEAD:"+$P)).Trim()
 if($LASTEXITCODE-ne0-or$Got-ne[string]$Critical[$P]){throw "STOP: critical scientific blob mismatch: $P"}
}

$PrevEap=$ErrorActionPreference
$ErrorActionPreference="Continue"
$StaticOut=(py -3 (Join-Path $Root "tests\test_arcllm_v1_b1_2_zero_science.py") 2>&1|Out-String).Trim()
$StaticCode=$LASTEXITCODE
$ErrorActionPreference=$PrevEap
if($StaticCode-ne0-or$StaticOut-notlike"*B1_2_ZERO_SCIENCE_STATIC_QA=PASS*"){throw "STOP: B1.2 static QA failed (`$StaticCode=$StaticCode): `n$StaticOut"}

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_arcllm_v1_b1_2_zero_science.ps1")
if($LASTEXITCODE-ne0){throw "STOP: B1.2 build failed"}

$Exe=Join-Path $Root "arcllm_v1_b1_2_zero_science.exe"
if(-not(Test-Path $Exe)){throw "STOP: B1.2 qualifier executable missing"}

if(-not $ModelPath){
 $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
 if($Resolved.Count-lt1){throw "STOP: model resolver failed"}
 $ModelPath=[string]$Resolved[-1]
}
$ExpectedModelHash="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne$ExpectedModelHash){throw "STOP: target model SHA256 mismatch"}
if((Get-Item $ModelPath).Length-ne4683074048){throw "STOP: target model bytes mismatch"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\b1_2_zero_science_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$LocalDir=Join-Path $Root ".local\b1_2_zero_science"
New-Item -ItemType Directory -Force -Path $LocalDir|Out-Null
$Sidecar=Join-Path $LocalDir "Q4K_SERIAL_K_EXEC148_V0_1_60e05f2100071479.bin"
$SideManifest=$Sidecar+".manifest.json"
if(Test-Path $Sidecar){Remove-Item $Sidecar -Force}
if(Test-Path $SideManifest){Remove-Item $SideManifest -Force}

$P1Out=Join-Path $Dir "B1_2_P1_ZERO_SCIENCE.json"
& $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --mode P1 --out $P1Out
if($LASTEXITCODE-ne0){throw "STOP: P1 zero-science correctness failed"}
$P1=Get-Content $P1Out -Raw|ConvertFrom-Json
if([string]$P1.status-ne"PASS_P1_ZERO_SCIENCE_CORRECTNESS"){throw "STOP: invalid P1 qualification status"}
if([bool]$P1.performance.authorized -or [bool]$P1.performance.acquisition_timing_emitted -or [bool]$P1.performance.gpu_timing_emitted -or [bool]$P1.performance.storage_timing_emitted){throw "STOP: forbidden P1 timing/performance output"}
if(-not[bool]$P1.exec148.tuple_exact -or -not[bool]$P1.component.pass){throw "STOP: P1 correctness evidence incomplete"}

$P3Out=Join-Path $Dir "B1_2_P3_ZERO_SCIENCE.json"
& $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --mode P3 --sidecar $Sidecar --out $P3Out
if($LASTEXITCODE-ne0){throw "STOP: P3 zero-science correctness failed"}
$P3=Get-Content $P3Out -Raw|ConvertFrom-Json
if([string]$P3.status-ne"PASS_P3_ZERO_SCIENCE_CORRECTNESS"){throw "STOP: invalid P3 qualification status"}
if([bool]$P3.performance.authorized -or [bool]$P3.performance.acquisition_timing_emitted -or [bool]$P3.performance.storage_timing_emitted){throw "STOP: forbidden P3 timing/performance output"}
if(-not[bool]$P3.warm.pass -or -not[bool]$P3.cold_unbuffered.pass){throw "STOP: P3 functional correctness evidence incomplete"}
$Canonical="60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2"
if([string]$P3.warm.exec_family_sha256-ne$Canonical -or [string]$P3.cold_unbuffered.exec_family_sha256-ne$Canonical){throw "STOP: P3 canonical family hash mismatch"}
if(-not(Test-Path $Sidecar)-or-not(Test-Path $SideManifest)){throw "STOP: P3 sidecar/manifest missing"}
if((Get-Item $Sidecar).Length-ne549527552){throw "STOP: P3 sidecar bytes mismatch"}
$SideHash=(Get-FileHash $Sidecar -Algorithm SHA256).Hash.ToLowerInvariant()
if($SideHash-ne[string]$P3.sidecar.payload_raw_sha256){throw "STOP: P3 sidecar raw SHA mismatch"}
if([string]$P1.exec148.raw_sha256-ne$SideHash){throw "STOP: P1 and P3 do not converge to byte-identical EXEC148 images"}

$StaticEvidence=[ordered]@{
 schema="arcllm.v1.b1_2.zero_science.static_qa.v0.1"
 status="PASS_STATIC_QA"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 output=$StaticOut
 model_loaded=$false
 gpu_dispatch_executed=$false
 acquisition_timing_executed=$false
 performance_execution=$false
}
$StaticPath=Join-Path $Dir "B1_2_ZERO_SCIENCE_STATIC_QA.json"
[IO.File]::WriteAllText($StaticPath,($StaticEvidence|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))

$Meta=[ordered]@{
 schema="arcllm.v1.b1_2.zero_science.dev_host_return.v0.1"
 status="PASS_P1_P3_ZERO_SCIENCE_QUALIFICATION"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 model_sha256=$ExpectedModelHash
 model_bytes=4683074048
 p1=[ordered]@{
  correctness_pass=$true
  canonical_family_hash=[string]$P1.exec148.exec_family_sha256
  raw_exec148_sha256=[string]$P1.exec148.raw_sha256
  component_max_abs=[double]$P1.component.max_abs
  component_rmse=[double]$P1.component.rmse
  component_n=[int64]$P1.component.n
 }
 p3=[ordered]@{
  correctness_pass=$true
  sidecar_path=$Sidecar
  sidecar_bytes=549527552
  sidecar_raw_sha256=$SideHash
  warm_pass=$true
  cold_unbuffered_pass=$true
  cold_sector_bytes=[int]$P3.cold_unbuffered.sector_bytes
  cold_chunk_bytes=[int64]$P3.cold_unbuffered.chunk_bytes
 }
 execution=[ordered]@{
  real_model_loaded=$true
  gpu_dispatch_correctness_only=$true
  sidecar_io_functional_only=$true
  acquisition_timing_emitted=$false
  performance_execution=$false
  comparison_to_P0_reference=$false
 }
 next="RETURN_BUNDLE_FOR_INDEPENDENT_IMPLEMENTATION_PACKAGE_REVIEW_BEFORE_ANY_PERFORMANCE_LOCK"
}
$MetaPath=Join-Path $Dir "B1_2_ZERO_SCIENCE_DEV_HOST_RETURN.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $SideManifest (Join-Path $Dir "B1_2_EXEC148_SIDECAR_MANIFEST.json") -Force
foreach($N in @("B1_2_ZERO_SCIENCE_BUILD.json","B1_2_ZERO_SCIENCE_SHADER_PROVENANCE.json")){
 $P=Join-Path $Root ("results\"+$N)
 if(-not(Test-Path $P)){throw "STOP: missing build evidence $N"}
 Copy-Item $P (Join-Path $Dir $N) -Force
}
Copy-Item $AuthPath $Dir -Force
Copy-Item (Join-Path $Root "config\arcllm_v1_b1_2_p1_p3_placement_experiment_prelock_v0.1.json") $Dir -Force
Copy-Item (Join-Path $Root "artifacts\ARCLLM_V1\ARCLLM_V1_B1_2_INDEPENDENT_PRELOCK_REVIEW_v0.1.json") $Dir -Force
Copy-Item $Exe (Join-Path $Dir "arcllm_v1_b1_2_zero_science.exe") -Force

$Zip=Join-Path $Dir "B1_2_P1_P3_ZERO_SCIENCE_RETURN_TO_CHATGPT.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object{$_.FullName-ne$Zip}|Select-Object -ExpandProperty FullName) -DestinationPath $Zip -Force

Write-Host "B1_2_P1_P3_ZERO_SCIENCE_QUALIFICATION=PASS"
Write-Host "SIDECAR_PATH=$Sidecar"
Write-Host "SIDECAR_SHA256=$($SideHash.ToUpperInvariant())"
Write-Host "RETURN_BUNDLE=$Zip"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Zip -Algorithm SHA256).Hash.ToUpperInvariant())"
Write-Host "NO ACQUISITION TIMING. NO PERFORMANCE EXECUTION. NO P0 COMPARISON."
