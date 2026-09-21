param([ValidateSet("A","B")][string]$Process,[string]$AuthorizationPath)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
if(-not$AuthorizationPath){$AuthorizationPath=Join-Path $Root "config\sa1_q6_execution_authorization.json"}
if(-not(Test-Path $AuthorizationPath)){throw "SA1 Q6 execution authorization missing; measurement forbidden"}
$A=Get-Content $AuthorizationPath -Raw|ConvertFrom-Json
if([string]$A.decision-ne"SA1_Q6_EXECUTION_AUTHORIZED"-or-not[bool]$A.measurement_permitted){throw "SA1 Q6 execution not authorized"}
$Impl=[string]$A.implementation_commit
$Critical=@("shaders/sa1_q6k_subgroup_splitk.comp","src/sa1_component_benchmark.cpp","tools/compile_sa1_component.ps1","tools/build_sa1_component.ps1","run_sa1_component_q6.ps1","tools/adjudicate_sa1_component.py")
foreach($Rel in $Critical){$Now=(& git -C $Root rev-parse ("HEAD:"+$Rel)).Trim();$Frozen=(& git -C $Root rev-parse ($Impl+":"+$Rel)).Trim();if($Now-ne$Frozen){throw "SA1 Q6 implementation drift: $Rel"}}
$BuildDir=Join-Path $Root "artifacts\SA1_K2\build";$Exe=Join-Path $BuildDir "sa1_component_benchmark.exe";$Base=Join-Path $BuildDir "p7_q6k_gemm_2d.spv";$Cand=Join-Path $BuildDir "sa1_q6k_subgroup_splitk.spv"
function Get-Sha256([string]$P){(Get-FileHash -Algorithm SHA256 -LiteralPath $P).Hash.ToUpperInvariant()}
if((Get-Sha256 $Exe)-ne[string]$A.executable_sha256-or (Get-Sha256 $Base)-ne[string]$A.baseline_spv_sha256-or (Get-Sha256 $Cand)-ne[string]$A.candidate_spv_sha256){throw "SA1 Q6 authorized binary/SPIR-V hash mismatch"}
$Out=Join-Path $Root ("results\sa1_k2_q6_process_"+$Process+".json")
& $Exe --quant q6 --mode measure --process $Process --baseline-spv $Base --candidate-spv $Cand --authorization $AuthorizationPath --out $Out
if($LASTEXITCODE-ne0){throw "SA1 Q6 measured process failed"}
Write-Host "SA1 Q6 process $Process complete: $Out"
