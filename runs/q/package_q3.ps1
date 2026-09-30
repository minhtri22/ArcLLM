param()
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Here "runs\_relocation_compat.ps1")
$Results=Join-Path $Here "results"
$A=Join-Path $Results "q3_session_A";$B=Join-Path $Results "q3_session_B"
foreach($D in @($A,$B)){if(-not(Test-Path (Join-Path $D "q3_session_complete.json"))){throw "Q3 packaging requires completed Session A and Session B"}}
$Design=Join-Path $Here "config\q3_confirmatory_design.json"
$Lock=Join-Path $Results "q3_preflight_lock.json"
$Auth=Join-Path $Here "config\q3_execution_authorization.json"
foreach($P in @($Design,$Lock,$Auth)){if(-not(Test-Path $P)){throw "Q3 packaging prerequisite missing: $P"}}
$Candidate=Join-Path $Results "q3_adjudication_candidate.json"
py -3 (Join-Path $Here "tools\adjudicate_q3.py") --session-a $A --session-b $B --design $Design --preflight-lock $Lock --authorization $Auth --out $Candidate
if($LASTEXITCODE -ne 0){throw "Q3 candidate adjudicator failed"}
$Zip=Join-Path $Results "q3_return_to_chatgpt.zip"
py -3 (Join-Path $Here "tools\package_q3_evidence.py") --root $Here --session-a $A --session-b $B --candidate $Candidate --out $Zip
if($LASTEXITCODE -ne 0){throw "Q3 evidence packaging failed"}
Write-Host "Q3 evidence bundle: $Zip"
Write-Host "Candidate verdict is machine-generated only; final independent adjudication is still required."
