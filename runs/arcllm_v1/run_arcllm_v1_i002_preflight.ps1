param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$LockPath=Join-Path $Root "config\arcllm_v1_i002_execution_lock_v0.1.1.json"
if(-not(Test-Path $LockPath)){throw "I002 execution lock missing"}
$L=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$L.schema -ne "arcllm.v1.i002.execution_lock.v0.1.1"){throw "I002 lock schema mismatch"}
if([bool]$L.fresh_target_model_execution_authorized){throw "I002 preflight lock unexpectedly authorizes science"}

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
$Head=(& git -C $Root rev-parse HEAD).Trim()
if($Branch -ne "research/arcllm-v1"){throw "I002 requires research/arcllm-v1"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "Tracked worktree dirty"}

foreach($P in $L.critical_git_blobs.PSObject.Properties){
 $Got=(Get-RunLogicalGitBlob -RepoRoot $Root -Path $P.Name)
 if($LASTEXITCODE -ne 0 -or $Got -ne [string]$P.Value){throw "I002 critical blob mismatch: $($P.Name)"}
}

$PythonFiles=@(
  (Join-Path $Root "tests\test_arcllm_v1_i002_package.py"),
  (Join-Path $Root "tools\summarize_arcllm_v1_i002_t1.py"),
  (Join-Path $Root "tools\summarize_arcllm_v1_i002_t3.py")
)
& py -3 -m py_compile @PythonFiles
if($LASTEXITCODE -ne 0){throw "I002 Python syntax preflight failed"}

py -3 (Join-Path $Root "tests\test_arcllm_v1_i002_package.py")
if($LASTEXITCODE -ne 0){throw "I002 static QA failed"}

& (Join-Path $Root "tools\compile_arcllm_v1_i002.ps1")
if($LASTEXITCODE -ne 0){throw "I002 shader build failed"}
& (Join-Path $Root "tools\build_arcllm_v1_i002.ps1")
if($LASTEXITCODE -ne 0){throw "I002 native build failed"}

$T1=Join-Path $Root "arcllm_v1_i002_t1.exe"
$T3=Join-Path $Root "arcllm_v1_i002_t3.exe"
$Cand=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
foreach($P in @($T1,$T3,$Cand)){if(-not(Test-Path $P)){throw "I002 preflight artifact missing: $P"}}
$CandHash=(Get-FileHash $Cand -Algorithm SHA256).Hash.ToUpperInvariant()
if($CandHash -ne "B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "I002 candidate SPIR-V hash mismatch"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\arcllm_v1_i002_preflight_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Report=[ordered]@{
 schema="arcllm.v1.i002.preflight.v0.1"
 result="PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD"
 git_head=$Head
 branch=$Branch
 science_executed=$false
 model_loaded=$false
 gpu_dispatches=0
 timing_observations=0
 t0_static_proof_pass=$true
 candidate_source_blob="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"
 candidate_spv_sha256=$CandHash
 t1_executable_sha256=(Get-FileHash $T1 -Algorithm SHA256).Hash.ToUpperInvariant()
 t1_executable_bytes=(Get-Item $T1).Length
 t3_executable_sha256=(Get-FileHash $T3 -Algorithm SHA256).Hash.ToUpperInvariant()
 t3_executable_bytes=(Get-Item $T3).Length
 q2_shader_provenance_sha256=(Get-FileHash (Join-Path $Root "results\q2_arcllm_shader_provenance.json") -Algorithm SHA256).Hash.ToUpperInvariant()
 i002_shader_provenance_sha256=(Get-FileHash (Join-Path $Root "results\i002_shader_provenance.json") -Algorithm SHA256).Hash.ToUpperInvariant()
 fresh_target_model_execution_authorized=$false
 next="RETURN_PREFLIGHT_BUNDLE_FOR_INDEPENDENT_AUTHORIZATION"
}
$ReportPath=Join-Path $Dir "I002_PREFLIGHT_REPORT.json"
[IO.File]::WriteAllText($ReportPath,($Report|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Copy-Item $LockPath (Join-Path $Dir "arcllm_v1_i002_execution_lock_v0.1.1.json") -Force
Copy-Item (Join-Path $Root "results\i002_shader_provenance.json") (Join-Path $Dir "i002_shader_provenance.json") -Force
Copy-Item (Join-Path $Root "results\q2_arcllm_shader_provenance.json") (Join-Path $Dir "q2_arcllm_shader_provenance.json") -Force

$Bundle=Join-Path $Dir "arcllm_v1_i002_preflight_return_to_chatgpt.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object {$_.FullName-ne$Bundle}|Select-Object -ExpandProperty FullName) -DestinationPath $Bundle -Force
Write-Host "I002_PREFLIGHT_RESULT=PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD"
Write-Host "SCIENCE_EXECUTED=false"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant())"
