$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$OutDir=Join-Path $Root "compiled_shaders"
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $OutDir,$Results|Out-Null

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_vulkan_backend_v4.ps1")
if($LASTEXITCODE-ne0){throw "canonical Q4 backend shader qualification failed"}

$Pinned=Join-Path $Root "tools\glslang-16.5.0\bin\glslang.exe"
if(Test-Path $Pinned){$Glslang=$Pinned;$CompilerMode="PINNED_16_5_0"}
else{
  $Cmd=Get-Command glslang.exe -ErrorAction SilentlyContinue
  if($Cmd){$Glslang=$Cmd.Source}
  elseif(Test-Path "C:\VulkanSDK\1.4.357.0\Bin\glslang.exe"){$Glslang="C:\VulkanSDK\1.4.357.0\Bin\glslang.exe"}
  else{throw "glslang unavailable"}
  $CompilerMode="INHERITED_SHADER_REBUILD_FALLBACK"
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
 "p7_q6k_gemm_2d.comp",
 "p8c_embedding_q4k_segmented_probe.comp",
 "p8q1_lmhead_q6k_segmented_chunk.comp"
)
$Rows=@()
foreach($Name in $Names){
  $Src=Join-Path $Root ("shaders\"+$Name)
  $Spv=Join-Path $OutDir ([IO.Path]::GetFileNameWithoutExtension($Name)+".spv")
  & $Glslang -V --target-env vulkan1.2 -S comp -o $Spv $Src
  if($LASTEXITCODE-ne0){throw "full-runtime inherited shader compile failed: $Name"}
  $Rel="shaders/"+$Name
  $Rows+=[ordered]@{
    source=$Rel
    source_blob=((& git -C $Root rev-parse ("HEAD:"+$Rel)).Trim())
    spv=[IO.Path]::GetFileName($Spv)
    spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
    spv_bytes=(Get-Item $Spv).Length
  }
}
$Obj=[ordered]@{
 schema="arcllm.v1.full_runtime.q4_v4.shader_provenance.v0.1"
 status="PASS_INHERITED_SHADER_BUILD_AND_Q4_CRITICAL_BYTE_LOCK"
 compiler_mode=$CompilerMode
 compiler_path=$Glslang
 inherited_non_target_shader_count=$Rows.Count
 inherited=$Rows
 q4_critical_byte_locked=$true
 performance_authorized=$false
}
[IO.File]::WriteAllText((Join-Path $Results "FULL_ARCLLM_RUNTIME_Q4_V4_SHADER_PROVENANCE.json"),($Obj|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "FULL_ARCLLM_RUNTIME_Q4_V4_SHADER_BUILD=PASS"
