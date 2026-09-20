param(
  [Parameter(Mandatory=$true)][int]$ProcessId,
  [Parameter(Mandatory=$true)][string]$OutputPath,
  [Parameter(Mandatory=$true)][string]$StopPath,
  [int]$SampleMilliseconds=100
)
$ErrorActionPreference="Stop"
$engineCompute="\GPU Engine(*pid_${ProcessId}*engtype_Compute*)\Utilization Percentage"
$engine3D="\GPU Engine(*pid_${ProcessId}*engtype_3D*)\Utilization Percentage"
$dedicated="\GPU Process Memory(pid_${ProcessId}_*)\Dedicated Usage"
$shared="\GPU Process Memory(pid_${ProcessId}_*)\Shared Usage"
try {
  while(-not (Test-Path $StopPath)){
    try {
      $s=Get-Counter -Counter @($engineCompute,$engine3D,$dedicated,$shared) -ErrorAction Stop
      $eng=@($s.CounterSamples|Where-Object {$_.Path -like "*GPU Engine*"}|ForEach-Object {[double]$_.CookedValue})
      $ded=@($s.CounterSamples|Where-Object {$_.Path -like "*Dedicated Usage*"}|ForEach-Object {[double]$_.CookedValue})
      $shr=@($s.CounterSamples|Where-Object {$_.Path -like "*Shared Usage*"}|ForEach-Object {[double]$_.CookedValue})
      $row=[ordered]@{
        timestamp_epoch_s=([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()/1000.0)
        valid=$true
        compute_percent=if($eng.Count){($eng|Measure-Object -Sum).Sum}else{$null}
        dedicated_bytes=if($ded.Count){($ded|Measure-Object -Sum).Sum}else{$null}
        shared_bytes=if($shr.Count){($shr|Measure-Object -Sum).Sum}else{$null}
      }
      Add-Content -LiteralPath $OutputPath -Value ($row|ConvertTo-Json -Compress) -Encoding UTF8
    } catch {
      $row=[ordered]@{timestamp_epoch_s=([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()/1000.0);valid=$false;error=$_.Exception.Message}
      Add-Content -LiteralPath $OutputPath -Value ($row|ConvertTo-Json -Compress) -Encoding UTF8
      break
    }
    Start-Sleep -Milliseconds $SampleMilliseconds
  }
} catch {
  $row=[ordered]@{timestamp_epoch_s=([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()/1000.0);valid=$false;error=$_.Exception.Message}
  Add-Content -LiteralPath $OutputPath -Value ($row|ConvertTo-Json -Compress) -Encoding UTF8
  exit 0
}
