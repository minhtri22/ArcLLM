# ARCLLM_LMAX_ARCH_P1 — Preregistration

Status: **FROZEN P SPECIFICATION — NO FRESH P1 OUTCOME AUTHORIZED**

Repository: `minhtri22/ArcLLM`  
Successor branch: `research/arcllm-lmax-arch-p1`  
Parent P0 terminal commit: `2cc2d788aa2782cf9fd6b723f9daa183a476d755`  
P0 terminal verdict: `UNRESOLVED`  
Canonical P0 raw evidence SHA-256: `ED3ED2659A9AD75A58DB2D4F7DAE537F7AFF5E7BC364A5A7A01F0536B01941F4`

## 1. Governance

P1 follows the user-facing scientific lifecycle:

`P -> R(internal machinery) -> E -> A -> M(if opened) -> C(if opened)`.

- **P** freezes the scientific question, arms, evidence schema, fresh execution contract, thresholds, and anti-rescue rules.
- **R** is internal machinery, not a scientific gate. Implementation, static preflight, dependencies, resource checks, hash binding, execution locking, transport/MCP recovery and zero-science fixtures may be handled autonomously.
- **R may not change this scientific contract.** Any change to the scientific question, arms, workloads, thresholds, outcome schema, or adjudication logic requires a new P revision before fresh outcomes.
- The agent stops user-facing only when a scientific contract must change, immediately before opening fresh outcome-bearing E, or at PASS/FAIL/UNRESOLVED.
- **E** is exactly one fresh exploratory campaign.
- **A** independently recomputes from frozen evidence and adjudicates automatically.
- **M** is opened only by the unlock rule below.
- **C** is opened only after a separately preregistered M result explicitly unlocks confirmation.

P1 is an **outcome-informed exploratory successor** to P0, not a confirmatory rerun of P0. P0 outcomes may motivate the successor architecture but are not pooled with P1 and do not alter P1 thresholds after E begins.

## 2. P0 findings that motivate P1, without importing them as P1 evidence

P0 established:
- semantic equivalence for 24/24 matched pairs;
- diagnostic latency/throughput improvements in some workload cells;
- large CPU-utilization overhead in the ring arm;
- formal `UNRESOLVED` because required timing/CPU primitives were not serialized and the preregistered control-path subtest was absent.

P1 addresses exactly those two evidence-contract defects and one architecture hypothesis: the P0 spin-then-yield wait policy may be responsible for practical CPU cost.

## 3. Scientific question

> On the same request-scoped ArcLLM-v1 inference semantics and Intel Arc 140V target, does a preallocated single-writer sequenced ring using a frozen low-duty adaptive blocking wait policy preserve exact inference semantics and produce either (a) an end-to-end practical benefit without unacceptable CPU cost, or (b) an independently measurable control-path benefit, relative to the canonical direct path?

This P1 E screens the successor package. It does not identify which component causes any benefit; component attribution belongs to M if E unlocks it.

## 4. Frozen arms

### Arm A — CURRENT_DIRECT

Canonical ArcLLM request orchestration with passive measurement only.

Frozen:
- exact model bytes;
- canonical Transformer math;
- exact shaders and Vulkan graph;
- same policy/binding behavior;
- same evidence profile and validated-domain flag;
- same greedy top-1 generation;
- same request-scoped model/KV/working-buffer lifecycle;
- same caller-provided token IDs and max-new-token count.

### Arm B — LMAX_RING_P1

The same canonical request is admitted through:
- ring capacity: **64**;
- preallocated slots;
- monotonically increasing unsigned 64-bit sequence numbers;
- one producer and exactly one runtime consumer;
- single-writer dispatcher ownership;
- no heap allocation in publish/consume after ring initialization;
- no speculative decode;
- no overlap that changes token dependency order;
- no change to canonical `generate(RunRequest)` semantics.

Frozen P1 wait strategy for every not-ready wait episode:

1. perform at most **16** active `YieldProcessor()` spins;
2. if still not ready, call **one `SwitchToThread()`**;
3. if still not ready, block with Windows **`WaitOnAddress`** on the relevant slot sequence value;
4. every sequence-state transition that can release the opposite side issues **`WakeByAddressSingle`**;
5. after wake, loop and re-check the sequence condition;
6. no `Sleep`, no timed polling, no adaptive threshold tuning during E.

The P1 ring remains a **request-admission/control-path mechanism**; MatMul, Attention, RoPE, RMSNorm, quantization, shaders, sampling, model residency semantics, and dispatch topology are not eventized or changed.

## 5. Evidence-complete inference observation schema

Every inference observation MUST serialize raw primitives, not only derived metrics.

Required identity fields:
- schema version;
- arm;
- workload cell;
- pair index and within-pair position;
- exact runner SHA-256;
- exact model SHA-256;
- exact shader-manifest SHA-256;
- exact authorization/evidence-contract SHA-256.

Required raw timing primitives:
- `request_start_ns`;
- `request_end_ns`;
- `first_token_ready_ns`;
- full `token_ready_ns[]`;
- monotonic clock identifier.

Required raw CPU primitives:
- `cpu_start_100ns`;
- `cpu_end_100ns`;
- `logical_processor_count`.

Required raw allocation primitives:
- request allocation counter start/end;
- decode-window allocation counter start/end;
- final request-wide allocation count;
- final decode-window allocation count.

Required semantic evidence:
- generated token IDs;
- generated token count;
- canonical `RuntimeStats`;
- finite flag;
- error state;
- evidence profile;
- validated-domain flag;
- greedy-generation flag.

Arm B additionally serializes:
- capacity;
- publish/consume counts;
- max occupancy;
- full/backpressure observations;
- active spin iterations;
- `SwitchToThread` count;
- `WaitOnAddress` count;
- wake count;
- sequence-gap count.

Arm A uses `null/not_applicable` for ring-only fields.

Derived metrics MAY be emitted by the runner but are non-authoritative. Independent A MUST recompute them from the raw primitives and require exact/in-tolerance agreement.

## 6. Frozen derived metric definitions

- TTFT = `first_token_ready_ns - request_start_ns`.
- E2E latency = `request_end_ns - request_start_ns`.
- ITL vector = consecutive differences in `token_ready_ns[]` after the first token.
- Decode throughput = `(generated_token_count - 1) / ((last_token_ready_ns - first_token_ready_ns)/1e9)`.
- CPU utilization percent = `((cpu_end_100ns - cpu_start_100ns)*100) / ((request_end_ns-request_start_ns)*logical_processor_count) * 100`.
- Request allocation count = request allocation end - start.
- Decode allocation count = decode-window allocation end - start.

Quantiles are frozen to **Hyndman-Fan Type 7 linear interpolation**:
`h=(n-1)p`, linearly interpolate between floor(h) and ceil(h).
Use p=0.50 for p50 and p=0.95 for p95.

No alternate quantile estimator may be used for the formal verdict.

## 7. Frozen inference workloads and fresh E schedule

Same bounded workload family as P0 for direct comparability, but all P1 outcomes are fresh.

- W1: prefill 8, max_new 8
- W2: prefill 8, max_new 32
- W3: prefill 64, max_new 8
- W4: prefill 64, max_new 32
- W5: prefill 256, max_new 8
- W6: prefill 256, max_new 32

Input IDs:
`token[i] = 1 + ((7919*i + 104729*cell_index) mod 152063)`, cell_index 1..6.

Common settings:
- `EvidenceProfile::PROFILE_0`;
- `request_within_validated_domain=false`;
- greedy generation;
- exact frozen model and canonical shaders.

Schedule:
- 2 excluded warmups per arm before measured inference;
- warmup order: A, B, A, B using W1;
- 4 matched measured pairs per cell;
- pair 0: A then B;
- pair 1: B then A;
- pair 2: A then B;
- pair 3: B then A;
- 24 measured pairs = **48 measured inference runs**.

No selective rerun, outlier deletion, workload substitution, seed substitution, threshold change, or post-hoc tuning.

## 8. Integrated zero-model control-path subtest — mandatory inside E

The control-path subtest is part of the same fresh P1 E contract. E is not complete without it.

For each service delay `0 ns`, `10,000 ns`, `100,000 ns`:
- 1 producer;
- 1 consumer;
- exactly **1,000,000 events per arm**;
- ring capacity 64 for Arm B;
- same fixed event payload for A and B;
- no model load;
- no Vulkan initialization;
- no shader creation;
- no inference;
- exact consumer service delay;
- exact event order required.

Frozen arm order by delay:
- 0 ns: A then B;
- 10 us: B then A;
- 100 us: A then B.

For every arm/delay run, serialize:
- monotonic `run_start_ns` and `run_end_ns`;
- event count;
- allocation counter start/end for the declared steady-state window;
- max occupancy;
- full/backpressure observation count;
- spin/yield/block/wake counters;
- first ordering mismatch, if any.

For independent order/latency recomputation, freeze two little-endian uint64 binary artifacts per arm/delay:
- `consumed_sequence_ids.u64le`, exactly 1,000,000 entries;
- `publish_to_consume_latency_ns.u64le`, exactly 1,000,000 entries.

Each binary artifact must have SHA-256 and byte length recorded in the JSON manifest. Independent A recomputes exact ordering, events/s, p50/p95 latency, and validates hashes/lengths.

## 9. Semantic gate

Any matched inference pair yields `SEMANTICS_FAIL` if:
1. generated token IDs differ at any position;
2. generated token counts differ;
3. either finite flag is false or finite differs;
4. prefill dispatch/submission counts differ;
5. decode dispatch/submission topology differs;
6. route/lifecycle counters differ;
7. runtime error is asymmetric;
8. raw evidence schema is incomplete for either arm.

If semantic gate fails, all performance values are diagnostic only.

## 10. Formal P1 E adjudication

After semantic PASS, independent A applies the following frozen rules.

### E2E_SUPPORTED

Requires:
- at least one of TTFT, E2E latency, or decode throughput improves by **>=5% at both p50 and p95** in at least one workload cell;
- no TTFT p95 regression >3% in any cell;
- no ITL p95 regression >3% in any cell;
- decode throughput p50 does not regress >3% in any cell;
- CPU utilization p50 and p95 do not regress >5% in any cell.

### CONTROL_PATH_ONLY

Only evaluated if E2E_SUPPORTED is false.

Requires:
- exact consumed sequence order for all six arm/delay runs;
- and either:
  - Arm B steady-state allocation count is reduced by >=20% relative to A in at least two of three service-delay cells, or
  - Arm B publish-to-consume p95 latency improves by >=10% in at least two of three service-delay cells;
- and Arm B full/backpressure observation rate is not worse than A in those qualifying cells.

### REGRESSION

After semantic PASS, classify `REGRESSION` if TTFT p95 or ITL p95 regresses >10%, or decode throughput p50 regresses >10%, in at least **4 of 6** workload cells.

### NO_SUPPORTED_BENEFIT

Semantics PASS, evidence contract complete, E2E_SUPPORTED false, CONTROL_PATH_ONLY false, and REGRESSION false.

### UNRESOLVED

Use only when evidence is validly collected but a preregistered category cannot be assigned because of incomplete/corrupt evidence, machine instability, or logically conflicting frozen criteria.

Priority:
`SEMANTICS_FAIL -> REGRESSION -> E2E_SUPPORTED -> CONTROL_PATH_ONLY -> NO_SUPPORTED_BENEFIT -> UNRESOLVED only for validity/assignment failure`.

## 11. Freshness and anti-rescue

Before `E_STARTED`:
- R may repair implementation/build/dependencies/static QA/transport and must prove the evidence schema with zero-science fixtures.
- R must independently verify that every required primitive field and both control-subtest raw binary streams are actually serialized.
- R must freeze exact source/config/executable hashes.

After `E_STARTED`:
- no implementation mutation;
- no evidence-schema repair;
- no threshold/quantile change;
- no rerun of valid observations;
- no selective replay;
- no post-hoc additional control-path test;
- no replacement machine/model/shader set.

A technical failure after E starts terminates the P1 E as `UNRESOLVED` unless the already-frozen contract explicitly permits continuation from a durable, outcome-blind checkpoint. No such continuation is authorized by this P.

Intermediate outcomes may not be inspected to decide whether to continue the predeclared campaign.

## 12. Resource and contamination contract

Fresh E requires:
- exact Intel Arc 140V target;
- exact frozen model SHA;
- exact canonical shader set;
- no concurrent ArcLLM scientific GPU job;
- no competing loaded model/runtime that materially reduces available UMA;
- AC/power scheme and process priority recorded;
- enough RAM/disk to finish all inference evidence plus mandatory control-path raw streams.

Another GPU/cloud target is forbidden.

## 13. E -> M -> C unlock rules

P1 E is exploratory.

- `E2E_SUPPORTED` or `CONTROL_PATH_ONLY` opens **M preregistration**.
- `NO_SUPPORTED_BENEFIT`, `REGRESSION`, or `SEMANTICS_FAIL` closes this P1 architecture package; M is not opened.
- `UNRESOLVED` closes P1 without rescue under the same P; any successor requires a new P.

If M opens, its scientific purpose must decompose the package, at minimum separating wait-policy effect from ring/single-writer structure. Exact M contrasts are not authorized until a new M preregistration is frozen.

C may open only after M produces a prespecified positive mechanism result and a separate C preregistration freezes a genuinely fresh confirmatory evidence partition.

## 14. Repository discipline

P1 files may exist only under:
- `docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P1_*.md`;
- `config/arcllm_lmax_arch_p1_*.json`;
- `experiments/arcllm_lmax_arch_p1/`;
- `results/arcllm_lmax_arch_p1_*/`.

No P1 implementation mutation is allowed in canonical `src/`, `include/`, or `shaders/`.

`lineage.md` is not updated at P or R. It is append-only after A reaches a formal scientific PASS/FAIL/UNRESOLVED state.

## 15. Current lifecycle state

`ARCLLM_LMAX_ARCH_P1_PREREGISTRATION = FROZEN`

Fresh P1 outcome execution is **not authorized** by this document.

The agent may now perform **R internally** without user-facing scientific gates. The next user-facing stop must be immediately before:

`ARCLLM_LMAX_ARCH_P1_E_FRESH_EXPLORATORY_ONE_SHOT`

unless R discovers that the scientific contract itself must change.
