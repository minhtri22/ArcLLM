# SA1-P Independent QA

**Date:** 2026-09-21  
**Preregistration commit:** `e98650ec3b2e0f0c8fa3168f3d97aed8ea599896`  
**Result:** **PASS**

Independent review recomputed all seven frozen packed-GEMM shape cells and verified 7/7 row-byte and packed-weight-byte calculations.

The preregistration commit changed only docs/config/governance. No `src/`, `shaders/`, build tools, tests or runner were added or modified.

Baseline identity is exact:

```text
Q4 Git blob   fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a
Q4 Q2 SPIR-V  2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A

Q6 Git blob   a0de99f972db6cd202606aad95fb9eab223639e6
Q6 Q2 SPIR-V  F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67
```

The single primary mechanism is unambiguous: subgroup-32 split-K per output row. Cooperative matrix, FP16/INT8 staging, fusion and geometry search are forbidden.

The four-bank worst-case Q6-down packed weight allocation is 222,781,440 bytes (~212.46 MiB), below the frozen 256 MiB per-cell process cap before the small activation/output/bias allocations.

Correctness, measurement, replication, performance and stop gates are all frozen before candidate data. Q6 remains blocked until an independently adjudicated Q4 PASS.

Therefore SA1-P may advance to an implementation lock. QA by itself does not authorize code or measurement.
