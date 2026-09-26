param()
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here

powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_arcllm_v1_q4_down_4arm.ps1")
if($LASTEXITCODE-ne0){throw "B1.2 inherited Q4 shader compile failed"}

$Glslang=Join-Path $Root "tools\glslang-16.5.0\bin\glslang.exe"
if(-not(Test-Path $Glslang)){throw "B1.2 pinned glslang missing"}
$OutDir=Join-Path $Root "compiled_shaders"
$Results=Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $OutDir,$Results|Out-Null

$Rel="shaders/b1_2_exec148_gpu_materialize.comp"
$Src=Join-Path $Root ($Rel-replace'/','\')
$Spv=Join-Path $OutDir "b1_2_exec148_gpu_materialize.comp.spv"
& $Glslang -V --target-env vulkan1.2 -S comp -o $Spv $Src
if($LASTEXITCODE-ne0){throw "B1.2 P1 shader compile failed"}

$Prov=[ordered]@{
 schema="arcllm.v1.b1_2.zero_science.shader_provenance.v0.1"
 status="PASS_SHADER_BUILD"
 performance_authorized=$false
 acquisition_timing_authorized=$false
 compiler="glslang.exe"
 compiler_release="16.5.0"
 target_env="vulkan1.2"
 p1=[ordered]@{
  source=$Rel
  source_git_blob=((& git -C $Root rev-parse ("HEAD:"+$Rel)).Trim())
  source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
  spv="b1_2_exec148_gpu_materialize.comp.spv"
  spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  spv_bytes=(Get-Item $Spv).Length
  local_size_x=256
  blocks_per_layer=265216
  workgroups_per_layer=1036
  dispatches=14
 }
 inherited=[ordered]@{
  base_q4_spv_sha256=(Get-FileHash (Join-Path $OutDir "p7_q4k_gemm_2d.spv") -Algorithm SHA256).Hash.ToUpperInvariant()
  b_exec148_serial_spv_sha256=(Get-FileHash (Join-Path $OutDir "q4_down_exec148_serial.spv") -Algorithm SHA256).Hash.ToUpperInvariant()
 }
}
[IO.File]::WriteAllText((Join-Path $Results "B1_2_ZERO_SCIENCE_SHADER_PROVENANCE.json"),($Prov|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
Write-Host "B1_2_ZERO_SCIENCE_SHADER_BUILD=PASS"
Write-Host "NO MODEL LOAD. NO GPU DISPATCH. NO TIMING."
