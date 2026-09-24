# ArcLLM v1 — ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL

**Version:** v0.1  
**Date:** 2026-09-24  
**Parent:** `fb91c75f60cde79c4e962742b32166855379706b`  
**Status:** STATIC HARDWARE-GROUNDING MODEL / NO NEW PERFORMANCE EXECUTION

## Core question

I003 established a fresh ~10x external gap. The next question is not which historical family is largest, but: **for one exact decode token, what work is unavoidable, how does ArcLLM map it to CPU/RAM/GPU, how does pinned llama.cpp organize the same work, and where is the excess cost above hardware/dataflow floors?**

## Exact token topology

```text
embedding
  -> 28 x [RMSNorm, Q, K, V, Q-RoPE, K-RoPE, KV-store, attention, O, residual, RMSNorm, gate, up, SwiGLU, down, residual]
  -> output norm
  -> 19 lm-head chunks
= 469 GPU dispatches/token
```

The Arc runtime records one command buffer, inserts 468 compute barriers between adjacent nodes, performs one queue submit and one fence wait per token.

## Whole-token hardware bound

Logical one-pass weight payload = **4,370,560,992 bytes/token (~4.371GB)**. This is resident model weight payload minus the unused embedding table plus one 2,016-byte embedding row.

Using Intel's 136GB/s Lunar Lake IP-bandwidth figure only as an optimistic roof gives **~32.14ms/token weight-only floor (~31.1 tok/s)**.

Dense-equivalent linear work excluding attention/pointwise is **14.14 GFLOP/token**. With a derived 3.9936 TFLOP/s FP32 peak proxy, its optimistic compute floor is **~3.54ms/token**. The first-order regime is therefore data-movement/useful-weight-throughput dominated under optimistic roofs.

## Recasting the I003 gap

| Cell | Arc ms/token | llama ms/token | Arc useful-weight GB/s | llama useful-weight GB/s |
|---|---:|---:|---:|---:|
| A/W-S | 675.84 | 77.52 | 6.47 | 56.38 |
| A/W-C | 716.08 | 86.20 | 6.10 | 50.70 |
| B/W-C | 1073.40 | 85.17 | 4.07 | 51.32 |
| B/W-S | 1001.80 | 93.13 | 4.36 | 46.93 |

These GB/s values are a **logical useful-model-weight throughput diagnostic**, not measured DRAM bandwidth.

## Node model

| Node | Shape | Quant/dtype | Semantic min bytes | Dense-equivalent work | Arc geometry |
|---|---|---|---:|---:|---|
| embedding | token->3584 | Q4_K->F32 | 16,356 | dequant | 14 WG x256 |
| RMSNorm | 3584->3584 | F32 | 43,008 | ~14.3K + rsqrt | 1 WG x256 |
| Q | 3584x3584 | Q4_K | 7,268,352 | 25.69 MFLOP | 56 WG x64; row-serial K |
| K | 3584x512 | Q4_K | 1,050,624 | 3.67 MFLOP | 8 WG x64; row-serial K |
| V | 3584x512 | Q4_K/Q6_K | 1.051-1.524MB | 3.67 MFLOP | 8 WG x64; row-serial K |
| cached GQA | Q28x128, KV T x4x128 | F32 | 28,672 + 4,096T | ~14,336T + softmax | 28 WG x128 |
| O | 3584x3584 | Q4_K | 7,254,016 | 25.69 MFLOP | 56 WG x64; row-serial K |
| gate | 3584x18944 | Q4_K | 38,281,216 | 135.79 MFLOP | 4,736 WG x128; **I002 subgroup32 split-K** |
| up | 3584x18944 | Q4_K | 38,281,216 | 135.79 MFLOP | same |
| SwiGLU | 18944 pair | F32 | 227,328 | exp+pointwise | 74 WG x256 |
| down | 18944x3584 | Q4_K/Q6_K | 38.28-55.79MB | 135.79 MFLOP | 56 WG x64; row-serial K |
| lm-head | 3584x152064 | Q6_K | **447,690,752** | **1.090 GFLOP** | 19 dispatches, 2,376 WG x64; row-serial H |

The JSON artifact expands all **469 individual dispatch nodes** and includes the requested fields: tensor shape, dtype/quant, bytes read/write, FLOPs/integer-op state, dispatch/workgroups/subgroup, temporary allocation, barrier dependency, CPU involvement, residency/cache model, theoretical floors, Arc observed latency, llama observed latency and excess ratio. Unsupported observed values are explicitly null rather than guessed.

## First architecture contrast

Outside I002 gate/up, Arc's Q/K/V/O/down and lm-head shaders still assign one output row to an invocation that serially scans K. The pinned llama Vulkan Q4_K/Q6_K matvec source instead distributes quant-block work across workgroup threads, vectorizes input loads, reduces partials and supports multiple output rows via specialization constants.

This is the first strong bottom-up explanation candidate for the 10x gap. It is a **mechanism hypothesis**, not yet permission to replace every kernel.

## Discoveries

1. Arc post-I002 useful-weight throughput is ~4.07-6.47GB/s versus llama ~46.93-56.38GB/s on the same logical payload.
2. Gate/up is the only major quant-linear family already converted to explicit split-K subgroup execution.
3. Pinned llama's quant matvec family is structurally K-parallel at workgroup level.
4. The optimistic token floor is far more constrained by weight movement (~32.14ms) than dense FP32 arithmetic (~3.54ms).
5. XMX is not the observed llama explanation: I003 runtime reported `matrix cores: none`.
6. Arc GQA has a source-level KV reuse question: seven Q heads share each KV head but execute as separate workgroups.
7. LM-head is structurally huge (447.07MB Q6_K) and row-serial in Arc, but is not selected until current timing/headroom is measured.
8. Raw 469 dispatch count remains a topology fact, not a causal conclusion.

## Intentionally unresolved

- exact per-layer Q4/Q6 census for V/down;
- post-I002 latency per node/family;
- pinned llama latency and actual Vulkan dispatch census per semantic family;
- sustained physical DRAM traffic/bandwidth;
- cache hit rates, occupancy and register pressure;
- per-node Arc/llama excess ratio.

## Next evidence, not next optimization

0. **Metadata only:** exact tensor-name/type/dims/bytes census; no quiet host.
1. **Minimal post-I002 timestamps:** populate Arc node/family latency; no machine re-profile.
2. **Pinned llama execution map:** populate llama node/family latency and dispatch topology.
3. **Quiet-host counters only if still needed:** bandwidth/cache/occupancy after a concrete hypothesis remains unresolved.

No new kernel/intervention is authorized by v0.1.

## Evidence anchors

ArcLLM: current I003 candidate source; Vulkan runtime source; P8A2/P8C/P8E authoritative model metadata; I001R historical profile; I003 final adjudication.

Pinned llama.cpp `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`: Qwen2 graph, llama-graph, Q4_K/Q6_K Vulkan mul_mat_vec shaders, mul_mat_vec_base, rms_norm.

Hardware: Intel Core Ultra 7 258V specifications; Intel oneAPI Xe2-LPG architecture guide; Intel Xe2 Tech Tour peak metrics; Intel Lunar Lake 136GB/s IP-bandwidth publication.

## M0 closure — exact per-layer V/down quant census

M0 completed as metadata-only evidence. Bundle SHA256: `3BD3102FFE8ABE4C13150E1B88641FE5BE3940610155F532564B255922D32453`.

Independent census: 56/56 target tensors present; `V = 14 Q4_K + 14 Q6_K`; `FFN-down = 14 Q4_K + 14 Q6_K`; no inference and no performance measurement.

| Layer | V | Down |
|---:|---|---|
| 0 | Q6_K | Q6_K |
| 1 | Q6_K | Q6_K |
| 2 | Q6_K | Q6_K |
| 3 | Q4_K | Q4_K |
| 4 | Q4_K | Q4_K |
| 5 | Q6_K | Q6_K |
| 6 | Q4_K | Q4_K |
| 7 | Q4_K | Q4_K |
| 8 | Q6_K | Q4_K |
| 9 | Q4_K | Q6_K |
| 10 | Q4_K | Q6_K |
| 11 | Q4_K | Q4_K |
| 12 | Q6_K | Q4_K |
| 13 | Q4_K | Q6_K |
| 14 | Q4_K | Q4_K |
| 15 | Q6_K | Q4_K |
| 16 | Q4_K | Q6_K |
| 17 | Q4_K | Q4_K |
| 18 | Q6_K | Q4_K |
| 19 | Q4_K | Q4_K |
| 20 | Q6_K | Q6_K |
| 21 | Q4_K | Q4_K |
| 22 | Q4_K | Q4_K |
| 23 | Q6_K | Q6_K |
| 24 | Q6_K | Q6_K |
| 25 | Q6_K | Q6_K |
| 26 | Q6_K | Q6_K |
| 27 | Q6_K | Q6_K |

The central machine-readable ledger is superseded by `ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL_v0.2.json`; all 56 V/down nodes now have exact quant type, exact shader family, exact semantic byte floor and exact branch-specific bandwidth floor. M0 is closed. M1 is now the next evidence step.

## M1 closure — post-I002 Arc timing localization

M1 bundle `D9345A4B94C3FC278D1459D7127464BDCBAD4AF142598AF3574F4E51934D634F` passed: 4 measured inferences, 12 profiled decode steps, 5,628 op timestamp observations, no quiet-host/counter run.

| Coarse family | Post-I002 tick share | Logical weight share | tick/weight-share ratio |
|---|---:|---:|---:|
| ffn_down | 35.13% | 30.07% | 1.17x |
| ffn_gate_up | 21.91% | 48.93% | 0.45x |
| lm_head | 21.38% | 10.23% | 2.09x |
| attn_qkv | 10.43% | 6.11% | 1.70x |
| attn_output | 3.77% | 4.63% | 0.81x |

The decisive M1 result is a **cost inversion after I002**: gate/up is no longer dominant; FFN-down is now the largest coarse family (~35.1%), while LM-head remains ~21.4% despite only ~10.2% logical weight payload. This does not authorize an intervention. M2 must map pinned llama timing for the same semantic shapes before an excess-cost conclusion.

## M2 closure — pinned llama same-semantic timing / excess map

M2 bundle SHA256: `546E07D4C739AC91D28CD2B6F67AAC1DFAB802F8AD28C0CC4BE7A04F359BB9F3`.

Independent recomputation from the two raw Vulkan perf logs matched the bundled summary exactly.
Each workload contains 64 timing blocks (32 warmup + 32 measured); the final 31 cached-decode
blocks are the measured decode path and probes 0/15/30 satisfy all frozen semantic call counts.

M2 major-family calibrated shares are stable across W-S/W-C: gate/up ~42%, Q4-down ~10%,
Q6-down ~17%, LM-head ~8.7%, Q+O ~10%, K+V(Q4) ~1.7%, V(Q6) ~0.8%.

Joining M1/M2 shares to matched I003 practical token latency yields the new cross-runtime
`ARCLLM_V1_EXCESS_COST_MAP_v0.1`. The mapped families cover >99.96% of logical weight payload.

M3 quiet-host counters are deferred. The next causal discriminator is a Q4_K FFN-down split-K
sentinel at K=18944 / rows=3584. No intervention is authorized until its specification is frozen.

