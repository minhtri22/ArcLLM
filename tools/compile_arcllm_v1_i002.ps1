param()
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here

& (Join-Path $Root "tools\compile_q2_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "I002 Q2 shader compile failed"}

& (Join-Path $Root "tools\compile_sa1_component.ps1") -Stage K1
if($LASTEXITCODE -ne 0){throw "I002 SA1 candidate compile failed"}

$Src=Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"
$Blob=(& git -C $Root rev-parse "HEAD:shaders/sa1_q4k_subgroup_splitk.comp").Trim()
if($Blob -ne "56999d88dc1bef6486e7e1908982f6de4b0f9f6a"){throw "I002 candidate source blob drift"}

$Candidate=Join-Path $Root "artifacts\SA1_K1\build\sa1_q4k_subgroup_splitk.spv"
if(-not(Test-Path $Candidate)){throw "I002 candidate SPIR-V missing"}
$Hash=(Get-FileHash $Candidate -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne "B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "I002 candidate SPIR-V drift"}

$OutDir=Join-Path $Root "compiled_shaders"
Copy-Item $Candidate (Join-Path $OutDir "sa1_q4k_subgroup_splitk.spv") -Force

$Prov=[ordered]@{
 schema="arcllm.v1.i002.shader_provenance.v0.1"
 candidate_source_blob=$Blob
 candidate_spv_sha256=$Hash
 candidate_historical_sa1_spv_sha256="B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"
 q2_shader_provenance_sha256=(Get-FileHash (Join-Path $Root "results\q2_arcllm_shader_provenance.json") -Algorithm SHA256).Hash.ToUpperInvariant()
}
[IO.File]::WriteAllText((Join-Path $Root "results\i002_shader_provenance.json"),($Prov|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "I002 shader compile/provenance PASS"
