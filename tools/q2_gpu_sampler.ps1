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
function Read-CounterValues([string]$Path){
  try { return @((Get-Counter -Counter $Path -ErrorAction Stop).CounterSamples|ForEach-Object {[double]$_.CookedValue}) }
  catch { return @() }
}
try {
  while(-not (Test-Path $StopPath)){
    try {
      $compute=Read-CounterValues $engineCompute
      $threeD=Read-CounterValues $engine3D
      $ded=Read-CounterValues $dedicated
      $shr=Read-CounterValues $shared
      if($compute.Count -eq 0 -and $threeD.Count -eq 0 -and $ded.Count -eq 0 -and $shr.Count -eq 0){throw "No supported per-process GPU counters available"}
      $computeSum=if($compute.Count){[double](($compute|Measure-Object -Sum).Sum)}else{$null}
      $threeDSum=if($threeD.Count){[double](($threeD|Measure-Object -Sum).Sum)}else{$null}
      $selected=$null;$source=$null
      if($null -ne $computeSum -and $null -ne $threeDSum){$selected=[Math]::Max($computeSum,$threeDSum);$source="max(Compute,3D)"}
      elseif($null -ne $computeSum){$selected=$computeSum;$source="Compute"}
      elseif($null -ne $threeDSum){$selected=$threeDSum;$source="3D"}
      $row=[ordered]@{
        timestamp_epoch_s=([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()/1000.0)
        valid=$true
        compute_percent=$selected
        compute_counter_source=$source
        compute_engine_percent=$computeSum
        three_d_engine_percent=$threeDSum
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
