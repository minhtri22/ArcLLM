# CORE-0B — Current-main Uninstrumented Matched Request Baseline Prelock

Date: 2026-09-29  
Parent: `PASS_CORE0A_TOKEN_XRAY_COMPATIBLE`  
Branch: `research/current-main-baseline-xray`

## Scientific question

For the exact canonical ArcLLM `main` product runtime, what is the fresh matched **request-level** performance gap versus exact pinned llama.cpp v0.4.1 on the same dev-host, exact model and exact W-S/W-C workloads, when neither arm carries Token-XRay or hardware-counter instrumentation?

CORE-0B characterizes the current gap. It does not select a kernel or mechanism.

## Why this contract differs from historical I003

Historical I003 measured inference endpoints after model/context setup inside a benchmark process.

The current canonical ArcLLM product API explicitly has:

```text
request_scoped_runtime_lifetime = true
persistent_model_session = false
```

Therefore ArcLLM cannot currently expose a product-authentic persistent-session inference-only endpoint without adding a capability that is absent from canonical `main`.

CORE-0B must not fabricate such a capability or silently benchmark the historical integration harness as though it were the current product API.

The primary matched endpoint is consequently a **cold request-level child-process wall time** where both systems load/setup and execute exactly one request per child process.

This measures the current product/runtime gap, including lifecycle maturity debt.

## Frozen systems

### ArcLLM

Canonical product parent:

`7a5672112dc22de15f0e9bb6445508fbb3099b15`

Required active runtime blobs remain those named by:

`config/arcllm_v1_runtime_active_v0.2.json`

The research branch may add benchmark-only runner/adapters/artifacts, but must not alter canonical runtime source, shader source, policy, binding, model logic or product API behavior.

### llama.cpp

Repository: `ggml-org/llama.cpp`  
Release: `v0.4.1`  
Commit:

`b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`

Backend: Vulkan  
Context: 4096  
Batch: 256  
Ubatch: 256  
Threads: 8  
Threads-batch: 8  
KV K/V: F32  
GPU offload: all layers requested  
Raw token IDs  
Greedy argmax  
No speculative decoding  
No EOS early stop

The baseline adapter may be minimally adapted for one cold request per child process. It may not alter llama.cpp itself or the frozen model/inference settings.

## Model

Exact GGUF for both arms:

```text
SHA256 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
bytes  4,683,074,048
```

No conversion or requantization.

## Workloads

### W-S

```text
[1,133151,133152,152062]
```

ArcLLM:
- PROFILE_0
- `request_within_validated_domain=true`

### W-C

256 tokens.

Positions 0..3 equal W-S. For `i=4..255`:

```text
token[i] = 1 + ((104729 + 7919*i) mod 152063)
```

ArcLLM:
- PROFILE_1
- `request_within_validated_domain=true`

Both arms generate exactly 32 tokens.

## Measurement authority

### Primary authority

`UNINSTRUMENTED_CHILD_WALL_MS`

Measured externally from immediately before process creation until the child exits after writing its result artifact.

No Token-XRay trace, Vulkan timestamp instrumentation, hardware counters, GPU sampler, Python resource sampler, or profiler may run during primary timing.

This is the only CORE-0B performance authority.

### Secondary validity evidence

For each request:
- process exit code;
- exactly 32 generated tokens;
- finite-result invariant where exposed;
- generated token hash;
- ArcLLM topology/lifecycle stats;
- llama full-Vulkan-offload qualification from a separate zero-science preflight.

### Resource evidence

Peak working set/private bytes, if collected, must be collected in a **separate resource-only pass** and may not be mixed into primary timing pairs.

Resource-only runs are descriptive and cannot replace timing authority.

## What CORE-0B does not claim

Because canonical ArcLLM lacks a persistent session, CORE-0B does not claim:
- matched warm persistent-session TTFT;
- matched steady-state decode ms/token;
- matched steady-state tokens/s;
- that request-level gap equals kernel-compute gap.

Historical I003 warm inference evidence may be shown only as historical context and must not be algebraically mixed into current CORE-0B results.

Current internal localization is deferred to CORE-0C Token-XRay diagnostic tracing after CORE-0B closes.

## Paired design

Two workload cells:

- W-S
- W-C

Two independent sessions, A and B.

Five adjacent pairs per workload per session.

Order:
- Session A pair 0 starts ArcLLM, then alternates;
- Session B reverses parity.

Total primary requests:

```text
2 workloads × 2 sessions × 5 pairs × 2 arms = 40
```

No selective pair/request rerun.

If a genuine operational failure prevents a complete valid collection, a later **whole collection** may be run under the identical frozen contract. The first complete valid collection remains primary.

## Primary statistic

For every valid matched pair:

```text
request_wall_ratio = ArcLLM request wall ms / llama request wall ms
```

Report per workload/session:
- median
- min
- max
- MAD
- n

Report global geometric mean across the four cell medians.

This is a descriptive current product-gap quantity, not a winner score.

## Required ArcLLM invariants

Every ArcLLM request must retain:

- prefill dispatches = 441
- prefill submits = 1
- decode dispatches/step = 469
- decode submits/step = 1
- decode steps = 31
- route A steps = 0
- route B steps = 31
- acquire events = 1
- evict events = 1
- B allocation/materialization/validation/release = 1/1/1/1
- P1 calls = 1
- P3/P0 calls = 0/0
- finite = true

Any drift invalidates that request as a current-canonical ArcLLM measurement.

## Baseline qualification

Before fresh primary timing:

- exact llama commit must build;
- exact cold-request adapter source/blob must be frozen;
- Vulkan backend must be present;
- model/layer/vocab/context settings must match;
- full GPU offload must be demonstrated without consuming a primary measured request;
- exact executable hashes must be frozen.

## Zero-science preflight gate

Before any of the 40 primary requests:

1. static contract QA;
2. canonical ArcLLM runtime build from frozen parent;
3. llama exact-commit build;
4. ArcLLM one-request functional qualification without primary timing retention;
5. llama one-request functional/offload qualification without primary timing retention;
6. exact executable/model/shader hashes freeze;
7. confirm no Token-XRay/counter/profiler instrumentation in primary runner;
8. independent preflight adjudication.

Only an explicit CORE-0B science authorization artifact may unlock the primary collection.

## Valid final classes

Exactly one:

### CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE
Require:
- exact frozen systems/model/workloads;
- all 20 pairs represented;
- at least 3 valid pairs in every workload/session cell;
- primary timing uninstrumented;
- complete provenance.

### CORE0B_BASELINE_NOT_MATCHED
Pinned llama.cpp cannot execute the frozen matched request contract.

### CORE0B_RUNTIME_INCOMPLETE
Fewer than 3 valid matched pairs in a cell because of genuine runtime failure.

### CORE0B_MEASUREMENT_INVALID
Timing/provenance does not support the matched request comparison.

## Decision boundary

CORE-0B selects no optimization mechanism.

After a valid CORE-0B result:

1. CORE-0C runs Token-XRay diagnostic trace on current ArcLLM.
2. CORE-0C measures observer effect separately and never replaces CORE-0B timing.
3. CORE-0D joins current request gap + Token-XRay localization + reference-runtime semantic evidence into a fresh excess-cost map.
4. Only then may CORE-1 apply an Amdahl gate and select one core intervention.

## Current authorization

Authorized now:
- benchmark-only adapter/runner implementation;
- exact baseline build;
- zero-science static/build/functional preflight;
- independent preflight adjudication.

Not authorized yet:
- the 40-request primary performance collection;
- Token-XRay instrumented collection;
- mechanism selection.
