param(
  [Parameter(Mandatory=$true)][int]$BlockId,
  [Parameter(Mandatory=$true)][int]$Position,
  [Parameter(Mandatory=$true)][int]$GlobalOrdinal,
  [Parameter(Mandatory=$true)][string]$PreviousCondition,
  [Parameter(Mandatory=$true)][string]$P3StartUtc,
  [string]$PreviousCellEndUtc = ""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

function OneCounter([string]$Path) {
  $s=(Get-Counter $Path -MaxSamples 1).CounterSamples
  if($null -eq $s -or $s.Count -ne 1){ throw "counter unavailable: $Path" }
  return [double]$s[0].CookedValue
}

$now=[DateTime]::UtcNow
$start=[DateTime]::Parse($P3StartUtc).ToUniversalTime()
$prevElapsed=$null
if($PreviousCellEndUtc){
  $prev=[DateTime]::Parse($PreviousCellEndUtc).ToUniversalTime()
  $prevElapsed=($now-$prev).TotalSeconds
}

$cpuUtil=OneCounter '\Processor(_Total)\% Processor Time'
$cpuPerf=OneCounter '\Processor Information(_Total)\% Processor Performance'
$os=Get-CimInstance Win32_OperatingSystem
$availBytes=[int64]$os.FreePhysicalMemory * 1024

$powerText=(powercfg /GETACTIVESCHEME | Out-String).Trim()
if($LASTEXITCODE -ne 0){ throw "powercfg failed" }
$m=[regex]::Match($powerText,'([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})')
if(-not $m.Success){ throw "active power scheme GUID unavailable" }
$powerGuid=$m.Groups[1].Value.ToLowerInvariant()

$src='using System; using System.Runtime.InteropServices; public static class P3PowerStatus { [StructLayout(LayoutKind.Sequential)] public struct SPS { public byte ACLineStatus; public byte BatteryFlag; public byte BatteryLifePercent; public byte SystemStatusFlag; public int BatteryLifeTime; public int BatteryFullLifeTime; } [DllImport("kernel32.dll")] public static extern bool GetSystemPowerStatus(out SPS s); }'
if(-not ("P3PowerStatus" -as [type])){ Add-Type -TypeDefinition $src }
$ps=New-Object P3PowerStatus+SPS
if(-not [P3PowerStatus]::GetSystemPowerStatus([ref]$ps)){ throw "GetSystemPowerStatus failed" }

$gpu=(Get-Counter '\GPU Engine(*)\Utilization Percentage' -MaxSamples 1).CounterSamples |
  Where-Object { $_.Status -eq 0 -and -not [double]::IsNaN([double]$_.CookedValue) -and -not [double]::IsInfinity([double]$_.CookedValue) -and $_.CookedValue -ge 0 }
if($null -eq $gpu -or $gpu.Count -eq 0){ throw "GPU Engine utilization surface unavailable" }
$nonzero=@($gpu | Where-Object {$_.CookedValue -gt 0})
$maxGpu=[double](($gpu | Measure-Object CookedValue -Maximum).Maximum)
$sumGpu=[double](($gpu | Measure-Object CookedValue -Sum).Sum)

$balancedGuid="381b4222-f694-41f0-9685-ff5bb260df2e"
$envPass=($powerGuid -eq $balancedGuid -and [int]$ps.ACLineStatus -eq 1)

$o=[ordered]@{
  schema="arcllm.anl64_crt.p3.precell_state.v0.2"
  wall_clock_utc=$now.ToString("o")
  global_cell_ordinal=$GlobalOrdinal
  block_id=$BlockId
  ordinal_position_within_block=$Position
  previous_condition=$PreviousCondition
  elapsed_seconds_since_P3_start=($now-$start).TotalSeconds
  elapsed_seconds_since_previous_cell_end=$prevElapsed
  system_CPU_utilization_percent=$cpuUtil
  available_physical_memory_bytes=$availBytes
  active_power_scheme_guid=$powerGuid
  AC_line_status=[int]$ps.ACLineStatus
  CPU_processor_performance_percent=$cpuPerf
  GPU_engine_counter_count=[int]$gpu.Count
  GPU_engine_nonzero_counter_count=[int]$nonzero.Count
  GPU_engine_max_utilization_percent=$maxGpu
  GPU_engine_sum_utilization_percent=$sumGpu
  environment_invariants_pass=$envPass
  unavailable_frozen_fields=@("CPU_temperature","GPU_temperature","GPU_frequency")
}
$o | ConvertTo-Json -Depth 5
if(-not $envPass){ exit 42 }
