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
$OutDir=Join-Path $Root "artifacts\ANL64\P4\compiled_shaders"
$ProvPath=Join-Path $Root "artifacts\ANL64\P4\shader_provenance.json"

New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$OutDir,(Split-Path $ProvPath -Parent)|Out-Null
if(-not(Test-Path $Exe)){
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
  $AssetHash=(Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant()
  if($AssetHash -ne $Expected){throw "ANL64 P4 glslang asset SHA mismatch"}
  if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools}
  Expand-Archive -Path $ZipPath -DestinationPath $Tools -Force
}

$Entries=@(
  @{source="p7_rmsnorm_seq.comp";spv="p7_rmsnorm_seq.spv"},
  @{source="p7c_ffn_q4k_tiled.comp";spv="p7c_ffn_q4k_tiled.spv"},
  @{source="p7c_ffn_q6k_tiled.comp";spv="p7c_ffn_q6k_tiled.spv"},
  @{source="p7_rope_seq.comp";spv="p7_rope_seq.spv"},
  @{source="p7_kv_store.comp";spv="p7_kv_store.spv"},
  @{source="p7_attention_prefill_online.comp";spv="p7_attention_prefill_online.spv"},
  @{source="p7_attention_kv_online.comp";spv="p7_attention_kv_online.spv"},
  @{source="p7_add.comp";spv="p7_add.spv"},
  @{source="p7l_ffn_q4k_gateup_fused.comp";spv="p7l_ffn_q4k_gateup_fused.spv"},
  @{source="p7_swiglu.comp";spv="p7_swiglu.spv"},
  @{source="p7g_ffn_q4k_tiled16.comp";spv="p7g_ffn_q4k_tiled16.spv"},
  @{source="p7g_ffn_q6k_tiled16.comp";spv="p7g_ffn_q6k_tiled16.spv"},
  @{source="p7_q4k_gemm_2d.comp";spv="p7_q4k_gemm_2d.spv"},
  @{source="p7_q6k_gemm_2d.comp";spv="p7_q6k_gemm_2d.spv"},
  @{source="p8c_embedding_q4k_segmented_probe.comp";spv="p8c_embedding_q4k_segmented_probe.spv"},
  @{source="p8q1_lmhead_q6k_segmented_chunk.comp";spv="p8q1_lmhead_q6k_segmented_chunk.spv"},
  @{source="sa1_q4k_subgroup_splitk.comp";spv="anl64_q4_fast.spv"}
)

$Compiled=@()
foreach($E in $Entries){
  $Src=Join-Path $Root ("shaders\"+$E.source)
  $Spv=Join-Path $OutDir $E.spv
  Write-Host ("Compile "+$E.source+" -> "+$E.spv)
  & $Exe -V --target-env vulkan1.2 -S comp -o $Spv $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  $Compiled += [ordered]@{
    source=$E.source
    source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
    spv=$E.spv
    spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
    spv_bytes=(Get-Item $Spv).Length
  }
}
$Obj=[ordered]@{
  schema="arcllm.anl64.p4.shader_provenance.v0.1"
  compiler="glslang.exe"
  compiler_release="16.5.0"
  compiler_asset_sha256=$Expected
  target_env="vulkan1.2"
  shader_count=$Compiled.Count
  compiled=$Compiled
}
[IO.File]::WriteAllText($ProvPath,($Obj|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
if($Compiled.Count -ne 17){throw "ANL64 P4 shader count mismatch"}
Write-Host "ANL64 P4 shader BuildOnly PASS"
