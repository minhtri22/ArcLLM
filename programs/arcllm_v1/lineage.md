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
