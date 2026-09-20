param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
$Tools=Join-Path $Root "tools\glslang-16.5.0";$ZipPath=Join-Path $Root "tools\glslang-16.5.0.zip"
$Url="https://github.com/KhronosGroup/glslang/releases/download/16.5.0/glslang-16.5.0-windows-x86_64-release.zip"
$Expected="06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE"
$Exe=Join-Path $Tools "bin\glslang.exe";$OutDir=Join-Path $Root "compiled_shaders";$ProvPath=Join-Path $Root "results\q2_arcllm_shader_provenance.json"
New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$OutDir,(Join-Path $Root "results")|Out-Null
if(-not(Test-Path $Exe)){
  Write-Host "Downloading pinned glslang 16.5.0..."
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
  $AssetHash=(Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant()
  if($AssetHash -ne $Expected){throw "glslang asset SHA mismatch"}
  if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools}
  Expand-Archive -Path $ZipPath -DestinationPath $Tools -Force
}
$Names=@(
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
$Compiled=@()
foreach($Name in $Names){
  $Src=Join-Path $Root ("shaders\"+$Name);$Spv=Join-Path $OutDir ([IO.Path]::GetFileNameWithoutExtension($Name)+".spv")
  Write-Host "Compile $Name"
  & $Exe -V --target-env vulkan1.2 -S comp -o $Spv $Src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  $Compiled += [ordered]@{source=$Name;source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant();spv=[IO.Path]::GetFileName($Spv);spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant();spv_bytes=(Get-Item $Spv).Length}
}
$Obj=[ordered]@{schema="arcllm.q2.arcllm_shader_provenance.v1";compiler="glslang.exe";compiler_release="16.5.0";compiler_asset_sha256=$Expected;target_env="vulkan1.2";shader_count=$Compiled.Count;compiled=$Compiled}
[IO.File]::WriteAllText($ProvPath,($Obj|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$Check=Get-Content $ProvPath -Raw -Encoding UTF8|ConvertFrom-Json
if(@($Check.compiled).Count -ne 16){throw "Q2 shader provenance count mismatch"}
foreach($Item in $Check.compiled){
  $Src=Join-Path $Root ("shaders\"+$Item.source);$Spv=Join-Path $OutDir $Item.spv
  if((Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant() -ne $Item.source_sha256){throw "Q2 source provenance mismatch"}
  if((Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant() -ne $Item.spv_sha256){throw "Q2 SPIR-V provenance mismatch"}
}
Write-Host "Q2 shader compile: PASS"
