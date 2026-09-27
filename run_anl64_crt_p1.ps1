param(
 [Parameter(Mandatory=$true)][string]$Model,
 [string]$ShaderDir = "",
 [string]$ImplementationCommit = ""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent $Here
if(-not $ShaderDir){$ShaderDir=Join-Path $Root "compiled_shaders"}
if(-not $ImplementationCommit){$ImplementationCommit=((& git -C $Root rev-parse HEAD).Trim())}
$Exe=Join-Path $Root "anl64_crt_p1_runtime.exe"
$Dir=Join-Path $Root "results\anl64_crt_p1\fresh_collection"
if(Test-Path $Dir){Remove-Item -Recurse -Force $Dir}
New-Item -ItemType Directory -Force -Path $Dir|Out-Null

$Sessions=@(
 [ordered]@{id="A";cells=@(
   @("A_CANONICAL","W-S"),
   @("B_PLAN_SHADOW","W-S"),
   @("C_PLAN_ACTIVE_RESIDUAL","W-S"),
   @("C_PLAN_ACTIVE_RESIDUAL","W-C"),
   @("B_PLAN_SHADOW","W-C"),
   @("A_CANONICAL","W-C")
 )},
 [ordered]@{id="B";cells=@(
   @("C_PLAN_ACTIVE_RESIDUAL","W-S"),
   @("B_PLAN_SHADOW","W-S"),
   @("A_CANONICAL","W-S"),
   @("A_CANONICAL","W-C"),
   @("B_PLAN_SHADOW","W-C"),
   @("C_PLAN_ACTIVE_RESIDUAL","W-C")
 )}
)
$Manifest=@()
foreach($S in $Sessions){
  $idx=0
  foreach($C in $S.cells){
    ++$idx
    $Arm=$C[0];$Work=$C[1]
    $SafeWork=$Work.Replace("-","")
    $Out=Join-Path $Dir ("S"+$S.id+"_"+("{0:D2}" -f $idx)+"_"+$Arm+"_"+$SafeWork+".json")
    Write-Host ("P1_CELL_BEGIN|S"+$S.id+"|"+$idx+"|"+$Arm+"|"+$Work)
    & $Exe --model $Model --shader-dir $ShaderDir --implementation-commit $ImplementationCommit --workload $Work --arm $Arm --warmups 1 --measured 5 --out $Out
    if($LASTEXITCODE-ne0){throw "P1 cell failed: S$($S.id) $idx $Arm $Work"}
    $Obj=Get-Content $Out -Raw|ConvertFrom-Json
    if($Obj.status-ne"PASS_CELL"){throw "P1 cell artifact not PASS"}
    $Manifest += [ordered]@{session=$S.id;order=$idx;arm=$Arm;workload=$Work;file=(Split-Path $Out -Leaf);sha256=(Get-FileHash $Out -Algorithm SHA256).Hash.ToUpperInvariant()}
    Write-Host ("P1_CELL_END|S"+$S.id+"|"+$idx+"|PASS")
  }
}
$M=[ordered]@{
 schema="arcllm.anl64_crt.p1.collection_manifest.v0.1"
 status="PASS_COMPLETE_FRESH_COLLECTION"
 implementation_commit=$ImplementationCommit
 cells=$Manifest
 measured_attempts=60
 historical_p6_timing_reused=$false
 selective_rerun=$false
}
[IO.File]::WriteAllText((Join-Path $Dir "MANIFEST.json"),($M|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Write-Host "ANL64_CRT_P1_COLLECTION=PASS"
