param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$RepairAuthPath=Join-Path $Root "config\anl64_p6_recovery_replay_authorization_v0.1.json"
$RecoveryLockPath=Join-Path $Root "config\anl64_p6_recovery_replay_lock_v0.1.json"
$PartialRoot=Join-Path $Root "results\anl64_p6_confirmatory"
$ArchiveRoot=Join-Path $Root "results\anl64_p6_confirmatory_interrupted_attempt1"
$Bundle=Join-Path $Root "results\anl64_p6_confirmatory_return_to_chatgpt.zip"
$CanonicalRunner=Join-Path $Root "runs/anl64/run_anl64_p6_confirmatory.ps1"

function Fail([string]$Message){throw "ANL64_P6_RECOVERY_FAIL_CLOSED: $Message"}
function Require([bool]$Condition,[string]$Message){if(-not $Condition){Fail $Message}}
function GitBlob([string]$Path){
  return Get-RunLogicalGitBlob -RepoRoot $Root -Path $Path
}
function Sha256([string]$Path){
  Require (Test-Path $Path -PathType Leaf) "missing file: $Path"
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

Write-Host "ANL64 P6 single-repair recovery wrapper"
Write-Host "Partial attempt is archived byte-exact; no partial timing is reused."
Write-Host "Then the canonical locked P6 runner replays BOTH sessions from the beginning."

Require (Test-Path $RepairAuthPath -PathType Leaf) "recovery authorization missing"
Require (Test-Path $RecoveryLockPath -PathType Leaf) "recovery lock missing"

$Tracked=(& git -C $Root status --porcelain --untracked-files=no | Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $RepairAuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $RecoveryLockPath -Raw -Encoding UTF8|ConvertFrom-Json
Require ($Auth.decision -eq "P6_SINGLE_INFRASTRUCTURE_REPAIR_FULL_REPLAY_AUTHORIZED") "wrong recovery authorization"
Require ($Lock.status -eq "P6_RECOVERY_LOCKED_NOT_YET_RUN") "wrong recovery lock state"
Require ([int]$Auth.repair_budget.remaining_after_this -eq 0) "repair budget must be exhausted by this replay"

foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("Git blob mismatch: "+$P.Name)
}
Require ((GitBlob "runs/anl64/recover_anl64_p6_after_interruption.ps1") -eq [string]$Lock.runner_git_blob) "recovery runner self blob mismatch"
Require ((GitBlob "runs/anl64/run_anl64_p6_confirmatory.ps1") -eq "40c4941b1764fdd7a1f83be554e39fda1764151c") "canonical P6 runner drift"
Require ((GitBlob "config/anl64_p6_execution_lock_v0.1.json") -eq "eb11fc2c898846442d628acf69e6bc27241cd74e") "canonical P6 execution lock drift"

Require (Test-Path $PartialRoot -PathType Container) "expected interrupted results root missing"
Require (-not(Test-Path $ArchiveRoot)) "interrupted-attempt archive already exists"
Require (-not(Test-Path $Bundle)) "P6 return bundle already exists; recovery must not overwrite"

$Preflight=Join-Path $PartialRoot "P6_PREFLIGHT.json"
$SessionA=Join-Path $PartialRoot "session_A"
$EnvA=Join-Path $SessionA "P6_SESSION_ENVIRONMENT.json"
$RefWS=Join-Path $SessionA "reference_W_S.json"

Require ((Sha256 $Preflight) -eq "CCC993AE10EC0919EB4B1DF71E2A5018DCD75CA135A11C2DB2BB7392A4644C58") "interrupted preflight hash mismatch"
Require ((Sha256 $EnvA) -eq "06623EC6482BF0F2FC6A6638D1AC79AB3CD4B48398AA433D806FCFEEC3E1C540") "interrupted Session A environment hash mismatch"
Require ((Sha256 $RefWS) -eq "7525B5457084BD885D1E386D41A3CDF34F8C5CDFA7B2C495A5597DE17BE602E9") "interrupted reference W-S hash mismatch"

$RefObj=Get-Content $RefWS -Raw -Encoding UTF8|ConvertFrom-Json
Require (@($RefObj.attempts).Count -eq 5) "interrupted reference W-S must contain exactly five measured attempts"

foreach($Missing in @(
  (Join-Path $SessionA "P6_SESSION_META.json"),
  (Join-Path $SessionA "candidate_W_S.json"),
  (Join-Path $SessionA "reference_W_C.json"),
  (Join-Path $SessionA "candidate_W_C.json"),
  (Join-Path $PartialRoot "session_B")
)){
  Require (-not(Test-Path $Missing)) ("forensic shape changed; expected missing: "+$Missing)
}

$ExpectedFiles=@(
  [IO.Path]::GetFullPath($Preflight),
  [IO.Path]::GetFullPath($EnvA),
  [IO.Path]::GetFullPath($RefWS)
)
$ObservedFiles=@(Get-ChildItem $PartialRoot -Recurse -File|ForEach-Object {[IO.Path]::GetFullPath($_.FullName)})
Require ($ObservedFiles.Count -eq 3) "interrupted attempt now contains unexpected extra files"
foreach($F in $ObservedFiles){Require ($ExpectedFiles -contains $F) ("unexpected interrupted evidence file: "+$F)}

Write-Host "Archiving interrupted attempt without deleting evidence"
Move-Item -LiteralPath $PartialRoot -Destination $ArchiveRoot

$ArchivedPreflight=Join-Path $ArchiveRoot "P6_PREFLIGHT.json"
$ArchivedEnvA=Join-Path $ArchiveRoot "session_A\P6_SESSION_ENVIRONMENT.json"
$ArchivedRefWS=Join-Path $ArchiveRoot "session_A\reference_W_S.json"
Require ((Sha256 $ArchivedPreflight) -eq "CCC993AE10EC0919EB4B1DF71E2A5018DCD75CA135A11C2DB2BB7392A4644C58") "archived preflight changed"
Require ((Sha256 $ArchivedEnvA) -eq "06623EC6482BF0F2FC6A6638D1AC79AB3CD4B48398AA433D806FCFEEC3E1C540") "archived environment changed"
Require ((Sha256 $ArchivedRefWS) -eq "7525B5457084BD885D1E386D41A3CDF34F8C5CDFA7B2C495A5597DE17BE602E9") "archived reference W-S changed"
Require (-not(Test-Path $PartialRoot)) "fresh canonical results root was not released"

$ArchiveManifest=[ordered]@{
  schema="arcllm.anl64.p6.interrupted_attempt_archive.v0.1"
  status="SPENT_NON_ADMISSIBLE"
  reason="operator terminal interruption before first matched candidate/reference comparison"
  original_results_root="results/anl64_p6_confirmatory"
  archived_results_root="results/anl64_p6_confirmatory_interrupted_attempt1"
  repair_budget_consumed=1
  repair_budget_remaining=0
  partial_science=[ordered]@{
    complete_cells=1
    expected_cells=8
    measured_attempts_present=5
    expected_measured_attempts=40
    matched_comparisons=0
    performance_adjudication_permitted=$false
  }
  preserved_sha256=[ordered]@{
    P6_PREFLIGHT="CCC993AE10EC0919EB4B1DF71E2A5018DCD75CA135A11C2DB2BB7392A4644C58"
    P6_SESSION_A_ENVIRONMENT="06623EC6482BF0F2FC6A6638D1AC79AB3CD4B48398AA433D806FCFEEC3E1C540"
    REFERENCE_W_S="7525B5457084BD885D1E386D41A3CDF34F8C5CDFA7B2C495A5597DE17BE602E9"
  }
}
$ArchiveManifestPath=Join-Path $ArchiveRoot "P6_INTERRUPTED_ATTEMPT_ARCHIVE_MANIFEST.json"
[IO.File]::WriteAllText($ArchiveManifestPath,($ArchiveManifest|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

Write-Host "Interrupted attempt archived and marked SPENT_NON_ADMISSIBLE."
Write-Host "Starting the ONLY authorized full frozen replay."
$Args=@("-NoProfile","-ExecutionPolicy","Bypass","-File",$CanonicalRunner)
if($ModelPath){$Args += @("-ModelPath",$ModelPath)}
if($OllamaModelsRoot){$Args += @("-OllamaModelsRoot",$OllamaModelsRoot)}
& powershell.exe @Args
$Code=$LASTEXITCODE
if($Code -ne 0){
  Write-Host ""
  Write-Host "ANL64_P6_RECOVERY_REPLAY_FAILED"
  Write-Host "REPAIR_BUDGET_EXHAUSTED"
  Write-Host "DO_NOT_RERUN"
  Write-Host "NEXT=P6_STOP_INFRASTRUCTURE_UNSTABLE_ADJUDICATION"
  exit $Code
}

Require (Test-Path $Bundle -PathType Leaf) "canonical replay exited 0 but return bundle missing"
$BundleHash=Sha256 $Bundle
Write-Host ""
Write-Host "ANL64_P6_RECOVERY_FULL_REPLAY_COMPLETE"
Write-Host "INTERRUPTED_ATTEMPT_ARCHIVED_SPENT"
Write-Host "REPAIR_BUDGET_EXHAUSTED"
Write-Host "P7_NOT_AUTHORIZED"
Write-Host ("archive="+$ArchiveRoot)
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
