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
$OutDir=Join-Path $Root "artifacts\TTFT_M2\P9B\runtime_payload\shaders"
$Manifest=Join-Path $Root "artifacts\TTFT_M2\P9B\runtime_payload\SHADER_BUILD.json"
New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$OutDir|Out-Null
if(-not(Test-Path $Exe)){
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
  $AssetHash=(Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant()
  if($AssetHash -ne $ExpectedCompiler){throw "TTFT M2 P9B glslang asset SHA mismatch"}
  if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools}
  Expand-Archive -Path $ZipPath -DestinationPath $Tools -Force
}
$Common=@(
 "p7_rmsnorm_seq.comp","p7c_ffn_q4k_tiled.comp","p7c_ffn_q6k_tiled.comp",
 "p7_rope_seq.comp","p7_kv_store.comp","p7_attention_prefill_online.comp",
 "p7_attention_kv_online.comp","p7_add.comp","p7l_ffn_q4k_gateup_fused.comp",
 "p7_swiglu.comp","p7g_ffn_q4k_tiled16.comp","p7g_ffn_q6k_tiled16.comp",
 "p7_q4k_gemm_2d.comp","p7_q6k_gemm_2d.comp",
 "p8c_embedding_q4k_segmented_probe.comp","p8q1_lmhead_q6k_segmented_chunk.comp"
)
$Rows=@()
foreach($Name in $Common){
  $Src=Join-Path $Root ("shaders\"+$Name)
  if(-not(Test-Path $Src -PathType Leaf)){throw "missing shader source: $Name"}
  $SpvName=[IO.Path]::GetFileNameWithoutExtension($Name)+".spv"
  $Spv=Join-Path $OutDir $SpvName
  & $Exe -V --target-env vulkan1.2 -S comp -o $Spv $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  $Rows += [ordered]@{
    source=$Name
    source_git_blob=((& git -C $Root rev-parse ("HEAD:shaders/"+$Name)).Trim())
    source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
    spv=$SpvName
    spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  }
}
$FastSrc=Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"
$FastSpv=Join-Path $OutDir "anl64_q4_fast.spv"
& $Exe -V --target-env vulkan1.2 -S comp -o $FastSpv $FastSrc
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$Obj=[ordered]@{
 schema="arcllm.ttft_m2.p9b.shader_build.v0.1"
 git_head=((& git -C $Root rev-parse HEAD).Trim())
 compiler="glslang.exe"
 compiler_release="16.5.0"
 compiler_asset_sha256=$ExpectedCompiler
 target_env="vulkan1.2"
 common_shader_count=$Common.Count
 common=$Rows
 q4fast=[ordered]@{
   source="sa1_q4k_subgroup_splitk.comp"
   source_git_blob=((& git -C $Root rev-parse "HEAD:shaders/sa1_q4k_subgroup_splitk.comp").Trim())
   source_sha256=(Get-FileHash $FastSrc -Algorithm SHA256).Hash.ToUpperInvariant()
   spv="anl64_q4_fast.spv"
   spv_sha256=(Get-FileHash $FastSpv -Algorithm SHA256).Hash.ToUpperInvariant()
 }
 h_art_mechanism_adjudication_performed=$false
 target_model_loaded=$false
 executable_launched=$false
 gpu_dispatch=$false
 performance_measurement=$false
 fresh_ttft_observations=0
 scientific_result="NONE"
}
[IO.File]::WriteAllText($Manifest,($Obj|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
Write-Host "TTFT M2 P9B shader BuildOnly PASS (H-ART not adjudicated)"
