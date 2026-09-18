param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
$Tools=Join-Path $Root "tools\glslang-16.5.0"
$Zip=Join-Path $Root "tools\glslang-16.5.0.zip"
$Url="https://github.com/KhronosGroup/glslang/releases/download/16.5.0/glslang-16.5.0-windows-x86_64-release.zip"
$Expected="06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE"
$Exe=Join-Path $Tools "bin\glslang.exe"
$Out=Join-Path $Root "compiled_shaders"
$ResultDir=Join-Path $Root "results"
$ProvPath=Join-Path $ResultDir "shader_provenance.json"
New-Item -ItemType Directory -Force -Path (Split-Path $Zip -Parent),$Out,$ResultDir|Out-Null
if(-not(Test-Path $Exe)){
  Write-Host "Downloading pinned glslang 16.5.0..."
  Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $Zip
  $h=(Get-FileHash -Algorithm SHA256 $Zip).Hash.ToUpperInvariant()
  if($h -ne $Expected){throw "glslang asset SHA mismatch"}
  if(Test-Path $Tools){Remove-Item -Recurse -Force $Tools}
  Expand-Archive -Path $Zip -DestinationPath $Tools -Force
}
if(-not(Test-Path $Exe)){throw "Pinned shader compiler missing after acquisition: $Exe"}
Write-Host "Shader compiler: $Exe"
$compilerVersion=((& $Exe --version) | Out-String).Trim()
$compiled=@()
$Names=@("p7_add.comp","p7_attention_kv_online.comp","p7_attention_prefill_online.comp","p7_embedding_q6k.comp","p7_kv_store.comp","p7_lmhead_q6k_chunk.comp","p7_q4k_gemm_2d.comp","p7_q6k_gemm_2d.comp","p7_rmsnorm_seq.comp","p7_rope_seq.comp","p7_swiglu.comp","p7c_ffn_q4k_tiled.comp","p7c_ffn_q6k_tiled.comp","p7g_ffn_q4k_tiled16.comp","p7g_ffn_q6k_tiled16.comp")
foreach($n in $Names){
  $src=Join-Path $Root ("shaders\"+$n)
  $spv=Join-Path $Out ([IO.Path]::GetFileNameWithoutExtension($n)+".spv")
  Write-Host "Compile $n"
  & $Exe -V --target-env vulkan1.2 -S comp -o $spv $src
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  if(-not(Test-Path $spv)){throw "Compiler returned success but SPIR-V is missing: $spv"}
  $bytes=(Get-Item $spv).Length
  if($bytes -le 0){throw "Compiler returned empty SPIR-V: $spv"}
  $compiled += [ordered]@{source=$n;source_sha256=(Get-FileHash -Algorithm SHA256 $src).Hash.ToUpperInvariant();spv=[IO.Path]::GetFileName($spv);spv_sha256=(Get-FileHash -Algorithm SHA256 $spv).Hash.ToUpperInvariant();spv_bytes=$bytes}
}
$ProvObj=[ordered]@{schema="arcllm.p7h.shader_provenance.v1";compiler="glslang.exe";compiler_release="16.5.0";compiler_asset="glslang-16.5.0-windows-x86_64-release.zip";compiler_asset_url=$Url;compiler_asset_sha256=$Expected;compiler_version_output=$compilerVersion;target_env="vulkan1.2";compiled=$compiled}
$json=$ProvObj|ConvertTo-Json -Depth 8
$utf8NoBom=New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($ProvPath,$json,$utf8NoBom)
if(-not(Test-Path $ProvPath)){throw "Shader provenance write returned but file is missing: $ProvPath"}
if((Get-Item $ProvPath).Length -le 0){throw "Shader provenance file is empty: $ProvPath"}
$ProvCheck=Get-Content $ProvPath -Raw -Encoding UTF8|ConvertFrom-Json
if($ProvCheck.schema -ne "arcllm.p7h.shader_provenance.v1"){throw "Shader provenance schema verification failed"}
if(@($ProvCheck.compiled).Count -ne $Names.Count){throw "Shader provenance compiled-count verification failed"}
foreach($entry in @($ProvCheck.compiled)){
  $src=Join-Path $Root ("shaders\"+$entry.source)
  $spv=Join-Path $Out $entry.spv
  if((Get-FileHash -Algorithm SHA256 $src).Hash.ToUpperInvariant() -ne $entry.source_sha256){throw "Source hash verification failed: $($entry.source)"}
  if((Get-FileHash -Algorithm SHA256 $spv).Hash.ToUpperInvariant() -ne $entry.spv_sha256){throw "SPIR-V hash verification failed: $($entry.spv)"}
}
Write-Host "Shader provenance materialized and verified: results\shader_provenance.json"
Write-Host "Shader compile: PASS"
