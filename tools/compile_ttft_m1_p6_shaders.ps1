param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Tools=Join-Path $Root "tools\glslang-16.5.0"
$ZipPath=Join-Path $Root "tools\glslang-16.5.0.zip"
$Url="https://github.com/KhronosGroup/glslang/releases/download/16.5.0/glslang-16.5.0-windows-x86_64-release.zip"
$Expected="06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE"
$Exe=Join-Path $Tools "bin\glslang.exe"
$Base=Join-Path $Root "artifacts\TTFT_M1\P6"
$SafeDir=Join-Path $Base "shaders_safe"
$FastDir=Join-Path $Base "shaders_q4fast"
$Out=Join-Path $Base "H_ART_BUILDONLY.json"

New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$SafeDir,$FastDir|Out-Null
if(-not(Test-Path $Exe)){
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
  $AssetHash=(Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant()
  if($AssetHash -ne $Expected){throw "TTFT M1 glslang asset SHA mismatch"}
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
$AllCommonMatch=$true
foreach($Name in $Common){
  $Src=Join-Path $Root ("shaders\"+$Name)
  $SpvName=[IO.Path]::GetFileNameWithoutExtension($Name)+".spv"
  $Safe=Join-Path $SafeDir $SpvName
  $Fast=Join-Path $FastDir $SpvName

  & $Exe -V --target-env vulkan1.2 -S comp -o $Safe $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  & $Exe -V --target-env vulkan1.2 -S comp -o $Fast $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}

  $SafeHash=(Get-FileHash $Safe -Algorithm SHA256).Hash.ToUpperInvariant()
  $FastHash=(Get-FileHash $Fast -Algorithm SHA256).Hash.ToUpperInvariant()
  $Match=$SafeHash -eq $FastHash
  if(-not $Match){$AllCommonMatch=$false}
  $Rows += [ordered]@{
    source=$Name
    source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
    spv=$SpvName
    safe_sha256=$SafeHash
    q4fast_set_sha256=$FastHash
    exact_match=$Match
  }
}

$FastSrc=Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"
$FastSpv=Join-Path $FastDir "anl64_q4_fast.spv"
& $Exe -V --target-env vulkan1.2 -S comp -o $FastSpv $FastSrc
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$FastSpvHash=(Get-FileHash $FastSpv -Algorithm SHA256).Hash.ToUpperInvariant()
$ExpectedFast="B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"
if($FastSpvHash -ne $ExpectedFast){throw "TTFT M1 Q4FAST SPIR-V identity mismatch"}

$Obj=[ordered]@{
  schema="arcllm.ttft_m1.p6.h_art_buildonly.v0.1"
  compiler="glslang.exe"
  compiler_release="16.5.0"
  compiler_asset_sha256=$Expected
  target_env="vulkan1.2"
  common_shader_count=$Common.Count
  common_prefill_artifacts_exact_match=$AllCommonMatch
  classification=if($AllCommonMatch){"H_ART_FALSIFIED_STATIC"}else{"H_ART_SUPPORTED_STATIC_STOP_TIMING"}
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
if(-not $AllCommonMatch){throw "TTFT M1 H-ART SUPPORTED STATIC: common SPIR-V mismatch; timing must stop"}
Write-Host "TTFT M1 H-ART BuildOnly: H_ART_FALSIFIED_STATIC"
