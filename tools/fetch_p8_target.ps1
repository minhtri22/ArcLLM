param([string]$OutPath)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
Write-Warning "P8-A-R1 uses the already-installed Ollama blob; no network download is performed."
if($OutPath){Write-Warning "-OutPath is ignored because the frozen target is a content-addressed Ollama blob."}
$Resolver=Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "resolve_p8_target.ps1"
$Path=& $Resolver
Write-Host "Frozen local P8 target: $Path"
