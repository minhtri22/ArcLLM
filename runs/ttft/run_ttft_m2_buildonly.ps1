# M2_PACKAGE_META {"package_id":"ARCLLM_TTFT_M2_ATOMIC_PACKAGE","package_version":"v0.1","lock_path":"config/arcllm_ttft_m2_execution_lock_v0.1.json","lock_version":"v0.1","success_result_schema":"arcllm.ttft_m2.buildonly_result.v0.1","failure_result_schema":"arcllm.ttft_m2.failure_result.v0.1","evidence_manifest_schema":"arcllm.ttft_m2.evidence_manifest.v0.1","package_manifest_schema":"arcllm.ttft_m2.package_manifest.v0.1","adjudicator_input_schema":"arcllm.ttft_m2.adjudicator_input.v0.1","evidence_manifest_template":"config/arcllm_ttft_m2_evidence_manifest_v0.1.json","package_manifest_path":"config/arcllm_ttft_m2_package_manifest_v0.1.json","p7_authorization_path":"config/arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json","p7_authorization_schema":"arcllm.ttft_m2.p7.buildonly_authorization.v0.1","p7_authorization_decision":"M2_P7_BUILDONLY_AUTHORIZED","output_bundle_name":"results/ttft_m2_buildonly_return_to_chatgpt.zip"}
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$LockPath = Join-Path $Root "config\arcllm_ttft_m2_execution_lock_v0.1.json"
$P7AuthPath = Join-Path $Root "config\arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json"
$PackageManifestPath = Join-Path $Root "config\arcllm_ttft_m2_package_manifest_v0.1.json"
$EvidenceTemplatePath = Join-Path $Root "config\arcllm_ttft_m2_evidence_manifest_v0.1.json"
$QaTool = Join-Path $Root "tools\ttft_m2_atomic_package_qa.py"
$ResultsDir = Join-Path $Root "results\ttft_m2_buildonly"
$Bundle = Join-Path $Root "results\ttft_m2_buildonly_return_to_chatgpt.zip"

function Fail([string]$Stage,[string]$Classification,[string]$Message) {
  if(-not (Test-Path $ResultsDir)) { New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null }
  $LockBlobForFailure = $null
  $lb = (& git -C $Root rev-parse "HEAD:config/arcllm_ttft_m2_execution_lock_v0.1.json" 2>$null)
  if($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($lb)) { $LockBlobForFailure = $lb.Trim() }
  $Failure = [ordered]@{
    schema = "arcllm.ttft_m2.failure_result.v0.1"
    status = "M2_INFRASTRUCTURE_FAIL_CLOSED"
    stage = $Stage
    classification = $Classification
    message = $Message
    package_version = "v0.1"
    lock = [ordered]@{
      path = "config/arcllm_ttft_m2_execution_lock_v0.1.json"
      version = "v0.1"
      blob = $LockBlobForFailure
    }
    zero_science = [ordered]@{
      target_model_loaded = $false
      diagnostic_executable_launched = $false
      gpu_dispatch = $false
      performance_measurement = $false
      fresh_ttft_observations = 0
    }
    scientific_result = "NONE"
  }
  $FailurePath = Join-Path $ResultsDir "TTFT_M2_FAILURE_RESULT.json"
  [IO.File]::WriteAllText($FailurePath,($Failure | ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
  throw "TTFT_M2_FAIL_CLOSED[$Stage/$Classification]: $Message"
}
function Require([bool]$Condition,[string]$Stage,[string]$Classification,[string]$Message) {
  if(-not $Condition) { Fail $Stage $Classification $Message }
}
function GitBlob([string]$RelPath) {
  $p = $RelPath -replace '\\','/'
  $v = (& git -C $Root rev-parse ("HEAD:" + $p) 2>$null)
  if($LASTEXITCODE -ne 0) { Fail "PREFLIGHT" "GIT_BLOB_RESOLUTION_FAILED" ("cannot resolve " + $RelPath) }
  return $v.Trim()
}
function Sha256([string]$Path) {
  Require (Test-Path $Path -PathType Leaf) "HASH" "MISSING_FILE" ("missing file " + $Path)
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

Write-Host "ARCLLM_TTFT_M2 infrastructure BuildOnly package runner"
Write-Host "This runner is zero-science: no target model load, diagnostic executable launch, GPU dispatch, or performance timing."

try {
  Require (Test-Path $LockPath -PathType Leaf) "PREFLIGHT" "MISSING_LOCK" "execution lock missing"
  Require (Test-Path $PackageManifestPath -PathType Leaf) "PREFLIGHT" "MISSING_PACKAGE_MANIFEST" "package manifest missing"
  Require (Test-Path $EvidenceTemplatePath -PathType Leaf) "PREFLIGHT" "MISSING_EVIDENCE_TEMPLATE" "evidence manifest template missing"
  Require (Test-Path $QaTool -PathType Leaf) "PREFLIGHT" "MISSING_QA_TOOL" "atomic QA tool missing"

  # P4/P5/P6 do not authorize execution. P7 must exist and match exactly.
  Require (Test-Path $P7AuthPath -PathType Leaf) "AUTHORIZATION" "P7_AUTHORIZATION_MISSING" "M2-P7 BuildOnly authorization does not exist"
  $P7 = Get-Content $P7AuthPath -Raw -Encoding UTF8 | ConvertFrom-Json
  Require ($P7.schema -eq "arcllm.ttft_m2.p7.buildonly_authorization.v0.1") "AUTHORIZATION" "P7_SCHEMA_MISMATCH" "wrong P7 schema"
  Require ($P7.decision -eq "M2_P7_BUILDONLY_AUTHORIZED") "AUTHORIZATION" "P7_DECISION_MISMATCH" "wrong P7 decision"
  Require (-not [bool]$P7.authorization.target_model_execution) "AUTHORIZATION" "TARGET_EXECUTION_FORBIDDEN" "target model execution must remain false"
  Require (-not [bool]$P7.authorization.gpu_dispatch) "AUTHORIZATION" "GPU_FORBIDDEN" "GPU dispatch must remain false"
  Require (-not [bool]$P7.authorization.performance_measurement) "AUTHORIZATION" "TIMING_FORBIDDEN" "performance measurement must remain false"

  $Tracked = (& git -C $Root status --porcelain --untracked-files=no | Out-String)
  Require ([string]::IsNullOrWhiteSpace($Tracked)) "PREFLIGHT" "DIRTY_TRACKED_TREE" "tracked working tree is dirty"

  py -3 $QaTool --repo-root $Root --mode runtime-preflight
  Require ($LASTEXITCODE -eq 0) "ATOMIC_QA" "RUNTIME_PREFLIGHT_QA_FAILED" "atomic package QA failed"

  if(Test-Path $ResultsDir) { Remove-Item -Recurse -Force $ResultsDir }
  if(Test-Path $Bundle) { Remove-Item -Force $Bundle }
  New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

  $Lock = Get-Content $LockPath -Raw -Encoding UTF8 | ConvertFrom-Json
  $PackageManifest = Get-Content $PackageManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
  $Evidence = Get-Content $EvidenceTemplatePath -Raw -Encoding UTF8 | ConvertFrom-Json

  foreach($Member in $PackageManifest.members) {
    if($Member.binding_mode -eq "STATIC_COMMITTED") {
      $Source = Join-Path $Root ([string]$Member.source_path)
      Require (Test-Path $Source -PathType Leaf) "PACKAGE" "STATIC_MEMBER_MISSING" ([string]$Member.id)
      Require ((GitBlob ([string]$Member.source_path)) -eq [string]$Member.git_blob) "PACKAGE" "STATIC_MEMBER_BLOB_MISMATCH" ([string]$Member.id)
      Copy-Item $Source (Join-Path $ResultsDir ([string]$Member.destination_name))
    }
  }

  # Self-manifest and future authorization cannot embed their own/current blob in the manifest;
  # they are required exact Git bindings resolved at execution.
  foreach($Member in $PackageManifest.members) {
    if($Member.binding_mode -eq "RUNTIME_REQUIRED_EXACT_GIT_BLOB") {
      $Source = Join-Path $Root ([string]$Member.source_path)
      Require (Test-Path $Source -PathType Leaf) "PACKAGE" "RUNTIME_GIT_MEMBER_MISSING" ([string]$Member.id)
      $ResolvedBlob = GitBlob ([string]$Member.source_path)
      Copy-Item $Source (Join-Path $ResultsDir ([string]$Member.destination_name))
    }
  }

  foreach($Member in $Evidence.members) {
    if($Member.binding_mode -eq "RUNTIME_REQUIRED_EXACT_GIT_BLOB" -and $null -ne $Member.source_path) {
      $Member.git_blob = GitBlob ([string]$Member.source_path)
    }
  }

  $P7Blob = GitBlob "config/arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json"

  $Result = [ordered]@{
    schema = "arcllm.ttft_m2.buildonly_result.v0.1"
    status = "M2_INFRASTRUCTURE_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION"
    git_head = ((& git -C $Root rev-parse HEAD).Trim())
    package_version = "v0.1"
    lock = [ordered]@{ path = "config/arcllm_ttft_m2_execution_lock_v0.1.json"; version = "v0.1"; blob = GitBlob "config/arcllm_ttft_m2_execution_lock_v0.1.json" }
    runner = [ordered]@{ path = "runs/ttft/run_ttft_m2_buildonly.ps1"; blob = GitBlob "runs/ttft/run_ttft_m2_buildonly.ps1" }
    package_manifest = [ordered]@{ path = "config/arcllm_ttft_m2_package_manifest_v0.1.json"; blob = GitBlob "config/arcllm_ttft_m2_package_manifest_v0.1.json" }
    evidence_manifest = [ordered]@{ path = "TTFT_M2_EVIDENCE_MANIFEST.json"; sha256 = ("0" * 64) }
    zero_science = [ordered]@{
      target_model_loaded = $false
      diagnostic_executable_launched = $false
      gpu_dispatch = $false
      performance_measurement = $false
      fresh_ttft_observations = 0
    }
    scientific_result = "NONE"
    mechanism_adjudication_performed = $false
  }

  $ResultPath = Join-Path $ResultsDir "TTFT_M2_BUILDONLY_RESULT.json"
  [IO.File]::WriteAllText($ResultPath,($Result | ConvertTo-Json -Depth 20),(New-Object Text.UTF8Encoding($false)))

  $Evidence.members += [pscustomobject]@{
    id = "p7_authorization"
    binding_mode = "RUNTIME_REQUIRED_EXACT_GIT_BLOB"
    source_path = "config/arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json"
    destination_name = "arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json"
    git_blob = $P7Blob
    schema_id = "arcllm.ttft_m2.p7.buildonly_authorization.v0.1"
    required = $true
  }
  $Evidence.members += [pscustomobject]@{
    id = "buildonly_result"
    binding_mode = "RUNTIME_GENERATED"
    source_path = $null
    destination_name = "TTFT_M2_BUILDONLY_RESULT.json"
    git_blob = $null
    schema_id = "arcllm.ttft_m2.buildonly_result.v0.1"
    required = $true
  }

  # P7 authorization and package manifest are already copied by the runtime exact-binding loop.
  $EvidencePath = Join-Path $ResultsDir "TTFT_M2_EVIDENCE_MANIFEST.json"
  [IO.File]::WriteAllText($EvidencePath,($Evidence | ConvertTo-Json -Depth 30),(New-Object Text.UTF8Encoding($false)))
  $EvidenceHash = Sha256 $EvidencePath

  $Result.evidence_manifest.sha256 = $EvidenceHash
  [IO.File]::WriteAllText($ResultPath,($Result | ConvertTo-Json -Depth 20),(New-Object Text.UTF8Encoding($false)))
  $ResultHash = Sha256 $ResultPath
  $PackageManifestHash = Sha256 $PackageManifestPath

  Compress-Archive -Path (Join-Path $ResultsDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
  $BundleHash = Sha256 $Bundle

  $AdjudicatorInput = [ordered]@{
    schema = "arcllm.ttft_m2.adjudicator_input.v0.1"
    package_id = "ARCLLM_TTFT_M2_ATOMIC_PACKAGE"
    package_version = "v0.1"
    package_manifest = [ordered]@{ path = "config/arcllm_ttft_m2_package_manifest_v0.1.json"; sha256 = $PackageManifestHash }
    result = [ordered]@{ path = "TTFT_M2_BUILDONLY_RESULT.json"; sha256 = $ResultHash }
    evidence_manifest = [ordered]@{ path = "TTFT_M2_EVIDENCE_MANIFEST.json"; sha256 = $EvidenceHash }
    bundle_sha256 = $BundleHash
    zero_science = [ordered]@{ scientific_result = "NONE" }
  }
  $AdjudicatorPath = Join-Path $ResultsDir "TTFT_M2_ADJUDICATOR_INPUT.json"
  [IO.File]::WriteAllText($AdjudicatorPath,($AdjudicatorInput | ConvertTo-Json -Depth 20),(New-Object Text.UTF8Encoding($false)))

  Write-Host "TTFT_M2_INFRASTRUCTURE_BUILDONLY_COMPLETE"
  Write-Host "TARGET_MODEL_NOT_LOADED"
  Write-Host "DIAGNOSTIC_EXECUTABLE_NOT_LAUNCHED"
  Write-Host "GPU_DISPATCH_NOT_RUN"
  Write-Host "PERFORMANCE_NOT_MEASURED"
  Write-Host "SCIENTIFIC_RESULT=NONE"
  Write-Host ("bundle=" + $Bundle)
  Write-Host ("SHA256=" + $BundleHash)
}
catch {
  Write-Error $_
  exit 1
}
