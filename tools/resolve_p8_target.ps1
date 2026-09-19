param([string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Cfg=Get-Content (Join-Path $Root "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
if($Cfg.source -ne "ollama-local"){throw "P8 target source is not ollama-local"}
if(-not $OllamaModelsRoot){if($env:OLLAMA_MODELS){$OllamaModelsRoot=$env:OLLAMA_MODELS}else{$OllamaModelsRoot=Join-Path $env:USERPROFILE ".ollama\models"}}
$ManifestRel=($Cfg.manifest_search_relative -replace "/","\")
$ManifestDir=Join-Path $OllamaModelsRoot $ManifestRel
if(-not(Test-Path $ManifestDir)){throw "Ollama manifest directory not found: $ManifestDir"}
$ExpectedDigest=([string]$Cfg.model_layer_digest).ToLowerInvariant()
$ExpectedSize=[int64]$Cfg.size_bytes
$ExpectedMedia=[string]$Cfg.model_layer_media_type
$BlobRel=([string]$Cfg.blob_relative -replace "/","\")
$BlobPath=Join-Path $OllamaModelsRoot $BlobRel
$Matches=@()
foreach($Manifest in Get-ChildItem -Path $ManifestDir -File -Recurse){
  try{$Obj=Get-Content $Manifest.FullName -Raw -Encoding UTF8|ConvertFrom-Json}catch{continue}
  foreach($Layer in @($Obj.layers)){
    if($null -ne $Layer -and ([string]$Layer.mediaType) -eq $ExpectedMedia -and ([string]$Layer.digest).ToLowerInvariant() -eq $ExpectedDigest -and [int64]$Layer.size -eq $ExpectedSize){
      $Matches+=@([pscustomobject]@{manifest=$Manifest.FullName;digest=[string]$Layer.digest;size=[int64]$Layer.size})
    }
  }
}
if($Matches.Count -eq 0){throw "No Ollama manifest references frozen P8 model layer digest $ExpectedDigest with size $ExpectedSize"}
if(-not(Test-Path $BlobPath)){throw "Frozen Ollama blob referenced by manifest is missing: $BlobPath"}
$File=Get-Item $BlobPath
if($File.Length -ne $ExpectedSize){throw "Frozen Ollama blob size mismatch: $($File.Length) expected $ExpectedSize"}
Write-Host "P8 Ollama target resolved from $($Matches.Count) matching manifest(s)."
Write-Host "manifest=$($Matches[0].manifest)"
Write-Host "blob=$BlobPath"
Write-Output $BlobPath
