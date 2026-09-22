# ArcLLM v1 — Foundation QA Checklist

**Date:** 2026-09-22  
**Branch:** `research/arcllm-v1`  
**QA parent HEAD:** `c82bd2ede3ba6af9cb06a24a1a12461835750f8f`  
**Scope:** methodology → architecture reframe → historical Headroom Map → I001 selection  
**Execution scope:** documentation / prior-art / semantic QA only; no performance execution and no I001 implementation

## Final state

```text
QA_RESULT=PASS
total_findings=20
resolved_findings=20
open_findings=0
blocking_open=0
major_open=0
minor_open=0
paper_transfer_gaps_open=0
count=0
```

## Patched canonical documents

| Document | Pre-QA blob | Post-QA blob |
|---|---|---|
| `ARCLLM_V1_ARCHITECTURE_LEARNING_METHODOLOGY.md` | `d41a357840435f52a682792b9a6608e4abaab312` | `bd943fe99e0a6758f8e0aa8060f9540a0b74a942` |
| `ARCLLM_V1_ARCHITECTURE_REFRAME.md` | `e410422551b982f3e7ea94a160b88d6edc2cfbf4` | `5f06a648d200366066e79c9355ae3bb448e1c8fb` |
| `ARCLLM_V1_HEADROOM_MAP.md` | `615ebec786517573b1c1e7fd3a9c17276f386675` | `e66a296134d5f9701a6f6b63a5954f52417b71cc` |
| `ARCLLM_V1_INTERVENTION_001_SELECTION.md` | `95340b858ac4fa302c312959937e12c049e07402` | `c76ceb06e00bbea731bb575f6646febc39eee466` |

## Finding checklist

| ID | Severity | Finding before patch | Resolution | Status |
|---|---|---|---|---|
| F01 | BLOCKING | Amdahl/CTR/Priority equations contained escaped control-character corruption from `\frac/\times/\to` serialization. | Replaced affected sections with clean Markdown/LaTeX and byte-audited for control characters. | RESOLVED |
| F02 | MAJOR | `ObservedGap = StructuralGap + MaturityDebt + MeasurementUncertainty` could be read as an identifiable additive equation. | Reframed as an explanatory taxonomy; explicitly states non-additivity/non-independence and permits `UNRESOLVED`. | RESOLVED |
| F03 | MAJOR | `StructuralGap`, `MaturityDebt`, `MeasurementUncertainty` looked like established literature terms. | Marked them as ArcLLM-v1 project-defined analytical terms and added operational definitions. | RESOLVED |
| F04 | MAJOR | `Headroom Map`, `Carry-Through Ratio`, `Architecture Epoch`, `PDEP`, and `graph compression` lacked terminology provenance. | Added project-term provenance and explicit non-standard status. | RESOLVED |
| F05 | MAJOR | `execution regime`, `mature baseline`, `structural advantage`, `structural ceiling`, `material`, and `confidence` were used without exact operational definitions. | Added a shared operational term contract with claim-strength requirements and non-meanings. | RESOLVED |
| F06 | MAJOR | Evidence status was mixed: measured values, derived ratios, bounds, and hypotheses were not explicitly distinguished. | Added `MEASURED / DERIVED / BOUND / HYPOTHESIS` evidence classes. | RESOLVED |
| F07 | MAJOR | `confidence` could be confused with a statistical confidence interval. | Defined LOW/MEDIUM/HIGH as evidence-quality categories; statistical CI must be named explicitly when used. | RESOLVED |
| F08 | MAJOR | `recoverable headroom` and total/system headroom were conflated in the initial map. | Split `system headroom envelope`, `attributed recoverable headroom`, and `unattributed gap`. | RESOLVED |
| F09 | MAJOR | Headroom table labeled decode recoverable headroom `VERY HIGH`, stronger than causal evidence supported. | Changed to `system envelope VERY HIGH; attributable recoverable share UNKNOWN` and lowered causal-confidence wording. | RESOLVED |
| F10 | MAJOR | ANL64 was worded as proving generic multi-x decode recoverability, despite bundled successor changes. | Narrowed claim: integrated ArcLLM-family successor shows multi-x movement; individual mechanism attribution remains unresolved. | RESOLVED |
| F11 | MAJOR | `large decode maturity debt is supported` over-attributed the integrated ANL64 result. | Replaced with `integrated recoverability supported; maturity-debt subclass unresolved`. | RESOLVED |
| F12 | MAJOR | Architecture document could be read as describing already-implemented v1 modules. | Explicitly labeled it a logical research architecture and defined each architecture object. | RESOLVED |
| F13 | MAJOR | `execution plane`, `Execution Graph IR`, `kernel portfolio`, `execution region`, `persistent/reusable execution`, and `residency` were ambiguous. | Added architecture-object contract; clarified logical vs physical meanings. | RESOLVED |
| F14 | MAJOR | `persistent` could be interpreted as requiring one always-running persistent kernel. | Defined persistence broadly as reusable prepared/GPU-side state; single infinite kernel is not required. | RESOLVED |
| F15 | MAJOR | `decode graph compression` could sound like semantic graph deletion. | Defined it as reducing explicitly scheduled execution regions while preserving the semantic graph. | RESOLVED |
| F16 | MAJOR | I001 hypothesis prematurely implied graph fragmentation/persistence was already the cause. | Rewrote H-I001 conditionally: implementation is justified only if a material attributable execution-topology term is measured. | RESOLVED |
| F17 | MAJOR | I001 lacked explicit competing mechanism alternatives and a null. | Added H-LAUNCH/LIFECYCLE, H-SYNC, H-LOCALITY, H-KERNEL, H-MIXED, and H-NULL-I001. | RESOLVED |
| F18 | MAJOR | CUDA/NVIDIA serving prior art could be silently transferred to Intel Arc/Vulkan. | Added transfer-limit language: prior art motivates measurement classes only; all Arc/Vulkan mechanisms require independent discrimination. | RESOLVED |
| F19 | MAJOR | Foundational documents lacked an explicit paper/prior-art basis for Amdahl, bounds, IO/locality, KV layout, and scheduling. | Added prior-art foundation/mapping covering Amdahl, Roofline, FlashAttention/2, PagedAttention/vLLM, Orca, Sarathi-Serve, FlashInfer, NanoFlow, and DeepSpeed-FastGen. | RESOLVED |
| F20 | MINOR | Derived documents still bound the pre-QA methodology blob and did not expose exact QA-amended derivation identities. | Rebound architecture → methodology, Headroom → methodology+architecture, and I001 → methodology+architecture+Headroom exact blobs. | RESOLVED |

## Prior-art review

The literature review was used to clarify concepts and transfer limits, **not** to import NVIDIA/CUDA results as ArcLLM evidence.

| Work reviewed | Venue/year | Relevance to ArcLLM v1 | QA disposition |
|---|---|---|---|
| Gene M. Amdahl, *Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities* | AFIPS 1967, DOI 10.1145/1465482.1465560 | system speedup ceiling from unimproved fraction | FOUNDATION |
| Williams, Waterman, Patterson, *Roofline* | CACM 2009, DOI 10.1145/1498765.1498785 | compute/data-movement bound discipline | FOUNDATION |
| Dao et al., *FlashAttention* | NeurIPS 2022, DOI 10.52202/068431-1189 | IO-aware optimization and wall-clock vs FLOP distinction | FOUNDATION / ATTENTION-SCOPED |
| Dao, *FlashAttention-2* | ICLR 2024 | work partitioning/occupancy can leave headroom after a strong implementation | FOUNDATION / HARDWARE-TRANSFER LIMITED |
| Kwon et al., *Efficient Memory Management for LLM Serving with PagedAttention* | SOSP 2023 / arXiv:2309.06180 | KV layout/fragmentation as a system object | FOUNDATION / SERVING-REGIME LIMITED |
| Yu et al., *Orca* | OSDI 2022 | iteration-level scheduling for autoregressive models | FOUNDATION / DISTRIBUTED-SERVING LIMITED |
| Agrawal et al., *Sarathi-Serve* | OSDI 2024 | prefill/decode interference and latency-throughput coupling | FOUNDATION / MULTI-REQUEST LIMITED |
| Ye et al., *FlashInfer* | MLSys 2025 | specialization, KV formats, scheduling, graph-compatible execution | FOUNDATION / CUDA TRANSFER LIMITED |
| Zhu et al., *NanoFlow* | OSDI 2025 | operation-level scheduling/intra-device overlap | FOUNDATION / THROUGHPUT-REGIME LIMITED |
| Holmes et al., *DeepSpeed-FastGen* | arXiv:2401.08671 (2024) | prompt/generation composition, persistent vs non-persistent serving | ADJACENT PRIOR ART |
| Chen, *Memory-Bound but Not Bandwidth-Limited: The Physical AI Inference Gap in Batch-1 LLM Decode* | arXiv:2605.30571 (2026) | recent batch-1 evidence that launch-side benefit can be strongly hardware-dependent | REVIEWED; NOT PROMOTED TO FOUNDATION |
| *Improving the Performance of LLM Inference: Understanding the Role of CUDA Graphs* | IEEE LANMAN 2026, DOI 10.1109/LANMAN69841.2026.11623563 | current evidence that graph benefit depends on model/system configuration | REVIEWED; CUDA/DISTRIBUTED TRANSFER LIMITED |

### Prior-art conclusion

The papers support four methodological requirements:

1. system share matters before local speedup (Amdahl);
2. compute and data movement require explicit bounds rather than intuition (Roofline / FlashAttention);
3. memory layout, scheduling, work partitioning, and phase composition are first-class system variables (PagedAttention, Orca, Sarathi, FlashInfer, NanoFlow, FastGen);
4. graph/launch optimizations are hardware- and regime-dependent, so PDEP must be discriminated on Intel Arc/Vulkan rather than imported from CUDA evidence.

They do **not** establish that PDEP is correct for ArcLLM.

## Static QA after patch

```text
control_characters(methodology)=0
control_characters(architecture)=0
control_characters(headroom)=0
control_characters(intervention)=0

methodology_math_frac_tokens=4
methodology_math_propto_tokens=1
headroom_math_frac_tokens=2

architecture→methodology_binding=PASS
headroom→methodology_binding=PASS
headroom→architecture_binding=PASS
intervention→methodology_binding=PASS
intervention→architecture_binding=PASS
intervention→headroom_binding=PASS

project_term_provenance=PASS
evidence_class_contract=PASS
headroom_envelope_vs_recoverable_split=PASS
ANL64_claim_scope=PASS
PDEP_definition=PASS
graph_compression_definition=PASS
H_NULL_I001_present=PASS
Intel_Arc_Vulkan_transfer_limit=PASS
I001_implementation_authorized=false
```

## Remaining ambiguity / debt

None is left at the **foundation-document QA level**.

This does not mean I001 is scientifically supported. It means the documents are now sufficiently explicit to specify the next discriminator without relying on undefined terminology or imported causal claims.

The following unknowns are intentionally carried into the next study rather than treated as documentation defects:

- exact decode cost decomposition;
- attributable dispatch/lifecycle cost;
- barrier/synchronization cost;
- memory/locality cost;
- kernel-compute dominance;
- persistent-state opportunity;
- unattributed remainder;
- numeric materiality threshold for I001;
- TTFT non-regression threshold for the future integrated experiment.

Those must be preregistered in:

`ARCLLM_V1_I001_DECODE_COST_MODEL_AND_MECHANISM_DISCRIMINATOR.md`

## Final counters

```text
total_findings=20
resolved_findings=20
open_findings=0
count=0
```
