param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
& (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE-ne0){throw "Q4-down inherited I003 shader compile failed"}

$Glslang=Join-Path $Root "tools\glslang-16.5.0\bin\glslang.exe"
if(-not(Test-Path $Glslang)){throw "Q4-down pinned glslang missing"}
$OutDir=Join-Path $Root "compiled_shaders";$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $OutDir,$Results|Out-Null

$Pairs=@(
 @("shaders\q4_down_exec148_serial.comp","q4_down_exec148_serial.spv"),
 @("shaders\q4_down_exec148_splitk32.comp","q4_down_exec148_splitk32.spv")
)
$Rows=@()
foreach($P in $Pairs){
 $Src=Join-Path $Root $P[0];$Spv=Join-Path $OutDir $P[1]
 & $Glslang -V --target-env vulkan1.2 -S comp -o $Spv $Src
 if($LASTEXITCODE-ne0){throw "Q4-down shader compile failed: $($P[0])"}
 $Rows+=[ordered]@{
  source=$P[0]
  source_git_blob=((& git -C $Root rev-parse ("HEAD:"+($P[0]-replace '\\','/'))).Trim())
  source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
  spv=$P[1]
  spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  spv_bytes=(Get-Item $Spv).Length
 }
}
$A=Join-Path $OutDir "sa1_q4k_subgroup_splitk.spv"
if(-not(Test-Path $A)){throw "Q4-down inherited Child-A SPIR-V missing"}
if((Get-FileHash $A -Algorithm SHA256).Hash.ToUpperInvariant()-ne"B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "Q4-down Child-A SPIR-V drift"}

$Prov=[ordered]@{
 schema="arcllm.v1.q4_down.4arm.shader_build.v0.1"
 performance_authorized=$false
 hardware_counters_authorized=$false
 compiler="glslang.exe"
 compiler_release="16.5.0"
 target_env="vulkan1.2"
 child_a=[ordered]@{source_blob="56999d88dc1bef6486e7e1908982f6de4b0f9f6a";spv_sha256="B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"}
 exec148_shaders=$Rows
}
$Path=Join-Path $Results "q4_down_4arm_shader_provenance.json"
[IO.File]::WriteAllText($Path,($Prov|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
Write-Host "Q4_DOWN_4ARM_SHADER_BUILD=PASS"
