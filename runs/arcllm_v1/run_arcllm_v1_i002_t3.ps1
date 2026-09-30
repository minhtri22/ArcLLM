param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$AuthPath=Join-Path $Root "config\arcllm_v1_i002_science_authorization.json"
if(-not(Test-Path $AuthPath)){throw "STOP: I002 fresh execution is not authorized"}
$A=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$A.schema -ne "arcllm.v1.i002.science_authorization.v0.4" -or -not[bool]$A.fresh_target_model_execution_authorized -or [bool]$A.t1_execution_authorized -or -not[bool]$A.t3_execution_authorized){throw "STOP: I002 T3 authorization false"}
$Head=(& git -C $Root rev-parse HEAD).Trim();$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch-ne"research/arcllm-v1"){throw "STOP: I002 execution branch mismatch"}
& git -C $Root merge-base --is-ancestor ([string]$A.implementation_head) $Head
if($LASTEXITCODE-ne0){throw "STOP: authorization HEAD is not a descendant of implementation HEAD"}
foreach($P in $A.critical_git_blobs.PSObject.Properties){
  $Got=(Get-RunLogicalGitBlob -RepoRoot $Root -Path $P.Name)
  if($LASTEXITCODE-ne0 -or $Got-ne[string]$P.Value){throw "STOP: I002 T3 critical blob mismatch: $($P.Name)"}
}
$Tracked=(git -C $Root status --porcelain --untracked-files=no|Out-String);if(-not[string]::IsNullOrWhiteSpace($Tracked)){throw "STOP: tracked worktree dirty"}
$Exe=Join-Path $Root "arcllm_v1_i002_t3.exe";if(-not(Test-Path $Exe)){throw "STOP: T3 executable missing; do not rebuild after authorization"}
$ExeHash=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
$ExpectedExe=[string]$A.t3_executable_sha256
if($ExeHash-ne$ExpectedExe){throw "STOP: T3 executable hash mismatch"}
$Cand=Join-Path $Root "compiled_shaders\sa1_q4k_subgroup_splitk.spv"
$ExpectedCandidateSpv=[string]$A.candidate.spv_sha256
if([string]::IsNullOrWhiteSpace($ExpectedCandidateSpv)){throw "STOP: authorization candidate.spv_sha256 missing"}
if((Get-FileHash $Cand -Algorithm SHA256).Hash.ToUpperInvariant()-ne$ExpectedCandidateSpv.ToUpperInvariant()){throw "STOP: candidate SPIR-V mismatch"}
if(-not $ModelPath){$Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot);if($Resolved.Count-lt1){throw "model resolver failed"};$ModelPath=[string]$Resolved[-1]}
if((Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()-ne[string]$A.target_model.sha256){throw "STOP: model hash mismatch"}
if((Get-Item $ModelPath).Length-ne[int64]$A.target_model.bytes){throw "STOP: model size mismatch"}
$OS=Get-CimInstance Win32_OperatingSystem;$GPU=@(Get-CimInstance Win32_VideoController);$CPU=@(Get-CimInstance Win32_Processor)
if([string]$OS.BuildNumber-ne[string]$A.environment.os_build){throw "STOP: OS build mismatch"}
if(@($CPU|Where-Object {$_.Name-like"*Ultra 7 258V*"}).Count-lt1){throw "STOP: CPU mismatch"}
if(@($GPU|Where-Object {$_.Name-like"*Arc*140V*" -and $_.DriverVersion-eq[string]$A.environment.gpu_driver}).Count-lt1){throw "STOP: GPU/driver mismatch"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
if($PowerScheme-notmatch[regex]::Escape([string]$A.environment.power_scheme_guid)){throw "STOP: power scheme mismatch"}
$PowerType=@"
using System;
using System.Runtime.InteropServices;
public static class ArcLlmI002T3Power {
  [StructLayout(LayoutKind.Sequential)]
  public struct SYSTEM_POWER_STATUS {
    public byte ACLineStatus; public byte BatteryFlag; public byte BatteryLifePercent; public byte SystemStatusFlag;
    public uint BatteryLifeTime; public uint BatteryFullLifeTime;
  }
  [DllImport("kernel32.dll", SetLastError=true)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool GetSystemPowerStatus(out SYSTEM_POWER_STATUS s);
}
"@
if(-not("ArcLlmI002T3Power"-as[type])){Add-Type -TypeDefinition $PowerType}
$PS=New-Object "ArcLlmI002T3Power+SYSTEM_POWER_STATUS"
if(-not[ArcLlmI002T3Power]::GetSystemPowerStatus([ref]$PS)){throw "STOP: cannot query AC state"}
if([int]$PS.ACLineStatus-ne1){throw "STOP: AC power required"}
$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$AttemptId=$Stamp+"_"+([guid]::NewGuid().ToString("N").Substring(0,8))
$Dir=Join-Path $Root ("results\arcllm_v1_i002_t3_"+$AttemptId)
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

# DEV_HOST context is observational metadata, never a hard blocker.
$CpuLoad=@($CPU|ForEach-Object {[int]$_.LoadPercentage})
$TopProcesses=@(Get-Process -ErrorAction SilentlyContinue | Sort-Object WorkingSet64 -Descending | Select-Object -First 20 Name,Id,CPU,WorkingSet64)
$DevHost=[ordered]@{
  schema="arcllm.v1.dev_host_context.v0.1"
  captured_utc=(Get-Date).ToUniversalTime().ToString("o")
  attempt_id=$AttemptId
  cpu_load_percent=$CpuLoad
  memory_total_kb=[int64]$OS.TotalVisibleMemorySize
  memory_free_kb=[int64]$OS.FreePhysicalMemory
  top_processes=$TopProcesses
  ambient_load_is_blocker=$false
}
[IO.File]::WriteAllText((Join-Path $Dir "I002_T3_DEV_HOST_CONTEXT.json"),($DevHost|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$AttemptMarker=Join-Path $Root ("results\I002_T3_ATTEMPT_"+$AttemptId+".json")
$Start=[ordered]@{
  schema="arcllm.v1.i002.t3.attempt_start.v0.2"
  attempt_id=$AttemptId
  started_utc=(Get-Date).ToUniversalTime().ToString("o")
  authorization_head=$Head
  implementation_head=[string]$A.implementation_head
  t1_bundle_sha256=[string]$A.t1_bundle_sha256
  governance_class="DEV_HOST"
  manual_full_collection_rerun_permitted=$true
  selective_cell_rerun_permitted=$false
}
[IO.File]::WriteAllText($AttemptMarker,($Start|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
$Cells=@(@("A_WS","A","W-S"),@("A_WC","A","W-C"),@("B_WC","B","W-C"),@("B_WS","B","W-S"))
$Executed=@()
foreach($C in $Cells){
 $Name=$C[0];$Session=$C[1];$Workload=$C[2];$Out=Join-Path $Dir ("i002_t3_"+$Name+".json")
 & $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --implementation-commit ([string]$A.implementation_head) --session $Session --workload $Workload --out $Out
 if($LASTEXITCODE-ne0){throw "STOP: I002 T3 cell $Name failed; no self-rerun"}
 $Executed+=$Name
}
$Summary=Join-Path $Dir "I002_T3_SUMMARY.json"
py -3 (Join-Path $Root "tools\summarize_arcllm_v1_i002_t3.py") --results-dir $Dir --out $Summary
if($LASTEXITCODE-ne0){throw "STOP: I002 T3 summarizer invalid; no self-rerun"}
$Meta=[ordered]@{
  schema="arcllm.v1.i002.t3.run_meta.v0.3"
  attempt_id=$AttemptId
  governance_class="DEV_HOST"
  authorization_head=$Head
  implementation_head=[string]$A.implementation_head
  t1_bundle_sha256=[string]$A.t1_bundle_sha256
  executed_cells=$Executed
  automatic_rerun=$false
  manual_full_collection_rerun_permitted=$true
  selective_cell_rerun_permitted=$false
  ambient_load_is_blocker=$false
  primary_dataset_rule="FIRST_COMPLETE_VALID_COLLECTION"
  t1_execution_authorized=$false
}
[IO.File]::WriteAllText((Join-Path $Dir "I002_T3_RUN_META.json"),($Meta|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Copy-Item $AuthPath (Join-Path $Dir "arcllm_v1_i002_science_authorization.json") -Force
Copy-Item $AttemptMarker (Join-Path $Dir "I002_T3_ATTEMPT_START.json") -Force
$Bundle=Join-Path $Dir "arcllm_v1_i002_t3_return_to_chatgpt.zip"
Compress-Archive -Path (Get-ChildItem -File $Dir|Where-Object {$_.FullName-ne$Bundle}|Select-Object -ExpandProperty FullName) -DestinationPath $Bundle -Force
Write-Host "I002_T3_COLLECTION_COMPLETE"
Write-Host "ATTEMPT_ID=$AttemptId"
Write-Host "DEV_HOST_GOVERNANCE=true"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$((Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant())"
