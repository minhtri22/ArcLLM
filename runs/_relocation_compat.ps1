Set-StrictMode -Version Latest

function Get-RunRelocationManifest {
  param([Parameter(Mandatory=$true)][string]$RepoRoot)
  $ManifestPath=Join-Path $RepoRoot "runs\MIGRATION_MANIFEST_2026-10-01.json"
  if(-not(Test-Path $ManifestPath -PathType Leaf)){throw "Run relocation manifest missing: $ManifestPath"}
  $M=Get-Content $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
  if([string]$M.schema -ne "arcllm.runs_relocation_manifest.v1"){throw "Unexpected run relocation manifest schema"}
  if([string]$M.cleanup_parent_commit -ne "c37521311e13398960b4d01ecb90a2804c2f5d3d"){throw "Unexpected run relocation cleanup parent"}
  return $M
}

function Get-RunRelocationEntry {
  param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$Path
  )
  $P=$Path.Replace("\","/")
  $M=Get-RunRelocationManifest -RepoRoot $RepoRoot
  $Matches=@($M.files | Where-Object { [string]$_.old_path -eq $P -or [string]$_.new_path -eq $P })
  if($Matches.Count -gt 1){throw "Ambiguous run relocation mapping: $P"}
  if($Matches.Count -eq 0){return $null}
  return $Matches[0]
}

function Resolve-RunPhysicalPath {
  param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$Path
  )
  $Entry=Get-RunRelocationEntry -RepoRoot $RepoRoot -Path $Path
  $Rel=if($null -ne $Entry){[string]$Entry.new_path}else{$Path.Replace("\","/")}
  return (Join-Path $RepoRoot ($Rel -replace "/","\"))
}

function Get-RunLogicalGitBlob {
  param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$Path
  )
  $P=$Path.Replace("\","/")
  $Entry=Get-RunRelocationEntry -RepoRoot $RepoRoot -Path $P
  if($null -ne $Entry){
    $M=Get-RunRelocationManifest -RepoRoot $RepoRoot
    $NewPath=[string]$Entry.new_path
    $OldPath=[string]$Entry.old_path
    $ExpectedNew=[string]$Entry.new_blob_sha
    $ExpectedOld=[string]$Entry.old_blob_sha

    $HeadBlob=(& git -C $RepoRoot rev-parse ("HEAD:"+$NewPath) 2>$null)
    if($LASTEXITCODE -ne 0 -or -not $HeadBlob){throw "Relocated run missing from HEAD: $NewPath"}
    $HeadBlob=$HeadBlob.Trim()
    if($HeadBlob -ne $ExpectedNew){throw "Relocated run HEAD blob mismatch: $NewPath"}

    $Physical=Resolve-RunPhysicalPath -RepoRoot $RepoRoot -Path $NewPath
    if(-not(Test-Path $Physical -PathType Leaf)){throw "Relocated run missing from worktree: $NewPath"}
    $WorktreeBlob=(& git -C $RepoRoot hash-object -- $NewPath 2>$null)
    if($LASTEXITCODE -ne 0 -or -not $WorktreeBlob){throw "Cannot hash relocated run worktree file: $NewPath"}
    if($WorktreeBlob.Trim() -ne $ExpectedNew){throw "Relocated run worktree drift: $NewPath"}

    $OldBlob=(& git -C $RepoRoot rev-parse (([string]$M.cleanup_parent_commit)+":"+$OldPath) 2>$null)
    if($LASTEXITCODE -ne 0 -or -not $OldBlob){throw "Cannot resolve pre-cleanup run blob: $OldPath"}
    if($OldBlob.Trim() -ne $ExpectedOld){throw "Pre-cleanup run provenance mismatch: $OldPath"}
    return $ExpectedOld
  }

  $Physical=Resolve-RunPhysicalPath -RepoRoot $RepoRoot -Path $P
  if(-not(Test-Path $Physical -PathType Leaf)){throw "Critical file missing: $P"}
  $Blob=(& git -C $RepoRoot hash-object -- $P 2>$null)
  if($LASTEXITCODE -ne 0 -or -not $Blob){throw "Cannot hash critical file: $P"}
  return $Blob.Trim()
}

function Get-RunLogicalGitBlobAtCommit {
  param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$Commit,
    [Parameter(Mandatory=$true)][string]$Path
  )
  $Entry=Get-RunRelocationEntry -RepoRoot $RepoRoot -Path $Path
  if($null -ne $Entry){
    [void](Get-RunLogicalGitBlob -RepoRoot $RepoRoot -Path $Path)
    return [string]$Entry.old_blob_sha
  }
  $P=$Path.Replace("\","/")
  $Blob=(& git -C $RepoRoot rev-parse ($Commit+":"+$P) 2>$null)
  if($LASTEXITCODE -ne 0 -or -not $Blob){throw ("Cannot resolve frozen blob "+$Commit+":"+$P)}
  return $Blob.Trim()
}
