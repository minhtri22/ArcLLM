param([ValidateSet("A","B")][string]$Process,[string]$AuthorizationPath)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
if(-not$AuthorizationPath){$AuthorizationPath=Join-Path $Root "config\sa1_q4_execution_authorization.json"}
if(-not(Test-Path $AuthorizationPath)){throw "SA1 Q4 execution authorization missing; measurement forbidden"}
$A=Get-Content $AuthorizationPath -Raw|ConvertFrom-Json
if([string]$A.decision-ne"SA1_Q4_EXECUTION_AUTHORIZED"-or-not[bool]$A.measurement_permitted){throw "SA1 Q4 execution not authorized"}
$Impl=[string]$A.implementation_commit
$Critical=@("shaders/sa1_q4k_subgroup_splitk.comp","src/sa1_component_benchmark.cpp","tools/compile_sa1_component.ps1","tools/build_sa1_component.ps1","runs/sa/run_sa1_component_q4.ps1","tools/adjudicate_sa1_component.py")
foreach($Rel in $Critical){$Now=(Get-RunLogicalGitBlob -RepoRoot $Root -Path $Rel);$Frozen=(Get-RunLogicalGitBlobAtCommit -RepoRoot $Root -Commit $Impl -Path $Rel);if($Now-ne$Frozen){throw "SA1 implementation drift: $Rel"}}
$BuildDir=Join-Path $Root "artifacts\SA1_K1\build";$Exe=Join-Path $BuildDir "sa1_component_benchmark.exe";$Base=Join-Path $BuildDir "p7_q4k_gemm_2d.spv";$Cand=Join-Path $BuildDir "sa1_q4k_subgroup_splitk.spv"
function Get-Sha256([string]$P){(Get-FileHash -Algorithm SHA256 -LiteralPath $P).Hash.ToUpperInvariant()}
if((Get-Sha256 $Exe)-ne[string]$A.executable_sha256-or (Get-Sha256 $Base)-ne[string]$A.baseline_spv_sha256-or (Get-Sha256 $Cand)-ne[string]$A.candidate_spv_sha256){throw "SA1 authorized binary/SPIR-V hash mismatch"}
$Out=Join-Path $Root ("results\sa1_k1_q4_process_"+$Process+".json")
& $Exe --mode measure --process $Process --baseline-spv $Base --candidate-spv $Cand --authorization $AuthorizationPath --out $Out
if($LASTEXITCODE-ne0){throw "SA1 Q4 measured process failed"}
Write-Host "SA1 Q4 process $Process complete: $Out"
