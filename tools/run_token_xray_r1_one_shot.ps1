param(
  [string]$ModelPath,
  [Parameter(Mandatory=$true)][string]$TokenXRayRoot,
  [Parameter(Mandatory=$true)][string]$ExpectedArcLLMHead,
  [Parameter(Mandatory=$true)][string]$ExpectedTokenXRayHead,
  [string]$OutDir
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
if(-not $OutDir){$OutDir=Join-Path $Root "results\token_xray_r1_one_shot"}
$ContractPath=Join-Path $Root "config\token_xray_r1_execution_contract_v0.1.json"
$Contract=Get-Content $ContractPath -Raw -Encoding UTF8|ConvertFrom-Json

$ArcHead=(git -C $Root rev-parse HEAD).Trim()
if($ArcHead-ne$ExpectedArcLLMHead){throw "STOP: ArcLLM HEAD mismatch"}
$ArcDirty=((git -C $Root status --porcelain --untracked-files=no)|Out-String).Trim()
if($ArcDirty){throw "STOP: ArcLLM tracked worktree dirty"}
$TxHead=(git -C $TokenXRayRoot rev-parse HEAD).Trim()
if($TxHead-ne$ExpectedTokenXRayHead){throw "STOP: Token-XRay HEAD mismatch"}
$TxDirty=((git -C $TokenXRayRoot status --porcelain --untracked-files=no)|Out-String).Trim()
if($TxDirty){throw "STOP: Token-XRay tracked worktree dirty"}

if(-not $ModelPath){
  $Resolved=& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\resolve_p8_target.ps1")
  if($LASTEXITCODE-ne0){throw "STOP: exact model resolver failed"}
  $ModelPath=($Resolved|Select-Object -Last 1)
}
if(-not(Test-Path $ModelPath)){throw "STOP: model file missing"}
$ModelItem=Get-Item $ModelPath
$ObservedSha=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($ModelItem.Length-ne[int64]$Contract.model.size_bytes){throw "STOP: model size mismatch"}
if($ObservedSha-ne([string]$Contract.model.sha256).ToUpperInvariant()){throw "STOP: model SHA256 mismatch"}

# Exact-GGUF tokenizer verification is zero-science and must complete before any generate() call.
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_token_xray_r1_tokenizer_probe.ps1")
if($LASTEXITCODE-ne0){throw "STOP: tokenizer probe build failed"}
$PreflightDir=Join-Path $OutDir "preflight"
New-Item -ItemType Directory -Force -Path $PreflightDir|Out-Null
$ExactTokPath=Join-Path $PreflightDir "R1_EXACT_GGUF_TOKENIZATION.json"
& (Join-Path $Root "token_xray_r1_tokenizer_probe.exe") --model $ModelPath --out $ExactTokPath
if($LASTEXITCODE-ne0){throw "STOP: exact-GGUF tokenizer probe failed"}
$ExactTok=Get-Content $ExactTokPath -Raw -Encoding UTF8|ConvertFrom-Json
foreach($Pid in @("P0","P1","P2","P3")){
  $Expected=@($Contract.prompts.$Pid.token_ids)
  $Observed=@($ExactTok.prompts.$Pid.token_ids)
  if($Expected.Count-ne$Observed.Count){throw "STOP: exact-GGUF token count mismatch for $Pid"}
  for($i=0;$i-lt$Expected.Count;$i++){
    if([uint32]$Expected[$i]-ne[uint32]$Observed[$i]){throw "STOP: exact-GGUF token ID mismatch for $Pid index $i"}
  }
}

# Build after all immutable identity/tokenization gates, still before science execution.
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_token_xray_r1_capture_harness.ps1")
if($LASTEXITCODE-ne0){throw "STOP: R1 capture harness build failed"}
$Harness=Join-Path $Root "token_xray_r1_capture_harness.exe"
$ShaderDir=Join-Path $Root "compiled_shaders"
if(-not(Test-Path $Harness)){throw "STOP: R1 harness missing"}

$StatePath=Join-Path $OutDir "EXECUTION_STATE.json"
if(Test-Path $StatePath){throw "STOP: R1 one-shot state already exists; rerun forbidden"}
$RawDir=Join-Path $OutDir "raw"
New-Item -ItemType Directory -Force -Path $RawDir|Out-Null

$State=[ordered]@{
 schema="token_xray.r1.one_shot_state.v0.1"
 status="PREFLIGHT_COMPLETE_NOT_STARTED"
 arcllm_head=$ArcHead
 token_xray_head=$TxHead
 model_sha256=$ObservedSha
 model_size_bytes=$ModelItem.Length
 exact_gguf_tokenization="PASS"
 pairs=@()
 current_pair=$null
 current_member=$null
 model_execution_started=$false
}
foreach($P in @($Contract.matched_pairs)){
  $State.pairs+=@([ordered]@{pair=[int]$P.pair;prompt=[string]$P.prompt;order=@($P.order);members=@()})
}
function Save-State(){
  [IO.File]::WriteAllText($StatePath,($State|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
}
Save-State

function Tokens-Csv([string]$Pid){
  return (@($Contract.prompts.$Pid.token_ids)|ForEach-Object{[string][uint32]$_}) -join ","
}
function Same-U32($A,$B){
  $X=@($A);$Y=@($B)
  if($X.Count-ne$Y.Count){return $false}
  for($i=0;$i-lt$X.Count;$i++){if([uint32]$X[$i]-ne[uint32]$Y[$i]){return $false}}
  return $true
}

$RawCandidate="PASS_CANDIDATE_PENDING_TOKEN_XRAY_VALIDATION"
$FailReason=$null
foreach($Pair in @($Contract.matched_pairs)){
  $PairNum=[int]$Pair.pair
  $Pid=[string]$Pair.prompt
  $PairState=$State.pairs|Where-Object {$_.pair-eq$PairNum}|Select-Object -First 1
  foreach($Member in @($Pair.order)){
    $Mode=[string]$Member
    $State.status="RUNNING"
    $State.current_pair=$PairNum
    $State.current_member=$Mode
    $State.model_execution_started=$true
    $PairState.members+=@([ordered]@{mode=$Mode;status="STARTED"})
    Save-State

    $OutFile=Join-Path $RawDir ("pair{0:D2}_{1}.json" -f $PairNum,$Mode)
    & $Harness --model $ModelPath --shader-dir $ShaderDir --tokens (Tokens-Csv $Pid) --mode $Mode --prompt-id $Pid --out $OutFile
    if($LASTEXITCODE-ne0){
      $PairState.members[-1].status="RUNTIME_FAILURE_AFTER_START"
      $State.status="STOPPED_RUNTIME_FAILURE_AFTER_MODEL_START"
      Save-State
      throw "R1 runtime member failed after model execution started; rerun forbidden"
    }
    $PairState.members[-1].status="COMPLETE"
    $PairState.members[-1]|Add-Member -NotePropertyName file -NotePropertyValue ([IO.Path]::GetFileName($OutFile))
    Save-State
  }

  $BasePath=Join-Path $RawDir ("pair{0:D2}_baseline.json" -f $PairNum)
  $InstPath=Join-Path $RawDir ("pair{0:D2}_instrumented.json" -f $PairNum)
  $B=Get-Content $BasePath -Raw -Encoding UTF8|ConvertFrom-Json
  $I=Get-Content $InstPath -Raw -Encoding UTF8|ConvertFrom-Json
  if(-not(Same-U32 $B.input_token_ids $I.input_token_ids)){
    $RawCandidate="FAIL_R1_REAL_RUNTIME_CAPTURE_OR_SEMANTICS";$FailReason="prompt_token_mismatch_pair_$PairNum"
  }elseif(-not(Same-U32 $B.generated_token_ids $I.generated_token_ids)){
    $RawCandidate="FAIL_R1_REAL_RUNTIME_CAPTURE_OR_SEMANTICS";$FailReason="generated_token_mismatch_pair_$PairNum"
  }elseif(-not[bool]$B.runtime.finite-or-not[bool]$I.runtime.finite){
    $RawCandidate="FAIL_R1_REAL_RUNTIME_CAPTURE_OR_SEMANTICS";$FailReason="runtime_finite_flag_pair_$PairNum"
  }
  if($FailReason){
    $State.status="RAW_FAIL_STOP"
    Save-State
    break
  }
}

$ValidationOut=Join-Path $OutDir "R1_RAW_EXECUTION_VALIDATION.json"
$Validator=Join-Path $TokenXRayRoot "tools\validate_r1_real_runtime_capture.py"
if(-not(Test-Path $Validator)){throw "STOP: frozen Token-XRay R1 validator missing"}
py -3 $Validator --raw-dir $RawDir --contract $ContractPath --out $ValidationOut
$ValidatorExit=$LASTEXITCODE
if($ValidatorExit-ne0 -and -not $FailReason){
  $RawCandidate="FAIL_R1_REAL_RUNTIME_CAPTURE_OR_SEMANTICS"
  $FailReason="token_xray_capture_validation_failed"
}

$State.current_pair=$null
$State.current_member=$null
$State.status="ONE_SHOT_COMPLETE"
$State|Add-Member -NotePropertyName raw_candidate -NotePropertyValue $RawCandidate -Force
$State|Add-Member -NotePropertyName fail_reason -NotePropertyValue $FailReason -Force
$State|Add-Member -NotePropertyName token_xray_validation_file -NotePropertyValue ([IO.Path]::GetFileName($ValidationOut)) -Force
Save-State

Write-Host "TOKEN_XRAY_R1_ONE_SHOT_EXECUTION=COMPLETE"
Write-Host "RAW_CANDIDATE=$RawCandidate"
Write-Host "OUT_DIR=$OutDir"
if($FailReason){exit 21}
exit 0
