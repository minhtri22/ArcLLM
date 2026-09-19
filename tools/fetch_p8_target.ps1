param([string]$OutPath)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Name="qwen2.5-coder-7b-instruct-q4_k_m.gguf"
$Url="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/13fb94bfda8c8cf22497dc57b78f391a9acb426a/qwen2.5-coder-7b-instruct-q4_k_m.gguf?download=true"
$ExpectedSize=4683073536
$ExpectedHash="509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C"
if(-not $OutPath){$Dir=Join-Path $Root ".models";New-Item -ItemType Directory -Force -Path $Dir|Out-Null;$OutPath=Join-Path $Dir $Name}
if(Test-Path $OutPath){$f=Get-Item $OutPath;if($f.Length -eq $ExpectedSize){$h=(Get-FileHash $OutPath -Algorithm SHA256).Hash.ToUpperInvariant();if($h -eq $ExpectedHash){Write-Host "P8 target already verified: $OutPath";exit 0}};Write-Host "Existing file does not match frozen target; replacing: $OutPath";Remove-Item -Force $OutPath}
Write-Host "Downloading frozen P8 target to $OutPath"
& curl.exe -L --fail --retry 3 --output $OutPath $Url
if($LASTEXITCODE -ne 0){throw "curl download failed"}
$f=Get-Item $OutPath;if($f.Length -ne $ExpectedSize){throw "P8 target size mismatch: $($f.Length)"}
$h=(Get-FileHash $OutPath -Algorithm SHA256).Hash.ToUpperInvariant();if($h -ne $ExpectedHash){throw "P8 target SHA256 mismatch: $h"}
Write-Host "P8 target verified: $OutPath"
Write-Host "size=$($f.Length)"
Write-Host "sha256=$h"
