# ArcLLM v1 — M3-C Independent Closeout

## Verdict

```text
STOP_M3C_UNRESOLVED_COUNTER_ADEQUACY
```

M3-C collection integrity passed, but the preregistered mechanism discriminator cannot produce a confirmatory mechanism result because the required clock/occupancy evidence is not adequate in this Vulkan performance-query configuration.

## What passed

- 6/6 quiet-host raw runs completed.
- 618/618 targeted HARDWARE_OBSERVATION records were preserved.
- 103 target dispatches per run:
  - 19 LM-head Q6;
  - 14 FFN-down Q4;
  - 14 FFN-down Q6;
  - 56 split-K Q4 controls.
- Full 469-dispatch decode graph remained executed on every required counter pass.
- Required pass counts were 3 / 3 / 2 for memory-cache / execution-occupancy / stall-cause.
- Both W-S and W-C semantic generated-hash guards passed.
- Frozen parser and frozen mechanism adjudicator executed successfully.

## Why the formal mechanism result is unresolved

The preregistered discriminator requires a target/control GPU-frequency guard. However:

- `AvgGpuCoreFrequencyMHz` = 0 for every queried dispatch;
- `GpuTime` = 0 in all three counter groups;
- `GPGPU_THREADGROUP_COUNT`, occupancy and XVE-active counters are also identically zero in the execution-occupancy group;
- the duplicated `XVE_STALL` counter is 0 in execution-occupancy but nonzero in all targeted dispatches in stall-cause.

Therefore the target/control frequency ratio is unavailable, not evidence of a physical zero-frequency condition. The frozen adjudicator correctly cannot promote a descriptive mechanism under its own guard, but its label `UNRESOLVED_CLOCK_CONFOUND` should be interpreted as counter-adequacy failure rather than a demonstrated clock confound.

## Directional observations only

These signals are preserved because they may motivate a separate future preregistration. They do not override the frozen M3-C result.

### LM-head Q6 vs FFN-down Q6

- DRAM-read amplification:
  - W-S: 4.268 vs 1.436 (2.972×);
  - W-C: 2.000 vs 1.212 (1.650×).
- LSC hit fraction:
  - W-S: 0.103 vs 0.738;
  - W-C: 0.365 vs 0.911.
- median XVE SBID stall:
  - W-S: 81.78% vs 56.50%;
  - W-C: 80.12% vs 56.13%.
- median ALU1 utilization:
  - W-S: ~1.58% vs ~13.84%;
  - W-C: ~1.15% vs ~14.04%.

### FFN-down Q4 vs split-K Q4 control

- DRAM-read amplification is workload-dependent:
  - W-S: 1.585 vs 1.043;
  - W-C: 1.044 vs 1.006.
- median XVE SBID stall is consistently separated:
  - W-S: 55.16% vs 4.52%;
  - W-C: 55.57% vs 5.39%.
- median ALU1 utilization is also consistently separated:
  - W-S: ~11.00% vs ~87.36%;
  - W-C: ~11.05% vs ~87.40%.

These observations are descriptive and non-confirmatory.

## Scientific close

M3-C is closed as unresolved. Do not:

- reinterpret zero-valued frequency/occupancy counters as physical zero;
- tune thresholds after seeing these data;
- add counters post hoc to M3-C;
- claim geometry, memory, cache or instruction mechanism as causal.

Any further work must be opened as a separate preregistered counter-validity or causal study.
