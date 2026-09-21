# ANL64 — Transfer Admissibility Contract

**Date:** 2026-09-21  
**Status:** BINDING BEFORE IMPLEMENTATION

## 1. Purpose

This contract prevents two symmetric errors:

1. re-proving well-established mechanisms unnecessarily;
2. importing external or NEXUS results as if they were local ArcLLM-NG evidence.

Every borrowed mechanism must be classified before implementation.

## 2. Transfer classes

### EXTERNALLY_ESTABLISHED
The mechanism/principle is supported by peer-reviewed/public prior art. ANL64 does not need to re-prove its existence in principle.

### SOURCE_VERIFIED
A concrete source implementation has been inspected and pinned by repository commit/blob.

### TRANSFERABLE_INVARIANT
The claim is independent enough from the original hardware/workload to be used as an architecture/design fact.

### LOCAL_COMPATIBILITY_REQUIRED
The mechanism depends on API/hardware/layout/precision assumptions that differ on Intel Arc/Vulkan.

### LOCAL_EFFECT_REQUIRED
Even when compatible, ANL64 must measure its effect locally before any performance claim.

### NOT_TRANSFERABLE
The claim cannot be used as positive ANL64 evidence.

## 3. Internal transfer matrix

| Mechanism/evidence | Inherited status | ANL64 use | New local proof required |
|---|---|---|---|
| ArcLLM exact-model E2E feasibility | established | baseline system fact | no |
| Legacy practical advantage | not established | negative boundary | no |
| Q4 subgroup32 split-K component result | positive in frozen Q4 scope | candidate Q4 executor | composition correctness + local integrated effect |
| unchanged Q4→Q6 split-K | falsified within SA1 | forbidden universal executor assumption | no rescue |
| Q6CB causal hypotheses | no result | none | cannot infer |
| Event Ledger direct frontier | confirmed in sparse NEXUS regimes | control-plane principle when analogous dynamic work exists | mapping + local compatibility + effect |
| Ledger64 region queue/mask semantics | structural + semantic PASS | descriptor/region representation candidate | local mapping correctness |
| Ledger64 performance | unresolved | no positive performance evidence | fresh local effect if used |

## 4. External transfer matrix

| Prior art | What may be inherited | What may not be inherited |
|---|---|---|
| FlashDecoding++ | shape/hardware-dependent dataflow principle; flat-GEMM specialization | reported NVIDIA/AMD speedups |
| FlashInfer | plan/run separation; multiple backend selection pattern | CUDA kernel performance/compatibility |
| Marlin | load/dequant/compute co-design; layout preprocessing; buffering/work partition principles | tensor-core CUDA implementation effect |
| QServe/OmniServe | dequant/layout/system co-design principle | W4A8KV4 precision contract or its accuracy/performance |
| llama.cpp Vulkan | concrete Vulkan quant-aware implementation reference | assumption that its choices are optimal for ANL64 |
| vLLM fusion system | conditional fusion/backend selection pattern | hardware-specific fusion speedups |

## 5. Source-reuse provenance labels

Any implementation borrowed from prior art must declare exactly one:

- `INHERITED` — same internal project code/evidence lineage, no semantic redesign.
- `ADAPTED` — source-derived implementation changed for ANL64.
- `REIMPLEMENTED_FROM_PAPER` — mechanism reimplemented from description, not copied source.
- `INSPIRED_ONLY` — architectural inspiration; implementation independently designed.

For `ADAPTED`, the exact upstream repository, pinned commit, files and license must be recorded before merge.

## 6. Ledger64-specific rule

ANL64 must not force a 64-wide representation merely because Ledger64 exists.

P2 must identify a natural ANL64 control-plane object for region width 64, such as:
- execution descriptors;
- tensor tiles;
- residency/prefetch blocks;
- another explicitly justified unit.

If no natural mapping preserves semantics and avoids extra scan/sort/repack, Ledger64 physical layout is rejected for that layer. Event-Ledger-style planning may still remain as a control-plane concept.

## 7. No duplicate proof rule

If a mechanism is `EXTERNALLY_ESTABLISHED` or an internal `TRANSFERABLE_INVARIANT`, ANL64 must not spend a scientific experiment merely to show the same abstract mechanism again.

ANL64 experiments must instead test one of:
- local compatibility;
- composition correctness;
- local causal effect;
- end-to-end value.

## 8. Performance claim rule

No external or NEXUS performance ratio is admissible as an ANL64 result.

A performance claim requires fresh ANL64 evidence under a frozen target contract.

## 9. Implementation gate

Implementation remains forbidden until P2:
1. freezes the architecture;
2. identifies each inherited/adapted component and provenance label;
3. produces a static/E2E upper-bound argument;
4. defines falsification and stop conditions;
5. passes zero-science architecture QA.
