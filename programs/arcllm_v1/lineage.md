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
