# ANL64 ORIGIN — ArcLLM + NEXUS / Ledger64

**Date:** 2026-09-21  
**Program:** ANL64  
**Mode:** NEW PROGRAM / SPECIFICATION-ONLY

## 1. Parent closure

ANL64 starts only after the previous ArcLLM successor line reached a terminal program-level decision:

- parent terminal head: `d76295cbb3da116eed28f2f6b5f77ce89f5bce2e`
- program decision blob: `745fbbd4e92d3aadf9db2eb8862c51ca6386f363`
- decision: `STOP_CURRENT_SUCCESSOR_LINE`

ANL64 is therefore **not**:
- SA-H1 continuation;
- Q6CB rescue;
- Q3 reopen;
- a renamed Q6 mechanism program.

## 2. ArcLLM evidence inherited as immutable prior evidence

### Established

1. Exact-model ArcLLM execution is feasible and reproducible on the target system.
2. The validated legacy architecture closed `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`.
3. Q4 subgroup-32 split-K component evidence is positive in its frozen Q4 scope (~3.14x geomean in both processes).
4. The unchanged Q4 mechanism did not satisfy the frozen Q6 correctness contract.
5. No end-to-end claim was established for the SA-H1 successor.
6. Q6CB produced no causal result because it terminated `STOP_INFRASTRUCTURE_UNSTABLE`.

### Consequences

ANL64 may reuse bounded evidence but may not:
- relabel Q4 PASS as an E2E PASS;
- infer a Q6 mechanism from Q6CB;
- assume one quant kernel must generalize across quant families;
- restart the previous successor line.

## 3. NEXUS / Event-Ledger / Ledger64 evidence inherited

The NEXUS evidence source is the separate branch `research/h2-event-ledger-d64-layout`.

Admissible inherited evidence:

- Event Ledger EL-L4: `CONFIRMED` under its tested sparse NEXUS regimes; formal adjudication blob `af3cce47905fd94a7109e462c6b128a05dc028b2`.
- Cross-branch review: direct frontier maintenance promoted as a NEXUS architecture candidate under its own governance; blob `6dea67ac30d7518a0c14659a7859b2e963ca4c67`.
- Ledger64 LD64-0: `PASS_STRUCTURAL`; blob `26c3d2a107622cc633d640d954f0a21523108ff3`.
- Ledger64 LD64-1: `CLOSED_PASS_SEMANTIC`; blob `ba641d1a5353a36272c66ddd3ebb3cb1f4245ce1`.
- Ledger64 LD64-2 performance: **UNRESOLVED** due measurement-sanity tails; postmortem blob `7de95b80e2d58f0ecdb82ba801ca10e0fcf97bd9`.

Therefore ANL64 may inherit Ledger64 representation/semantic ideas but **must not** inherit a Ledger64 performance claim for dense LLM inference.

## 4. New scientific question

> Can an exact-model LLM runtime separate control-plane execution planning/representation from quant-specific data-plane execution, using NEXUS/Event-Ledger/Ledger64-derived structures where transfer-admissible, such that heterogeneous Q4/Q6 and other operator families can use different correct executors while preserving exact GGUF semantics and producing material end-to-end improvement on the target Intel Arc/Vulkan system?

## 5. Initial architecture boundary

```text
NEXUS / Ledger64-derived control plane
  - execution-plan representation
  - work/region descriptors
  - dependency/frontier authority where justified
  - resource/residency/prefetch plan
  - executor selection

ArcLLM data plane
  - exact GGUF semantics
  - quant-specific executors
  - attention/KV operators
  - fused operator clusters only when separately admitted
  - Vulkan execution
```

Ledger64 is **not** assumed to be a sparse LLM executor. Direct transfer of sparse-NEXUS performance claims is forbidden.

## 6. Immediate stop boundary

No implementation or target science is authorized by this ORIGIN. The next allowed work is prior-art/source review and transfer-admissibility specification.
