# P8-B Contract — segmented residency allocation/copy bring-up

## Parent evidence

P8-A2 PASS is authoritative:
- exact target SHA256 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463;
- 19 physical weight arenas, each <=256 MiB;
- 341 physical tensor pieces;
- token_embd.weight Q4_K: 306,561,024 bytes, row_bytes=2016, rows=152064, split 133152 + 18912;
- output.weight Q6_K: 447,068,160 bytes, row_bytes=2940, rows=152064, split 91304 + 60760;
- addressability PASS;
- total planned residency=5,347,770,372 bytes;
- usable budget=16,374,562,816 bytes.

Authoritative raw SHA256:
- p8a2_segment_plan.json: 7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481
- p8a2_summary.json: 2534F8AC68134562F8D0AD1F5CB6464440C368E48659FC0DE1EB051C02B74876

## Question

Can the exact P8-A2 segmented physical plan be instantiated simultaneously in Vulkan device-local + host-visible + coherent memory on the target Arc 140V, with byte-exact weight copies and the frozen KV/working-buffer residency held at the same time?

## Frozen scope

P8-B may allocate and map buffers only. It must not create shader modules, pipelines, command buffers, dispatch compute, execute prefill/decode, or change the P8-A2 segmentation.

Weight residency:
- recompute the P8-A2 plan from GGUF and require exact equality to the frozen 19 arena boundaries;
- allocate all 19 storage buffers simultaneously;
- initialize every arena from the exact GGUF payload bytes;
- full memcmp every resident arena against the mapped GGUF source before PASS.

KV residency:
- allocate K and V as two independent FP32 buffers;
- each buffer size = layers * 4096 * kv_dim * sizeof(float);
- combined exact bytes = 469,762,048.

Working residency:
- 11 independent pp512*hidden FP32 buffers;
- 3 independent pp512*kv_dim FP32 buffers;
- 3 independent pp512*ffn FP32 buffers;
- one vocab FP32 logits buffer;
- one pp512 uint32 token-id buffer;
- one uint32 decode-id buffer;
- exact combined working bytes = 200,888,324.

All weight, KV and working buffers must remain alive simultaneously until all verification gates complete.

## Addressability carry-forward

Exhaustively validate every vocab row for both segmented tensors: global row -> segment -> arena + arena_byte_base + local_row*row_bytes must resolve to the same logical byte offset as GGUF.

## PASS gate

P8-B PASS requires:
- exact target size/SHA checked by runner;
- TensorStore map/bounds/no-overlap/supported-types PASS;
- exact parent P8-A2 arena and segment plan match;
- Vulkan initialization PASS;
- selected memory type contains DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT;
- all 19 weight arenas allocated/mapped simultaneously;
- full byte compare PASS for all 4,677,120,000 weight bytes;
- exact exhaustive row-translation PASS for embedding and output;
- K+V allocation PASS at exact frozen bytes;
- all frozen working buffers allocated simultaneously;
- requested resident bytes = 5,347,770,372 exactly;
- actual Vulkan allocation bytes are reported and <= frozen 15.25 GiB usable budget;
- zero shader modules, zero compute dispatches, zero inference.

No performance gate.

## Interpretation

A Vulkan initialization/file-map/package failure is harness/environment and must not be called a scientific negative. A reproducible Vulkan allocation failure after the exact plan is reconstructed is a P8-B residency failure to adjudicate without shrinking the frozen plan post hoc.

PASS -> P8-C segmented embedding/LM-head access correctness bring-up. Full model inference remains forbidden.