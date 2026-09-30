param(
  [string]$VulkanSdkRoot,
  [string]$DeviceSubstring="Arc"
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Here "runs\_relocation_compat.ps1")
$Results=Join-Path $Here "results";New-Item -ItemType Directory -Force $Results|Out-Null
$ExpectedParent="1ade625ae60acaa9ebeb43890d53a198c200d576"
$ExpectedDriver="32.0.101.8860"
$Critical=@(
 "src/sa0_capability_probe.cpp",
 "tools/build_sa0_cap.ps1",
 "runs/sa/run_sa0_capability_preflight.ps1",
 "tests/test_sa0_capability_package.py",
 "config/sa0_capability_preflight_v0.1.json",
 "docs/SA0_CAPABILITY_PREFLIGHT_IMPLEMENTATION.md"
)
$Head=(& git -C $Here rev-parse HEAD).Trim()
if(-not $Head){throw "SA0-CAP cannot resolve HEAD"}
foreach($Rel in $Critical){
  & git -C $Here diff --quiet HEAD -- $Rel
  if($LASTEXITCODE -ne 0){throw "SA0-CAP tracked critical file differs from HEAD: $Rel"}
  & git -C $Here diff --cached --quiet HEAD -- $Rel
  if($LASTEXITCODE -ne 0){throw "SA0-CAP staged critical file differs from HEAD: $Rel"}
}
$Contract=Get-Content (Join-Path $Here "config\sa0_capability_preflight_v0.1.json") -Raw|ConvertFrom-Json
if([string]$Contract.parent_sa0_commit -ne $ExpectedParent){throw "SA0-CAP parent SA0 binding mismatch"}
if([string]$Contract.status -ne "SA0_CAP_PROBE_IMPLEMENTATION_STATIC_LOCKED"){throw "SA0-CAP implementation is not statically locked"}
if([bool]$Contract.model_load_permitted -or [bool]$Contract.shader_execution_permitted -or [bool]$Contract.successor_kernel_implementation_permitted){throw "SA0-CAP zero-science contract violation"}

py -3 (Join-Path $Here "tests\test_sa0_capability_package.py")
if($LASTEXITCODE -ne 0){throw "SA0-CAP static QA failed"}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "SA0-CAP exact CPU mismatch"}
$Arc=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($Arc.Count -lt 1){throw "SA0-CAP exact Arc 140V target not found"}
if(@($Arc|Where-Object {$_.DriverVersion -eq $ExpectedDriver}).Count -lt 1){throw "SA0-CAP driver drift from Q3: expected $ExpectedDriver"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()

$BuildArgs=@("-NoProfile","-ExecutionPolicy","Bypass","-File",(Join-Path $Here "tools\build_sa0_cap.ps1"))
if($VulkanSdkRoot){$BuildArgs+=@("-VulkanSdkRoot",$VulkanSdkRoot)}
& powershell.exe @BuildArgs
if($LASTEXITCODE -ne 0){throw "SA0-CAP BuildOnly failed"}

$Exe=Join-Path $Here "artifacts\SA0_CAP\arcllm_sa0_cap.exe"
$BuildManifest=Join-Path $Here "artifacts\SA0_CAP\build_manifest.json"
if(-not(Test-Path $Exe) -or -not(Test-Path $BuildManifest)){throw "SA0-CAP build artifacts missing"}
$Raw=Join-Path $Results "sa0_capability_raw.json"
& $Exe --device-substring $DeviceSubstring --out $Raw
$Code=$LASTEXITCODE
if(-not(Test-Path $Raw)){throw "SA0-CAP raw capability output missing"}
$Cap=Get-Content $Raw -Raw -Encoding UTF8|ConvertFrom-Json
if($Code -ne 0 -or [string]$Cap.status -ne "PASS_QUERY"){throw "SA0-CAP Vulkan query failed"}
if([string]$Cap.scientific_workload -ne "NOT_RUN" -or [bool]$Cap.model_loaded){throw "SA0-CAP zero-science invariant violated"}
if([int]$Cap.shader_modules_created -ne 0 -or [int]$Cap.compute_pipelines_created -ne 0 -or [int]$Cap.dispatches_submitted -ne 0){throw "SA0-CAP unexpectedly executed GPU shader work"}
if([int64]$Cap.device.vendor_id -ne 32902 -or [string]$Cap.device.name -notmatch "Arc.*140V"){throw "SA0-CAP selected Vulkan device is not Intel Arc 140V"}
if(-not[bool]$Cap.baseline_capability_gate){throw "SA0-CAP baseline capability gate failed"}
if([int]$Cap.selected_compute_queue_family -lt 0){throw "SA0-CAP no compute queue"}
$Q=@($Cap.queue_families|Where-Object {[int]$_.index -eq [int]$Cap.selected_compute_queue_family})|Select-Object -First 1
if(-not $Q -or -not[bool]$Q.compute -or [int]$Q.timestamp_valid_bits -le 0){throw "SA0-CAP compute queue/timestamp capability invalid"}
if(-not[bool]$Cap.subgroup.compute_stage_supported -or [int]$Cap.subgroup.size -le 0){throw "SA0-CAP compute subgroup capability invalid"}
if([int64]$Cap.compute_limits.max_compute_shared_memory_size -le 0 -or [int]$Cap.compute_limits.max_compute_workgroup_invocations -le 0){throw "SA0-CAP compute limits invalid"}
if([int]$Cap.memory.heap_count -le 0 -or [int]$Cap.memory.type_count -le 0){throw "SA0-CAP memory topology invalid"}

function Sha([string]$P){(Get-FileHash -Algorithm SHA256 -LiteralPath $P).Hash.ToUpperInvariant()}
$CriticalHashes=[ordered]@{};$CriticalBlobs=[ordered]@{}
foreach($Rel in $Critical){
  $P=Join-Path $Here $Rel
  $CriticalHashes[$Rel]=Sha $P
  $CriticalBlobs[$Rel]=((Get-RunLogicalGitBlob -RepoRoot $Here -Path $Rel))
}
$Build=Get-Content $BuildManifest -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Build.git_head -ne $Head){throw "SA0-CAP build manifest HEAD mismatch"}
if(([string]$Build.executable_sha256).ToUpperInvariant() -ne (Sha $Exe)){throw "SA0-CAP executable SHA mismatch"}

$Lock=[ordered]@{
 schema="arcllm.sa0.capability.preflight_lock.v0.1"
 status="PASS_BASELINE_CAPABILITIES_OPTIONALS_RECORDED"
 created_utc=(Get-Date).ToUniversalTime().ToString("o")
 implementation_commit=$Head
 parent_sa0_commit=$ExpectedParent
 parent_sa0_status="SA0_SPECIFICATION_QUALIFIED_CAPABILITY_PREFLIGHT_REQUIRED"
 scientific_workload="NOT_RUN"
 model_loaded=$false
 shader_modules_created=0
 compute_pipelines_created=0
 dispatches_submitted=0
 successor_kernel_implemented=$false
 environment=[ordered]@{
   os_caption=$OS.Caption;os_version=$OS.Version;os_build=$OS.BuildNumber
   cpu=$CPU;gpu=$GPU;power_scheme=$PowerScheme
   q3_reference_driver=$ExpectedDriver
 }
 raw_capability_sha256=(Sha $Raw)
 build_manifest_sha256=(Sha $BuildManifest)
 executable_sha256=(Sha $Exe)
 critical_file_sha256=$CriticalHashes
 critical_git_blobs=$CriticalBlobs
 baseline_capabilities=[ordered]@{
   intel_vendor=$true
   arc_140v=$true
   vulkan_1_2_or_later=$true
   compute_queue=$true
   timestamps=$true
   compute_subgroup=$true
   compute_limits=$true
   memory_topology=$true
 }
 optional_capabilities=[ordered]@{
   subgroup_size=[int]$Cap.subgroup.size
   subgroup_supported_operations=[uint64]$Cap.subgroup.supported_operations
   subgroup_extended_types=[bool]$Cap.subgroup.extended_types
   subgroup_size_control_available=[bool]$Cap.subgroup.size_control_available
   subgroup_size_control_feature=[bool]$Cap.subgroup.size_control_feature
   compute_full_subgroups=[bool]$Cap.subgroup.compute_full_subgroups
   min_subgroup_size=[int]$Cap.subgroup.min_size
   max_subgroup_size=[int]$Cap.subgroup.max_size
   storage_buffer_8bit_access=[bool]$Cap.scalar_features.storage_buffer_8bit_access
   storage_buffer_16bit_access=[bool]$Cap.scalar_features.storage_buffer_16bit_access
   shader_float16=[bool]$Cap.scalar_features.shader_float16
   shader_int8=[bool]$Cap.scalar_features.shader_int8
   cooperative_matrix_khr_extension=[bool]$Cap.cooperative_matrix.khr_extension
   cooperative_matrix_khr_property_count=[int]$Cap.cooperative_matrix.khr_property_count
   cooperative_matrix_nv_extension=[bool]$Cap.cooperative_matrix.nv_extension
   cooperative_matrix_nv_property_count=[int]$Cap.cooperative_matrix.nv_property_count
 }
 authorization=[ordered]@{
   sa1_preregistration=$false
   successor_kernel_implementation=$false
   target_model_execution=$false
   q3_reopen=$false
 }
 next="Return SA0-CAP evidence for independent adjudication. Only an independently accepted exact-device PASS may open SA1-P preregistration."
}
$LockPath=Join-Path $Results "sa0_capability_preflight_lock.json"
[IO.File]::WriteAllText($LockPath,($Lock|ConvertTo-Json -Depth 16),(New-Object Text.UTF8Encoding($false)))
$Bundle=Join-Path $Results "sa0_capability_return_to_chatgpt.zip"
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
Compress-Archive -Path @($Raw,$LockPath,$BuildManifest,(Join-Path $Here "config\sa0_capability_preflight_v0.1.json"),(Join-Path $Here "artifacts\SA0\SA0_SPECIFICATION_QA_v0.1.json")) -DestinationPath $Bundle -Force
Write-Host "SA0-CAP PASS / ZERO SCIENCE"
Write-Host "  implementation_commit=$Head"
Write-Host "  raw=$Raw"
Write-Host "  lock=$LockPath"
Write-Host "  bundle=$Bundle"
Write-Host "  SHA256=$(Sha $Bundle)"
