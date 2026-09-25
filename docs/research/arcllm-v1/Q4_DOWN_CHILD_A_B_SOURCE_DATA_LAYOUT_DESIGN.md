# Q4-down Child A / Child B source-data-layout freeze

Status: **DESIGN FROZEN — NO PERFORMANCE EXECUTION**

## Child A — minimal work-decomposition transfer

Child A does not invent a new kernel. It reuses the exact proven SA1 Q4_K subgroup32 shader and
SPIR-V unchanged:

```text
source blob 56999d88dc1bef6486e7e1908982f6de4b0f9f6a
SPIR-V SHA256 B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569
```

For Q4-down:

```text
K=18944
rows=3584
local_size=128
4 subgroup32 / workgroup
4 rows / workgroup
dispatch_x=896

one lane:
18944 / 32 = 592 K contributions
            = 74 Q4 blocks × 8 q values/lane/block
```

Only the 14 Q4_K decode ffn_down nodes switch to the exact SA1 binary. Original Q4_K representation,
prefill, all Q6 down nodes and every other semantic node stay frozen.

## Child B — Q4K_SERIAL_K_EXEC148_V0_1

B keeps one-invocation-per-row Serial-K and increasing-k FP32 accumulation. The only factor changed
is the persistent representation.

Source block: 144 bytes. Execution block: 148 bytes.

```text
EXEC148 block
offset  size
0       2      d raw FP16 bits
2       2      dmin raw FP16 bits
4       16     [sc0,mn0, sc1,mn1, ... sc7,mn7] direct uint8
20      128    q values repacked in increasing-K pairs
                 low  nibble = q[2p]
                 high nibble = q[2p+1]
```

Exact geometry:

```text
74 blocks / row
10,952 bytes / row
3,584 rows / layer
39,251,968 bytes / target layer
14 layers
549,527,552-byte execution image
```

The format itself is only 14,852,096 bytes (2.777...%) larger than the original Q4_K payload, but
the factorial harness must retain the original model for arms 0/A and unchanged prefill. Therefore
B/AB architecture cost reports the full **549,527,552 incremental resident bytes**, not 14.85 MB.

The image is materialized once by CPU directly into the final mapped UMA Vulkan buffer. There is no
per-token CPU model math and no temporary full-image copy.

### Exact semantics guard

Both source Q4_K and EXEC148 are independently decoded to:

```text
d_bits16
dmin_bits16
8 × (sc8,mn8)
256 × q8
```

= 276 canonical bytes/block.

Every block of all 14 tensors must match byte-for-byte, with per-tensor and family SHA256 hashes,
before B/AB may enter model correctness qualification.

## B Serial-K reader

```text
for block ib = 0..73:
    d,dmin = block header
    for kin = 0..255:
        j  = kin >> 5
        sc = block[4 + 2*j]
        mn = block[5 + 2*j]
        qb = block[20 + (kin >> 1)]
        q  = low/high nibble selected by kin&1
        sum += (d*sc*q - dmin*mn) * X[ib*256 + kin]
```

No accumulation-order change is allowed.

## AB mechanical derivation

```text
AB
= exact A:
    subgroup32 ownership
    lane-stride32
    subgroupAdd
+ exact B:
    same EXEC148 image
    same block reader semantics
```

AB is forbidden from adding SLM staging, prefetch, vectorization, unrolling, fusion, alternate
materialization or any third mechanism.

## Next allowed step

Bounded implementation is now sufficiently specified to create:
- the 4-arm harness;
- B materializer + independent tuple validator;
- B Serial-K EXEC148 shader;
- AB mechanically-derived Split-K32 EXEC148 shader;
- A binding to the already-frozen SA1 binary.

Only static/synthetic/real-model correctness qualification may follow. Fresh performance timing and
hardware-counter execution remain forbidden until an independent implementation lock passes.
