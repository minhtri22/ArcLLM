param(
  [Parameter(Mandatory=$true)][string]$Model,
  [string]$ShaderDir = "",
  [string]$OutDir = "",
  [string]$LockPath = ""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
if(-not $ShaderDir){$ShaderDir=Join-Path $Root "compiled_shaders"}
if(-not $OutDir){$OutDir=Join-Path $Root "results\anl64_crt_p3\fresh_collection"}
if(-not $LockPath){$LockPath=Join-Path $Root "artifacts\ANL64_CRT\ANL64_CRT_P3_EXECUTION_LOCK_v0.1.json"}

function GitBlob([string]$Rel){
  $v=(& git -C $Root rev-parse ("HEAD:"+$Rel) 2>$null)
  if($LASTEXITCODE-ne0){throw "cannot resolve git blob: $Rel"}
  return $v.Trim()
}
function Sha256([string]$Path){ return (Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant() }
function WriteJsonUtf8([string]$Path,$Obj){
  [IO.File]::WriteAllText($Path,($Obj|ConvertTo-Json -Depth 20),(New-Object Text.UTF8Encoding($false)))
}

if(-not(Test-Path $LockPath)){throw "P3 execution lock missing"}
$Lock=Get-Content $LockPath -Raw|ConvertFrom-Json
if(-not $Lock.fresh_execution_authorized){throw "P3 execution not authorized"}
foreach($p in $Lock.locked_git_blobs.PSObject.Properties){
  if((GitBlob $p.Name)-ne[string]$p.Value){throw "locked git blob mismatch: $($p.Name)"}
}
if((Sha256 $Model)-ne[string]$Lock.model_sha256){throw "model SHA256 mismatch"}

$Exe=Join-Path $Root "anl64_crt_p1r_runtime.exe"
if(-not(Test-Path $Exe)){throw "locked runtime executable missing"}
if((Sha256 $Exe)-ne[string]$Lock.runtime_exe_sha256){throw "runtime executable SHA256 mismatch"}

$SchedulePath=Join-Path $Root "config\anl64_crt_p3_blocked_randomized_schedule_v0.3.json"
$Collector=Join-Path $Root "tools\anl64_crt_p3_precell_state_v0_2.ps1"
$Schedule=Get-Content $SchedulePath -Raw|ConvertFrom-Json
if(Test-Path $OutDir){
  if(@(Get-ChildItem $OutDir -Force).Count-ne0){throw "P3 output directory is not empty; no resume/overwrite allowed"}
}else{New-Item -ItemType Directory -Force -Path $OutDir|Out-Null}

$start=[DateTime]::UtcNow
$prevCondition="NONE"
$prevEnd=""
$rows=@()
$global=0
try{
  foreach($block in $Schedule.blocks){
    $pos=0
    foreach($condition in $block.order){
      ++$pos;++$global
      $parts=[string]$condition -split '/',2
      if($parts.Count-ne2){throw "invalid frozen condition: $condition"}
      $arm=$parts[0];$work=$parts[1]
      $safeWork=$work.Replace("-","")
      $prefix=("B{0:D2}_P{1:D2}_G{2:D2}_{3}_{4}" -f [int]$block.block,$pos,$global,$arm,$safeWork)
      $stateFile=Join-Path $OutDir ($prefix+"_PRESTATE.json")
      $cellFile=Join-Path $OutDir ($prefix+"_CELL.json")

      $args=@("-NoProfile","-ExecutionPolicy","Bypass","-File",$Collector,
        "-BlockId",[string]$block.block,"-Position",[string]$pos,"-GlobalOrdinal",[string]$global,
        "-PreviousCondition",$prevCondition,"-P3StartUtc",$start.ToString("o"))
      if($prevEnd){$args+=@("-PreviousCellEndUtc",$prevEnd)}
      $stateRaw=(& powershell.exe @args | Out-String).Trim()
      $stateExit=$LASTEXITCODE
      if($stateExit-ne0){throw "precell state collector failed before G$global exit=$stateExit"}
      $state=$stateRaw|ConvertFrom-Json
      if(-not $state.environment_invariants_pass){throw "precell environment invariant failed before G$global"}
      [IO.File]::WriteAllText($stateFile,$stateRaw+[Environment]::NewLine,(New-Object Text.UTF8Encoding($false)))

      & $Exe --model $Model --shader-dir $ShaderDir --implementation-commit ((& git -C $Root rev-parse HEAD).Trim()) --workload $work --arm $arm --warmups 1 --measured 5 --out $cellFile
      if($LASTEXITCODE-ne0){throw "cell process failed at G$global"}
      $cell=Get-Content $cellFile -Raw|ConvertFrom-Json
      if($cell.status-ne"PASS_CELL" -or $cell.arm-ne$arm -or $cell.workload-ne$work){throw "invalid completed cell artifact at G$global"}
      if(@($cell.attempts).Count-ne5){throw "attempt count mismatch at G$global"}

      $rows += [ordered]@{
        block=[int]$block.block;position=$pos;global_ordinal=$global;condition=$condition;
        arm=$arm;workload=$work;previous_condition=$prevCondition;
        prestate_file=(Split-Path $stateFile -Leaf);prestate_sha256=(Sha256 $stateFile);
        cell_file=(Split-Path $cellFile -Leaf);cell_sha256=(Sha256 $cellFile)
      }
      $prevCondition=$condition
      $prevEnd=[DateTime]::UtcNow.ToString("o")
    }
  }
  if($global-ne36){throw "frozen schedule did not yield 36 cells"}
  $manifest=[ordered]@{
    schema="arcllm.anl64_crt.p3.collection_manifest.v0.1"
    status="PASS_COMPLETE_FRESH_P3_COLLECTION"
    repository_head=((& git -C $Root rev-parse HEAD).Trim())
    execution_lock_blob=(GitBlob "artifacts/ANL64_CRT/ANL64_CRT_P3_EXECUTION_LOCK_v0.1.json")
    schedule_blob=(GitBlob "config/anl64_crt_p3_blocked_randomized_schedule_v0.3.json")
    collector_blob=(GitBlob "tools/anl64_crt_p3_precell_state_v0_2.ps1")
    runtime_source_blob=(GitBlob "src/anl64_crt_p1r_runtime.cpp")
    runtime_exe_sha256=(Sha256 $Exe)
    model_sha256=(Sha256 $Model)
    cells=$rows
    measured_attempts=180
    valid_completed_cell_reruns=0
    selective_reruns=0
    adaptive_reordering=$false
  }
  WriteJsonUtf8 (Join-Path $OutDir "MANIFEST.json") $manifest
  Write-Host "ANL64_CRT_P3_COLLECTION=PASS"
}catch{
  $stop=[ordered]@{
    schema="arcllm.anl64_crt.p3.collection_stop.v0.1"
    status="STOPPED_NO_RESUME_UNDER_CURRENT_AUTHORIZATION"
    utc=[DateTime]::UtcNow.ToString("o")
    message=$_.Exception.Message
    completed_cells=@($rows).Count
    next_global_ordinal=$global
    automatic_retry=$false
    resume_permitted=$false
  }
  WriteJsonUtf8 (Join-Path $OutDir "COLLECTION_STOP.json") $stop
  throw
}
