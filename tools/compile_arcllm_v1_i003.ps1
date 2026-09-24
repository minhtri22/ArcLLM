param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
& (Join-Path $Root "tools\compile_arcllm_v1_i002.ps1")
if($LASTEXITCODE-ne0){throw "I003 inherited I002 shader compile failed"}
$Src=Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"
$Blob=(& git -C $Root rev-parse "HEAD:shaders/sa1_q4k_subgroup_splitk.comp").Trim()
if($Blob-ne"56999d88dc1bef6486e7e1908982f6de4b0f9f6a"){throw "I003 candidate source drift"}
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
$Hash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash-ne"B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "I003 candidate SPIR-V drift"}
$Prov=[ordered]@{
 schema="arcllm.v1.i003.shader_provenance.v0.1"
 inherited_i002_candidate=$true
 candidate_source_blob=$Blob
 candidate_spv_sha256=$Hash
 q2_shader_provenance_sha256=(Get-FileHash (Join-Path $Root "results\q2_arcllm_shader_provenance.json") -Algorithm SHA256).Hash.ToUpperInvariant()
}
[IO.File]::WriteAllText((Join-Path $Root "results\i003_shader_provenance.json"),($Prov|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "I003 shader provenance PASS"
