# Token-XRay ArcLLM Adapter Upgrade Handoff

Date: 2026-09-29

Owner of this handoff: ArcLLM CORE-0A  
Downstream implementation owner: separate Token-XRay agent

## Do not change ArcLLM

This handoff does **not** authorize changes to ArcLLM runtime behavior, kernels, workloads, model, thresholds, or performance policy.

The Token-XRay agent should work from:

`minhtri22/token-xray`  
branch: `research/core-v0.1-completeness`  
frozen starting commit: `17e786285d202f79e6856584961f1953fd0bc8af`

ArcLLM compatibility target:

`minhtri22/ArcLLM`  
canonical parent: `7a5672112dc22de15f0e9bb6445508fbb3099b15`

CORE-0A evidence:

`results/CORE0A_TOKEN_XRAY_COMPATIBILITY.json`

Current verdict:

`STOP_BEFORE_INSTRUMENTED_RUN_TOKEN_XRAY_ADAPTER_DRIFT`

Current open findings: **4**

No fresh performance inference was executed and no ArcLLM scientific mechanism was selected.

---

## Baseline Token-XRay state that already passes

On `research/core-v0.1-completeness@17e786285d202f79e6856584961f1953fd0bc8af`:

- `python -m compileall -q src`: PASS
- pytest: **36 passed, 1 skipped, 0 failed**
- CPU/GPU/NPU execution-domain completeness work is present.
- Existing decode semantic suffix mapping remains complete for the original ArcLLM decode names.

Do not regress this baseline.

---

# Required upgrade list

## P0 — Decode shader drift: add current Q4V4 route-B FFN-down

### Finding

Current ArcLLM canonical decode has 469 physical dispatches.

Pinned Token-XRay adapters can map geometry for **455/469**.

The missing 14 dispatches are the 14 Q4_K FFN-down layers using:

`q4_down_exec148_serial.spv`

### Required changes

Update both:

- `src/token_xray/adapters/arcllm_v1.py`
- `sdk/vulkan/token_xray_arcllm_v1_adapter.h`

Add shader geometry:

```text
q4_down_exec148_serial.spv
local_size = [64, 1, 1]
subgroup_size = null / 0 (no explicit fixed subgroup contract)
```

The runtime name remains `Lxx.ffn_down`; it must map to the existing FFN-down semantic node, not create a new model semantic operation merely because the execution representation changed.

### Acceptance

- Decode geometry coverage = **469/469**
- Decode semantic-node support = **451/451**
- Unknown current decode shader count = 0

---

## P0 — Prefill coverage for current canonical runtime

### Finding

Current ArcLLM prefill topology is **441 dispatches**.

Pinned Token-XRay adapter geometry coverage is only **245/441**.

Missing current prefill shader mappings:

| Shader | Dispatches | Local size |
|---|---:|---|
| `p7c_ffn_q4k_tiled.spv` | 98 | `[8,8,1]` |
| `p7c_ffn_q6k_tiled.spv` | 14 | `[8,8,1]` |
| `p7_attention_prefill_online.spv` | 28 | `[128,1,1]` |
| `p7l_ffn_q4k_gateup_fused.spv` | 28 | `[8,8,1]` |
| `p7g_ffn_q4k_tiled16.spv` | 14 | `[8,8,1]` |
| `p7g_ffn_q6k_tiled16.spv` | 14 | `[8,8,1]` |

Total unmapped: **196/441**.

### Semantic/API gaps

The current adapter API is decode-centric and cannot cleanly express the current prefill path.

Required support:

- runtime name `causal_gqa`;
- runtime name `ffn_gate_up_fused`;
- phase/mode-aware mapping;
- one physical fused dispatch mapping to multiple semantic nodes.

Token Trace already has a `token.mode` enum containing `prefill`, and dispatch records already carry `semantic_node_ids[]`. Reuse those capabilities rather than inventing a second incompatible trace format.

A recommended adapter surface is conceptually:

```text
semantic_node_ids(runtime_name, mode) -> list[str]
```

Exact API spelling is owned by Token-XRay, but the following must hold:

- `causal_gqa` maps to the attention-core semantic work for prefill;
- `ffn_gate_up_fused` maps to both FFN gate and FFN up semantic nodes;
- prefill execution identity must not be silently mislabeled as decode execution;
- existing decode behavior must remain backward-compatible.

Do not create fake per-node timing for two semantic nodes sharing one fused dispatch. Preserve shared-dispatch attribution.

### Acceptance

- Prefill geometry coverage = **441/441**
- `causal_gqa` supported
- `ffn_gate_up_fused` supported
- fused dispatch can carry two semantic node IDs
- prefill/decode execution phase remains explicit

---

## P0 — Add Q4V4 runtime lifecycle evidence surface

### Finding

Current ArcLLM no longer consists only of model-semantic dispatches.

Q4V4 has request-scoped representation lifecycle:

```text
policy select
  -> acquire
  -> GPU materialize
  -> validate
  -> resident/reuse
  -> evict/release
```

The P1 GPU materializer executes 14 dispatches per acquisition:

`b1_2_exec148_gpu_materialize.comp.spv`

Exact local size:

`[256,1,1]`

Runtime operation names are shaped as:

`Q4V4.P1.L<layer>`

These are **not model semantic FFN-down nodes** and must not be forced through `semantic_node_id()`.

### Required representation

Token-XRay needs an explicit request/runtime lifecycle evidence surface.

The implementation may be:

- a separate request-level `RUNTIME_LIFECYCLE_TRACE`; or
- an explicit lifecycle-event section in a broader request trace.

The exact format belongs to Token-XRay, but it must preserve at least:

- event identity;
- event type;
- request scope;
- execution domain;
- representation/resource identity;
- related dispatch IDs when device work exists;
- timestamp/provenance when measured;
- relation to subsequent semantic execution without attributing lifecycle cost to a model node.

Required event concepts for current ArcLLM:

- `representation_acquire`
- `representation_materialize`
- `representation_validate`
- `representation_resident` / reuse state
- `representation_evict` / release

P1 materialization executes on `gpu.arc_140v`.

Validation or eviction may have host-side work/no standalone GPU dispatch and must still be representable without inventing a device dispatch.

### Acceptance

- `b1_2_exec148_gpu_materialize.comp.spv` geometry is known: `[256,1,1]`
- 14 P1 dispatches can be attributed to one representation-acquisition lifecycle, not FFN semantic compute
- Q4V4 lifecycle names do not fail the adapter
- lifecycle and model-semantic execution remain distinguishable

---

## P1 — Observer-effect and timing-authority contract

### Finding

The current `TOKEN_TRACE` schema does not explicitly contain:

- `measurement_mode`
- `timing_authority`
- `instrumentation_overhead`

CORE-0 requires this distinction because ArcLLM-vs-llama performance claims must use **uninstrumented matched runs** as timing authority.

Token-XRay runs are diagnostic/localization evidence unless a separate benchmark contract says otherwise.

### Required contract

Add an explicit measurement context with at least the concepts:

```text
measurement_mode
timing_authority
instrumentation_overhead
```

Recommended measurement modes:

- `UNINSTRUMENTED`
- `TIMESTAMP_ONLY`
- `TOKEN_XRAY_TRACE`
- `HARDWARE_COUNTERS`

Recommended timing-authority classes:

- `BENCHMARK_AUTHORITY`
- `DIAGNOSTIC_ONLY`

`instrumentation_overhead` may be null until paired observer-effect evidence exists; null must not be converted to zero.

The later ArcLLM observer-effect study will compare:

```text
uninstrumented
vs timestamp-only
vs Token-XRay trace
vs hardware-counter instrumentation
```

### Acceptance

The schema and documentation must make it impossible to silently present instrumented counter timing as the matched performance authority.

---

# Required tests to add in Token-XRay

At minimum add/extend tests for:

1. Python adapter geometry:
   - `q4_down_exec148_serial.spv -> [64,1,1]`
   - `b1_2_exec148_gpu_materialize.comp.spv -> [256,1,1]`
   - all six missing prefill shaders with exact local sizes above.

2. C++ adapter geometry with the same values.

3. Decode semantic compatibility:
   - existing `Lxx.ffn_down` identity remains unchanged when Q4V4 execution shader changes.

4. Prefill semantic compatibility:
   - `causal_gqa`;
   - `ffn_gate_up_fused`;
   - multi-semantic fused dispatch;
   - explicit prefill mode/phase.

5. Lifecycle:
   - `Q4V4.P1.L03`-style event is accepted by the lifecycle adapter/path;
   - it is rejected from false model-semantic attribution if passed to the wrong API.

6. Schema:
   - lifecycle evidence validates;
   - observer measurement context validates;
   - absent overhead remains null/unknown, not zero;
   - execution-domain identity is retained.

7. Existing Token-XRay test suite remains green.

---

# Return package required from the Token-XRay agent

Return:

- exact Token-XRay branch + HEAD;
- changed-file list;
- `python -m compileall -q src` result;
- complete pytest result;
- C++ SDK syntax smoke result;
- adapter/schema change summary;
- any new/changed schema IDs or versions;
- claim-boundary note;
- no ArcLLM runtime modifications.

Then ArcLLM CORE-0A will rerun:

```powershell
py -3 tools/audit_core0a_token_xray_compatibility.py \
  --token-xray-root <upgraded-token-xray-checkout> \
  --out results/CORE0A_TOKEN_XRAY_COMPATIBILITY.json
```

Required revalidation target:

```text
CORE0A_VERDICT = PASS_CORE0A_TOKEN_XRAY_COMPATIBLE
OPEN_FINDINGS = 0
DECODE_GEOMETRY = 469/469
PREFILL_GEOMETRY = 441/441
```

Only after this PASS may ArcLLM proceed to instrumented CORE-0C collection.

The matched ArcLLM-vs-llama performance authority remains the separate uninstrumented CORE-0B path.

---

# Explicit non-goals

Do not:

- add NPU execution to ArcLLM;
- change ArcLLM kernels;
- optimize any ArcLLM operator;
- change W-S/W-C;
- change the model;
- change llama.cpp baseline;
- use Token-XRay timing to claim ArcLLM speedup;
- reinterpret prior M3-C zero/inconsistent counters as physical zero;
- promote descriptive counter patterns to causal claims.
