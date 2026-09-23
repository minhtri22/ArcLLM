param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath=Join-Path $Root "config\arcllm_v1_i002_science_authorization.json"
if(-not(Test-Path $AuthPath)){throw "STOP: I002 fresh execution is not authorized"}
$A=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$A.schema -ne "arcllm.v1.i002.science_authorization.v0.1" -or -not[bool]$A.fresh_target_model_execution_authorized -or -not[bool]$A.t1_execution_authorized){throw "STOP: I002 T1 authorization false"}
$Head=(& git -C $Root rev-parse HEAD).Trim();$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"-or$Head-ne[string]$A.implementation_head){throw "STOP: I002 execution identity mismatch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String);if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
$Exe=Join-Path $Root "arcllm_v1_i002_t1.exe";if(-not(Test-Path $Exe)){throw "STOP: T1 executable missing; do not rebuild after authorization"}
$ExeHash=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
$ExpectedExe=if("T1"-eq"T1"){[string]$A.t1_executable_sha256}else{[string]$A.t3_executable_sha256}
if($ExeHash-ne$ExpectedExe){throw "STOP: T1 executable hash mismatch"}
$Cand=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
if((Get-FileHash $Cand -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.candidate_spv_sha256){throw "STOP: candidate SPIR-V mismatch"}
if(-not $ModelPath){$Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot);if($Resolved.Count-lt1){throw "model resolver failed"};$ModelPath=[string]$Resolved[-1]}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.target_model.sha256){throw "STOP: model hash mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$A.target_model.bytes){throw "STOP: model size mismatch"}
$OS=Get-CimInstance Win32_OperatingSystem;$GPU=@(Get-CimInstance Win32_VideoController);$CPU=@(Get-CimInstance Win32_Processor)
if([string]$OS.BuildNumber-ne[string]$A.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object {$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object {$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$A.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ");$Dir=Join-Path $Root ("results\arcllm_v1_i002_t1_"+$Stamp);New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Cells=@(@("A_WS","A","W-S"),@("A_WC","A","W-C"),@("B_WC","B","W-C"),@("B_WS","B","W-S"))
$Executed=@()
foreach($C in $Cells){
 $Name=$C[0];$Session=$C[1];$Workload=$C[2];$Out=Join-Path $Dir ("i002_t1_"+$Name+".json")
 & $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --implementation-commit $Head --session $Session --workload $Workload --out $Out
 if($LASTEXITCODE-ne0){throw "STOP: I002 T1 cell $Name failed; no self-rerun"}
 $Executed+=$Name
}
$Summary=Join-Path $Dir "I002_T1_SUMMARY.json"
py -3 (Join-Path $Root "tools\summarize_arcllm_v1_i002_t1.py") --results-dir $Dir --out $Summary
if($LASTEXITCODE-ne0){throw "STOP: I002 T1 summarizer invalid; no self-rerun"}
$Meta=[ordered]@{schema="arcllm.v1.i002.t1.run_meta.v0.1";git_head=$Head;executed_cells=$Executed;automatic_rerun=$false;selective_rerun=$false}
[IO.File]::WriteAllText((Join-Path $Dir "I002_T1_RUN_META.json"),($Meta|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
$Bundle=Join-Path $Dir "arcllm_v1_i002_t1_return_to_chatgpt.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object {$_.FullName-ne$Bundle}|Select-Object -ExpandProperty FullName) -DestinationPath $Bundle -Force
Write-Host "I002_T1_COLLECTION_COMPLETE"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant())"
