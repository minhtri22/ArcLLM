# TTFT_M2 — P9D Raw Collection Integrity and Frozen Adjudication

**Result:** `PASS_M2_P9D_VALID_FRESH_COLLECTION_H_NULL_SUPPORTED`

**Scientific classification:** `H_NULL_SUPPORTED_NO_STABLE_MATERIAL_REGISTERED_TTFT_MECHANISM`

This is a narrow statement about the exact P9A-registered mechanism set and frozen gates. It does **not** claim that no TTFT mechanism exists outside this design.

## Execution identity and evidence integrity

The returned P9C bundle is bound to:

```text
HEAD
b3e49b54e99c05908939269c3b59c726a5810a41

execution authorization
fe4c45ba7675a2d10cc8a1cc4294d0f84adcdfe2

execution binding
90dc9d87fb987a3892d0a64c27e89baa2cbd3a32

science runner
f0a627de05c509286cdb51a070d409b297c7dbaa

P9B lock
7b06da63fe09597ebdcf641d1cd992d4a973075c

qualified executable
9E0C0A7CCCBB2DA767B4DE90354EC7ADF4463286264187843521419113C019C7
```

Returned bundle SHA256:

`3BCA56E562D15F14B16D16AECA56192AA1184726A6E3ACF5AA172B92B05E191E`

The bundled authorization, execution binding, and P9B lock resolve byte-for-byte to their exact Git blobs.

Terminal state:

```text
science exit code       0
classification          FRESH_COLLECTION_COMPLETE_AWAITING_ADJUDICATION
F0                      PASS
H-ART                   H_ART_FALSIFIED_STATIC
automatic rerun         false
selective rerun         false
parent observations     0
```

## Raw collection integrity

The evidence contains the complete frozen design:

```text
16 / 16 required cells
5 measured attempts per cell
80 / 80 fresh TTFT observations
```

Across all 80 attempts:

- success = true;
- conditioning success = true;
- final logits finite = true;
- dispatch census = PASS;
- error string empty.

Matched first-token equality also passes for every arm at every matched attempt.

```text
W-S first token = 128275
W-C first token = 198
```

The exact authorized runner deterministically encodes the frozen A/B cell order. The returned run binds that exact runner, reports no rerun/selective rerun, and contains the complete required cell set.

## Primary TTFT medians

| Session / workload | SP ms | SF ms | QP ms | QF ms |
|---|---:|---:|---:|---:|
| A / W-S | 614.650 | 1532.976 | 1270.069 | 1182.234 |
| A / W-C | 13931.263 | 14732.218 | 14149.205 | 14047.273 |
| B / W-S | 1091.049 | 1029.828 | 1166.761 | 817.874 |
| B / W-C | 14176.850 | 13845.819 | 14416.232 | 12580.215 |

No pooling is used for mechanism gates.

## Frozen contrast table

| Session / workload | QP/SP | SF/SP | QF/QP | QF/SF | Interaction |
|---|---:|---:|---:|---:|---:|
| A / W-S | 2.0663 | 2.4941 | 0.9308 | 0.7712 | 0.3732 |
| A / W-C | 1.0156 | 1.0575 | 0.9928 | 0.9535 | 0.9388 |
| B / W-S | 1.0694 | 0.9439 | 0.7010 | 0.7942 | 0.7427 |
| B / W-C | 1.0169 | 0.9766 | 0.8726 | 0.9086 | 0.8935 |

Frozen materiality threshold:

`1.10`

## F4 — H-DPIPE

Rule:

`QP/SP >= 1.10 in W-S and W-C independently in both sessions`

Result:

`NOT SUPPORTED`

A/W-S is material at 2.0663, but the other three required comparisons are below 1.10. Strict replication fails.

## F5 — H-PRECOND

Rule:

`QF/QP >= 1.10 AND SF/SP < 1.10 in both workloads and both sessions`

Result:

`NOT SUPPORTED`

QF/QP is below 1.10 in all four required comparisons.

## F6 — H-STATE-INTERACTION

Rule:

`QF/SF >= 1.10 AND interaction >= 1.10 in both workloads and both sessions`

Result:

`NOT SUPPORTED`

Both required ratios remain below 1.10 in all four comparisons.

## F7 — H-NULL

Frozen rule:

`No H-DPIPE/H-PRECOND/H-STATE-INTERACTION gate passes AND QF/SF < 1.10 in W-S and W-C independently in both sessions after H-ART falsification.`

Observed QF/SF:

```text
A / W-S  0.7712
A / W-C  0.9535
B / W-S  0.7942
B / W-C  0.9086
```

All four are below 1.10.

Therefore:

`H_NULL = SUPPORTED`

The alternate classifications `UNRESOLVED_STABLE_TTFT_HARM` and `HISTORICAL_HARM_NOT_STABLY_REPRODUCED` do not apply under the frozen rules.

## Anti-rescue audit

No post-outcome change was made to:

- threshold;
- arm set;
- workload set;
- endpoint;
- session pooling;
- mechanism registry.

No parent timing was reused as fresh evidence.

## Scientific conclusion

The valid fresh P9 collection does not support any registered positive mechanism:

```text
H-ART               falsified
H-DPIPE              not supported
H-PRECOND            not supported
H-STATE-INTERACTION  not supported
H-NULL               supported
```

The correct interpretation is:

> Under the exact frozen P9A design, no stable material TTFT mechanism among the registered candidates replicated across both workloads and both sessions.

This is not a universal no-mechanism claim.

## Next admissible step

`M2_P10_FINAL_PROGRAM_ADJUDICATION_POST_A1_CONTINUATION`

That final program adjudication should close TTFT_M2 using the valid P9D scientific result while preserving the complete historical infrastructure-failure and governance-amendment lineage.
