# ArcLLM v1 — Lineage

## Origin — 2026-09-22

ArcLLM v1 is opened as a **new successor research line** from ArcLLM terminal evidence commit:

`e30199cd54be1e0e7390e2453c8935c847fe0dd1`

Source branch at origin:

`research/arcllm-ttft-m2`

New branch:

`research/arcllm-v1`

Historical ArcLLM and TTFT-M2 verdicts remain immutable and are not reopened.

### Motivation

ArcLLM v0 established technical feasibility but did not demonstrate matched end-to-end advantage against a mature llama.cpp baseline.

ArcLLM v1 does not assume that this negative implies a permanent structural ceiling.

The new program explicitly studies the distinction between:

- structural architecture disadvantage;
- recoverable maturity debt;
- unresolved measurement/attribution uncertainty.

### First artifact

`docs/research/arcllm-v1/ARCLLM_V1_ARCHITECTURE_LEARNING_METHODOLOGY.md`

The methodology defines:
- Maturity-Aware Architecture Research;
- Headroom Map;
- Amdahl-first intervention selection;
- Carry-Through Ratio;
- architecture epochs;
- researchable-runtime layers;
- lighter four-invariant scientific governance;
- rules separating self-rescue from legitimate prospective learning.

### Derivation order

The next documents must be produced in this order:

1. architecture reframe;
2. Headroom Map from historical ArcLLM evidence;
3. intervention-001 selection.

No ArcLLM v1 performance intervention is implemented at this origin step.


## Architecture reframe — 2026-09-22

Derived from methodology blob:

`d41a357840435f52a682792b9a6608e4abaab312`

Artifact:

`docs/research/arcllm-v1/ARCLLM_V1_ARCHITECTURE_REFRAME.md`

Key reframe:
- legacy ArcLLM becomes evidence/reference rather than immutable architecture;
- semantic graph is separated from execution topology;
- prefill and decode receive independent execution policies;
- quant formats default to specialized execution families;
- execution graph IR becomes the architecture center;
- observability/cost model becomes a first-class layer;
- matched baseline comparison moves earlier in each architecture epoch.

No intervention selected or implemented here.

Next: build the historical-evidence Headroom Map.


## Initial Headroom Map — 2026-09-22

Artifact:

`docs/research/arcllm-v1/ARCLLM_V1_HEADROOM_MAP.md`

Historical evidence was converted into a system-level headroom model.

Key derived result from Q2:
- post-TTFT share ~99.1% for W-S;
- post-TTFT share ~83.8% for W-C;
- post-TTFT ArcLLM/baseline envelope ~39.98× / ~32.08×.

Amdahl envelopes:
- TTFT elimination ceiling ~1.009× W-S / ~1.193× W-C;
- 3× post-TTFT improvement would imply ~2.945× W-S / ~2.267× W-C E2E speedup if the historical cost shares held.

ANL64 provides direct evidence that ArcLLM-family decode maturity can move by ~1.22–3.20× and propagate to ~1.14–2.51× E2E improvement versus the safe ArcLLM reference, though TTFT coupling blocked that program.

Initial priority:
1. decode/post-TTFT execution plane;
2. prefill/TTFT architecture;
3. memory/working-set topology.

The map does not yet choose a specific decode mechanism.

Next: Intervention-001 selection.


## Intervention-001 selection — 2026-09-22

Artifact:

`docs/research/arcllm-v1/ARCLLM_V1_INTERVENTION_001_SELECTION.md`

Selected:

`I001-PDEP — Persistent Decode Execution Plane / Decode Graph Compression`

Selection basis:
- historical post-TTFT share ~99.1% W-S / ~83.8% W-C;
- coarse post-TTFT external gap ~39.98× / ~32.08×;
- 469 dispatches per cached decode step;
- decode received less optimization depth than prefill;
- ANL64 demonstrates ~1.22–3.20× recoverable decode movement and ~1.14–2.51× E2E movement versus the safe ArcLLM reference.

I001 does not assume host submission is the bottleneck. It targets the broader fine-grained decode execution topology and persistent-state opportunity.

No implementation is authorized.

Next required artifact:

`ARCLLM_V1_I001_DECODE_COST_MODEL_AND_MECHANISM_DISCRIMINATOR.md`

The next study must falsify I001 before implementation if graph fragmentation/persistence opportunity does not account for enough recoverable decode cost.


## Foundation terminology / prior-art QA — 2026-09-22

A zero-science QA pass was performed before opening the I001 decode cost-model discriminator.

Canonical QA-amended documents:
- methodology `bd943fe99e0a6758f8e0aa8060f9540a0b74a942`;
- architecture reframe `5f06a648d200366066e79c9355ae3bb448e1c8fb`;
- Headroom Map `e66a296134d5f9701a6f6b63a5954f52417b71cc`;
- I001 selection `c76ceb06e00bbea731bb575f6646febc39eee466`.

QA checklist:
`docs/research/arcllm-v1/checklist.md`

Resolved 20/20 findings. Final `open_findings=0`, `count=0`.

Key corrections:
- project-specific terms separated from established literature terminology;
- operational term contract added;
- evidence classes MEASURED/DERIVED/BOUND/HYPOTHESIS added;
- broken math escaping repaired;
- headroom envelope separated from attributable recoverability;
- ANL64 causal scope narrowed;
- architecture objects explicitly defined;
- PDEP/graph-compression/persistence semantics defined;
- I001 made conditional with competing mechanism alternatives and H-NULL-I001;
- prior-art foundation and Intel-Arc/Vulkan transfer limits added;
- exact derived-document blob bindings refreshed.

No performance execution or I001 implementation occurred.

Next remains specification-only:
`ARCLLM_V1_I001_DECODE_COST_MODEL_AND_MECHANISM_DISCRIMINATOR.md`.


## I001 historical decode cost-model discriminator — 2026-09-22

Canonical specification/synthesis:

`docs/research/arcllm-v1/ARCLLM_V1_I001_DECODE_COST_MODEL_AND_MECHANISM_DISCRIMINATOR.md`

Formal adjudication:

`artifacts/ARCLLM_V1/ARCLLM_V1_I001_HISTORICAL_COST_MODEL_ADJUDICATION_v0.1.json`

Result:

`FALSIFY_I001_PDEP_AS_FIRST_IMPLEMENTATION`

Key evidence:
- exact Q2 7B path: 469 decode dispatches per step, ~3140.26 ms/token W-S and ~2506.23 ms/token W-C;
- P7-H/P7-M 469-dispatch proxy profiles: outside-submit bucket ~0.43–3.65%;
- device barrier+unattributed ~0.35%;
- six major compute families account for ~99.2% of device-chain time;
- ANL64 keeps a 469-node plan while producing ~1.22–3.20× fresh 7B decode speedup through changed quant-linear execution.

Mechanism disposition:
- H-LAUNCH/LIFECYCLE: LOW_HEADROOM_AS_PRIMARY_MECHANISM;
- H-SYNC: FALSIFIED_AS_DOMINANT_PRIMARY_MECHANISM_BY_HISTORICAL_PROXY;
- H-LOCALITY: UNRESOLVED;
- H-KERNEL: SUPPORTED_AS_DOMINANT_NEXT_DISCRIMINATION_CLASS;
- H-MIXED: POSSIBLE_BUT_UNQUANTIFIED;
- H-NULL-I001: SUPPORTED_FOR_PDEP_AS_FIRST_IMPLEMENTATION.

No PDEP code was written.

The decode plane remains priority #1, but the next step is measurement-only:

`ARCLLM_V1_I001R_EXACT_7B_DECODE_DEVICE_WORK_PROFILE.md`

Only after an exact 7B per-family profile may a replacement implementation be selected.


## I001R exact 7B decode device-work profile — implementation freeze — 2026-09-22

Study:

`ARCLLM_V1_I001R_EXACT_7B_DECODE_DEVICE_WORK_PROFILE`

Purpose:
- replace 1.5B proxy attribution with exact Q2/Q3-safe 7B device-work evidence;
- measure dominant decode families before selecting any replacement implementation.

Frozen instrumentation:
- exact Q2 graph builder preserved byte-for-byte;
- exact 441 prefill / 469 decode dispatch census;
- exact Q2 16-shader payload;
- decode probes at indices 0, 15, 30 only;
- remaining 28 measured decode steps use unchanged production `execute_prepared`;
- Vulkan timestamp period and raw 469 op ticks are emitted;
- exact GGUF model-weight bytes are emitted only as logical tensor-byte accounting, not DRAM traffic.

Collection:
- Session A: W-S → W-C;
- Session B: W-C → W-S;
- 1 warmup + 5 measured per cell;
- 20 measured full inferences;
- 60 profiled decode steps;
- 560 production lifecycle decode steps;
- no automatic/selective rerun.

Zero-science implementation QA:
`PASS_I001R_ZERO_SCIENCE_IMPLEMENTATION_QA`

No performance measurement has been executed at commit time.

Next:
`LOCAL_EXACT_I001R_COLLECTION`


## I001R pre-execution static-QA matcher correction — 2026-09-22

Local execution at frozen HEAD `8c75da2ec9831aeaa48d251f8c0f5c8e8228537c` stopped in the first static-QA step before build/model/GPU/timing.

Observed failure:

`AssertionError: missing I001R instrumentation contract: "pdep_implementation":false`

Root cause:
- C++ source correctly contains escaped quotes because it emits JSON from a string literal;
- Python QA matcher accidentally searched the unescaped runtime JSON form;
- defect is isolated to the QA matcher.

Scientific contamination:

```text
model_load=0
gpu_dispatch=0
timing_observation=0
fresh_science=0
```

Repair:
- corrected static matcher;
- original frozen contract `v0.1` preserved historically;
- new `v0.1.1` contract supersedes it;
- runner changed only to bind/read the `v0.1.1` contract.

All scientific fields remain unchanged.

Zero-science revalidation:

`PASS_I001R_ZERO_SCIENCE_REVALIDATION_AFTER_MATCHER_CORRECTION`

Next:

`LOCAL_EXACT_I001R_COLLECTION_V0_1_1`


## I001R second pre-execution static-QA matcher correction — 2026-09-22

Local execution at HEAD `c00932963c1dc628e6f8c5ce102cb1e8b3242b55` again stopped in static QA before build/model/GPU/timing.

Observed failure:
`AssertionError` on the `decode_op_names` source matcher.

Root cause:
- two remaining source-level JSON-key matchers used normal Python string escaping;
- Python removed the backslashes before comparison against C++ source text;
- the profiler source itself was correct.

The complete JSON matcher set was converted to Python raw strings and the entire static-test logic was independently revalidated against repository blobs before release.

Scientific contamination remains zero:
```text
model_load=0
gpu_dispatch=0
timing_observation=0
fresh_science=0
```

Contract lineage:
- v0.1 preserved;
- v0.1.1 preserved;
- v0.1.2 supersedes v0.1.1 and changes only QA matcher + runner contract binding.

Scientific design remains unchanged.

Zero-science result:
`PASS_I001R_ZERO_SCIENCE_REVALIDATION_AFTER_JSON_MATCHER_CORRECTION`

Next:
`LOCAL_EXACT_I001R_COLLECTION_V0_1_2`


## I001R pre-execution native-build query ABI correction — 2026-09-22

Local execution at HEAD `cf362bd08d3d1658e83d983c09b7f6920e3b2c0d` passed static QA and all Q2 shader compilation, then failed in native C++ compilation before model load or GPU execution.

Root cause:
- I001R introduced Vulkan timestamp-query functions into the repository's minimal hand-declared Vulkan shim;
- required query aliases/handle/struct/constants were omitted;
- an additional `VkPhysicalDeviceProperties` dependency was introduced only to obtain `timestampPeriod`.

Repair:
- imported the query ABI declaration set already used by historical P7-M;
- removed the new `VkPhysicalDeviceProperties` ABI dependency;
- amended I001R output from absolute ns/GB/s to raw Vulkan timestamp ticks and scale-invariant ratios;
- retained exact graph, collection, decision thresholds, and no-rerun rules.

Scientific contamination:
```text
model_load=0
gpu_dispatch=0
timing_observation=0
fresh_science=0
```

Contract lineage:
- v0.1, v0.1.1, v0.1.2 preserved;
- v0.1.3 supersedes v0.1.2.

Measurement amendment:
- scientific question unchanged;
- decision logic unchanged;
- collection design unchanged;
- measurement representation changed from attempted ns conversion to raw ticks;
- absolute ns conversion is deferred unless needed by the family-specific lower-bound study.

Zero-science QA:
`PASS_I001R_ZERO_SCIENCE_REVALIDATION_AFTER_QUERY_ABI_CORRECTION`

Next:
`LOCAL_EXACT_I001R_COLLECTION_V0_1_3`


## I001R exact 7B profile adjudication — 2026-09-22

Returned bundle SHA256:

`5C69C5ADD37A5C54F58942A4A4A2B74F21111FBED17D028AFDCCB02418A1ECBE`

Independent validity:
- 20/20 measured attempts;
- 60/60 profile probes;
- 560/560 normal lifecycle steps;
- exact environment/model/HEAD;
- no rerun;
- no optimization during measurement;
- semantic tokens/logits/hidden hashes stable within each workload.

Formal result:

`PASS_VALID_COLLECTION_DOMINANT_DEVICE_FAMILY_FFN_GATE_UP`

Independent median device-chain shares:
- ffn_gate_up 59.6727%;
- lm_head 21.4557%;
- ffn_down 15.8168%;
- attn_qkv 2.5975%;
- attn_output 1.0218%;
- attention 0.2093%.

Barrier/unattributed median = 0.04596%.
Lifecycle outside-submit median = 0.08170%.

Exact source mapping:
- 56 decode gate/up nodes/token;
- Q4_K;
- batch 1;
- n=3584;
- rows=18944;
- baseline `p7_q4k_gemm_2d.spv`.

Cross-evidence:
SA1 exact gate/up geometry `Q4_H3584_R18944_NOBIAS` already passed correctness and showed 5.33376× / 5.50431× speedup for subgroup32 split-K.

I001R is formally closed.

Selected successor intervention:

`I002-Q4-GU-SG32`

No I002 implementation is authorized yet.

Next artifact:

`ARCLLM_V1_I002_REAL_MODEL_TRANSFER_AND_CARRY_THROUGH_SPEC.md`


## I002 real-model transfer/carry-through specification — 2026-09-22

Parent evidence/adjudication commit:
`e4e643262a36e35ef79cb6a190a6a46019dbe39d`

Frozen successor:
`I002-Q4-GU-SG32`

Evidence basis:
- exact I001R 7B `ffn_gate_up` share 59.6727%;
- exact 56 Q4_K gate/up decode nodes;
- SA1 exact gate/up geometry speedup 5.33376× / 5.50431× with correctness PASS.

Specification:
`docs/research/arcllm-v1/ARCLLM_V1_I002_REAL_MODEL_TRANSFER_AND_CARRY_THROUGH_SPEC.md`

The study changes only the 56 decode gate/up nodes and freezes:
- real-model component correctness transfer;
- full-model semantic guard;
- paired decode carry-through;
- TTFT non-regression;
- E2E direction;
- no rescue / no kernel search.

No fresh I002 model execution is authorized by this specification alone.

Next:
I002 bounded implementation package + T0/T1/T3 zero-science QA.


## I002 bounded implementation package — 2026-09-23

Intervention:
`I002-Q4-GU-SG32`

Exact candidate:
`shaders/sa1_q4k_subgroup_splitk.comp`
blob `56999d88dc1bef6486e7e1908982f6de4b0f9f6a`

Bounded delta:
- decode only;
- 56 Q4_K FFN gate/up nodes/token;
- prefill unchanged;
- all non-target decode families unchanged;
- no kernel search.

Implemented:
- T0 static/provenance package;
- T1 real-model correctness + component-timestamp harness;
- T2 full-model token-semantic guard before T3 measurements;
- T3 paired carry-through harness;
- stage-specific T1/T3 science authorization;
- fail-closed analyzers/runners.

Static source/provenance QA:

`PASS_I002_STATIC_SOURCE_PROVENANCE_QA_PENDING_LOCAL_BUILD`

No model load, GPU dispatch, timing observation, or fresh science occurred during implementation.

Fresh science remains locked.

Next:
run `run_arcllm_v1_i002_preflight.ps1` locally. This preflight performs only static QA, shader compilation/provenance verification, and native build. It must return a valid zero-science PASS bundle before T1 science authorization can be created.


## I002 preflight static-QA Python syntax repair — 2026-09-23

Local zero-science preflight at HEAD `9e7278c6f31980f28819a4219e59cda02cb3fc4e` stopped while Python parsed the static QA file.

Observed:
`SyntaxError: unterminated string literal`

Failure occurred before:
- shader compile;
- native build;
- model load;
- GPU dispatch;
- timing collection.

Scientific contamination:
```text
science=0
model_load=0
gpu_dispatch=0
timing=0
```

Root cause:
an escape-sensitive C++ JSON source matcher was encoded as an invalid Python string literal.

Repair:
- removed the escaped-fragment matcher entirely;
- static QA now locates the source line by the semantic key `candidate_gate_up_nodes_per_step` and checks value `56`;
- added `python -m py_compile` for all three Python QA/analyzer files before static QA execution;
- preserved T1/T3 source, exact SA1 candidate, model, thresholds, sessions and no-rerun rules.

Lock lineage:
`config/arcllm_v1_i002_execution_lock_v0.1.1.json`
supersedes v0.1.

Fresh I002 science remains locked.

Next:
run the zero-science Windows preflight against the repair HEAD and return its bundle.


## I002 zero-science build preflight adjudication — 2026-09-23

Returned bundle:
`9AC9C320206A6AF1B99A56DED3474F309A4FFC3A5E3ADD7C212C84A0107531E5`

Exact implementation HEAD:
`cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`

Independent result:
`PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD`

Verified:
- 23/23 critical Git blobs;
- exact SA1 candidate source;
- exact historical candidate SPIR-V hash;
- exact 16-shader Q2 baseline provenance;
- T1 executable built: `3D3F6A...00C5E`, 452608 bytes;
- T3 executable built: `5B067B...E9BD`, 443392 bytes;
- science/model/GPU/timing all zero.

T0 is closed PASS.

Only T1 is eligible for science authorization.
T3 remains locked pending independent T1 adjudication.


## I002 T1-only science authorization — 2026-09-23

Preflight basis:
`PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD`

Preflight adjudication commit:
`ef18cd80fb22ecb5ab949d382edcbaf433322891`

Implementation payload HEAD:
`cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`

Authorization:
```text
fresh science = true
T1            = true
T3            = false
one-shot T1   = true
```

The T1 runner binds the implementation payload by exact Git blobs plus the preflight-built executable/SPIR-V hashes. The later authorization commit is required only to be a descendant of the implementation HEAD.

A science-start marker prevents any T1 rerun after fresh execution begins.

Next:
run the exact T1 authorization once and return the T1 bundle for independent adjudication.


## I002 T1 pre-science authorization schema-path repair — 2026-09-23

Local T1 invocation at authorization HEAD `327605953092a3a12aaec15904597eecbc04b63e` stopped before the science-start marker.

Observed:
`PropertyNotFoundStrict: candidate_spv_sha256`

Root cause:
- authorization JSON correctly stored the candidate hash at `candidate.spv_sha256`;
- runner incorrectly read a nonexistent top-level `candidate_spv_sha256`.

No science was consumed:
```text
science_start_marker=absent
model_load=0
gpu_dispatch=0
timing=0
```

Repair:
- T1 runner now reads `candidate.spv_sha256`;
- authorization schema superseded to `v0.2.1`;
- exact executable/model/candidate hashes, T1 contract, thresholds and one-shot semantics are unchanged;
- T1 remains authorized;
- T3 remains locked.

Revalidation:
`PASS_I002_T1_AUTHORIZATION_SCHEMA_REVALIDATION`


## I002 T1 real-model component transfer adjudication — 2026-09-23

Returned T1 bundle:
`9E731E1438F38B90A50E2C5AC8B91A2F69EF160265932BDB8B6CB118D189293E`

Authorization HEAD:
`045326009244db56082b281f329d87178125b7f0`

Formal result:
`PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`

Independent recomputation:
- 72/72 exact real-model comparisons;
- 72/72 correctness PASS;
- max_abs worst 3.8147e-05 vs 0.02 gate;
- RMSE worst 3.6279e-06 vs 0.005 gate;
- 72/72 individual component speedups >1.50×;
- cell medians 3.6897× / 7.7901× / 6.3047× / 9.0023×;
- global minimum 2.9232×;
- candidate-first median 6.7958×;
- baseline-first median 6.4716×.

T1 is closed PASS.

T3 is now eligible for separate one-shot authorization. No T3 evidence was consumed by T1.


## I002 T3-only science authorization — 2026-09-23

T1 basis:
`PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`

T1 bundle:
`9E731E1438F38B90A50E2C5AC8B91A2F69EF160265932BDB8B6CB118D189293E`

Authorization:
```text
fresh science = true
T1            = false
T3            = true
one-shot T3   = true
```

T3 runner binds:
- exact implementation ancestry;
- exact T3-critical blobs;
- preflight-built T3 executable hash;
- exact candidate SPIR-V;
- model/environment/power;
- T1 formal PASS artifact.

A T3 science-start marker prevents rerun once fresh T3 begins.

Next:
run exact T3 once and return bundle for final I002 adjudication.


## ArcLLM v1 DEV_HOST governance amendment — 2026-09-23

This amendment was made prospectively before any I002 T3 outcome exposure.

Reason:
the research machine is also an active development workstation. Governance must preserve provenance and anti-cherry-pick discipline without requiring an unrealistically idle dedicated host.

New rule:
- ambient CPU/RAM/GPU/process load is metadata, not a hard blocker;
- manual full-collection rerun is allowed under the exact same frozen payload;
- every attempt is append-only;
- no selective cell rerun;
- no automatic rerun;
- first complete valid collection is the primary confirmatory dataset;
- later complete runs are replication/robustness evidence only;
- no completed primary run may be replaced/deleted;
- no candidate/threshold/workload mutation after outcome exposure.

I002 T3 changes:
- permanent global science-start marker removed;
- unique per-attempt marker added;
- DEV_HOST context snapshot added;
- T3 executable/candidate/gates/workloads unchanged;
- T2 token-semantic guard remains a correctness gate.

Authorization remains T3-only:
```text
T1=false
T3=true
```

Next:
run I002 T3 under DEV_HOST governance and return the bundle. Operational rerun is permitted without changing the frozen payload.


## DEV_HOST governance provenance rebind — 2026-09-23

Post-commit audit of execution lock v0.1.2 found one stale historical binding for `run_arcllm_v1_i002_t1.ps1`.

No T3 science had run.

Correction:
- supersede lock to `v0.1.3`;
- bind the already-current T1 runner blob `5129617ab16cbd3b784e4d049f66337dba8266d3`;
- rebind T3 authorization to lock v0.1.3;
- no candidate, executable, T3 runner, threshold, workload, or governance rule changed.


## I002 primary T3 final adjudication — 2026-09-24

Primary returned bundle:
`B198CCDEB1996FCCBB0A7F2BD8275CE4C74D6055E731E6278F95FA7545ABA09E`

Chronology:
the primary collection was generated under authorization v0.3 / HEAD `d5f144ce...`, before the later DEV_HOST governance amendment. It is therefore adjudicated under the original frozen contract; DEV_HOST governance is future-only.

Independent result:
`PASS_I002_REAL_MODEL_CARRY_THROUGH`

Evidence:
- T2 semantic guard PASS 4/4;
- 20/20 measured pairs semantic-equal;
- 20/20 decode pairs improve;
- global decode geomean speedup 2.1998×;
- G2 PASS all cells;
- G3 TTFT median PASS all cells;
- 20/20 E2E pairs improve;
- global geomean of cell-median E2E speedups 2.0026×;
- G4 PASS all cells.

I002 scientific chain:
```text
T0 PASS
T1 PASS
T2 PASS
G2 PASS
G3 PASS
G4 PASS
→ PASS_I002_REAL_MODEL_CARRY_THROUGH
```

I002 is closed. Existing science authorization is closed.

Next:
fresh matched external baseline comparison of the closed I002 candidate versus pinned llama.cpp under one matched DEV_HOST design. No next kernel is selected yet.


## I003-MEB matched external baseline program — 2026-09-24

Parent:
`PASS_I002_REAL_MODEL_CARRY_THROUGH`

Question:
what fresh practical gap remains between the closed I002 candidate and exact pinned llama.cpp v0.4.1 under matched DEV_HOST execution?

Frozen design:
- same exact GGUF;
- exact W-S/W-C;
- closed I002 candidate only;
- llama.cpp commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- 4 DEV_HOST cells;
- 5 adjacent candidate/llama pairs per cell;
- 40 measured inferences total;
- one warmup before each measured arm;
- pair order counterbalanced A/B;
- ambient load recorded, not blocking;
- no selective pair/cell rerun;
- no new kernel selection.

Bounded implementation and static QA:
`PASS_I003_STATIC_SOURCE_PROVENANCE_QA_PENDING_LOCAL_BUILD`

Fresh external comparison remains locked.

Next:
run `run_arcllm_v1_i003_preflight.ps1` locally and return its zero-science bundle.
