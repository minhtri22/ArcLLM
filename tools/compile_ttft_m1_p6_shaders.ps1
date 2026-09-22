param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Tools=Join-Path $Root "tools\glslang-16.5.0"
$ZipPath=Join-Path $Root "tools\glslang-16.5.0.zip"
$Url="https://github.com/KhronosGroup/glslang/releases/download/16.5.0/glslang-16.5.0-windows-x86_64-release.zip"
$ExpectedCompiler="06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE"
$Exe=Join-Path $Tools "bin\glslang.exe"
$Base=Join-Path $Root "artifacts\TTFT_M1\P6"
$ReproDir=Join-Path $Base "candidate_reproduction"
$Out=Join-Path $Base "H_ART_BUILDONLY.json"

$SafeAuthPath=Join-Path $Root "config\q2_execution_authorization.json"
$CandidateAdjPath=Join-Path $Root "artifacts\ANL64\ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json"
if(-not(Test-Path $SafeAuthPath)){throw "TTFT M1 missing frozen SAFE authorization"}
if(-not(Test-Path $CandidateAdjPath)){throw "TTFT M1 missing frozen candidate BuildOnly adjudication"}
$SafeAuth=Get-Content $SafeAuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$CandAdj=Get-Content $CandidateAdjPath -Raw -Encoding UTF8|ConvertFrom-Json

if($CandAdj.shader_build.compiler_release -ne "16.5.0"){throw "TTFT M1 candidate compiler release mismatch"}
if($CandAdj.shader_build.target_env -ne "vulkan1.2"){throw "TTFT M1 candidate target-env mismatch"}
if($CandAdj.shader_build.compiler_asset_sha256 -ne $ExpectedCompiler){throw "TTFT M1 candidate compiler asset mismatch"}

New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$ReproDir|Out-Null
if(-not(Test-Path $Exe)){
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
  $AssetHash=(Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant()
  if($AssetHash -ne $ExpectedCompiler){throw "TTFT M1 glslang asset SHA mismatch"}
  if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools}
  Expand-Archive -Path $ZipPath -DestinationPath $Tools -Force
}

$Common=@(
 "p7_rmsnorm_seq.comp",
 "p7c_ffn_q4k_tiled.comp",
 "p7c_ffn_q6k_tiled.comp",
 "p7_rope_seq.comp",
 "p7_kv_store.comp",
 "p7_attention_prefill_online.comp",
 "p7_attention_kv_online.comp",
 "p7_add.comp",
 "p7l_ffn_q4k_gateup_fused.comp",
 "p7_swiglu.comp",
 "p7g_ffn_q4k_tiled16.comp",
 "p7g_ffn_q6k_tiled16.comp",
 "p7_q4k_gemm_2d.comp",
 "p7_q6k_gemm_2d.comp",
 "p8c_embedding_q4k_segmented_probe.comp",
 "p8q1_lmhead_q6k_segmented_chunk.comp"
)

$Rows=@()
$AllHistoricalMatch=$true
foreach($Name in $Common){
  $Src=Join-Path $Root ("shaders\"+$Name)
  $SpvName=[IO.Path]::GetFileNameWithoutExtension($Name)+".spv"
  $Spv=Join-Path $ReproDir $SpvName
  $SourceHash=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()

  $SafeSourceHash=[string]$SafeAuth.shader_source_sha256.$Name
  if([string]::IsNullOrWhiteSpace($SafeSourceHash)){throw "TTFT M1 SAFE source hash absent for $Name"}
  if($SourceHash -ne $SafeSourceHash.ToUpperInvariant()){throw "TTFT M1 common source drift vs historical SAFE: $Name"}

  & $Exe -V --target-env vulkan1.2 -S comp -o $Spv $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}

  $ReproducedHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  $SafeHistoricalHash=[string]$SafeAuth.compiled_shader_sha256.$SpvName
  if([string]::IsNullOrWhiteSpace($SafeHistoricalHash)){throw "TTFT M1 SAFE compiled hash absent for $SpvName"}
  $SafeHistoricalHash=$SafeHistoricalHash.ToUpperInvariant()
  $Match=$ReproducedHash -eq $SafeHistoricalHash
  if(-not $Match){$AllHistoricalMatch=$false}

  $Rows += [ordered]@{
    source=$Name
    source_sha256=$SourceHash
    spv=$SpvName
    historical_safe_sha256=$SafeHistoricalHash
    candidate_reproduction_sha256=$ReproducedHash
    exact_match=$Match
  }
}

$Q4Safe=($Rows|Where-Object {$_.spv -eq "p7_q4k_gemm_2d.spv"}).candidate_reproduction_sha256
$Q6Safe=($Rows|Where-Object {$_.spv -eq "p7_q6k_gemm_2d.spv"}).candidate_reproduction_sha256
if($Q4Safe -ne ([string]$CandAdj.shader_build.q4_safe_spv_sha256).ToUpperInvariant()){throw "TTFT M1 candidate Q4-safe sentinel mismatch"}
if($Q6Safe -ne ([string]$CandAdj.shader_build.q6_safe_spv_sha256).ToUpperInvariant()){throw "TTFT M1 candidate Q6-safe sentinel mismatch"}

$FastSrc=Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"
$FastSpv=Join-Path $ReproDir "anl64_q4_fast.spv"
& $Exe -V --target-env vulkan1.2 -S comp -o $FastSpv $FastSrc
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$FastSpvHash=(Get-FileHash $FastSpv -Algorithm SHA256).Hash.ToUpperInvariant()
$ExpectedFast=([string]$CandAdj.shader_build.q4_fast_spv_sha256).ToUpperInvariant()
if($ExpectedFast -ne "B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "TTFT M1 frozen candidate Q4FAST hash drift"}
if($FastSpvHash -ne $ExpectedFast){throw "TTFT M1 Q4FAST SPIR-V historical reproduction mismatch"}

$Obj=[ordered]@{
  schema="arcllm.ttft_m1.p6.h_art_buildonly.v0.2"
  method="CANDIDATE_REPRODUCTION_VS_FROZEN_HISTORICAL_SAFE"
  safe_authorization_git_blob=((& git -C $Root rev-parse "HEAD:config/q2_execution_authorization.json").Trim())
  candidate_p4_adjudication_git_blob=((& git -C $Root rev-parse "HEAD:artifacts/ANL64/ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json").Trim())
  compiler="glslang.exe"
  compiler_release="16.5.0"
  compiler_asset_sha256=$ExpectedCompiler
  target_env="vulkan1.2"
  common_shader_count=$Common.Count
  common_prefill_artifacts_exact_match=$AllHistoricalMatch
  classification=if($AllHistoricalMatch){"H_ART_FALSIFIED_STATIC"}else{"H_ART_SUPPORTED_STATIC_STOP_TIMING"}
  q4safe_candidate_reproduction_sha256=$Q4Safe
  q4safe_candidate_historical_sha256=([string]$CandAdj.shader_build.q4_safe_spv_sha256).ToUpperInvariant()
  q6safe_candidate_reproduction_sha256=$Q6Safe
  q6safe_candidate_historical_sha256=([string]$CandAdj.shader_build.q6_safe_spv_sha256).ToUpperInvariant()
  q4fast_source="sa1_q4k_subgroup_splitk.comp"
  q4fast_source_sha256=(Get-FileHash $FastSrc -Algorithm SHA256).Hash.ToUpperInvariant()
  q4fast_spv_sha256=$FastSpvHash
  q4fast_spv_expected_sha256=$ExpectedFast
  target_model_loaded=$false
  gpu_dispatch=$false
  performance_measurement=$false
  common=$Rows
}
[IO.File]::WriteAllText($Out,($Obj|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
if(-not $AllHistoricalMatch){throw "TTFT M1 H-ART SUPPORTED STATIC: candidate reproduction differs from frozen historical SAFE; timing must stop"}
Write-Host "TTFT M1 H-ART BuildOnly: H_ART_FALSIFIED_STATIC"
