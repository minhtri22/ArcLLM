param([string]$ModelPath,[string]$OllamaModelsRoot)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Results=Join-Path $Here "results"
New-Item -ItemType Directory -Force -Path $Results|Out-Null
$Cfg=Get-Content (Join-Path $Here "config\p8_target.json") -Raw -Encoding UTF8|ConvertFrom-Json
$ExpectedSize=[int64]$Cfg.size_bytes
$ExpectedHash=([string]$Cfg.sha256).ToUpperInvariant()
$Head=(& git -C $Here rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0 -or -not $Head){throw "Q1 cannot resolve implementation commit"}
$Critical=@("src/q1_end_to_end.cpp","shaders/p8q1_lmhead_q6k_segmented_chunk.comp","tools/compile_q1_shaders.ps1","tools/build_q1.ps1","run_q1.ps1","tests/test_q1_package.py","docs/P8_Q1_END_TO_END_CONTRACT.md","docs/P8_Q1_IMPLEMENTATION.md","manifest.json")
& git -C $Here diff --quiet HEAD -- @Critical
if($LASTEXITCODE -ne 0){throw "Q1 critical working-tree files differ from HEAD"}
& git -C $Here diff --cached --quiet HEAD -- @Critical
if($LASTEXITCODE -ne 0){throw "Q1 critical index files differ from HEAD"}
if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "P8 target resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$f=Get-Item $ModelPath
if($f.Length -ne $ExpectedSize){throw "Q1 target size mismatch"}
$Hash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($Hash -ne $ExpectedHash){throw "Q1 target SHA mismatch"}
$ParentShader=Join-Path $Here "inputs\p8g6_shader_provenance.authoritative.raw"
$ParentResult=Join-Path $Here "inputs\p8g6_fresh_production_semantic_results.authoritative.raw"
$ParentSummary=Join-Path $Here "inputs\p8g6_summary.authoritative.raw"
if((Get-FileHash $ParentShader -Algorithm SHA256).Hash.ToUpperInvariant() -ne "1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84"){throw "Q1 parent shader SHA mismatch"}
if((Get-FileHash $ParentResult -Algorithm SHA256).Hash.ToUpperInvariant() -ne "0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259"){throw "Q1 parent result SHA mismatch"}
if((Get-FileHash $ParentSummary -Algorithm SHA256).Hash.ToUpperInvariant() -ne "13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303"){throw "Q1 parent summary SHA mismatch"}
py -3 (Join-Path $Here "tests\test_q1_package.py")
if($LASTEXITCODE -ne 0){throw "Q1 static contract failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_q1_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "Q1 shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_q1.ps1")
if($LASTEXITCODE -ne 0){throw "Q1 native build failed"}
$OS=Get-CimInstance Win32_OperatingSystem
$CPU=Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
$Vulkan=Get-Command vulkaninfo.exe -ErrorAction SilentlyContinue
if($Vulkan){try{$VulkanSummary=((& $Vulkan.Source --summary 2>&1)|Out-String)}catch{$VulkanSummary="vulkaninfo failed: $($_.Exception.Message)"}}else{$VulkanSummary="vulkaninfo.exe unavailable; Vulkan validity is independently exercised by Q1 runtime"}
$Dirty=((& git -C $Here status --porcelain 2>$null)|Out-String)
$Env=[ordered]@{schema="arcllm.q1.environment.v1";captured_utc=(Get-Date).ToUniversalTime().ToString("o");implementation_commit=$Head;os=[ordered]@{caption=$OS.Caption;version=$OS.Version;build_number=$OS.BuildNumber;architecture=$OS.OSArchitecture};cpu=@($CPU);gpu=@($GPU);vulkan_summary=$VulkanSummary;git_status_porcelain=$Dirty;target_path=$ModelPath;target_sha256=$Hash;target_size_bytes=$f.Length}
$EnvPath=Join-Path $Results "q1_environment.json"
[IO.File]::WriteAllText($EnvPath,($Env|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
$Out=Join-Path $Results "q1_end_to_end_results.json"
$Log=Join-Path $Results "q1_console.log"
& (Join-Path $Here "arcllm_q1.exe") --model $ModelPath --shader-dir (Join-Path $Here "compiled_shaders") --parent-result $ParentResult --parent-shader $ParentShader --parent-summary $ParentSummary --implementation-commit $Head --out $Out 2>&1 | Tee-Object -FilePath $Log
$Code=$LASTEXITCODE
if(-not(Test-Path $Out)){throw "Q1 result JSON missing"}
$Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
$F0A=$false;$F1A=$false;$F0B=$false;$F1B=$false;$F2=$false;$ExecPass=$false
if($Obj.status -eq "COMPLETE" -and $Obj.execution_gate){
  $F0A=[bool]$Obj.execution_gate.F0_A;$F1A=[bool]$Obj.execution_gate.F1_A;$F0B=[bool]$Obj.execution_gate.F0_B;$F1B=[bool]$Obj.execution_gate.F1_B;$F2=[bool]$Obj.execution_gate.F2;$ExecPass=[bool]$Obj.execution_gate.execution_pass
}
$ShaderProv=Join-Path $Results "q1_shader_provenance.json"
if(-not(Test-Path $ShaderProv)){throw "Q1 shader provenance missing"}
$Contract=Join-Path $Here "docs\P8_Q1_END_TO_END_CONTRACT.md"
$SummaryPath=Join-Path $Results "q1_summary.json"
$ManifestPath=Join-Path $Results "q1_evidence_manifest.json"
$Classification="Q1_INVALID"
if($Obj.status -eq "COMPLETE"){
  if($F0A -and $F1A -and $F0B -and $F1B -and -not $F2){$Classification="Q1_REPRODUCIBILITY_FAILURE"}
  elseif($ExecPass -and $F2){$Classification="Q1_FEASIBILITY_ESTABLISHED"}
}
$Summary=[ordered]@{schema="arcllm.q1.summary.v1";target_sha256=$Hash;target_size_bytes=$f.Length;implementation_commit=$Head;process_exit_code=$Code;status=$Obj.status;F0_A=$F0A;F1_A=$F1A;F0_B=$F0B;F1_B=$F1B;F2_exact_repeat=$F2;F3_evidence_complete=$false;classification="Q1_INVALID";performance_gate_defined=$false;q2_started=$false;q3_started=$false;next_step="complete Q1 evidence package before adjudication"}
[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$RequiredBeforeManifest=@($Out,$ShaderProv,$EnvPath,$Log,$Contract,$SummaryPath)
foreach($Path in $RequiredBeforeManifest){if(-not(Test-Path $Path)){throw "Q1 F3 missing evidence: $Path"}}
function ArtifactRow([string]$Path,[string]$Role){return [ordered]@{role=$Role;path=(Resolve-Path $Path).Path;bytes=(Get-Item $Path).Length;sha256=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()}}
$Evidence=[ordered]@{schema="arcllm.q1.evidence_manifest.v1";created_utc=(Get-Date).ToUniversalTime().ToString("o");implementation_commit=$Head;target_sha256=$Hash;artifacts=@((ArtifactRow $Out "raw_execution_result"),(ArtifactRow $ShaderProv "shader_provenance"),(ArtifactRow $EnvPath "environment"),(ArtifactRow $Log "console_log"),(ArtifactRow $Contract "frozen_contract"),(ArtifactRow $ParentResult "authoritative_parent_result"),(ArtifactRow $ParentShader "authoritative_parent_shader"),(ArtifactRow $ParentSummary "authoritative_parent_summary"));f3_complete=$true}
[IO.File]::WriteAllText($ManifestPath,($Evidence|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
$Summary.F3_evidence_complete=$true
$Summary.classification=$Classification
$Summary.next_step=if($Classification -eq "Q1_FEASIBILITY_ESTABLISHED"){"freeze Q1 evidence and design Q2 matched performance/resource benchmark"}elseif($Classification -eq "Q1_REPRODUCIBILITY_FAILURE"){"adjudicate Q1 determinism blocker"}else{"adjudicate Q1 execution/package evidence before any repair"}
[IO.File]::WriteAllText($SummaryPath,($Summary|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
$Evidence.artifacts += (ArtifactRow $SummaryPath "final_summary")
[IO.File]::WriteAllText($ManifestPath,($Evidence|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
$Bundle=Join-Path $Results "q1_return_to_chatgpt.zip"
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
Compress-Archive -Path @($ShaderProv,$Out,$EnvPath,$Log,$SummaryPath,$ManifestPath) -DestinationPath $Bundle -Force
Write-Host ""
Write-Host "Q1 evidence:"
Write-Host "  results\q1_shader_provenance.json"
Write-Host "  results\q1_end_to_end_results.json"
Write-Host "  results\q1_environment.json"
Write-Host "  results\q1_console.log"
Write-Host "  results\q1_summary.json"
Write-Host "  results\q1_evidence_manifest.json"
Write-Host "  results\q1_return_to_chatgpt.zip"
Write-Host "Q1 classification=$Classification"
exit $Code
