# ArcLLM v1 — M3-C Targeted Mechanism Discrimination

## Scientific question

After Token X-Ray Phase 2 localized post-I002 decode cost, which physical hardware condition best distinguishes:

1. **LM-head Q6** — primary hotspot;
2. **FFN-down Q4/Q6** — secondary hotspot;
3. **FFN gate/up Q4 split-K** — positive control?

M3-C is a descriptive hardware-counter study. It is **not** an optimization run and it cannot by itself establish causal mechanism.

## Frozen Phase-2 basis

The accepted W-S Token X-Ray trace recorded:

- 469/469 timestamped Vulkan dispatches;
- 451/451 exact semantic nodes;
- 0 unknown/unmapped dispatches;
- device span 1,240,766,914 ns;
- summed dispatch time 1,239,803,214 ns;
- unattributed time 963,700 ns.

Localized dispatch-time shares:

- LM-head: 63.91%;
- FFN-down: 16.27%;
- FFN-up: 6.01%;
- FFN-gate: 5.98%.

This is why M3-C does not profile the entire graph.

## Target census

M3-C executes the complete 469-dispatch decode graph on every required performance-query pass so model state and dependencies are preserved. Performance queries are opened only around the frozen target dispatches:

| Family | Role | Queried dispatches |
|---|---|---:|
| lm_head_q6 | primary | 19 |
| ffn_down_q4 | matched-quant secondary target | 14 |
| ffn_down_q6 | Q6 secondary target | 14 |
| split_k_q4_control | positive control | 56 |
| **Total** |  | **103** |

The exact dispatch contract is:

- gate: `12 + 16*layer`;
- up: `13 + 16*layer`;
- down: `15 + 16*layer`;
- LM-head chunks: `450..468`.

The full graph still executes 469 dispatches per pass. Only 103 dispatches receive COMMAND-scope queries and the required bottom-of-pipe attribution boundary.

## Provider and counter groups

M3-A already qualified `VK_KHR_performance_query` on the exact Intel Arc 140V. M3-C inherits the locked M3-B counter groups unchanged:

- `memory_cache`;
- `execution_occupancy`;
- `stall_cause`.

The selected counter group is replayed for the driver-required number of `counterPassIndex` submissions. Results are consumed only after all required passes complete and are labeled:

`COMBINED_AFTER_REQUIRED_PASSES`.

Counter timing is instrumented diagnostic timing and must not replace Phase-2/I003 timing.

## Collection

Six independent quiet-host processes:

```text
W-S × memory_cache
W-S × execution_occupancy
W-S × stall_cause
W-C × memory_cache
W-C × execution_occupancy
W-C × stall_cause
```

Each process uses:

- one warmup;
- one measured inference;
- decode index 15;
- full 469-dispatch decode execution;
- 103 queried dispatches;
- only the fixed 4-byte decode token restored between counter passes;
- generated-hash semantic guard;
- exact target-family census guard.

Expected output after parsing:

`6 × 103 = 618 HARDWARE_OBSERVATION records`.

## Frozen comparisons

### Q4_DOWN_VS_Q4_SPLIT_K

Primary geometry discrimination because quantization is matched.

Useful for:

- execution geometry / insufficient parallelism;
- occupancy/resource pressure;
- memory-traffic amplification;
- cache-reuse deficit;
- instruction density.

### Q6_LM_HEAD_VS_Q6_DOWN

Primary same-quantization comparison for the dominant LM-head hotspot.

Useful for:

- physical DRAM amplification normalized by semantic bytes;
- L3/LSC reuse;
- memory queue pressure;
- XVE occupancy/stall;
- instruction/dequant density.

### Q6_DOWN_VS_Q4_SPLIT_K_GENERALIZATION

Secondary generalization only. Quantization is not matched, so it cannot by itself identify a mechanism.

## Preregistered descriptive thresholds

Frozen before collection:

- high ratio: >= 1.5;
- low ratio: <= 0.67;
- percentage-point separation: >= 10 pp;
- acceptable target/control GPU-frequency ratio: 0.80–1.25.

If frequency lies outside the bound, the corresponding mechanism status becomes `UNRESOLVED_CLOCK_CONFOUND`.

Possible descriptive statuses:

- `SUPPORTED_DESCRIPTIVE`;
- `NOT_SUPPORTED_DESCRIPTIVE`;
- `UNRESOLVED_CLOCK_CONFOUND`.

None is causal proof.

## Stop rule

After the single six-process M3-C collection:

1. run the frozen parser;
2. run the frozen mechanism adjudicator;
3. do not tune thresholds;
4. do not add counters post hoc;
5. if no mechanism pattern is supported, stop `UNRESOLVED`;
6. if a pattern is supported, preregister **exactly one minimal causal intervention**.

No architecture change is permitted from counter evidence alone.
