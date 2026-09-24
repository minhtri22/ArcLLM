param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$LockPath=Join-Path $Root "config\arcllm_v1_i003_execution_lock_v0.1.1.json"
if(-not(Test-Path $LockPath)){throw "I003 execution lock missing"}
$L=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$L.schema-ne"arcllm.v1.i003.execution_lock.v0.1.1"){throw "I003 lock schema mismatch"}
if([bool]$L.fresh_measurement_authorized){throw "I003 preflight lock unexpectedly authorizes measurement"}

$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
$Head=(& git -C $Root rev-parse HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "I003 requires research/arcllm-v1"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "Tracked worktree dirty"}
foreach($P in $L.critical_git_blobs.PSObject.Properties){
  $Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim()
  if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "I003 critical blob mismatch: $($P.Name)"}
}

$PythonFiles=@(
 (Join-Path $Root "tests\test_arcllm_v1_i003_package.py"),
 (Join-Path $Root "tools\summarize_arcllm_v1_i003.py")
)
& py -3 -m py_compile @PythonFiles
if($LASTEXITCODE-ne0){throw "I003 Python syntax preflight failed"}
& py -3 (Join-Path $Root "tests\test_arcllm_v1_i003_package.py")
if($LASTEXITCODE-ne0){throw "I003 static QA failed"}

& (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
if($LASTEXITCODE-ne0){throw "I003 shader compile failed"}
& (Join-Path $Root "tools\build_arcllm_v1_i003_candidate.ps1")
if($LASTEXITCODE-ne0){throw "I003 candidate build failed"}
& (Join-Path $Root "tools\qualify_arcllm_v1_i003_baseline.ps1")
if($LASTEXITCODE-ne0){throw "I003 baseline build qualification failed"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count-lt1){throw "I003 model resolver failed"}
  $ModelPath=[string]$Resolved[-1]
}
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
$ModelBytes=(Get-Item $ModelPath).Length
if($ModelHash-ne[string]$L.target_model.sha256-or$ModelBytes-ne[int64]$L.target_model.bytes){throw "I003 target model mismatch"}

$CandidateExe=Join-Path $Root "arcllm_v1_i003_candidate.exe"
$LlamaExe=Join-Path $Root "artifacts\i003_baseline\i003_llama_adapter.exe"
$Spv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
foreach($P in @($CandidateExe,$LlamaExe,$Spv)){if(-not(Test-Path $P)){throw "I003 preflight artifact missing: $P"}}
$CandidateHash=(Get-FileHash $CandidateExe -Algorithm SHA256).Hash.ToUpperInvariant()
$LlamaHash=(Get-FileHash $LlamaExe -Algorithm SHA256).Hash.ToUpperInvariant()
$SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
if($SpvHash-ne"B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"){throw "I003 candidate SPIR-V mismatch"}

$Results=Join-Path $Root "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
foreach($W in @("W-S","W-C")){
  $Safe=$W.Replace("-","_")
  $Out=Join-Path $Results ("i003_baseline_runtime_qualification_"+$Safe+".json")
  & $LlamaExe --model $ModelPath --workload $W --qualify-only --out $Out
  if($LASTEXITCODE-ne0){throw "I003 baseline runtime qualification failed: $W"}
  $Q=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
  if([string]$Q.status-ne"QUALIFIED"-or-not[bool]$Q.runtime.full_offload-or[bool]$Q.decode_executed-or[int]$Q.measured_attempts-ne0){throw "I003 baseline runtime qualification mismatch: $W"}
  if([string]$Q.baseline_commit-ne"b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"){throw "I003 baseline commit mismatch: $W"}
}

$OS=Get-CimInstance Win32_OperatingSystem;$GPU=@(Get-CimInstance Win32_VideoController);$CPU=@(Get-CimInstance Win32_Processor)
if([string]$OS.BuildNumber-ne[string]$L.environment.os_build){throw "I003 OS build mismatch"}
if(@($CPU|Where-Object {$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "I003 CPU mismatch"}
if(@($GPU|Where-Object {$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$L.environment.gpu_driver}).Count-lt1){throw "I003 GPU/driver mismatch"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\arcllm_v1_i003_preflight_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Report=[ordered]@{
 schema="arcllm.v1.i003.preflight.v0.1"
 execution_lock_schema=[string]$L.schema
 execution_lock_file="arcllm_v1_i003_execution_lock_v0.1.1.json"
 result="PASS_I003_ZERO_SCIENCE_MATCHED_PACKAGE"
 git_head=$Head
 branch=$Branch
 science_measured_inferences=0
 baseline_qualification_model_load_only=$true
 candidate_inference_executed=$false
 fresh_measurement_authorized=$false
 candidate_exe_sha256=$CandidateHash
 candidate_exe_bytes=(Get-Item $CandidateExe).Length
 llama_exe_sha256=$LlamaHash
 llama_exe_bytes=(Get-Item $LlamaExe).Length
 candidate_spv_sha256=$SpvHash
 target_model_sha256=$ModelHash
 target_model_bytes=$ModelBytes
 baseline_commit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
 baseline_release="v0.4.1"
 baseline_runtime_qualification=@("W-S","W-C")
 next="RETURN_PREFLIGHT_BUNDLE_FOR_INDEPENDENT_AUTHORIZATION"
}
[IO.File]::WriteAllText((Join-Path $Dir "I003_PREFLIGHT_REPORT.json"),($Report|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Copy-Item $LockPath (Join-Path $Dir "arcllm_v1_i003_execution_lock_v0.1.1.json") -Force
Copy-Item (Join-Path $Results "i003_shader_provenance.json") (Join-Path $Dir "i003_shader_provenance.json") -Force
Copy-Item (Join-Path $Results "i003_baseline_qualification.json") (Join-Path $Dir "i003_baseline_qualification.json") -Force
Copy-Item (Join-Path $Results "i003_baseline_runtime_qualification_W_S.json") (Join-Path $Dir "i003_baseline_runtime_qualification_W_S.json") -Force
Copy-Item (Join-Path $Results "i003_baseline_runtime_qualification_W_C.json") (Join-Path $Dir "i003_baseline_runtime_qualification_W_C.json") -Force
$Bundle=Join-Path $Dir "arcllm_v1_i003_preflight_return_to_chatgpt.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object {$_.FullName-ne$Bundle}|Select-Object -ExpandProperty FullName) -DestinationPath $Bundle -Force
Write-Host "I003_PREFLIGHT_RESULT=PASS_I003_ZERO_SCIENCE_MATCHED_PACKAGE"
Write-Host "MEASURED_INFERENCES=0"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant())"
