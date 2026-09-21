$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath = Join-Path $Root "config\q6cb2_execution_authorization.json"
$ContractPath = Join-Path $Root "config\q6cb1_execution_contract_v0.1.json"
$ContractLockPath = Join-Path $Root "config\q6cb1_execution_contract_lock_v0.1.json"
$OutDir = Join-Path $Root "results\q6cb2_identification"
$RawDir = Join-Path $OutDir "raw"
$Bundle = Join-Path $Root "results\q6cb2_identification_return_to_chatgpt.zip"

function Fail([string]$Message) {
    throw "Q6CB2_FAIL_CLOSED: $Message"
}

function Require([bool]$Condition, [string]$Message) {
    if (-not $Condition) { Fail $Message }
}

function GitBlob([string]$Path) {
    $p = $Path -replace '\\','/'
    $v = (& git -C $Root rev-parse ("HEAD:" + $p) 2>$null)
    if ($LASTEXITCODE -ne 0) { Fail "cannot resolve Git blob for $Path" }
    return $v.Trim()
}

function Sha256([string]$Path) {
    if (-not (Test-Path $Path -PathType Leaf)) { Fail "missing file: $Path" }
    return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

function StringSha256([string]$Text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
        return (($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString("X2") }) -join "")
    }
    finally {
        $sha.Dispose()
    }
}

Write-Host "Q6CB-2 frozen identification runner"
Write-Host "Preflight only until all provenance/environment checks pass."

if (-not (Test-Path $AuthPath -PathType Leaf)) { Fail "authorization file missing" }
if (-not (Test-Path $ContractPath -PathType Leaf)) { Fail "execution contract missing" }
if (-not (Test-Path $ContractLockPath -PathType Leaf)) { Fail "execution contract lock missing" }

$trackedDiff = & git -C $Root status --porcelain --untracked-files=no
if ($LASTEXITCODE -ne 0) { Fail "git status failed" }
Require ([string]::IsNullOrWhiteSpace(($trackedDiff | Out-String))) "tracked working tree is dirty"

$Auth = Get-Content $AuthPath -Raw | ConvertFrom-Json
$Contract = Get-Content $ContractPath -Raw | ConvertFrom-Json
$ContractLock = Get-Content $ContractLockPath -Raw | ConvertFrom-Json

Require ($Auth.decision -eq "Q6CB2_FROZEN_IDENTIFICATION_AUTHORIZED") "wrong authorization decision"
Require ($Auth.authorization_marker -eq "Q6CB1_SCIENTIFIC_EXECUTION_AUTHORIZED") "authorization marker mismatch"
Require ($Auth.scope -eq "Q6CB2_IDENTIFICATION_ONLY") "authorization scope is not identification-only"
Require ($Auth.authorization.q6cb2_identification -eq $true) "Q6CB-2 not authorized"
Require ($Auth.authorization.q6cb3_confirmatory -eq $false) "Q6CB-3 must remain unauthorized"
Require ($Auth.authorization.performance_timing -eq $false) "performance timing must remain forbidden"
Require ($Auth.authorization.target_model_load -eq $false) "target model load must remain forbidden"
Require ($Auth.q6cb3.authorized -eq $false) "Q6CB-3 authorization unexpectedly open"
Require ($Auth.identification_partition.fixture_count -eq 30) "fixture count is not 30"
Require ($Auth.identification_partition.fixtures.Count -eq 30) "fixture census is not 30"
Require ($Auth.one_shot_policy.valid_partition_execution_count -eq 1) "partition execution count is not one"
Require ($Auth.one_shot_policy.valid_fixture_execution_count_per_partition -eq 1) "fixture execution count is not one"
Require ($Auth.one_shot_policy.selective_fixture_replay_permitted -eq $false) "selective replay must be forbidden"
Require ($Auth.one_shot_policy.adaptive_fixture_substitution_permitted -eq $false) "fixture substitution must be forbidden"
Require ($Auth.one_shot_policy.threshold_or_rule_change_permitted -eq $false) "threshold mutation must be forbidden"
Require ($Auth.one_shot_policy.implementation_change_permitted -eq $false) "implementation mutation must be forbidden"

$RunnerRel = "run_q6cb2_identification.ps1"
Require ($null -ne $Auth.execution_runner) "authorization does not bind execution runner"
$RunnerBlob = GitBlob $RunnerRel
Require ($RunnerBlob -eq $Auth.execution_runner.git_blob) "runner Git blob mismatch"

Require ((GitBlob "config/q6cb1_execution_contract_v0.1.json") -eq $Auth.execution_contract.git_blob) "execution contract blob mismatch"
Require ((GitBlob "config/q6cb1_execution_contract_lock_v0.1.json") -eq $Auth.execution_contract_lock.git_blob) "execution contract lock blob mismatch"
Require ($ContractLock.status -eq "Q6CB_EXECUTION_CONTRACT_LOCKED_EXECUTION_CLOSED") "execution contract lock status mismatch"
Require ($ContractLock.contract.git_blob -eq $Auth.execution_contract.git_blob) "lock does not bind authorized contract"

$BlobChecks = [ordered]@{
    "src/q6cb_q6_reference.hpp" = $Auth.exact_source_blobs.canonical_reference
    "src/q6cb_fixture_generator.hpp" = $Auth.exact_source_blobs.fixture_generator
    "src/q6cb_causal_harness.cpp" = $Auth.exact_source_blobs.causal_harness
    "shaders/q6cb_t32_gpu_packed.comp" = $Auth.exact_source_blobs.gpu_packed
    "shaders/q6cb_t32_gpu_expanded.comp" = $Auth.exact_source_blobs.gpu_expanded
}
foreach ($kv in $BlobChecks.GetEnumerator()) {
    Require ((GitBlob $kv.Key) -eq $kv.Value) "locked source blob mismatch: $($kv.Key)"
}

$Shapes = @($Contract.fixture_generator_usage.shape_order)
$Strata = @($Contract.fixture_generator_usage.conditioning_strata_order)
$Seeds = @($Contract.seed_derivation.identification_seed_hex_ordered)
Require ($Shapes.Count -eq 2) "shape census mismatch"
Require ($Strata.Count -eq 5) "strata census mismatch"
Require ($Seeds.Count -eq 30) "identification seed census mismatch"

$SeenSeeds = @{}
$idx = 0
foreach ($shape in $Shapes) {
    foreach ($stratum in $Strata) {
        foreach ($rep in 1..3) {
            $f = $Auth.identification_partition.fixtures[$idx]
            $ordinal = $idx + 1
            $expectedId = "Q6CB2-ID-{0:D3}" -f $ordinal
            $expectedLabel = "ArcLLM|Q6CB1|v0.1|IDENTIFICATION|$($shape.id)|$stratum|R$rep"
            $expectedSeed = $Seeds[$idx]

            Require ([int]$f.ordinal -eq $ordinal) "fixture ordinal mismatch at $ordinal"
            Require ($f.fixture_id -eq $expectedId) "fixture id mismatch at $ordinal"
            Require ($f.partition -eq "IDENTIFICATION") "partition mismatch at $ordinal"
            Require ($f.shape_id -eq $shape.id) "shape mismatch at $ordinal"
            Require ([int]$f.n -eq [int]$shape.n) "n mismatch at $ordinal"
            Require ([int]$f.rows -eq [int]$shape.rows) "rows mismatch at $ordinal"
            Require ($f.stratum -eq $stratum) "stratum mismatch at $ordinal"
            Require ([int]$f.replicate -eq $rep) "replicate mismatch at $ordinal"
            Require ($f.seed_label -eq $expectedLabel) "seed label mismatch at $ordinal"
            Require ($f.seed_hex -eq $expectedSeed) "seed mismatch at $ordinal"

            $seedKey = $f.seed_hex.ToUpperInvariant()
            Require (-not $SeenSeeds.ContainsKey($seedKey)) "duplicate identification seed: $seedKey"
            $SeenSeeds[$seedKey] = $true
            $idx++
        }
    }
}
Require ($idx -eq 30) "canonical enumeration did not produce 30 fixtures"
Require ($Auth.identification_partition.seed_schedule_sha256 -eq $Contract.seed_derivation.seed_schedule_sha256) "authorization seed-schedule digest mismatch"
Require ($Contract.seed_derivation.seed_schedule_sha256 -eq "C6C47227DA52D41F485A21319DEAB81514A6A7555D0EFAD8113D75F304BA273E") "contract seed-schedule digest drift"

$Exe = Join-Path $Root ($Auth.exact_runtime_artifacts.executable_path -replace '/','\')
$PackedSpv = Join-Path $Root ($Auth.exact_runtime_artifacts.packed_spv_path -replace '/','\')
$ExpandedSpv = Join-Path $Root ($Auth.exact_runtime_artifacts.expanded_spv_path -replace '/','\')

$ExeHash = Sha256 $Exe
$PackedHash = Sha256 $PackedSpv
$ExpandedHash = Sha256 $ExpandedSpv
Require ($ExeHash -eq $Auth.exact_runtime_artifacts.executable_sha256) "executable SHA-256 mismatch"
Require ($PackedHash -eq $Auth.exact_runtime_artifacts.packed_spv_sha256) "packed SPIR-V SHA-256 mismatch"
Require ($ExpandedHash -eq $Auth.exact_runtime_artifacts.expanded_spv_sha256) "expanded SPIR-V SHA-256 mismatch"

$CpuNames = @(Get-CimInstance Win32_Processor | ForEach-Object { $_.Name })
Require ($CpuNames -contains $Auth.hard_environment_contract.cpu) "CPU identity mismatch"

$GpuRows = @(
    Get-CimInstance Win32_VideoController |
    Where-Object { $_.Name -match "Intel.*Arc.*140V" }
)
Require ($GpuRows.Count -ge 1) "Intel Arc 140V not found"
Require ($GpuRows.DriverVersion -contains $Auth.hard_environment_contract.windows_driver) "GPU driver mismatch"

# Q6CB-2 bounded infrastructure repair: use the already frozen SA0 Vulkan
# capability probe instead of assuming portable SDK contains vulkaninfo.exe.
Require ($null -ne $Auth.environment_probe) "authorization does not bind environment probe"
Require ((GitBlob "src/sa0_capability_probe.cpp") -eq $Auth.environment_probe.source_git_blob) "SA0 capability probe source blob mismatch"
Require ((GitBlob "tools/build_sa0_cap.ps1") -eq $Auth.environment_probe.build_tool_git_blob) "SA0 capability build tool blob mismatch"

$ProbeBuild = Join-Path $Root "tools\build_sa0_cap.ps1"
$ProbeExe = Join-Path $Root "artifacts\SA0_CAP\arcllm_sa0_cap.exe"
$ProbeRaw = Join-Path $Root "artifacts\SA0_CAP\q6cb2_environment_probe.json"

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ProbeBuild
Require ($LASTEXITCODE -eq 0) "SA0 capability probe BuildOnly failed"
Require (Test-Path $ProbeExe -PathType Leaf) "SA0 capability probe executable missing"

if (Test-Path $ProbeRaw) { Remove-Item -Force $ProbeRaw }
& $ProbeExe --device-substring "Arc 140V" --out $ProbeRaw
Require ($LASTEXITCODE -eq 0) "SA0 capability probe execution failed"
Require (Test-Path $ProbeRaw -PathType Leaf) "SA0 capability probe output missing"

$Cap = Get-Content $ProbeRaw -Raw -Encoding UTF8 | ConvertFrom-Json
Require ($Cap.status -eq "PASS_QUERY") "SA0 capability probe did not PASS_QUERY"
Require ($Cap.scientific_workload -eq "NOT_RUN") "environment probe scientific-workload invariant violated"
Require ([bool]$Cap.model_loaded -eq $false) "environment probe model-load invariant violated"
Require ([int]$Cap.shader_modules_created -eq 0) "environment probe created shader modules"
Require ([int]$Cap.compute_pipelines_created -eq 0) "environment probe created compute pipelines"
Require ([int]$Cap.dispatches_submitted -eq 0) "environment probe submitted GPU dispatches"
Require ([int64]$Cap.device.vendor_id -eq [int64]$Auth.hard_environment_contract.intel_vendor_id) "Vulkan vendor id mismatch"
Require ([string]$Cap.device.name -match "Arc.*140V") "Vulkan target GPU mismatch"
Require ([string]$Cap.device.driver_info -eq [string]$Auth.hard_environment_contract.vulkan_driver_info) "Vulkan driverInfo mismatch"
Require ([string]$Cap.device.api_version.text -ge [string]$Auth.hard_environment_contract.minimum_vulkan_device_api) "Vulkan device API below contract minimum"
Require ([bool]$Cap.subgroup.compute_stage_supported) "compute-stage subgroup support missing"
Require ([int]$Cap.subgroup.size -eq [int]$Auth.hard_environment_contract.subgroup_size) "Vulkan subgroup size mismatch"
Require ([bool]$Cap.subgroup.basic) "Vulkan subgroup BASIC capability missing"
Require ([bool]$Cap.subgroup.arithmetic) "Vulkan subgroup ARITHMETIC capability missing"

Require (-not (Test-Path $OutDir)) "result directory already exists; partition may already be consumed"
Require (-not (Test-Path $Bundle)) "return bundle already exists; partition may already be consumed"

Write-Host "Q6CB2_PREFLIGHT_PASS"
Write-Host "Scientific execution is now starting: exact frozen 30-fixture identification partition."

New-Item -ItemType Directory -Path $RawDir -Force | Out-Null
Copy-Item $ProbeRaw (Join-Path $OutDir "environment_probe.json")
$StartUtc = [DateTime]::UtcNow.ToString("o")

foreach ($f in @($Auth.identification_partition.fixtures)) {
    $ordinal = [int]$f.ordinal
    $rawName = "{0:D3}_{1}.json" -f $ordinal, $f.fixture_id
    $rawPath = Join-Path $RawDir $rawName

    Write-Host ("Q6CB2 {0:D2}/30 {1} {2} {3} R{4}" -f $ordinal, $f.fixture_id, $f.shape_id, $f.stratum, $f.replicate)

    $HarnessArgs = @(
        "--authorization", $AuthPath,
        "--fixture-id", $f.fixture_id,
        "--stratum", $f.stratum,
        "--n", ([string]$f.n),
        "--rows", ([string]$f.rows),
        "--seed", $f.seed_hex,
        "--packed-spv", $PackedSpv,
        "--expanded-spv", $ExpandedSpv,
        "--out", $rawPath
    )
    & $Exe @HarnessArgs

    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Fail "F0 candidate at $($f.fixture_id), exit=$exitCode. Do not replay an individual fixture."
    }
    Require (Test-Path $rawPath -PathType Leaf) "raw result missing for $($f.fixture_id)"
}

$EndUtc = [DateTime]::UtcNow.ToString("o")

$RawEvidence = @()
foreach ($f in @($Auth.identification_partition.fixtures)) {
    $ordinal = [int]$f.ordinal
    $rawName = "{0:D3}_{1}.json" -f $ordinal, $f.fixture_id
    $rawPath = Join-Path $RawDir $rawName
    $RawEvidence += [ordered]@{
        ordinal = $ordinal
        fixture_id = $f.fixture_id
        partition = "IDENTIFICATION"
        shape_id = $f.shape_id
        n = [int]$f.n
        rows = [int]$f.rows
        stratum = $f.stratum
        replicate = [int]$f.replicate
        seed_label = $f.seed_label
        seed_label_sha256 = StringSha256 $f.seed_label
        seed_hex = $f.seed_hex
        process_exit_code = 0
        raw_file = "raw/$rawName"
        raw_file_sha256 = Sha256 $rawPath
    }
}

$Os = Get-CimInstance Win32_OperatingSystem
$PowerScheme = (powercfg /GETACTIVESCHEME | Out-String).Trim()
$Battery = @(
    Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue |
    Select-Object Name, BatteryStatus
)

$PartitionManifest = [ordered]@{
    schema = "arcllm.q6cb2.identification_partition_manifest.v0.1"
    stage = "Q6CB-2"
    status = "COLLECTION_COMPLETE_NOT_ADJUDICATED"
    git_head = ((& git -C $Root rev-parse HEAD).Trim())
    runner = [ordered]@{
        path = $RunnerRel
        git_blob = $RunnerBlob
    }
    authorization = [ordered]@{
        file = "config/q6cb2_execution_authorization.json"
        git_blob = (GitBlob "config/q6cb2_execution_authorization.json")
        decision = $Auth.decision
        scope = $Auth.scope
    }
    execution_contract = [ordered]@{
        git_blob = $Auth.execution_contract.git_blob
        file_sha256 = Sha256 $ContractPath
    }
    execution_contract_lock = [ordered]@{
        git_blob = $Auth.execution_contract_lock.git_blob
        file_sha256 = Sha256 $ContractLockPath
        lock_commit = $Auth.execution_contract_lock.lock_commit
    }
    implementation_evidence_lock_commit = $Auth.implementation_evidence_lock_commit
    runtime = [ordered]@{
        executable_sha256 = $ExeHash
        packed_spv_sha256 = $PackedHash
        expanded_spv_sha256 = $ExpandedHash
    }
    fixture_count = 30
    row_observations = 1920
    seed_schedule_sha256 = $Auth.identification_partition.seed_schedule_sha256
    environment = [ordered]@{
        cpu = $CpuNames
        gpu = @($GpuRows | Select-Object Name, DriverVersion)
        os_caption = $Os.Caption
        os_version = $Os.Version
        os_build = $Os.BuildNumber
        power_scheme = $PowerScheme
        battery = $Battery
        vulkan_driver_required = $Auth.hard_environment_contract.vulkan_driver_info
        vulkan_probe_source_git_blob = $Auth.environment_probe.source_git_blob
        vulkan_probe_build_tool_git_blob = $Auth.environment_probe.build_tool_git_blob
        vulkan_device_name = [string]$Cap.device.name
        vulkan_device_api = [string]$Cap.device.api_version.text
        vulkan_driver_info = [string]$Cap.device.driver_info
        subgroup_size = [int]$Cap.subgroup.size
        subgroup_basic = [bool]$Cap.subgroup.basic
        subgroup_arithmetic = [bool]$Cap.subgroup.arithmetic
    }
    collection = [ordered]@{
        start_utc = $StartUtc
        end_utc = $EndUtc
        valid_fixture_execution_count = 30
        selective_fixture_rerun = $false
        fixture_substitution = $false
        partial_outcome_inspection = $false
        scientific_adjudication_performed = $false
    }
    raw_evidence = $RawEvidence
    q6cb3 = [ordered]@{
        eligible = $false
        authorized = $false
        executed = $false
    }
}

$ManifestPath = Join-Path $OutDir "partition_manifest.json"
$PartitionManifest | ConvertTo-Json -Depth 12 | Set-Content $ManifestPath -Encoding UTF8

Copy-Item $AuthPath (Join-Path $OutDir "q6cb2_execution_authorization.json")
Copy-Item $ContractPath (Join-Path $OutDir "q6cb1_execution_contract_v0.1.json")
Copy-Item $ContractLockPath (Join-Path $OutDir "q6cb1_execution_contract_lock_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\Q6CB\Q6CB1_EXECUTION_CONTRACT_QA_v0.1.json") (Join-Path $OutDir "q6cb1_execution_contract_qa_v0.1.json")

Compress-Archive -Path (Join-Path $OutDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash = Sha256 $Bundle

Write-Host ""
Write-Host "Q6CB2_IDENTIFICATION_COLLECTION_COMPLETE"
Write-Host "SCIENTIFIC_ADJUDICATION_NOT_PERFORMED"
Write-Host "Q6CB3_NOT_AUTHORIZED"
Write-Host ("bundle=" + $Bundle)
Write-Host ("SHA256=" + $BundleHash)
