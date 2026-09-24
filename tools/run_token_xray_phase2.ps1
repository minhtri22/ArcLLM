param(
    [Parameter(Mandatory=$true)][string]$Model,
    [Parameter(Mandatory=$true)][string]$TokenXrayRepo,
    [ValidateSet("W-S","W-C")][string]$Workload="W-S",
    [ValidateRange(0,30)][int]$DecodeIndex=0,
    [double]$TimestampPeriodNs=0,
    [string]$OutputDir=""
)

$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Model=(Resolve-Path $Model).Path
$TokenXrayRepo=(Resolve-Path $TokenXrayRepo).Path
if(-not $OutputDir){
    $Stamp=Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputDir=Join-Path $Root ".local\token_xray_phase2\$Stamp"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$Profile=Join-Path $TokenXrayRepo "profiles\intel\xe2_lpg\arc_140v_core_ultra_7_258v_32gib.v0.1.json"
if(-not(Test-Path $Profile)){throw "Token X-Ray hardware profile missing: $Profile"}

$TokenXrayPython=Join-Path $TokenXrayRepo ".venv\Scripts\python.exe"
if(-not(Test-Path $TokenXrayPython)){
    throw "Token X-Ray virtual environment missing: $TokenXrayPython. Create it once inside the token-xray repo before running Phase 2."
}

if($TimestampPeriodNs -le 0){
    $VulkanInfo=Get-Command vulkaninfo -ErrorAction SilentlyContinue
    if(-not $VulkanInfo){throw "timestampPeriod is required. Install Vulkan Tools or rerun with -TimestampPeriodNs <value>."}

    # Avoid stdout/stderr pipe deadlock: vulkaninfo can emit a large report.
    # Redirect both streams to files and read them only after the native process exits.
    $VkOut=Join-Path $OutputDir "vulkaninfo.stdout.txt"
    $VkErr=Join-Path $OutputDir "vulkaninfo.stderr.txt"
    Remove-Item -Force -ErrorAction SilentlyContinue $VkOut,$VkErr

    $Proc=Start-Process `
      -FilePath $VulkanInfo.Source `
      -RedirectStandardOutput $VkOut `
      -RedirectStandardError $VkErr `
      -NoNewWindow `
      -Wait `
      -PassThru

    $VulkanExit=$Proc.ExitCode
    $StdOut=if(Test-Path $VkOut){Get-Content $VkOut -Raw}else{""}
    $StdErr=if(Test-Path $VkErr){Get-Content $VkErr -Raw}else{""}

    if($VulkanExit -ne 0){
        throw "vulkaninfo failed with exit code $VulkanExit. STDERR: $StdErr"
    }

    $Raw=$StdOut+[Environment]::NewLine+$StdErr
    $Matches=[regex]::Matches($Raw,'timestampPeriod\s*=\s*([0-9]+(?:\.[0-9]+)?)')
    $Values=@($Matches | ForEach-Object {[double]$_.Groups[1].Value} | Select-Object -Unique)
    if($Values.Count -ne 1){
        throw "Could not resolve one unique Vulkan timestampPeriod from vulkaninfo. Values=[$($Values -join ',')]. Rerun with -TimestampPeriodNs <value>."
    }
    $TimestampPeriodNs=$Values[0]
}

$ModelSha=(Get-FileHash -Algorithm SHA256 $Model).Hash.ToUpperInvariant()
$Head=(git -C $Root rev-parse HEAD).Trim()
$IntegrationHead=(git -C $Root rev-parse "origin/integration/token-xray-phase2").Trim()
if($Head -ne $IntegrationHead){
    throw "Token X-Ray worktree HEAD mismatch. HEAD=$Head origin/integration/token-xray-phase2=$IntegrationHead"
}
# This repo declares *.ps1 text eol=crlf. On some Windows worktrees Git can
# report PS1 files modified immediately after checkout/restore even when the
# only difference is line-ending normalization. Reject real content changes,
# but tolerate EOL-only differences. Untracked .local outputs are irrelevant.
git -C $Root diff --quiet --ignore-space-at-eol -- .
if($LASTEXITCODE -ne 0){
    throw "Token X-Ray worktree has real tracked content changes (not EOL-only). Restore or inspect them before tracing."
}
git -C $Root diff --cached --quiet --ignore-space-at-eol -- .
if($LASTEXITCODE -ne 0){
    throw "Token X-Ray worktree has staged tracked content changes. Restore or inspect them before tracing."
}

Write-Host "=== Token X-Ray Phase 2 ==="
Write-Host "ArcLLM HEAD       : $Head"
Write-Host "Model SHA256       : $ModelSha"
Write-Host "Workload           : $Workload"
Write-Host "Decode index       : $DecodeIndex"
Write-Host "Timestamp period ns: $TimestampPeriodNs"
Write-Host "Output             : $OutputDir"

$OldPythonPath=$env:PYTHONPATH
try{
    $env:PYTHONPATH=(Join-Path $TokenXrayRepo "src")
    $StaticDir=Join-Path $OutputDir "static"
    & $TokenXrayPython -m token_xray modellens $Model --hardware-profile $Profile --out $StaticDir
    if($LASTEXITCODE -ne 0){throw "Token X-Ray ModelLens failed"}

    & (Join-Path $Root "tools\compile_arcllm_v1_i003.ps1")
    if($LASTEXITCODE -ne 0){throw "ArcLLM traced shader compile/provenance failed"}

    & (Join-Path $Root "tools\build_arcllm_v1_i003_candidate.ps1")
    if($LASTEXITCODE -ne 0){throw "ArcLLM traced candidate build failed"}

    $Exe=Join-Path $Root "arcllm_v1_i003_candidate.exe"
    $Trace=Join-Path $OutputDir "TOKEN_TRACE.json"
    $CandidateResult=Join-Path $OutputDir "ARCLLM_TRACE_ATTEMPT.json"
    $ShaderDir=Join-Path $Root "compiled_shaders"
    $RequiredSpv=@(
      "p8c_embedding_q4k_segmented_probe.spv",
      "p7_rmsnorm_seq.spv",
      "p7_q4k_gemm_2d.spv",
      "p7_q6k_gemm_2d.spv",
      "p7_rope_seq.spv",
      "p7_kv_store.spv",
      "p7_attention_kv_online.spv",
      "p7_add.spv",
      "sa1_q4k_subgroup_splitk.spv",
      "p7_swiglu.spv",
      "p8q1_lmhead_q6k_segmented_chunk.spv"
    )
    foreach($SpvName in $RequiredSpv){
      $SpvPath=Join-Path $ShaderDir $SpvName
      if(-not(Test-Path $SpvPath)){throw "Required traced SPIR-V missing after compile: $SpvPath"}
    }

    $Args=@(
      "--model",$Model,
      "--shader-dir",$ShaderDir,
      "--implementation-commit",$Head,
      "--workload",$Workload,
      "--warmups","1",
      "--measured","1",
      "--out",$CandidateResult,
      "--token-xray-trace-out",$Trace,
      "--token-xray-model-sha256",$ModelSha,
      "--token-xray-hardware-profile","intel.core_ultra_7_258v.arc_140v.devhost_32gib.v0.1",
      "--token-xray-decode-index",[string]$DecodeIndex,
      "--token-xray-timestamp-period-ns",[string]$TimestampPeriodNs
    )
    & $Exe @Args
    if($LASTEXITCODE -ne 0){throw "ArcLLM Token X-Ray trace run failed"}
    if(-not(Test-Path $Trace)){throw "TOKEN_TRACE.json was not produced"}

    $Ledger=Join-Path $StaticDir "NODE_LEDGER.json"
    $Joined=Join-Path $OutputDir "NODE_LEDGER_TRACED.json"
    $Summary=Join-Path $OutputDir "RUNTIME_TRACE_SUMMARY.json"
    & $TokenXrayPython -m token_xray runtime-trace join --ledger $Ledger --trace $Trace --out $Joined --summary $Summary
    if($LASTEXITCODE -ne 0){throw "Token X-Ray trace join failed"}

    $TraceObj=Get-Content $Trace -Raw | ConvertFrom-Json
    $SummaryObj=Get-Content $Summary -Raw | ConvertFrom-Json
    if($TraceObj.dispatches.Count -ne 469){throw "Expected 469 traced dispatches; got $($TraceObj.dispatches.Count)"}
    if($SummaryObj.unknown_semantic_ids.Count -ne 0){throw "Trace contains unknown semantic NodeIDs"}
    if($SummaryObj.unmapped_dispatch_ids.Count -ne 0){throw "Trace contains unmapped dispatches"}
    $JoinedObj=Get-Content $Joined -Raw | ConvertFrom-Json
    $MissingShaderNodes=@($JoinedObj.nodes | Where-Object {
      $_.execution.state -eq "MEASURED" -and @($_.execution.shaders).Count -eq 0
    })
    if($MissingShaderNodes.Count -ne 0){
      throw "Joined ledger is missing kernel/shader mapping for $($MissingShaderNodes.Count) measured semantic nodes"
    }

    Write-Host ""
    Write-Host "TOKEN_XRAY_PHASE2=PASS"
    Write-Host "TRACE=$Trace"
    Write-Host "JOINED_LEDGER=$Joined"
    Write-Host "SUMMARY=$Summary"
    Write-Host "DISPATCHES=$($TraceObj.dispatches.Count)"
    Write-Host "DEVICE_SPAN_NS=$($TraceObj.timing.device_span_ns)"
    Write-Host "SUM_DISPATCH_NS=$($TraceObj.timing.sum_dispatch_duration_ns)"
    Write-Host "UNATTRIBUTED_NS=$($TraceObj.timing.unattributed_device_time_ns)"
} finally {
    $env:PYTHONPATH=$OldPythonPath
}
