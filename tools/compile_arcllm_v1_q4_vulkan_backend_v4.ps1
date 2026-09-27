$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$OutDir=Join-Path $Root "compiled_shaders"
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $OutDir,$Results|Out-Null

$Pinned=Join-Path $Root "tools\glslang-16.5.0\bin\glslang.exe"
if(Test-Path $Pinned){$Glslang=$Pinned;$CompilerMode="PINNED_16_5_0"}
else{
  $Cmd=Get-Command glslang.exe -ErrorAction SilentlyContinue
  if(-not $Cmd){$Fallback="C:\VulkanSDK\1.4.357.0\Bin\glslang.exe";if(Test-Path $Fallback){$Glslang=$Fallback}else{throw "glslang unavailable"}}
  else{$Glslang=$Cmd.Source}
  $CompilerMode="BYTE_EQUIVALENCE_FALLBACK"
}
$Version=(& $Glslang --version 2>&1|Out-String).Trim()

$Specs=@(
  @("shaders\p7_q4k_gemm_2d.comp","p7_q4k_gemm_2d.spv","2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A"),
  @("shaders\sa1_q4k_subgroup_splitk.comp","sa1_q4k_subgroup_splitk.spv","B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"),
  @("shaders\q4_down_exec148_serial.comp","q4_down_exec148_serial.spv","140B8C7ADF5E88D4F1A50C4F8807874F4978070F301FCEEFC67A6A51A383D2A3"),
  @("shaders\b1_2_exec148_gpu_materialize.comp","b1_2_exec148_gpu_materialize.comp.spv","3A67C8F4FF7441CB0F13963380C1115770E7ADDA07048FC2F8476E1D271532C6")
)
$Rows=@()
foreach($S in $Specs){
  $Src=Join-Path $Root $S[0];$Dst=Join-Path $OutDir $S[1];$Expected=$S[2]
  & $Glslang -V --target-env vulkan1.2 -S comp -o $Dst $Src
  if($LASTEXITCODE-ne0){throw "shader compile failed: $($S[0])"}
  $Hash=(Get-FileHash $Dst -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Hash-ne$Expected){throw "frozen shader byte identity mismatch: $($S[1])"}
  $Rows+=[ordered]@{source=$S[0];source_blob=((& git -C $Root rev-parse ("HEAD:"+($S[0]-replace '\\','/'))).Trim());spv=$S[1];sha256=$Hash;bytes=(Get-Item $Dst).Length}
}
$Prov=[ordered]@{
 schema="arcllm.v1.phase2.q4_vulkan_backend_v4.shader_provenance.v0.1"
 status="PASS_FROZEN_SHADER_BYTE_IDENTITY"
 compiler_mode=$CompilerMode
 compiler_path=$Glslang
 compiler_version=$Version
 target_env="vulkan1.2"
 shaders=$Rows
 performance_authorized=$false
}
[IO.File]::WriteAllText((Join-Path $Results "Q4_VULKAN_BACKEND_V4_SHADER_PROVENANCE.json"),($Prov|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_VULKAN_BACKEND_V4_SHADER_BYTES=PASS"
