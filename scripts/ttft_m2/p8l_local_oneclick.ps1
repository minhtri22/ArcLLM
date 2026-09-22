$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BranchExpected = "research/arcllm-ttft-m2"
$AuthorizationPath = Join-Path $Root "config\arcllm_ttft_m2_p8l_local_buildonly_authorization_v0.1.json"
$GovernancePath = Join-Path $Root "config\arcllm_ttft_m2_governance_v0.1.json"
$RunnerPath = Join-Path $Root "run_ttft_m2_buildonly.ps1"
$ResultsDir = Join-Path $Root "results\ttft_m2_p8l_local"
$ReportPath = Join-Path $ResultsDir "TTFT_M2_P8L_LOCAL_REPORT.json"
$ReturnZip = Join-Path $Root "results\ttft_m2_p8l_local_return_to_chatgpt.zip"
$BuildOnlyDir = Join-Path $Root "results\ttft_m2_buildonly"
$BuildOnlyZip = Join-Path $Root "results\ttft_m2_buildonly_return_to_chatgpt.zip"

function GitBlob([string]$RelPath) {
  $p = $RelPath -replace '\\','/'
  $v = (& git -C $Root rev-parse ("HEAD:" + $p) 2>$null)
  if($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($v)) {
    throw "P8L cannot resolve Git blob: $RelPath"
  }
  return $v.Trim()
}
function Sha256([string]$Path) {
  if(-not (Test-Path $Path -PathType Leaf)) { return $null }
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}
function WriteReport([hashtable]$Data) {
  if(-not (Test-Path $ResultsDir)) { New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null }
  [IO.File]::WriteAllText($ReportPath,($Data | ConvertTo-Json -Depth 30),(New-Object Text.UTF8Encoding($false)))
}
function PackReturn() {
  if(Test-Path $ReturnZip) { Remove-Item -Force $ReturnZip }
  $items = @()
  if(Test-Path $ReportPath) { $items += $ReportPath }
  if(Test-Path $BuildOnlyDir) { $items += $BuildOnlyDir }
  if(Test-Path $BuildOnlyZip) { $items += $BuildOnlyZip }
  if($items.Count -gt 0) {
    Compress-Archive -Path $items -DestinationPath $ReturnZip -CompressionLevel Optimal
  }
}

$Report = [ordered]@{
  schema = "arcllm.ttft_m2.p8l.local_report.v0.1"
  program_id = "ARCLLM_TTFT_M2"
  stage = "M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION"
  status = "P8L_LOCAL_PREFLIGHT_NOT_STARTED"
  machine_substrate = "LOCAL_WINDOWS_WORKSTATION"
  branch = $null
  git_head = $null
  tracked_worktree_clean = $false
  authorization = [ordered]@{
    path = "config/arcllm_ttft_m2_p8l_local_buildonly_authorization_v0.1.json"
    blob = $null
    decision = $null
  }
  frozen_bindings = [ordered]@{}
  exact_runner_started = $false
  exact_runner_exit_code = $null
  buildonly_result_present = $false
  failure_result_present = $false
  buildonly_bundle_present = $false
  buildonly_bundle_sha256 = $null
  zero_science = [ordered]@{
    target_model_loaded = $false
    diagnostic_executable_launched = $false
    gpu_dispatch = $false
    performance_measurement = $false
    fresh_ttft_observations = 0
  }
  scientific_result = "NONE"
  mechanism_adjudication_performed = $false
  note = $null
}

try {
  Set-Location $Root

  $Branch = (& git branch --show-current).Trim()
  $Head = (& git rev-parse HEAD).Trim()
  $Tracked = (& git status --porcelain --untracked-files=no | Out-String)

  $Report.branch = $Branch
  $Report.git_head = $Head
  $Report.tracked_worktree_clean = [string]::IsNullOrWhiteSpace($Tracked)

  if($Branch -ne $BranchExpected) {
    throw "P8L wrong branch: expected $BranchExpected, got $Branch"
  }
  if(-not $Report.tracked_worktree_clean) {
    throw "P8L tracked worktree is dirty. Commit/stash/revert tracked changes before execution."
  }
  if(-not (Test-Path $AuthorizationPath -PathType Leaf)) {
    throw "P8L local authorization missing"
  }
  if(-not (Test-Path $GovernancePath -PathType Leaf)) {
    throw "P8L governance missing"
  }
  if(-not (Test-Path $RunnerPath -PathType Leaf)) {
    throw "P8L exact frozen runner missing"
  }

  $Auth = Get-Content $AuthorizationPath -Raw -Encoding UTF8 | ConvertFrom-Json
  $Gov = Get-Content $GovernancePath -Raw -Encoding UTF8 | ConvertFrom-Json

  if($Auth.schema -ne "arcllm.ttft_m2.p8l.local_buildonly_authorization.v0.1") {
    throw "P8L authorization schema mismatch"
  }
  if($Auth.decision -ne "M2_P8L_LOCAL_BUILDONLY_AUTHORIZED") {
    throw "P8L authorization decision mismatch"
  }
  if(-not [bool]$Auth.authorization.buildonly_execution) {
    throw "P8L BuildOnly execution is not authorized"
  }
  if([bool]$Auth.authorization.target_model_execution -or
     [bool]$Auth.authorization.diagnostic_executable_launch -or
     [bool]$Auth.authorization.gpu_dispatch -or
     [bool]$Auth.authorization.performance_measurement -or
     [bool]$Auth.authorization.fresh_ttft_observation) {
    throw "P8L scientific boundary violation in authorization"
  }
  if($Gov.next -ne "M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION") {
    throw "P8L governance next-stage mismatch"
  }

  $AuthBlob = GitBlob "config/arcllm_ttft_m2_p8l_local_buildonly_authorization_v0.1.json"
  $WrapperBlob = GitBlob "scripts/ttft_m2/p8l_local_oneclick.ps1"
  $Report.authorization.blob = $AuthBlob
  $Report.authorization.decision = [string]$Auth.decision

  if($WrapperBlob -ne [string]$Auth.local_wrapper.blob) {
    throw "P8L wrapper blob mismatch"
  }

  foreach($P in $Auth.frozen_bindings.PSObject.Properties) {
    $Rel = [string]$P.Value.path
    $Expected = [string]$P.Value.blob
    $Actual = GitBlob $Rel
    $Report.frozen_bindings[$P.Name] = [ordered]@{ path=$Rel; expected_blob=$Expected; actual_blob=$Actual; match=($Expected -eq $Actual) }
    if($Actual -ne $Expected) {
      throw "P8L frozen binding mismatch: $($P.Name)"
    }
  }

  $Report.status = "P8L_LOCAL_PREFLIGHT_PASS"
  $Report.note = "Local wrapper preflight passed; invoking exact frozen BuildOnly runner."
  WriteReport $Report

  Write-Host "P8L_LOCAL_PREFLIGHT_PASS"
  Write-Host ("branch=" + $Branch)
  Write-Host ("HEAD=" + $Head)
  Write-Host "Invoking exact frozen run_ttft_m2_buildonly.ps1 ..."
  Write-Host ""

  $Report.exact_runner_started = $true
  & $RunnerPath
  $RunnerCode = $LASTEXITCODE
  if($null -eq $RunnerCode) { $RunnerCode = 0 }
  $Report.exact_runner_exit_code = [int]$RunnerCode

  $BuildResultPath = Join-Path $BuildOnlyDir "TTFT_M2_BUILDONLY_RESULT.json"
  $FailurePath = Join-Path $BuildOnlyDir "TTFT_M2_FAILURE_RESULT.json"
  $Report.buildonly_result_present = Test-Path $BuildResultPath -PathType Leaf
  $Report.failure_result_present = Test-Path $FailurePath -PathType Leaf
  $Report.buildonly_bundle_present = Test-Path $BuildOnlyZip -PathType Leaf
  $Report.buildonly_bundle_sha256 = Sha256 $BuildOnlyZip

  if($RunnerCode -eq 0 -and $Report.buildonly_result_present -and $Report.buildonly_bundle_present) {
    $R = Get-Content $BuildResultPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if($R.status -ne "M2_INFRASTRUCTURE_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION") {
      throw "P8L runner returned unexpected success status"
    }
    if([bool]$R.zero_science.target_model_loaded -or
       [bool]$R.zero_science.diagnostic_executable_launched -or
       [bool]$R.zero_science.gpu_dispatch -or
       [bool]$R.zero_science.performance_measurement -or
       [int]$R.zero_science.fresh_ttft_observations -ne 0) {
      throw "P8L zero-science invariant violated by runner result"
    }
    $Report.status = "P8L_LOCAL_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION"
    $Report.note = "Exact frozen runner completed successfully. No scientific execution occurred."
  } else {
    $Report.status = "P8L_LOCAL_RUNNER_FAIL_CLOSED_AWAITING_ADJUDICATION"
    $Report.note = "Exact frozen runner started but did not produce a complete success bundle. Preserve outputs; do not repair or rerun until adjudicated."
  }

  WriteReport $Report
  PackReturn

  Write-Host ""
  Write-Host ("P8L_STATUS=" + $Report.status)
  Write-Host ("report=" + $ReportPath)
  if(Test-Path $ReturnZip) {
    Write-Host ("return_bundle=" + $ReturnZip)
    Write-Host ("return_bundle_sha256=" + (Sha256 $ReturnZip))
  }

  if($Report.status -ne "P8L_LOCAL_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION") {
    exit 2
  }
}
catch {
  $Report.status = "P8L_LOCAL_WRAPPER_FAIL_CLOSED"
  $Report.note = $_.Exception.Message
  try { WriteReport $Report; PackReturn } catch {}
  Write-Error $_
  if(Test-Path $ReportPath) { Write-Host ("report=" + $ReportPath) }
  if(Test-Path $ReturnZip) { Write-Host ("return_bundle=" + $ReturnZip) }
  exit 1
}
