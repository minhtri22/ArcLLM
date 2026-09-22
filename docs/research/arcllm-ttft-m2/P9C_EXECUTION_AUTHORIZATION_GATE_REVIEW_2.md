# TTFT_M2 — P9C Execution Authorization Gate Review 2

**Result:** `PASS_M2_P9C_EXECUTION_AUTHORIZATION_GATE_REVIEW_2`

**Decision:** `AUTHORIZE_ONE_EXACT_FRESH_MECHANISM_IDENTIFICATION_COLLECTION`

Review 2 re-evaluated execution only after P9C-A corrected all five pre-execution binding defects.

## Exact authority

```text
P9C execution authorization
fe4c45ba7675a2d10cc8a1cc4294d0f84adcdfe2

P9C-A execution binding
90dc9d87fb987a3892d0a64c27e89baa2cbd3a32

corrected science runner
f0a627de05c509286cdb51a070d409b297c7dbaa

P9B lock v0.2
7b06da63fe09597ebdcf641d1cd992d4a973075c

P9B adjudication
d6a9947694cb35044251bac363236f69b06e4d9f

P9A canonical design
abce0545cec33367f9b3a82f15d5ffd4fb026f64
```

## Why Review 2 passes

All five Review-1 blockers are now resolved.

The corrected runner now proves, before launch:

- active P9B lock is v0.2;
- executable SHA256 is exactly `9E0C0A7C...`;
- executable size is exactly `441856`;
- native and shader manifest byte hashes match P9B qualification;
- all actual runtime SPIR-V files rehash to the qualified manifest;
- F0 precedes F1;
- executable launch occurs only after F0 and F1.

The historical SAFE H-ART authority exposes exactly the same 16 common shader keys required by the P9B common shader set.

The P9A target contract also matches prior exact local-environment evidence from ANL64 BuildOnly:

```text
CPU     Intel(R) Core(TM) Ultra 7 258V
GPU     Intel(R) Arc(TM) 140V GPU (16GB)
driver  32.0.101.8860
OS      build 26200
```

That historical match does not waive runtime F0. The fresh run must still revalidate every target invariant before F1 or executable launch.

## Authorized science

Exactly one fresh collection is authorized.

```text
arms                         SP / SF / QP / QF
workloads                    W-S / W-C
sessions                     A / B
measured attempts per cell   5
planned fresh TTFT values    80
parent observations reused   0
selective rerun              false
material threshold           1.10
```

Authorized:

```text
diagnostic executable launch  true, only after F0/F1
target model execution        true
model load                    true
GPU dispatch                  true
performance measurement       true
fresh TTFT observation        true
H-ART static adjudication     true
```

Not authorized:

```text
final mechanism adjudication  false
scientific-design mutation    false
threshold mutation            false
workload/endpoint mutation    false
session-order mutation        false
selective rerun               false
post-outcome tuning           false
```

## Mandatory stop semantics

```text
F0 mismatch
  → STOP
  → no performance interpretation
  → no automatic rerun

H-ART supported at F1
  → STOP_TIMING_H_ART_SUPPORTED_STATIC
  → executable remains unlaunched

provenance/payload mismatch
  → STOP_NO_EXECUTION

cell failure
  → STOP
  → preserve partial evidence
  → no selective rerun
```

## Scientific accounting at authorization

No scientific outcome existed when this gate closed:

```text
executable launches       0
model loads               0
GPU dispatches            0
performance measurements  0
fresh TTFT observations   0
scientific result         NONE
```

## Next

`EXECUTE_EXACT_P9C_AUTHORIZED_FRESH_COLLECTION_ON_LOCAL_WINDOWS`

After execution, raw evidence must be returned intact. Independent adjudication remains a separate step.
