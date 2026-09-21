# ANL64 P3 — Zero-Science Local Compatibility

**Date:** 2026-09-21  
**Mode:** ZERO-SCIENCE / SOURCE + CLOSED-EVIDENCE REUSE  
**Decision:** `P3_PASS_P4_BOUNDED_IMPLEMENTATION_ELIGIBLE`

## 1. Why P3 does not rerun the hardware probe

P3 is an integration-compatibility stage, not a capability-discovery stage.

The exact target has already been bound by closed SA0 evidence:

```text
GPU                 Intel(R) Arc(TM) 140V GPU (16GB)
Windows driver      32.0.101.8860
subgroup size       32
subgroup arithmetic available
subgroup size control available
computeFullSubgroups available
max workgroup invocations 1024
```

The exact Q4_FAST candidate shader blob was also already compiled and run through a zero-measurement correctness preflight on that same target/driver:

```text
candidate shader blob
56999d88dc1bef6486e7e1908982f6de4b0f9f6a

candidate SPIR-V SHA256
B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569

SA1 K1 zero-measurement preflight
PASS

candidate vs CPU
PASS

candidate vs baseline
PASS
```

Therefore rerunning the same capability/candidate preflight would not answer a new scientific question. P3 reuses those exact closed facts and tests only whether the frozen ANL64 architecture can integrate them without semantic or representation mismatch.

No new local runner is required for P3.

## 2. Q4_FAST interface compatibility

The frozen Q2 baseline Q4 shader and inherited Q4_FAST shader use the same four storage bindings:

```text
binding 0 = packed W
binding 1 = FP32 X
binding 2 = FP32 Bias
binding 3 = FP32 Y
```

They also use the same push-constant fields in the same semantic order:

```text
n
rows
batch
row_bytes
add_bias
w_base_bytes
bias_base
```

The Q2 runtime already constructs exactly this `PCGemm` contract.

Therefore replacing the executor identity for an admitted decode node does not require:
- a different packed-weight representation;
- an additional W buffer;
- a dequantized copy;
- an alternate bias representation;
- a new output representation.

## 3. Batch-domain compatibility

There is one important constraint.

The baseline shader supports `batch > 1` through token offsets.

The inherited Q4_FAST candidate uses:

```text
X.x[k]
Y.y[row]
```

and is therefore semantically equivalent to the baseline only for:

```text
batch = 1
```

That exactly matches the ANL64 decode use case.

P3 therefore freezes:

```text
Q4_FAST_FIXED:
decode batch=1 only

prefill:
existing safe path only
```

Q4_FAST must not be silently reused for multi-token prefill.

## 4. Exact admitted Q4 nodes

The frozen Q2 source requires the following tensors to be Q4_K in every decoder layer:

```text
Q projection
K projection
O projection
gate projection
up projection
```

Exact geometries:

| Role | n | rows | Region64 count | Q4_FAST workgroups |
|---|---:|---:|---:|---:|
| Q | 3584 | 3584 | 56 | 896 |
| K | 3584 | 512 | 8 | 128 |
| O | 3584 | 3584 | 56 | 896 |
| gate | 3584 | 18944 | 296 | 4736 |
| up | 3584 | 18944 | 296 | 4736 |

Q4_FAST uses four rows/workgroup.

Every admitted row count is divisible by both 64 and 4, so:

```text
1 Region64
= 64 logical output rows
= 16 Q4_FAST workgroups
```

There is no tail workgroup and no partial Region64 mask on the exact target.

## 5. Exact PlanNode mapping

The decode graph remains:

```text
469 logical PlanNodes
```

Partition:

```text
140  guaranteed Q4_FAST_FIXED nodes
 56  V/down conditional quant nodes
 19  safe LM-head nodes
254  non-quant safe nodes
-------------------------------
469
```

P3 permits only:

> preserve the frozen Q2 node order and replace the executor identity of the 140 guaranteed fixed-Q4 decode nodes.

P3 does not permit:
- operator reorder;
- operator deletion;
- fusion;
- sparse skipping;
- extra dependency edges;
- online executor selection.

## 6. Arena/base-offset compatibility

Q2 already binds every decoder tensor through the existing resident weight arenas.

For decoder tensors the frozen source requires one slice per tensor and exposes:

```text
AB(name)    -> existing arena buffer
BASE(name)  -> byte offset within that arena
FBASE(name) -> F32 bias offset
```

Q4_FAST consumes the same:

```text
W buffer
w_base_bytes
bias_base
```

Therefore the 140 admitted Q4_FAST nodes can address current packed weights directly.

P3 establishes that integration does not require:

```text
weight copy
payload repack
predequantization
new scratch storage
offline weight conversion
```

## 7. Ledger64 compatibility

Ledger64 remains control-plane metadata.

It is not the GPU workgroup geometry.

```text
Region64 descriptor
       ↓
64 logical output rows
       ↓
16 Q4_FAST workgroups
```

The full exact target uses a full mask:

```text
0xffffffffffffffff
```

No dynamic frontier exists.

Therefore there is no:
- per-token Region64 reconstruction;
- region sorting;
- frontier scan;
- Event-Ledger queue dispatch;
- extra queue submission.

The immutable plan is built at model load.

## 8. Exact-device compatibility

Closed evidence already establishes on the same driver:

```text
default subgroup size      32
required subgroup size     32
candidate local_size_x    128
four subgroups/workgroup
subgroup arithmetic        available
computeFullSubgroups       available
```

The inherited Q4 candidate also passed 10 zero-measurement correctness cases over all five Q4 shape cells.

Thus there is no unresolved hardware expressibility problem for the P2 Q4_FAST_FIXED path.

## 9. BuildOnly disposition

P3 does not rebuild the same standalone shader because exact-build evidence already exists for the exact shader blob and compiler route.

The next new BuildOnly question is different:

> does the ANL64 production integration compile after the planner and executor substitution are implemented?

That belongs to P4 implementation/BuildOnly, not P3.

## 10. P3 falsification checks

None of the P2 stop conditions is triggered:

```text
source interface mismatch             false
batch-domain mismatch                 false
unsupported subgroup contract         false
weight repack/copy required           false
per-token control-plane work required false
dependency reorder required           false
metadata budget violation             false
```

## 11. P3 adjudication

```text
EXACT GGUF/Q2 → PlanNode MAPPING      PASS
REGION64 → Q4_FAST MAPPING            PASS
W/X/B/Y INTERFACE                     PASS
PUSH-CONSTANT COMPATIBILITY           PASS
ARENA/BASE-OFFSET COMPATIBILITY       PASS
BATCH=1 DECODE DOMAIN                 PASS
EXACT ARC 140V SUBGROUP CONTRACT      PASS (inherited exact evidence)
NO REPACK/COPY REQUIREMENT            PASS
NO ONLINE SEARCH/DISCOVERY            PASS
NO NEW SCIENTIFIC DATA                CONFIRMED
```

Final result:

> **P3_PASS_P4_BOUNDED_IMPLEMENTATION_ELIGIBLE**

This does **not** authorize P4 automatically.

A separate explicit P4 bounded implementation authorization must freeze:
- exact implementation allowlist;
- exact production files that may change;
- Q4_FAST integration scope = 140 guaranteed fixed-Q4 decode nodes only;
- Ledger64 descriptor implementation scope;
- static metadata budget;
- no fusion/Q6 optimization/repack/prefetch;
- BuildOnly and correctness gates;
- no performance measurement or target-model benchmark until later stages.
