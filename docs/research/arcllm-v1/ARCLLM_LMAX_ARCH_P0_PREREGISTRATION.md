# ARCLLM_LMAX_ARCH_P0 — Preregistration

Status: **FROZEN SPECIFICATION ONLY — NO OUTCOME-BEARING EXECUTION AUTHORIZED**

Base repository: `minhtri22/ArcLLM`  
Base commit: `c6c9b2e0a198b8af43b120e2b269e2ddd0429ef1`  
Research branch: `research/arcllm-lmax-arch-p0`  
Canonical runtime source blob at freeze: `src/arcllm_v1_runtime.cpp` = `0c613f6f740931a88ddd3ee5b904533a58001a06`  
Public runtime API blob at freeze: `include/arcllm/v1/runtime.h` = `d7821270ed253e1d1d96e3916b22b3daee50f5bc`  
Frozen model SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`

## 1. Scientific question

For the current request-scoped ArcLLM-v1 runtime on Intel Arc 140V, does an LMAX-inspired control path based on a preallocated sequenced ring and single-writer ownership preserve exact inference semantics while producing a measurable practical improvement in host-side control behavior and/or end-to-end inference metrics relative to the current direct orchestration path?

This P0 study is deliberately narrow. It tests the **control/scheduling architecture**, not new Transformer math, kernels, quantization, model residency policy, sampling, or a new performance claim against llama.cpp.

## 2. Governance and independence

This study is scientifically independent from the active `research/current-main-baseline-xray` line.

- It branches from canonical `main`, not from the Token-XRay/CORE0 line.
- No CORE0A-E result, trace, threshold, adapter, or outcome may be imported into the P0 result.
- The active CORE0 line remains unchanged.
- Target-machine execution must be serialized with other ArcLLM science so competing jobs cannot contaminate timing.
- `lineage.md` is not modified at preregistration or implementation time. It is append-only only after valid independent P0 adjudication.
- Infrastructure/build failures are not scientific FAIL results.

## 3. Frozen arms

### Arm A — CURRENT_DIRECT

The canonical ArcLLM request executes through the current direct orchestration path.

The following semantics are frozen:
- same GGUF bytes and model SHA;
- same shader set and Vulkan graph;
- same policy/binding v4 behavior;
- same evidence profile and evidence-boundary flag;
- same greedy top-1 sampling;
- same prefill/decode dispatch topology;
- same request-scoped residency lifetime;
- same input token IDs and max_new_tokens.

Passive measurement hooks may observe timestamps, CPU time and allocation counters but may not alter control decisions.

### Arm B — LMAX_RING

The same request is admitted through one experimental LMAX-inspired dispatcher with:
- a fixed-size power-of-two ring;
- preallocated event slots;
- monotonically increasing 64-bit sequence numbers;
- single-writer ownership of dispatcher state;
- no heap allocation in publish/consume operations after dispatcher initialization;
- bounded wait strategy frozen as spin then yield;
- exactly one runtime consumer;
- no speculative decode and no overlap that changes token dependency order.

The ring may schedule a request, but it may not change the canonical `generate(RunRequest)` inference semantics or any Vulkan/model decision.

P0 does **not** authorize internal kernel, shader, quantization, policy, residency, or sampling changes.

## 4. Instrumentation contract

Both arms must use the same passive instrumentation build.

Per request record:
- request start timestamp;
- first-token-ready timestamp;
- every subsequent token-ready timestamp;
- request completion timestamp;
- process CPU time at start/end;
- request-wide C++ allocation count;
- decode-window C++ allocation count;
- generated token IDs;
- canonical `RuntimeStats`;
- error state.

Definitions:
- **TTFT** = first-token-ready - request start.
- **Inter-token latency (ITL)** = consecutive token-ready timestamp differences after the first token.
- **Decode throughput** = number of post-first generated tokens divided by elapsed time from first-token-ready to final-token-ready.
- **CPU utilization** = process CPU time / (wall time × logical processor count) × 100.
- **Allocation count** = successful instrumented C++ dynamic allocation calls in the declared measurement window. It is not claimed to include driver/internal allocations.
- All clocks must use one monotonic high-resolution source.

Arm B additionally records:
- ring capacity;
- publish count;
- consume count;
- maximum observed occupancy;
- full-ring observations;
- spin iterations;
- yield count;
- sequence-gap count.

Arm A records the same schema with ring fields marked `not_applicable`; they must not be fabricated as zero-benefit evidence.

## 5. Workload freeze

All P0 inference requests use:
- `EvidenceProfile::PROFILE_0`;
- `request_within_validated_domain=false`;
- exact frozen model;
- canonical shaders from the frozen base;
- raw caller-provided token IDs;
- greedy generation.

Six workload cells:

| Cell | Prefill tokens | max_new_tokens |
|---|---:|---:|
| W1 | 8 | 8 |
| W2 | 8 | 32 |
| W3 | 64 | 8 |
| W4 | 64 | 32 |
| W5 | 256 | 8 |
| W6 | 256 | 32 |

Input IDs are deterministic and frozen by:
`token[i] = 1 + ((7919*i + 104729*cell_index) mod 152063)`, with `cell_index` = 1..6.

Execution count:
- 2 excluded warmups per arm before measured collection;
- 4 matched measured pairs per workload cell;
- total = 24 measured A/B pairs = 48 measured inference runs.

Pair order is frozen:
- pair 0 and 2: A then B;
- pair 1 and 3: B then A.

No selective rerun, outlier deletion, threshold change, workload substitution, or post-hoc tuning is allowed.

## 6. Control-path backpressure subtest

Because the canonical runtime has no existing request queue, queue/backpressure is evaluated separately from model execution in a zero-model control-plane subtest.

The same event payload and dispatcher implementation used by Arm B are exercised against a direct-reference dispatcher using no-op work items.

Frozen conditions:
- ring capacity = 64;
- 1 producer, 1 consumer;
- 1,000,000 events;
- three consumer service delays: 0 ns, 10 us, 100 us;
- exact event order must be preserved;
- no model load, Vulkan initialization, shader creation, or inference.

Recorded fields:
- events/s;
- p50/p95 publish-to-consume latency;
- maximum queue occupancy;
- full/backpressure observations;
- spin/yield counts;
- allocation count inside steady-state event processing.

This subtest may establish a control-path mechanism result only. It cannot by itself establish an ArcLLM inference speedup.

## 7. Semantic gate

P0 is scientifically invalid for performance interpretation if any matched A/B inference pair violates any item below:
1. generated token IDs differ at any position;
2. generated token count differs;
3. `finite` differs or is false;
4. prefill dispatch/submission counts differ;
5. decode dispatch/submission topology differs;
6. route/lifecycle counters differ under the same request;
7. runtime error occurs in only one arm.

Any semantic mismatch yields `SEMANTICS_FAIL`. Performance numbers remain diagnostic only.

## 8. Frozen adjudication categories

After semantic PASS, classify without rescue:

- **E2E_SUPPORTED**: at least one end-to-end latency/throughput endpoint improves by >=5% at both p50 and p95 or median-equivalent where applicable, no TTFT/ITL p95 regresses by >3%, decode tok/s does not regress by >3%, and CPU utilization does not regress by >5% relative.
- **CONTROL_PATH_ONLY**: no E2E_SUPPORTED result, but the zero-model control-path subtest shows >=20% allocation reduction or >=10% p95 event-latency improvement at at least two of three service delays, with exact ordering and no worse full/backpressure rate.
- **NO_SUPPORTED_BENEFIT**: semantics PASS but neither E2E_SUPPORTED nor CONTROL_PATH_ONLY is reached.
- **REGRESSION**: semantics PASS but TTFT or ITL p95 regresses by >10% or decode tok/s regresses by >10% in at least four of six workload cells.
- **UNRESOLVED**: evidence is valid but machine instability or mutually conflicting endpoints prevent the frozen rules from assigning one of the categories above.

These are P0 bounded categories, not general claims about LMAX or future persistent-session ArcLLM designs.

## 9. Resource and contamination gate

Outcome-bearing inference execution is allowed only when:
- exact Intel Arc 140V target is available;
- exact model SHA is verified;
- no other ArcLLM scientific GPU job is running;
- target power state and process priority are recorded;
- all 48 measured runs can be completed under one frozen implementation and evidence schema.

If this gate cannot be met, execution must stop before outcome collection. Do not substitute another GPU or cloud machine.

## 10. Repository discipline

Allowed P0 locations only:
- `docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P0_*.md`
- `config/arcllm_lmax_arch_p0_*.json`
- `experiments/arcllm_lmax_arch_p0/`
- `results/arcllm_lmax_arch_p0_*/`

Do not place P0 code in unrelated directories. Do not modify canonical shaders. Do not modify `lineage.md` before adjudication.

## 11. Lifecycle lock

Preregistration is now complete only when the Markdown contract and machine-readable JSON contract are committed on the research branch.

The next permitted stage is:

`ARCLLM_LMAX_ARCH_P0_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK`

That stage may implement the two arms and measurement harness, run only zero-science/static/build checks, verify exact semantic call structure, and freeze executable/config hashes.

It may **not** run the 48 measured inference executions or inspect outcome-bearing P0 performance results.
