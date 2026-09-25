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

## I003 zero-science preflight repair — 2026-09-24

Observed preflight parent HEAD:
`88fe45b96e78746a3c7b13b48b73235f3610400a`

Observed failure:
`tests/test_arcllm_v1_i003_package.py` asserted at least three unescaped occurrences of
`"sa1_q4k_subgroup_splitk.spv"`, but the candidate source contains exactly two unescaped
dispatch literals (FFN gate and FFN up). The output metadata occurrence is embedded inside
a C++ JSON string and is therefore escaped in source.

Adjudication:
`PRE_SCIENCE_INFRASTRUCTURE_STATIC_MATCHER_FALSE_NEGATIVE`

No science was consumed:
- failure occurred during static QA before shader compile/build/runtime qualification;
- science measured inferences = 0;
- candidate inference executed = false;
- fresh measurement authorization remains false.

Repair scope:
- no candidate change;
- no baseline change;
- no workload/metric/threshold change;
- matcher now requires exactly two unescaped dispatch literals plus the escaped
  `candidate_shader` metadata field;
- original execution lock v0.1 is preserved;
- superseding execution lock is
  `config/arcllm_v1_i003_execution_lock_v0.1.1.json`;
- next action is a full zero-science preflight rerun, not a partial retry.

## I003 zero-science preflight repair 2 — 2026-09-24

Observed parent HEAD:
`2e7540528a4a214f260c810ca32c042f1c1c365f`

The first repair was insufficiently validated. It corrected the intended matcher concept but
used a Python raw string containing two literal backslashes where the C++ source contains one
escape backslash. The rerun therefore failed again in static QA before any science.

A full prospective simulation of the static QA then exposed a second latent textual matcher:
the candidate and llama baseline encode the same frozen W-S token IDs using different C++ syntax.
The prior assertion incorrectly required one source spelling to exist in both files.

Repair 2:
- metadata check is structural: exactly one `candidate_shader` source line and it must name
  `sa1_q4k_subgroup_splitk.spv`;
- workload checks validate the exact frozen token IDs/formula in each adapter's own syntax;
- candidate/baseline/workload/metrics/thresholds are unchanged;
- science measured inferences remain 0;
- fresh science authorization remains false;
- full prospective static assertion simulation: 47/47 PASS before this commit.

Superseding lock:
`config/arcllm_v1_i003_execution_lock_v0.1.2.json`

Next:
full zero-science preflight rerun from the beginning.

## I003 zero-science preflight repair 3 — 2026-09-24

Observed parent HEAD:
`32fc472da8e1e80cae002d6a3acd11f164a59be1`

The preflight passed static QA, shader/provenance compilation, candidate native build, and
pinned llama.cpp build. The baseline qualifier emitted
`I003 baseline BUILD_API_QUALIFIED` for exact commit
`b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`, then the parent preflight falsely threw
`I003 baseline build qualification failed`.

Adjudication:
`PRE_SCIENCE_INFRASTRUCTURE_POWERSHELL_STAGE_EXITCODE_LEAK`

Root cause:
the caller interpreted `$LASTEXITCODE` left by an internal native command of a successful
child PowerShell script as the status of the child `.ps1` stage.

Bounded repair:
- baseline qualifier now checks the CMake-version native command explicitly;
- successful baseline qualifier terminates with explicit `exit 0`;
- preflight additionally validates the baseline qualification artifact, exact pinned commit,
  clean source, Vulkan backend, no target-model execution, adapter existence and adapter SHA;
- candidate, baseline, workloads, metrics and thresholds are unchanged.

Science state:
- science measured inferences = 0;
- candidate inference executed = false;
- baseline runtime model qualification had not started;
- fresh measurement authorization remains false.

Superseding lock:
`config/arcllm_v1_i003_execution_lock_v0.1.3.json`

Next:
full zero-science preflight rerun from the beginning.

## I003 zero-science preflight independent adjudication + science authorization — 2026-09-24

Returned preflight bundle SHA256:
`FF26D03B073CCA099EBD37355D227E0167DCAD70822FDC0F45C2538E50669509`

Implementation HEAD:
`0180a410645c2ee40fd1dcd5185e84a7ad6de63f`

Independent adjudication:
`PASS_I003_ZERO_SCIENCE_PREFLIGHT_INDEPENDENTLY_ADJUDICATED`

Verified:
- bundled execution lock recomputes to exact Git blob `d28f3db39eaf2f51245a37d9183000969e53eaec`;
- all 29 frozen critical Git blobs match implementation HEAD;
- exact target model SHA256/size match;
- candidate SA1 SPIR-V matches historical frozen hash;
- pinned llama.cpp is v0.4.1 commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- W-S and W-C baseline runtime qualifications are QUALIFIED with Vulkan full offload 29/29;
- both runtime qualifications have `decode_executed=false` and `measured_attempts=0`;
- preflight has `science_measured_inferences=0` and candidate inference was not executed.

Frozen runtime hashes authorized:
- candidate exe SHA256 `F700AF47AA7E78A29FC96EE52EB47A5A229ADDDA19626832D37B9E9A2B0B7CB3`, bytes 442368;
- llama exe SHA256 `62DA22E6384D4F3A426FD23D1664FE2F37AF967F8401DABDE10169C1B81AB242`, bytes 47113728;
- candidate SPIR-V SHA256 `B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569`.

The candidate executable itself is not embedded in the return ZIP; its exact reported hash/size
is therefore frozen into the authorization and is revalidated by the science launcher before
any measured inference.

Science authorization:
`arcllm.v1.i003.science_authorization.v0.1`

Authorization QA:
`PASS_I003_SCIENCE_AUTHORIZATION_QA`

Next:
run the exact frozen `run_arcllm_v1_i003.ps1` collection. Do not select or optimize another kernel.

## I003-MEB primary collection final adjudication — 2026-09-24

Primary collection:
`20260924T143132554Z_cf9a222c`

Returned bundle SHA256:
`2FE89BCCFA9FB1ABE5782D628BF860F0026334F2ADC0BBE69FFF7B8A65253FE6`

Independent raw-evidence audit:
- 20/20 matched pairs valid;
- 40 measured inferences represented;
- zero raw validity issues;
- exact A/B counterbalanced pair order;
- all process exit codes zero;
- all measured attempts successful, finite, and exactly 32 tokens;
- candidate dispatch census PASS throughout;
- independent median/min/max/MAD and global geometric-mean recomputation matches the frozen summary exactly.

Final classification:
`I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE`

Fresh global post-I002 gap:
- decode latency candidate/llama geomean = `10.378702063787069×`;
- E2E latency candidate/llama geomean = `9.972198817849302×`;
- decode throughput candidate/llama geomean = `0.0963511616244538×`.

Cross-system token identity is not required by the frozen I003 specification. Candidate and llama
each produced stable within-system hashes across 5/5 repetitions for each workload; the cross-system
continuations differ and remain recorded as descriptive evidence.

Scientific decision:
the fresh external gap remains large. I003 selects no next kernel. Per its preregistered decision
boundary, the next program must be a fresh post-I002 exact device-work profile before selecting
any new mechanism. Historical Q2 shares are not reused as the current localization.

I003 primary science authorization is now closed. Any later I003 collection is replication/
robustness only and requires a fresh explicit authorization.

## Hardware-grounding phase — One-Token Hardware Model v0.1 — 2026-09-24

After I003 established a fresh ~10x external gap, the research method moved from dominant-profile-family selection to a hardware-grounded top-down/bottom-up join.

Central artifact: `ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL`.

v0.1 consumes no fresh performance execution. It statically maps all 469 post-I002 decode dispatch nodes to exact model geometry, current Arc shader/workgroup organization, hardware roofs, pinned llama source organization, and explicit unknown measurement fields.

Key derived findings: logical one-pass weight payload = 4,370,560,992 bytes/token; optimistic 136GB/s weight-only floor ~32.14ms/token; dense-equivalent linear FP32 peak proxy floor ~3.54ms/token; I003 useful-weight throughput Arc ~4.07–6.47GB/s versus llama ~46.93–56.38GB/s.

Source comparison shows gate/up is the only major Arc quant-linear family already converted to subgroup split-K. Q/K/V/O/down/lm-head remain row-owned serial-K shaders, while pinned llama Q4_K/Q6_K Vulkan matvec distributes K across workgroup threads and reduces partials.

No next kernel is selected. Next evidence is metadata-only tensor quant census, then minimal current Arc and pinned-llama node/family timing. Quiet-host counters are deferred unless a concrete hardware hypothesis remains unresolved.

## M0 exact tensor census — static package lock — 2026-09-24

M0 is metadata-only and targets exactly 56 unresolved tensors: blk.0..27.attn_v.weight and blk.0..27.ffn_down.weight.

Static QA PASS at package head `04dc315a06a1fb5e59e62d585281b10e7656302a`.
No Vulkan, inference, timing, counters, or performance measurement is present or authorized. The runner verifies the exact frozen model SHA256/size and critical Git blobs before reading GGUF tensor descriptors.

The global quant mix implies exactly 28 Q4_K + 28 Q6_K among the 56 target tensors, but this is used only as a consistency gate; layer ownership is never inferred.

M1 remains blocked until local M0 returns PASS, independent adjudication confirms the mapping, and the central one-token hardware model is patched/audited.

## M0 exact tensor census — PASS / hardware model v0.2 — 2026-09-24

Returned bundle SHA256: `3BD3102FFE8ABE4C13150E1B88641FE5BE3940610155F532564B255922D32453`.
M0 PASS: 56/56 target tensors resolved from exact GGUF metadata with no inference/performance run. V tensors are 14 Q4_K + 14 Q6_K; FFN-down tensors are 14 Q4_K + 14 Q6_K. The exact layer mapping is committed into `ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL_v0.2.json`.

M0 closes the quant-census uncertainty. Next evidence is M1 minimal post-I002 node/family timing; machine profiling is not repeated and quiet-host mode is not required.

## M1 minimal post-I002 node timing — package lock — 2026-09-24

M1 reuses the proven 469-op Vulkan timestamp profiler but binds the current post-I002 gate/up shader `sa1_q4k_subgroup_splitk.spv`. Discovery design is intentionally light: W-S + W-C, one warmup and two measured attempts/workload, probes at decode indices 0/15/30, for 4 measured inferences and 12 profiled token steps.

No machine-profile repeat, quiet-host gate, hardware counters, A/B sessions, or intervention is included. M1 reports raw GPU tick shares plus an explicitly labeled `wall_attributed_ms_proxy`; this proxy is not timestampPeriod-calibrated pure GPU milliseconds.

Next after valid M1: patch the central one-token hardware model and open M2 pinned-llama same-semantic execution mapping.

## M1 minimal post-I002 node timing — PASS / hardware model v0.3 — 2026-09-24

Bundle SHA256 `D9345A4B94C3FC278D1459D7127464BDCBAD4AF142598AF3574F4E51934D634F`. Four measured inferences yielded 12 valid 469-op timestamp probes. Independent family recomputation matched the summary.

Post-I002 coarse tick shares: FFN-down ~35.13%; gate/up split-K ~21.91%; LM-head ~21.38%; QKV ~10.43%; O projection ~3.77%. This is a cost inversion relative to historical I001R: gate/up is no longer dominant after the successful I002 mechanism.

No intervention selected. Next: M2 pinned llama same-semantic Vulkan timing map using exact baseline commit; then compute cross-runtime excess map.

## M2 pinned llama same-semantic map — package lock — 2026-09-24

Exact pinned llama Vulkan source already contains `GGML_VK_PERF_LOGGER`; no baseline source patch is needed. M2 enables the logger through environment only, preserves the exact I003 adapter/baseline/model, and maps calibrated Vulkan microseconds for semantic matmul shapes at decode n=1.

M2 uses W-S and W-C, one warmup + one measured attempt each, and selects decode indices 0/15/30 from the measured attempt. No quiet-host gate, machine re-profile, counters, or intervention.

After M2 PASS: build the cross-runtime EXCESS-COST MAP using M1 Arc shares, M2 llama shares, and closed I003 practical token latencies.

## M2 pre-valid-collection PowerShell native-stderr transport repair — 2026-09-25

At M2 package HEAD `b505053d7e13812020d853af5c10aec25a026dbd`, the first pinned-llama invocation emitted normal Vulkan startup diagnostics to stderr (`ggml_vulkan: Found 1 Vulkan devices:`). Windows PowerShell surfaced that native stderr as `NativeCommandError`; because the runner used `$ErrorActionPreference="Stop"`, the parent script terminated before producing a valid M2 collection.

Repair is infrastructure-only: replace direct `& $Exe ... 1> ... 2> ...` with `Start-Process` using redirected stdout/stderr and explicit `ExitCode`. A zero-science native stderr probe now runs before llama and must prove that stderr text with exit code 0 is captured without terminating the runner.

Pinned llama commit/binary, model, workloads, logger, semantic anchors, probe indices and parser are unchanged. No valid M2 bundle/summary was produced by the failed invocation; it is not scientific evidence.

## M2 pre-rerun parser hardening — 2026-09-25

A deeper pre-rerun audit identified a second prospective infrastructure risk before any valid M2 collection: the Vulkan perf logger may emit the same semantic matmul shape under multiple fusion-name-prefixed logger keys. The original parser rejected the second row as a duplicate even when the aggregate semantic call count was correct.

Repair: aggregate all logger rows by semantic shape/type first, then validate the frozen total call counts. Added a synthetic 64-block perf-log regression where gate/up is split across two logger names (28+28) and must parse to the expected 56 calls. This test runs locally before llama execution.

Scientific payload remains unchanged: exact pinned llama commit/binary, model, W-S/W-C, warmup/measured counts, probes 0/15/30, and perf logger mode.

## M2 pinned llama same-semantic map — PASS / EXCESS-COST MAP — 2026-09-25

Returned bundle SHA256:
`546E07D4C739AC91D28CD2B6F67AAC1DFAB802F8AD28C0CC4BE7A04F359BB9F3`

Independent adjudication:
`PASS_M2_PINNED_LLAMA_SAME_SEMANTIC_MAP`

Integrity:
- exact pinned llama v0.4.1 commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- exact baseline adapter/model hashes;
- W-S/W-C generated hashes match I003;
- 64 Vulkan timing blocks/workload = 32 warmup + 32 measured;
- measured cached decode = final 31 blocks;
- probes 0/15/30;
- all seven semantic call-count contracts exact;
- independent raw-log recomputation differs from summary by 0.

Cross-runtime excess map now covers 4,369,225,728 / 4,370,560,992 logical weight bytes.
Derived geomean Arc/llama excess ratios:
- gate/up split-K ~5.34×;
- FFN-down Q4_K ~15.89×;
- FFN-down Q6_K ~10.02×;
- LM-head Q6_K ~27.56×;
- Q+O Q4_K ~7.57×;
- K+V(Q4) ~29.03×;
- V(Q6) ~20.47×.

Hardware interpretation:
the successful I002 gate/up split-K family is materially closer to llama than the remaining
row-serial-K families. The same-semantic timing + source topology is already sufficient to open
a direct causal work-partitioning discriminator.

M3 quiet-host counters are DEFERRED, not cancelled. They become justified only if the causal
sentinel fails or leaves cache/bandwidth/occupancy attribution unresolved.

Next artifact:
`H1_Q4_DOWN_SPLIT_K_SENTINEL_SPEC`

No implementation is authorized yet.

## M3 re-opened as Token-XRay counter foundation — M3-A capability stage — 2026-09-25

The prior excess-cost map deferred M3 because counters were not needed to choose the next causal
sentinel. M3 is now re-opened for a broader scientific/tooling objective: establish measured
hardware-counter evidence usable by both ArcLLM and Token-XRay HardwareLens.

Token-XRay Phase-3 counter contract:
`minhtri22/token-xray@ba19a34c54c0290cbed2f54f6a1175554bcfae16`.

M3-A is zero-science capability qualification only. It enumerates the active Arc 140V Vulkan
compute queue's `VK_KHR_performance_query` counters and pass requirements, and detects Intel
VTune availability. No model load, inference, counter collection or quiet-host state is consumed.

M3-B will be designed only from the actual provider/counter inventory returned by M3-A.

## M3-A capability qualification — PASS / M3-B opened — 2026-09-25

Bundle SHA256 `6F0A39836896EAB0755B9A3BE54AF057971E90F9ABE47FC4CA1F397D5B533BAA`.

Exact Arc 140V provider result: `VK_KHR_performance_query` present; performance query pools enabled; queue family 0 with 64 timestamp bits; 268 COMMAND-scope counters; all-counter set requires 12 passes; VTune CLI absent. No model, inference or counter collection occurred in M3-A.

Token-XRay counter profile grounded from this evidence: `minhtri22/token-xray@67e34cd8be4b9b36a9b5cc65a6bffad0a576c579`.

M3-B is opened as quiet-host targeted per-dispatch collection using three curated groups (memory/cache, execution/occupancy, stall-cause). Instrumented timing is diagnostic only and cannot replace I003/M1 baseline timing.

## M3-B implementation lock — Vulkan command-scope counter collector — 2026-09-25

M3-B collector is bound to the exact M3-A-qualified Vulkan KHR provider and Token-XRay Phase-3
contract `03e4c7ef5fc0dbba52f99d96669ac40ce18b5e6e`.

The collector profiles decode index 15 for six independent workload/group processes. Every
selected group asks the driver for its own required pass count. Before each required pass the
mutable decode state (working buffers, full K/V cache, feedback token) is restored to the exact
pre-probe snapshot. The performance query pool is reset only before pass 0, then the same semantic
command sequence is submitted once per required pass using `counterPassIndex`. Results are read
only after all passes and labeled `COMBINED_AFTER_REQUIRED_PASSES`.

The profiling lock spans command-buffer recording/executable/pending lifetime. Counter runs require
manual quiet-host confirmation but do not repeat machine profiling. Instrumented timing is forbidden
as a replacement for I003/M1 timing.

Raw collection emits 469 COMMAND-scope dispatch rows/run and the parser emits 2,814 Token-XRay
`HARDWARE_OBSERVATION` JSONL records across W-S/W-C × three counter groups.

### M3-B pre-run Vulkan query-reset validity repair

Static/spec review caught a pre-run Vulkan validity issue before any M3-B data collection. A performance query cannot begin in a command buffer that also resets that same query, and resetting between passes would clear all pass state. M3-B now records/submits one dedicated reset-only command buffer under the profiling lock before pass 0. Each required counter pass then re-records the same semantic workload without a reset and submits it with its counterPassIndex.

No M3-B science payload, model, workload, counter selection or semantic mapping changed.



### M3-A bundle-hash provenance correction

Direct SHA256 of the uploaded M3-A ZIP is `6F0A39836896EAB0755B9A3BE54AF057971E90F9ABE47FC4CA1F397D5B533BAA`. The previously recorded M3-A bundle hash
`546E07D4C739AC91D28CD2B6F67AAC1DFAB802F8AD28C0CC4BE7A04F359BB9F3` was a stale reused context value. All five inner artifact hashes independently match
the prior adjudication, so the capability result and counter inventory are unchanged. Token-XRay
counter profile provenance was corrected at `2e8408f482b9aa5de2f1f0f49133ab8e8ce6af67`.

## M3-B pre-run attribution hardening — 2026-09-25

Before any valid M3-B collection, each COMMAND-scope query was tightened with a
compute-to-bottom-of-pipe barrier before `vkCmdEndQuery`. Counter timing is diagnostic only.

The prior full transient+KV host restore between required passes was removed because the decode
graph overwrites those locations before read and the large host memcpy can perturb shared
LPDDR/cache state on Arc 140V. Only the fixed 4-byte decode token input is restored.

M3-B is rebound to Token-XRay main
`4a60e510b3974aa9273b620285711c035cce665b`; combined Vulkan observations record all executed
counter pass indices.

## M3-B quiet-host hardware counters — PASS / mechanism localized — 2026-09-25

Returned bundle SHA256:
`C425DC6C32794B8214BA9DC3088DFC3279AF8AF32E6241089E5E94E780CCAF42`

Final adjudication:
`PASS_M3B_COMMAND_SCOPE_COUNTER_COLLECTION_WITH_PARTIAL_COUNTER_SUPPORT`

Collection provenance:
- collection HEAD `818c522c8c31d845c0ae6c358a769ebe0da5fc7b`;
- packaging/recovery HEAD `03173d2262f80398efa6297477cd61d486d09693`;
- 6/6 raw runs valid;
- 2,814 COMMAND-scope dispatch observations;
- memory/cache and execution groups require 3 passes; stall-cause requires 2;
- recovery reran no collector/counter science.

Hardware mechanism:
- I002 gate/up split-K: ~1.02x device-read/weight, ~98.3% LSC hit, ~87% ALU1 utilization, ~6% XVE stall.
- Q/O row-serial-K: ~1.08-1.15x device-read/weight and ~99% LSC hit, but ~16% ALU1 and ~71% XVE stall, SBID ~61%.
- Q4 FFN-down: ~3.34-4.40x device-read/weight, ~11% ALU1, ~61-66% XVE stall, SBID ~55-59%.
- Q6 FFN-down: ~1.27-1.36x device-read/weight, ~13% ALU1, ~70-74% XVE stall.
- LM-head: ~2.4-3.7% ALU1, ~82-91% stall/SBID, ~16-33% LSC hit; W-C physical reads are ~19x W-S and are pinned as a separate context-conditioned observation.

Provider limitation:
direct occupancy, GpuTime, and several L3/activity counters are all-zero at COMMAND scope and are
not interpreted. M1/I003 remain timing authority.

H1 residual serial-K penalty is now mechanistically supported but not causally proven. The next
scientific artifact is the Q4 FFN-down split-K sentinel specification, with frozen predictions that
traffic amplification falls, ALU utilization rises, SBID/XVE stall falls, component latency improves,
and semantic correctness remains exact.

## Q4_DOWN_ARCHITECTURE_CAUSAL_DECOMPOSITION — parent freeze — 2026-09-25

M3-B motivates two separable candidate mechanisms for exact Q4_K FFN-down:
(1) row-serial-K work decomposition causing low ALU utilization / high scoreboard stall, and
(2) execution representation causing physical device-read amplification.

A four-arm 2x2 causal study is now frozen before implementation:

```text
0  = Serial-K + storage-native Q4_K
A  = Split-K32 + storage-native Q4_K
B  = Serial-K + one-time CPU materialized RAM image
AB = exact A Split-K32 + exact B image
```

Only the 14 Q4_K down layers are in scope. A cannot change representation; B cannot change
parallelism; AB cannot add a third mechanism. B/AB perform no per-token CPU model math and may not
fully dequantize the Q4_K weights.

Frozen evaluation vector:
component latency; device-read/weight; ALU1 utilization; XVE stall; SBID stall; exact generated
tokens/hash + I002 component correctness limits; materialization time; extra RAM; break-even tokens.

The identity
`L0-LAB = (L0-LA) + (L0-LB) + (LA+LB-L0-LAB)`
will report work-decomposition gain, representation gain, and interaction gain explicitly.

Eight balanced paired timing blocks/workload are frozen. Mechanism counters are collected only
after timing and cannot substitute for timing. Architecture selection will report non-dominated
arms and crossover token horizons rather than assuming an arbitrary usage horizon.

No implementation or fresh performance execution is authorized by this parent freeze.

## Q4-down Child A/B source + data-layout design freeze — 2026-09-25

Child A is the minimum possible transfer: reuse the exact proven SA1 Q4_K subgroup32 source/blob and
SPIR-V unchanged for the 14 Q4_K decode ffn_down nodes. At K=18944, rows=3584 this gives 896
workgroups, four subgroup32-owned rows/workgroup, and 592 K contributions/lane. No width search,
layout change, fusion or prefill change is allowed.

Child B freezes execution image `Q4K_SERIAL_K_EXEC148_V0_1`. Each source 144-byte Q4_K block becomes
148 bytes: raw d/dmin bits, eight direct uint8 scale/min pairs, then 128 bytes packing q values in
strict increasing-K pairs. Serial-K row ownership and k=0..18943 accumulation order remain unchanged.

The 14-layer B image is 549,527,552 bytes. Its format expansion over the source family is only
14,852,096 bytes, but the factorial harness must retain original residency for arms 0/A and prefill,
so architecture cost records the full 549,527,552 incremental resident bytes.

B materializes once on CPU directly into the final mapped UMA Vulkan buffer; no per-token CPU model
math and no full FP16/F32 dequantization. Independent canonical logical-tuple reconstruction
(d/dmin bits, 8 scale/min pairs, 256 q values) must match byte-exact for every block before model
correctness.

AB is mechanically derived: exact A subgroup/lane/reduction geometry + exact B EXEC148 image/reader,
with no AB-only optimization.

Bounded implementation and correctness-only qualification are now authorized. Fresh performance
timing and hardware counters remain unauthorized.



## Q4-down 4-arm correctness canonical freeze — 2026-09-25

Returned correctness bundle SHA256:
`EA6894D76816215D7BF01435C36BE0DB34942B0E4CD9685A3250021B7A6EAB2`.

Formal result:
`PASS_Q4_DOWN_4ARM_CORRECTNESS_QUALIFICATION`.

The exact 0/A/B/AB implementation at `15f77d64dc2ea61751384aec2d1c527d9d0a300f` passed the frozen real-model correctness gate on W-S and W-C. All four arms produced the exact preregistered 32-token workload hashes and passed dispatch census. B is component-bit-equivalent to arm 0; A/AB remain far inside the inherited max_abs/RMSE limits while final hidden/logit hashes differ as preregistered for a changed FP32 reduction order.

EXEC148 logical validation is exact for all 14 target tensors; source and execution-image canonical family SHA256 are both
`60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2`.

No latency, materialization timing, validation timing, hardware counter, Token-XRay or Child-C evidence is opened by this result. Correctness is now closed and does not require rerun.

Canonical evidence:
`ARCLLM_V1_Q4_DOWN_4ARM_CORRECTNESS_CANONICAL_v0.1.json`.

Next:
open a separate authorization for the already-preregistered four-arm primary timing campaign. Mechanism counters remain a later stage.


## Q4-down 4-arm primary timing authorization — 2026-09-25

Correctness canonical evidence at commit `b109b0ac974ddb6995548527a08878074cab9400` independently closes the required hard gate. A separate authorization now opens only the preregistered primary performance stage.

Authorized now:
- bounded timing-harness implementation and zero-science QA;
- fresh 0/A/B/AB primary timing on W-S and W-C;
- exactly 8 paired measured blocks/workload with the frozen arm order;
- calibrated Q4-down component timing exactly as preregistered;
- one-time materialization time and separate validation time for architecture-cost accounting.

Still closed:
- the targeted four-native-counter mechanism campaign;
- Token-XRay;
- Child-C;
- any new kernel/layout/subgroup-width search;
- any arm, workload, metric, threshold or mechanism change after correctness.

Primary timing by itself may establish latency ratios/gains and timing composition evidence, but it cannot close the full preregistered A/B mechanism-support claims because those still require the later counter stage.

Authorization:
`arcllm_v1_q4_down_4arm_primary_timing_authorization_v0.1.json`.

Gate:
`PASS_OPEN_PRIMARY_4ARM_TIMING`.

Next:
implement and freeze the exact bounded performance execution package before the first measured block.


## Q4-down 4-arm primary timing execution package — implementation freeze candidate — 2026-09-25

Canonical four-arm correctness is frozen and primary timing is separately authorized under the
pre-registered 2x2 design. A bounded timing surface is now implemented without changing any arm,
workload, shader mechanism, EXEC148 representation, correctness threshold, or analysis threshold.

The measurement surface places calibrated Vulkan timestamp queries only around the exact 14 Q4_K
FFN-down dispatches on each of the 31 decode steps. The primary per-attempt statistic remains the
median of those 31 family sums. The exact eight-block arm order is embedded in the harness.
Hardware performance counters, Token-XRay and Child-C remain closed.

EXEC148 materialization and logical-tuple validation wall times are recorded separately. W-S is
pre-designated as the canonical architecture-cost reference; W-C records an integrity replicate.
No measured primary timing has been executed at this stage.

Next gate: zero-science static/native-build qualification of the exact package. Only a PASS may
freeze the execution lock and permit the first measured block.


## Q4-down 4-arm primary timing package — static PASS / CI infra-blocked native build — 2026-09-25

Bounded primary-timing implementation candidate is frozen at implementation head `3ce860465ffa4a16bc2cc49b6f22e2b39dc08532`. Independent static audit PASS confirms exact four-arm order, mechanism routing equality with the correctness harness, 14-target-only timestamp scope, 31 decode samples/attempt, calibrated timestamp-period path, separate materialization/validation wall times, and no native performance-counter/Token-XRay/Child-C path.

GitHub Actions run `36103464653` failed twice before any workflow step was allocated. This is CI infrastructure only, not implementation or scientific failure.

Candidate execution lock v0.1 remains `execution_authorized=false`. Backlog/gate: obtain one DEV_HOST zero-science native build-only PASS and independently review its return bundle. Only then may immutable execution lock v0.2 authorize the first measured timing block.


## Q4-down 4-arm primary timing — pre-measurement MSVC compile repair — 2026-09-25

DEV_HOST zero-science build reached static QA and full frozen shader provenance PASS, then stopped before executable creation with MSVC C3493 in the `q4_names` initializer lambda. No model load, GPU dispatch, timing, counter collection, or performance outcome occurred.

The repair changes only the lambda capture list from `[]` to `[&]` so the already-frozen `Q4_DOWN_LAYERS` set can be read while constructing the exact 14 target names. No mechanism, arm routing, decode graph, measurement scope, campaign order, threshold, or analysis rule changed. Candidate execution remains unauthorized pending a fresh DEV_HOST native build-only PASS.


## Q4-down 4-arm primary timing — DEV_HOST build-only PASS / exact execution authorized — 2026-09-25

DEV_HOST zero-science native build-only PASS at source head `875b45004d3e16b694731a67ed47153c316be1f6`. Returned bundle SHA256 `ACDE58AE0269D6233FD13BB013E08403B82423F78B66A338BD9D65C21FB95230`. Qualified executable SHA256 `233343628E6D08E0374693613CD3512DECD34A270995B44442AD34C99E727364`, 475,648 bytes. No model load, GPU dispatch, primary timing or hardware-counter science occurred.

Build-only evidence and shader provenance are canonicalized. Final execution lock v0.2 authorizes only the already-preregistered 0/A/B/AB primary timing campaign. The runner verifies the exact qualified binary and no longer rebuilds it before measurement; it reruns only static QA and deterministic shader compilation/provenance, then checks the frozen DEV_HOST environment/model.

Native counter campaign, Token-XRay, Child-C and all post-hoc mechanism/layout/threshold search remain closed. Next: one complete exact primary timing collection; no selective reruns after valid measured outcomes are exposed.


## Q4-down 4-arm primary timing — canonical PASS / antagonistic interaction — 2026-09-25

Returned primary timing bundle SHA256 `B16EE3747D1B64F4B28E449003749B18DD3B8468D03DBD606CA36AB41B292F2C`. Independent audit validates all 64 measured attempts, exact frozen arm order, 31 decode samples/attempt, generated hashes, dispatch census, EXEC148 tuple identity, and zero counter/Token-XRay/Child-C leakage.

Workload medians (ms/token for the 14 Q4-down family): W-S `0=100.5810, A=38.3734, B=24.1891, AB=34.4086`; W-C `0=213.3338, A=37.8352, B=31.4130, AB=34.2426`. A/0 and B/0 latency CIs are fully below 1 in both workloads. AB/A is below 1, but AB/B is above 1 in both workloads. `G_INT` is negative with 95% CI fully below zero in both workloads. Timing-stage result is therefore `ANTAGONISTIC_INTERACTION`; preregistered composition support fails.

Architecture-cost frontier is A vs B: arm 0 is dominated by A and AB is dominated by B. Using the frozen canonical materialization time 231.6382 ms, the preregistered A-vs-B total-cost crossover is 16.3307 tokens for W-S and 36.0687 tokens for W-C. This fills an omitted deterministic report item in the returned adjudicator; no raw result, threshold, or rule is changed.

Arm 0 timing is descriptively variable, especially W-C; no post-hoc exclusion or rerun is performed. Full A/B mechanism-supported claims remain pending the preregistered native-counter stage.

A separate counter authorization now opens only bounded implementation + zero-science QA for the exact four native counters at decode index 15. Fresh counter execution remains locked until an exact counter package is frozen. Token-XRay and Child-C remain closed.


## Q4-down 4-arm native-counter package — bounded implementation candidate — 2026-09-25

Primary timing evidence is frozen with antagonistic interaction; full A/B mechanism claims remain pending the preregistered counter evidence. A bounded native-counter package is implemented without reopening timing or changing any 0/A/B/AB mechanism.

The collector reuses the proven M3-B `VK_KHR_performance_query` multipass contract (profiling lock, dedicated reset-before-pass0, driver-derived pass count, `counterPassIndex`, compute-to-bottom-of-pipe query end, and only 4-byte decode-token host restore). The exact 469-dispatch decode graph still executes, but performance queries are opened only around the 14 frozen Q4_K `ffn_down` dispatches. The selected counter set is exactly indices `{56,66,64,233}` and the only counter probe is decode index 15.

Each arm/workload is one independent process with one unmeasured warmup and one measured inference; the complete campaign is exactly 8 probes. Counter timing remains diagnostic only. Primary timing rerun, Token-XRay, Child-C, new mechanisms/arms, and post-hoc counter selection remain forbidden.

Fresh counter execution is not authorized by this implementation commit. Next gate: independent static audit + DEV_HOST native build-only PASS, followed by an immutable execution lock.


## Q4-down 4-arm native-counter package — independent static audit PASS / candidate lock — 2026-09-25

Independent structural audit PASS on implementation payload `3f7f9eca6ea5e9d9fd9f282f1769bcc8f8381119`: exact four-arm routing/decode graph matches the frozen timing harness byte-for-byte; counter set is exactly 56/66/64/233; probe index is exactly decode 15; performance query begin/end is limited to the 14 frozen Q4-down nodes while the full 469-dispatch graph executes for state correctness. M3-B multipass/reset/profiling-lock semantics are retained.

Candidate execution lock v0.1 is frozen with `counter_execution_authorized=false`. Native Windows build remains unqualified at this point. No model load, GPU dispatch, performance query, primary timing rerun, Token-XRay or Child-C execution has occurred.

Next gate: DEV_HOST zero-science native build-only and return bundle for independent review. Only then may final lock v0.2 authorize the eight probes.


## Q4-down 4-arm native-counter package — pre-collection static-QA repair — 2026-09-25

DEV_HOST build-only stopped in the Python static test before shader build, native compilation, model load, GPU dispatch, or counter execution. The failing assertion incorrectly required the literal `counterPassIndex` to appear in the targeted runtime source. In the proven M3-B layering, the targeted runtime calls `m3_perf_submit_pass(...)`; the unchanged counter shim constructs `VkPerformanceQuerySubmitInfoKHR` and assigns `counterPassIndex`.

The repair changes only the static test: runtime must contain `m3_perf_submit_pass`, while the unchanged M3-B shim must contain `VkPerformanceQuerySubmitInfoKHR` and `counterPassIndex`. Collector, runtime, shim, counter set, decode index, 14-node target scope, campaign order, and all scientific payload remain unchanged. Candidate execution remains unauthorized pending a fresh DEV_HOST build-only PASS.


## Q4-down 4-arm native counters — DEV_HOST build-only PASS / exact eight-probe execution authorized — 2026-09-25

DEV_HOST zero-science native-counter build-only PASS at HEAD `c8f7495fa166c9758c9c145fa82dc30c6cdf330b`. Return bundle SHA256 `22D57FB624C0F46D2933CE38F5AE2E68ED1798338AC810EF730E1C6C1956E28F`. Qualified executable SHA256 `D0DCA49801D9C4A817CCF9DC3E60C8FD221760FACBAB4BCC6B952836AE9F1B42`, 482,304 bytes. No model load, GPU dispatch, counter probe or primary-timing science occurred during qualification. MSVC C4996 `strncpy` deprecation warnings in the reused M3-B shim were non-blocking and do not change the scientific payload.

Build-only evidence and shader provenance are canonicalized. Final counter execution lock v0.2 authorizes exactly eight independent arm-workload probes: W-S and W-C crossed with 0/A/B/AB, one warmup and one measured inference per process, with the only performance query at decode index 15 and only the 14 frozen Q4-down dispatches queried for counters 56/66/64/233. Driver-derived multipass semantics remain unchanged.

The execution runner verifies the exact qualified binary, exact environment/model, quiet-host confirmation and critical blobs, and writes a one-shot science-start marker before the first process. Primary timing rerun, Token-XRay, Child-C, new mechanisms/arms, post-hoc counter selection and threshold changes remain closed.


## Token-XRay core v0.1 completeness pin — provenance update — 2026-09-25

Token-XRay is pinned for future ArcLLM instrumentation revalidation at `minhtri22/token-xray` branch `freeze/token-xray-core-v0.1-completeness-validated`, commit `2359f68175bf2e231722c1fe6694ebd2e2d11aeb`. The freeze artifact records tested core head `100ccb070d12a0c34ecd241015bb1b1f62761d58` with `PASS_LOCAL_CORE_COMPLETENESS_QA`; release remains blocked pending a fresh ArcLLM one-token runtime revalidation.

Frozen acceptance for that future validation is: 469 dispatches, 469 timestamped, 451 measured semantic nodes, 451 exact timing nodes, no unknown semantic IDs, no unmapped dispatch IDs, and execution domain `gpu.arc_140v` propagated to every measured semantic node. The hardware profile may represent CPU/GPU/NPU independently, but this ArcLLM Vulkan path must not imply NPU execution or aggregate cross-domain TOPS.

This update does not modify or reopen the currently authorized Q4-down eight-probe native-counter campaign. Token-XRay remains disabled in that campaign. The fresh one-token Token-XRay revalidation is a separate future instrumentation-validation task, not an ArcLLM mechanism study.


## Q4-down 4-arm native counters — post-collection adjudicator recovery — 2026-09-26

All eight authorized counter processes completed before adjudication: W-S and W-C crossed with 0/A/B/AB, each reporting driver pass_count=2. The original adjudicator then stopped at `W-S/0 shader`.

Source inspection classifies this as a post-collection QA-harness mismatch, not a counter-collection failure. The collector emits `op.spv_path`, which is constructed by `join_path_p8c(shader_dir, sh)` and therefore contains the full compiled-shader path. The original parser compared that field directly with the frozen shader basename. No counter value or mechanism is implicated by this mismatch.

No collector or selective probe rerun is permitted. A separate recovery parser changes only shader identity comparison to normalized basename equality. A recovery script reads the original `result_dir` from the preserved science-start marker, requires all eight raw PASS files, hashes them before and after adjudication to enforce byte identity, and packages the existing collection. Primary timing, Token-XRay, Child-C, counter selection and mechanisms remain unchanged.


## Q4-down native counters — second post-collection parser repair — 2026-09-26

The first recovery attempt also stopped before adjudication at `W-S/0 shader`. The v0.1.1 parser normalized `\\\\` (two consecutive backslash characters) rather than each single Windows path separator. After JSON decoding, the emitted shader path contains one backslash per separator, so the basename comparison still failed.

Recovery parser v0.1.2 uses a dedicated `shader_basename()` helper that replaces each single backslash with `/` and takes the final component. A full Windows `compiled_shaders` path regression is embedded. No collector, counter probe, primary timing, mechanism, counter selection, or raw evidence is changed. The existing collection remains the only scientific evidence and recovery-only execution remains required.
