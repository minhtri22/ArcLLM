$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$ResultsRoot=Join-Path $Root "results\anl64_p6_confirmatory"

function Sha256([string]$Path){
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

Write-Host "ANL64 P6 existing-evidence forensic inspection"
Write-Host "READ-ONLY: no model execution, no GPU dispatch, no result mutation, no rerun."

if(-not(Test-Path $ResultsRoot -PathType Container)){
  Write-Host "P6_FORENSIC_NO_RESULTS_ROOT"
  exit 2
}

$ExpectedRoot=@("P6_PREFLIGHT.json")
$ExpectedSession=@(
  "P6_SESSION_ENVIRONMENT.json",
  "P6_SESSION_META.json",
  "reference_W_S.json",
  "candidate_W_S.json",
  "reference_W_C.json",
  "candidate_W_C.json"
)

$Report=[ordered]@{
  schema="arcllm.anl64.p6.forensic_inventory.v0.1"
  results_root=$ResultsRoot
  inspected_at=(Get-Date).ToString("o")
  read_only=$true
  root_files=[ordered]@{}
  sessions=[ordered]@{}
  extra_files=@()
  conclusion=$null
}

foreach($Name in $ExpectedRoot){
  $P=Join-Path $ResultsRoot $Name
  $Report.root_files[$Name]=[ordered]@{
    exists=(Test-Path $P -PathType Leaf)
    bytes=if(Test-Path $P -PathType Leaf){(Get-Item $P).Length}else{$null}
    sha256=if(Test-Path $P -PathType Leaf){Sha256 $P}else{$null}
  }
}

$AllSessionComplete=$true
foreach($S in @("A","B")){
  $Dir=Join-Path $ResultsRoot ("session_"+$S)
  $Entry=[ordered]@{
    directory_exists=(Test-Path $Dir -PathType Container)
    files=[ordered]@{}
    meta_summary=$null
    raw_attempt_counts=[ordered]@{}
  }
  if(-not $Entry.directory_exists){$AllSessionComplete=$false}
  foreach($Name in $ExpectedSession){
    $P=Join-Path $Dir $Name
    $Exists=Test-Path $P -PathType Leaf
    $Entry.files[$Name]=[ordered]@{
      exists=$Exists
      bytes=if($Exists){(Get-Item $P).Length}else{$null}
      sha256=if($Exists){Sha256 $P}else{$null}
    }
    if(-not $Exists){$AllSessionComplete=$false}
  }
  $MetaPath=Join-Path $Dir "P6_SESSION_META.json"
  if(Test-Path $MetaPath -PathType Leaf){
    try{
      $M=Get-Content $MetaPath -Raw -Encoding UTF8|ConvertFrom-Json
      $Entry.meta_summary=[ordered]@{
        session=$M.session
        runner_process_id=$M.runner_process_id
        execution_order=$M.execution_order
        warmups_per_cell=$M.warmups_per_cell
        measured_attempts_per_cell=$M.measured_attempts_per_cell
        measured_attempts_expected=$M.measured_attempts_expected
      }
    }catch{
      $Entry.meta_summary=[ordered]@{parse_error=$_.Exception.Message}
      $AllSessionComplete=$false
    }
  }
  foreach($Name in @("reference_W_S.json","candidate_W_S.json","reference_W_C.json","candidate_W_C.json")){
    $P=Join-Path $Dir $Name
    if(Test-Path $P -PathType Leaf){
      try{
        $J=Get-Content $P -Raw -Encoding UTF8|ConvertFrom-Json
        $Entry.raw_attempt_counts[$Name]=if($null -ne $J.attempts){@($J.attempts).Count}else{0}
        if(@($J.attempts).Count -ne 5){$AllSessionComplete=$false}
      }catch{
        $Entry.raw_attempt_counts[$Name]="PARSE_ERROR"
        $AllSessionComplete=$false
      }
    }else{
      $Entry.raw_attempt_counts[$Name]=0
    }
  }
  $Report.sessions[$S]=$Entry
}

$ExpectedFull=@(
  (Join-Path $ResultsRoot "P6_PREFLIGHT.json"),
  (Join-Path $ResultsRoot "session_A\P6_SESSION_ENVIRONMENT.json"),
  (Join-Path $ResultsRoot "session_A\P6_SESSION_META.json"),
  (Join-Path $ResultsRoot "session_A\reference_W_S.json"),
  (Join-Path $ResultsRoot "session_A\candidate_W_S.json"),
  (Join-Path $ResultsRoot "session_A\reference_W_C.json"),
  (Join-Path $ResultsRoot "session_A\candidate_W_C.json"),
  (Join-Path $ResultsRoot "session_B\P6_SESSION_ENVIRONMENT.json"),
  (Join-Path $ResultsRoot "session_B\P6_SESSION_META.json"),
  (Join-Path $ResultsRoot "session_B\reference_W_S.json"),
  (Join-Path $ResultsRoot "session_B\candidate_W_S.json"),
  (Join-Path $ResultsRoot "session_B\reference_W_C.json"),
  (Join-Path $ResultsRoot "session_B\candidate_W_C.json")
)
$ExpectedNorm=@($ExpectedFull|ForEach-Object {[IO.Path]::GetFullPath($_)})
$AllFiles=@(Get-ChildItem $ResultsRoot -Recurse -File)
foreach($F in $AllFiles){
  if($ExpectedNorm -notcontains [IO.Path]::GetFullPath($F.FullName)){
    $Report.extra_files += [ordered]@{path=$F.FullName;bytes=$F.Length;sha256=Sha256 $F.FullName}
  }
}

if(-not $Report.root_files["P6_PREFLIGHT.json"].exists){
  $Report.conclusion="P6_FORENSIC_INCOMPLETE_PRE_SCIENCE_OR_INVALID"
}elseif($AllSessionComplete){
  $Report.conclusion="P6_FORENSIC_COLLECTION_COMPLETE_PACKAGING_MAY_BE_RECONSTRUCTABLE_WITHOUT_RERUN"
}else{
  $Report.conclusion="P6_FORENSIC_PARTIAL_COLLECTION_DO_NOT_PACKAGE_DO_NOT_RERUN"
}

$Json=$Report|ConvertTo-Json -Depth 12
Write-Host $Json
Write-Host ""
Write-Host ("CONCLUSION="+$Report.conclusion)
Write-Host "NO_FILES_MODIFIED"
Write-Host "NO_MODEL_EXECUTION"
Write-Host "NO_GPU_DISPATCH"
