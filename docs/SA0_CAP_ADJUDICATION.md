# SA0-CAP Independent Adjudication

**Date:** 2026-09-21  
**Returned bundle SHA256:** `C272F50AA5730D417BC0234127F72E345A4168F922794783FE6743CDD0C91570`  
**Implementation commit:** `e32ed1ccfe0d6e675e3186af5ca7db606692bd98`  
**Decision:** **SA0_CAP_PASS**

## Integrity

The returned ZIP contains exactly five frozen SA0-CAP artifacts. Independent recomputation verified:

- raw capability hash matches the preflight lock;
- BuildOnly manifest hash matches the preflight lock;
- executable SHA is identical in BuildOnly manifest and preflight lock;
- implementation commit is identical across evidence;
- all six critical Git blobs match the exact ArcLLM commit;
- frozen SA0-CAP contract hash and parent-SA0 binding are consistent.

The executable bytes were not included in the frozen return bundle, so its SHA is cross-bound rather than independently byte-rehashed here. The frozen bundle contract did not require executable packaging; this is recorded as an evidence limitation, not a failed gate.

## Zero-science boundary

Verified:

```text
scientific_workload       NOT_RUN
model_loaded              false
shader_compile            NOT_PERFORMED
shader_modules_created    0
compute_pipelines_created 0
dispatches_submitted      0
successor_kernel          NOT IMPLEMENTED
Q2/Q3                     NOT RUN
```

Therefore SA0-CAP remains a capability inventory and does not consume successor performance evidence.

## Exact target

The returned evidence identifies:

```text
CPU                 Intel Core Ultra 7 258V
GPU                 Intel Arc 140V GPU (16GB)
Windows driver      32.0.101.8860
Vulkan driver info  101.8860
Vulkan loader       1.4.313
Vulkan device API   1.4.348
OS build            26200
power scheme        Balanced
```

The driver matches the frozen Q3 environment.

## Baseline capability gate

All required SA0 capabilities PASS:

- compute-capable Vulkan queue;
- 64 timestamp-valid bits;
- compute subgroup available;
- subgroup size 32;
- 49,152-byte max compute shared memory;
- 1,024 max workgroup invocations;
- max workgroup size 1024×1024×64;
- valid memory heap/type topology.

This is sufficient for the primary SA-H1a study to proceed to preregistration.

## Optional capability routes

The exact device also reports:

- subgroup extended types: supported;
- subgroup-size-control: supported;
- `computeFullSubgroups`: supported;
- allowed subgroup range: 16–32;
- 8-bit storage: supported;
- 16-bit storage: supported;
- shader FP16: supported;
- shader INT8: supported;
- `VK_KHR_cooperative_matrix`: supported.

Four KHR cooperative-matrix combinations were enumerated at subgroup scope:

```text
M=8 N=16 K=16   FP16 × FP16 -> FP32 accumulator/result
M=8 N=16 K=32   UINT8 × UINT8 -> UINT32 accumulator/result
M=8 N=16 K=32   SINT8 × SINT8 -> SINT32 accumulator/result
M=8 N=16 K=16   FP16 × FP16 -> FP16 accumulator/result
```

These are implementation options, not evidence that cooperative-matrix execution is optimal for Q4_K/Q6_K. The packed quantization/dequantization representation still has to be causally mapped before any such path can be selected.

## Scientific decision

```text
SA0 causal qualification          PASS
SA0 exact-device capability       PASS
SA0                              COMPLETE

SA-H1a primary mechanism          RETAIN
subgroup path                     AVAILABLE
subgroup-size-control path        AVAILABLE
FP16 / INT8 path                  AVAILABLE
KHR cooperative-matrix path       OPTIONAL / AVAILABLE

SA1-P preregistration             PERMITTED
successor kernel implementation   NOT PERMITTED
target-model execution            NOT PERMITTED
Q3 reopen                         FORBIDDEN
```

## Next boundary

The next phase is **SA1-P specification-only component-study preregistration**.

Before any new GEMM shader is written, SA1-P must freeze:

1. exact batch-1 Q4_K/Q6_K target shapes;
2. exact current generic-kernel baseline identity;
3. one primary implementation mechanism;
4. optional capability route disposition;
5. component timing method and metric;
6. semantic/correctness tolerance;
7. preregistered performance threshold;
8. memory/register/shared-memory resource budget;
9. falsification and stop conditions;
10. implementation allowlist and implementation-lock procedure.

No target-model benchmark is opened by SA0-CAP PASS.
