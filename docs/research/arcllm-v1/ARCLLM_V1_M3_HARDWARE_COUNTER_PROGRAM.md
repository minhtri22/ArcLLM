# ArcLLM v1 — M3 Hardware-Counter Program

## Reframe

M3 is no longer optional only for choosing the next ArcLLM kernel. It is also the first empirical
hardware-counter foundation for Token-XRay HardwareLens.

Therefore M3 is split into two stages.

### M3-A — provider capability qualification

Zero science:
- no model load;
- no inference;
- no performance-counter collection;
- no quiet-host requirement.

It enumerates the exact Arc 140V Vulkan compute queue, checks `VK_KHR_performance_query`,
enumerates native counters and their unit/scope/storage/flags, computes pass requirements, and
detects whether Intel VTune CLI is installed.

Output follows Token-XRay `COUNTER_CAPABILITY` v0.1 from commit
`ba19a34c54c0290cbed2f54f6a1175554bcfae16`.

### M3-B — targeted quiet-host collection

Only after M3-A provider adjudication.

Counter families of interest:
- physical/system-memory read/write traffic;
- L3/LLC traffic/misses where exposed;
- XVE/EU active, stalled, idle;
- occupancy;
- memory/send stalls;
- relevant instruction/SIMD utilization if exposed.

Collection scopes must be explicit:
`DISPATCH / SEMANTIC_NODE / TOKEN / RUN`.

M3-B will use quiet-host mode because counter accuracy is now the object of the experiment. The
machine profile itself is not re-audited.

## Provider order

1. Vulkan KHR performance query if the actual driver exposes meaningful counters.
2. Intel VTune vendor adapter if KHR is absent/insufficient.
3. Do not build the foundation on Intel GPA because GPA 2025.1 is EOL/discontinued in 2026.

## Multi-pass

If Vulkan reports multiple passes, preserve each pass as separate raw evidence. Do not pretend
multi-pass samples are one simultaneous counter state.

## Relationship to the excess-cost map

M3 does not erase the existing causal hypothesis. It enriches it:

```text
row-serial K hypothesis
   ↓
M3 physical traffic/cache/occupancy/stall evidence
   ↓
Q4-down causal sentinel
```

The sentinel remains the intervention test; M3 explains the hardware state leading into it.
