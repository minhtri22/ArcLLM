param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$AuthPath=Join-Path $Root "config\arcllm_v1_i003_science_authorization.json"
if(-not(Test-Path $AuthPath)){throw "STOP: I003 fresh comparison is not authorized"}
$A=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$A.schema-ne"arcllm.v1.i003.science_authorization.v0.1"-or-not[bool]$A.authorized){throw "STOP: invalid I003 authorization"}

$Head=(& git -C $Root rev-parse HEAD).Trim();$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: wrong branch"}
& git -C $Root merge-base --is-ancestor ([string]$A.implementation_head) $Head
if($LASTEXITCODE-ne0){throw "STOP: implementation head not ancestor"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String)
if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
foreach($P in $A.critical_git_blobs.PSObject.Properties){
  $Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim()
  if($LASTEXITCODE-ne0-or$Got-ne[string]$P.Value){throw "STOP: I003 critical blob mismatch: $($P.Name)"}
}

$CandidateExe=Join-Path $Root "arcllm_v1_i003_candidate.exe"
$LlamaExe=Join-Path $Root "artifacts\i003_baseline\i003_llama_adapter.exe"
$CandidateSpv=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
foreach($P in @($CandidateExe,$LlamaExe,$CandidateSpv)){if(-not(Test-Path $P)){throw "STOP: preflight artifact missing: $P"}}
if((Get-FileHash $CandidateExe -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.candidate_exe_sha256){throw "STOP: candidate exe hash mismatch"}
if((Get-FileHash $LlamaExe -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.llama_exe_sha256){throw "STOP: llama exe hash mismatch"}
if((Get-FileHash $CandidateSpv -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.candidate_spv_sha256){throw "STOP: candidate SPIR-V mismatch"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count-lt1){throw "STOP: model resolver failed"}
  $ModelPath=[string]$Resolved[-1]
}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.target_model.sha256){throw "STOP: model hash mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$A.target_model.bytes){throw "STOP: model size mismatch"}

$OS=Get-CimInstance Win32_OperatingSystem;$GPU=@(Get-CimInstance Win32_VideoController);$CPU=@(Get-CimInstance Win32_Processor)
if([string]$OS.BuildNumber-ne[string]$A.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object {$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object {$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$A.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$CollectionId=$Stamp+"_"+([guid]::NewGuid().ToString("N").Substring(0,8))
$Dir=Join-Path $Root ("results\arcllm_v1_i003_"+$CollectionId)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null
$Sampler=Join-Path $Root "tools\q2_resource_sampler.py"
$GpuSampler=Join-Path $Root "tools\q2_gpu_sampler.ps1"

function Write-DevHostContext([string]$Path,[string]$Cell,[int]$Pair,[string]$Arm,[int]$Ordinal){
  $osNow=Get-CimInstance Win32_OperatingSystem
  $cpuNow=@(Get-CimInstance Win32_Processor)
  $ctx=[ordered]@{
    schema="arcllm.v1.i003.dev_host_context.v0.1"
    captured_utc=(Get-Date).ToUniversalTime().ToString("o")
    collection_id=$CollectionId
    cell=$Cell;pair=$Pair;arm=$Arm;ordinal=$Ordinal
    cpu_load_percent=@($cpuNow|ForEach-Object {[int]$_.LoadPercentage})
    memory_total_kb=[int64]$osNow.TotalVisibleMemorySize
    memory_free_kb=[int64]$osNow.FreePhysicalMemory
    active_power_scheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
    top_processes=@(Get-Process -ErrorAction SilentlyContinue|Sort-Object WorkingSet64 -Descending|Select-Object -First 20 Name,Id,CPU,WorkingSet64)
    ambient_load_is_blocker=$false
  }
  [IO.File]::WriteAllText($Path,($ctx|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
}

function Invoke-I003Arm([string]$Cell,[int]$Pair,[string]$Arm,[string]$Workload,[int]$Ordinal){
  $Prefix="i003_"+$Cell+"_p"+$Pair+"_"+$Arm
  $Result=Join-Path $Dir ($Prefix+"_result.json")
  $Trace=Join-Path $Dir ($Prefix+"_resources.json")
  $Log=Join-Path $Dir ($Prefix+".log")
  $Context=Join-Path $Dir ($Prefix+"_dev_host_context.json")
  Write-DevHostContext $Context $Cell $Pair $Arm $Ordinal
  if($Arm-eq"candidate"){
    $Exe=$CandidateExe
    $Child=@("--model",$ModelPath,"--shader-dir",(Join-Path $Root "compiled_shaders"),"--implementation-commit","cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1")
    $System="ArcLLM-v1-I002"
  }else{
    $Exe=$LlamaExe;$Child=@("--model",$ModelPath);$System="llama.cpp"
  }
  $Args=@("-3",$Sampler,"--system",$System,"--workload",$Workload,"--trace",$Trace,"--log",$Log,"--gpu-script",$GpuSampler,"--sample-ms","100","--",$Exe)
  $Args+=$Child
  $Args+=@("--workload",$Workload,"--warmups","1","--measured","1","--out",$Result)
  & py @Args | Out-Host
  $Code=$LASTEXITCODE
  if(-not(Test-Path $Result)){
    $Fail=[ordered]@{schema="arcllm.v1.i003.arm_failure.v0.1";cell=$Cell;pair=$Pair;arm=$Arm;workload=$Workload;process_exit_code=$Code;error="child result missing"}
    [IO.File]::WriteAllText($Result,($Fail|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
  }
  return $Code
}

$Cells=@(
  @("A_WS","A","W-S"),
  @("A_WC","A","W-C"),
  @("B_WC","B","W-C"),
  @("B_WS","B","W-S")
)
$RunPairs=@()
foreach($C in $Cells){
  $Cell=$C[0];$Session=$C[1];$Workload=$C[2]
  for($p=0;$p-lt5;$p++){
    $Offset=if($Session-eq"B"){1}else{0}
    $CandidateFirst=((($p+$Offset)%2)-eq0)
    $Order=if($CandidateFirst){@("candidate","llama")}else{@("llama","candidate")}
    $Codes=[ordered]@{}
    for($o=0;$o-lt2;$o++){
      $Arm=$Order[$o]
      $Codes[$Arm]=Invoke-I003Arm $Cell $p $Arm $Workload $o
    }
    $RunPairs+=[ordered]@{cell=$Cell;session=$Session;workload=$Workload;pair=$p;order=$Order;exit_codes=$Codes}
  }
}

$Summary=Join-Path $Dir "I003_MATCHED_EXTERNAL_SUMMARY.json"
& py -3 (Join-Path $Root "tools\summarize_arcllm_v1_i003.py") --results-dir $Dir --out $Summary
$SummaryCode=$LASTEXITCODE

$Meta=[ordered]@{
 schema="arcllm.v1.i003.run_meta.v0.1"
 collection_id=$CollectionId
 governance_class="DEV_HOST"
 authorization_head=$Head
 implementation_head=[string]$A.implementation_head
 execution_design="4_cells_x_5_adjacent_pairs_x_2_arms"
 measured_inferences_expected=40
 pairs=$RunPairs
 automatic_rerun=$false
 manual_full_collection_rerun_permitted=$true
 selective_pair_or_cell_rerun_permitted=$false
 primary_dataset_rule="FIRST_COMPLETE_VALID_COLLECTION"
}
[IO.File]::WriteAllText((Join-Path $Dir "I003_RUN_META.json"),($Meta|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
Copy-Item $AuthPath (Join-Path $Dir "arcllm_v1_i003_science_authorization.json") -Force
Copy-Item (Join-Path $Root "results\i003_shader_provenance.json") (Join-Path $Dir "i003_shader_provenance.json") -Force
Copy-Item (Join-Path $Root "results\i003_baseline_qualification.json") (Join-Path $Dir "i003_baseline_qualification.json") -Force
foreach($N in @("i003_baseline_runtime_qualification_W_S.json","i003_baseline_runtime_qualification_W_C.json")){
  Copy-Item (Join-Path $Root ("results\"+$N)) (Join-Path $Dir $N) -Force
}

$Bundle=Join-Path $Dir "arcllm_v1_i003_return_to_chatgpt.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object {$_.FullName-ne$Bundle}|Select-Object -ExpandProperty FullName) -DestinationPath $Bundle -Force
Write-Host "I003_COLLECTION_FINISHED"
Write-Host "COLLECTION_ID=$CollectionId"
Write-Host "SUMMARY_EXIT_CODE=$SummaryCode"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant())"
exit $SummaryCode
