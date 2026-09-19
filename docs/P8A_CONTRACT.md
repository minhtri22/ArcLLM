# P8-A Contract — frozen 7B memory-plan-only bring-up

## Frozen target

- Repository: Qwen/Qwen2.5-Coder-7B-Instruct-GGUF
- Revision: 13fb94bfda8c8cf22497dc57b78f391a9acb426a
- File: qwen2.5-coder-7b-instruct-q4_k_m.gguf
- Size: 4,683,073,536 bytes
- SHA256: 509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C
- Initial max context: 4096
- Initial prefill chunk: 512
- Arena cap inherited from P7: 256 MiB

The artifact identity is immutable for P8-A. A different file, revision, size or SHA is a different experiment.

## Question

Can the exact 7B artifact inherit the P7 packed-residency architecture without allocating Vulkan memory or running inference?

## Planner

P8-A parses GGUF metadata and tensor descriptors only. It does not create a Vulkan instance, allocate GPU buffers, compile shaders, or execute inference.

It must report:
1. architecture metadata and tensor count;
2. exact quantization-type mix;
3. tensor payload bytes and payload span;
4. deterministic tensor-aware arena placement with <=256 MiB arenas;
5. any single tensor that cannot fit one inherited arena;
6. FP32 KV bytes for max_ctx=4096 using model metadata;
7. P7-L-equivalent working-buffer bytes for prefill chunk=512;
8. total planned GPU-visible bytes;
9. headroom against the frozen 17.25 GiB observed Vulkan budget after a pre-registered 2 GiB safety reserve.

## Frozen production-memory formula

P8-A deliberately models the current P7-L buffer architecture rather than inventing a lower-memory implementation after seeing 7B.

KV bytes = 2 * layers * max_ctx * kv_dim * sizeof(float).
Working buffers include 11 prefill*hidden float buffers, 3 prefill*kv_dim float buffers, 3 prefill*ffn float buffers, one vocab-sized float logits buffer, prefill token IDs and one decode ID.

## PASS gate

P8-A PASS requires:
- exact frozen file size and runner SHA256;
- general.architecture == qwen2;
- planned context 4096 <= model context metadata;
- only P7-supported F32/Q4_K/Q6_K tensors;
- valid non-overlapping tensor byte ranges;
- deterministic arena packing with every arena <=256 MiB;
- no tensor crossing an arena;
- no single tensor >256 MiB;
- total planned GPU-visible bytes <=15.25 GiB usable budget after safety reserve.

P8-A has no performance gate.

## Interpretation

A planner FAIL is not a runtime-performance negative. It identifies the exact memory-contract obstruction that must be solved before full 7B inference. Gates must not be relaxed after observing the result.
