$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=(Resolve-Path (Join-Path $Here "..\..")).Path
$EvidenceDir=Join-Path $Root "results\arcllm_lmax_arch_p0_preflight"
$BuildDir=Join-Path $EvidenceDir "build"
New-Item -ItemType Directory -Force -Path $BuildDir|Out-Null

$StaticQa=Join-Path $Here "static_qa.py"
$Generator=Join-Path $Here "generate_instrumented_runtime.py"
$Generated=Join-Path $BuildDir "arcllm_v1_runtime_p0_instrumented.cpp"
$TransformManifest=Join-Path $EvidenceDir "RUNTIME_TRANSFORM.json"
$SelftestExe=Join-Path $BuildDir "arcllm_lmax_p0_ring_selftest.exe"
$RunnerExe=Join-Path $BuildDir "arcllm_lmax_p0_runner.exe"
$DescribeOut=Join-Path $EvidenceDir "RUNNER_DESCRIBE.txt"
$GuardOut=Join-Path $EvidenceDir "RUNNER_AUTH_GUARD.txt"
$EvidenceOut=Join-Path $EvidenceDir "BUILDONLY_EVIDENCE.json"

py -3 $StaticQa
if($LASTEXITCODE-ne0){throw "P0 static QA failed"}

py -3 $Generator --root $Root --out $Generated --manifest $TransformManifest
if($LASTEXITCODE-ne0){throw "P0 runtime instrumentation generation failed"}

$PF86=[Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if(-not $PF86){$PF86="C:\Program Files (x86)"}
$VsWhere=Join-Path $PF86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(-not(Test-Path $VsWhere)){throw "vswhere.exe not found"}
$VSInstall=& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $VSInstall){throw "Visual Studio Build Tools not found"}
$DevCmd=Join-Path $VSInstall "VC\Auxiliary\Build\vcvars64.bat"

Push-Location $BuildDir
try{
    $SelfSrc=Join-Path $Here "p0_ring_selftest.cpp"
    $SelfCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /Brepro /I:"'+$Here+'" /Fe:"'+$SelftestExe+'" "'+$SelfSrc+'"'
    cmd.exe /d /s /c $SelfCmd
    if($LASTEXITCODE-ne0-or-not(Test-Path $SelftestExe)){throw "P0 ring selftest build failed"}

    $SelfOutput=& $SelftestExe 2>&1 | Out-String
    if($LASTEXITCODE-ne0-or $SelfOutput-notmatch "ARCLLM_LMAX_P0_RING_SELFTEST=PASS"){
        throw "P0 ring invariant selftest failed: $SelfOutput"
    }

    $Sources=@(
      $Generated,
      (Join-Path $Here "p0_runner.cpp"),
      (Join-Path $Root "src\gguf.cpp"),
      (Join-Path $Root "src\tensor_store.cpp"),
      (Join-Path $Root "src\arcllm_v1_primitive_registry_v2.cpp"),
      (Join-Path $Root "src\arcllm_v1_generic_policy_engine_v4.cpp"),
      (Join-Path $Root "src\arcllm_v1_generic_backend_binding_v4.cpp"),
      (Join-Path $Root "src\registrations\arcllm_v1_q4k_down_reference_registration_v2.cpp")
    )|ForEach-Object{'"'+$_+'"'}

    $RunnerCmd='"'+$DevCmd+'" >nul && cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /bigobj /Brepro /I:"'+(Join-Path $Root "src")+'" /I:"'+(Join-Path $Root "include")+'" /I:"'+$Here+'" /Fe:"'+$RunnerExe+'" '+($Sources -join ' ')
    cmd.exe /d /s /c $RunnerCmd
    if($LASTEXITCODE-ne0-or-not(Test-Path $RunnerExe)){throw "P0 matched-arm runner BuildOnly failed"}

    $Describe=& $RunnerExe --describe 2>&1 | Out-String
    if($LASTEXITCODE-ne0-or $Describe-notmatch "MODEL_EXECUTED=false"-or $Describe-notmatch "VULKAN_INITIALIZED=false"){
        throw "P0 BuildOnly describe guard failed: $Describe"
    }
    [IO.File]::WriteAllText($DescribeOut,$Describe,(New-Object Text.UTF8Encoding($false)))

    $Old=$ErrorActionPreference
    $ErrorActionPreference="Continue"
    $Guard=& $RunnerExe 2>&1 | Out-String
    $GuardCode=$LASTEXITCODE
    $ErrorActionPreference=$Old
    if($GuardCode-ne2-or $Guard-notmatch "outcome execution blocked"){
        throw "P0 execution authorization fail-closed guard failed: code=$GuardCode output=$Guard"
    }
    [IO.File]::WriteAllText($GuardOut,$Guard,(New-Object Text.UTF8Encoding($false)))

    $CompilerCmd='"'+$DevCmd+'" >nul && cl.exe /Bv'
    $Old=$ErrorActionPreference
    $ErrorActionPreference="Continue"
    $CompilerInfo=cmd.exe /d /s /c $CompilerCmd 2>&1 | Out-String
    $ErrorActionPreference=$Old

    $Tracked=@(
      "experiments/arcllm_lmax_arch_p0/p0_ring.h",
      "experiments/arcllm_lmax_arch_p0/p0_ring_selftest.cpp",
      "experiments/arcllm_lmax_arch_p0/p0_runner.cpp",
      "experiments/arcllm_lmax_arch_p0/generate_instrumented_runtime.py",
      "experiments/arcllm_lmax_arch_p0/static_qa.py",
      "experiments/arcllm_lmax_arch_p0/buildonly.ps1",
      "config/arcllm_lmax_arch_p0_preregistration_v0.1.json",
      "docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P0_PREREGISTRATION.md"
    )
    $SourceHashes=[ordered]@{}
    $GitBlobs=[ordered]@{}
    foreach($Rel in $Tracked){
        $Path=Join-Path $Root $Rel
        $SourceHashes[$Rel]=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()
        $GitBlobs[$Rel]=((& git -C $Root hash-object $Rel).Trim())
    }

    $Evidence=[ordered]@{
      schema="arcllm.lmax_arch_p0.buildonly_evidence.v0.1"
      status="PASS_IMPLEMENTATION_STATIC_PREFLIGHT_BUILDONLY"
      git_head=((& git -C $Root rev-parse HEAD).Trim())
      canonical_runtime_git_blob=((& git -C $Root hash-object "src/arcllm_v1_runtime.cpp").Trim())
      canonical_api_git_blob=((& git -C $Root hash-object "include/arcllm/v1/runtime.h").Trim())
      source_sha256=$SourceHashes
      source_git_blobs=$GitBlobs
      generated_runtime_sha256=(Get-FileHash $Generated -Algorithm SHA256).Hash.ToUpperInvariant()
      transform_manifest_sha256=(Get-FileHash $TransformManifest -Algorithm SHA256).Hash.ToUpperInvariant()
      ring_selftest_exe_sha256=(Get-FileHash $SelftestExe -Algorithm SHA256).Hash.ToUpperInvariant()
      runner_exe_sha256=(Get-FileHash $RunnerExe -Algorithm SHA256).Hash.ToUpperInvariant()
      runner_exe_bytes=(Get-Item $RunnerExe).Length
      compiler_info=$CompilerInfo.Trim()
      static_qa="PASS"
      reversible_runtime_transform="PASS"
      ring_ordering="PASS"
      forced_backpressure_invariant="PASS"
      ring_steady_state_allocation_count=0
      runner_authorization_guard="PASS_FAIL_CLOSED"
      model_executed=$false
      vulkan_initialized=$false
      shader_execution=$false
      outcome_performance_measured=$false
      measured_inference_runs=0
      preregistered_control_path_performance_subtest_executed=$false
      lineage_modified=$false
    }
    [IO.File]::WriteAllText($EvidenceOut,($Evidence|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
    Write-Host "ARCLLM_LMAX_ARCH_P0_BUILDONLY=PASS"
    Write-Host "RUNNER_SHA256=$($Evidence.runner_exe_sha256)"
    Write-Host "SELFTEST_SHA256=$($Evidence.ring_selftest_exe_sha256)"
    Write-Host "MODEL_EXECUTED=false"
    Write-Host "MEASURED_INFERENCE_RUNS=0"
}finally{
    Get-ChildItem -Path $BuildDir -Filter "*.obj" -ErrorAction SilentlyContinue|Remove-Item -Force -ErrorAction SilentlyContinue
    Pop-Location
}
