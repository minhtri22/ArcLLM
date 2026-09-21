param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path;$Root=Split-Path -Parent $Here
$Tools=Join-Path $Root "tools\glslang-16.5.0";$ZipPath=Join-Path $Root "tools\glslang-16.5.0.zip"
$Url="https://github.com/KhronosGroup/glslang/releases/download/16.5.0/glslang-16.5.0-windows-x86_64-release.zip"
$Expected="06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE"
$BaselineExpected="2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A"
$Exe=Join-Path $Tools "bin\glslang.exe";$OutDir=Join-Path $Root "artifacts\SA1_K1\build"
New-Item -ItemType Directory -Force -Path (Split-Path $ZipPath -Parent),$OutDir|Out-Null
if(-not(Test-Path $Exe)){
 Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $ZipPath
 if((Get-FileHash -Algorithm SHA256 $ZipPath).Hash.ToUpperInvariant() -ne $Expected){throw "SA1 glslang asset SHA mismatch"}
 if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools};Expand-Archive -Path $ZipPath -DestinationPath $Tools -Force
}
$Pairs=@(
 @("shaders\p7_q4k_gemm_2d.comp","p7_q4k_gemm_2d.spv"),
 @("shaders\sa1_q4k_subgroup_splitk.comp","sa1_q4k_subgroup_splitk.spv")
)
$Rows=@()
foreach($P in $Pairs){
 $Src=Join-Path $Root $P[0];$Spv=Join-Path $OutDir $P[1]
 & $Exe -V --target-env vulkan1.2 -S comp -o $Spv $Src
 if($LASTEXITCODE -ne 0){throw "SA1 shader compile failed: $($P[0])"}
 $Rows+=[ordered]@{source=$P[0];source_git_blob=((& git -C $Root rev-parse ("HEAD:"+($P[0]-replace '\\','/'))).Trim());source_sha256=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant();spv=$P[1];spv_sha256=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant();spv_bytes=(Get-Item $Spv).Length}
}
if($Rows[0].spv_sha256 -ne $BaselineExpected){throw "SA1 baseline SPIR-V drift"}
$Prov=[ordered]@{schema="arcllm.sa1.k1.shader_build.v0.1";compiler="glslang.exe";compiler_release="16.5.0";compiler_asset_sha256=$Expected;target_env="vulkan1.2";shader_count=2;shaders=$Rows}
$Path=Join-Path $OutDir "shader_build.json";[IO.File]::WriteAllText($Path,($Prov|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "SA1-K1 shader compile PASS"
