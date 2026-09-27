# ANL64 CRT P1 — Prior Evidence and Source Review

This review is fresh for the canonical-transfer successor and is limited to transfer admissibility. It does not reuse historical timing as confirmatory evidence.

## Historical ANL64

The immutable plan encodes 469 decode PlanNodes and 24,104 Region64 descriptors. Exactly 140 nodes were marked Q4FastFixed: QProj, KProj, OProj, FfnGate and FfnUp for each of 28 layers.

Historical P6 combined the immutable plan with executor substitutions. It did not isolate an independent plan-only performance effect. Therefore its timing cannot answer the current incremental-value question.

## Canonical runtime at successor origin

The canonical full runtime already executes FfnGate and FfnUp through the same subgroup split-K mechanism carried through by I002. Those 56 historical ANL64 fast nodes are no longer an ANL64-specific intervention.

Canonical QProj, KProj and OProj still use the safe Q4 decode executor. These 84 nodes are the residual historical ANL64 Q4_FAST execution surface.

Canonical Q4 FfnDown is now governed by Q4VulkanBackendV4 with represented A/B acquisition/lifecycle semantics. Historical ANL64 classified FfnDown as QuantSafe rather than Q4FastFixed. Therefore Q4VulkanBackendV4 is orthogonal to the historical ANL64 fast-node intervention and must remain untouched.

## TTFT evidence

TTFT_M2 obtained a fresh valid 80-observation collection and supported H_NULL for the registered TTFT mechanism set. The old ANL64 P7 TTFT negative remains historical evidence, but it is not a license to presume either harm or safety in the materially changed canonical runtime. CRT P1 therefore retains a fresh TTFT blocking guard.

## Transfer design implication

A two-arm current-vs-ANL64 comparison would confound immutable-plan cost/effect with residual Q/K/O executor substitution.

The minimum discriminating design is therefore A/B/C:

- A: current canonical runtime.
- B: exact ANL64 plan present and structurally consumed in shadow mode, while canonical executors remain unchanged.
- C: same plan plus only the residual 84 Q/K/O Q4_FAST substitutions.

B-A isolates plan/control-plane presence under unchanged executor choices. C-B isolates residual executor effect. C-A measures total transfer value.
