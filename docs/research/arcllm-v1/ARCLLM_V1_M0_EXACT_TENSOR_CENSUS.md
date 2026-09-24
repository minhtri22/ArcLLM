# ArcLLM v1 — M0 Exact Tensor Census

Date: 2026-09-24
Branch: research/arcllm-v1
Class: metadata-only; no inference; no performance measurement.

## Question

Resolve exact per-layer quantization for the 56 intentionally unresolved tensors:
- blk.0..27.attn_v.weight
- blk.0..27.ffn_down.weight

Frozen target SHA256: 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
Frozen target bytes: 4,683,074,048
Whole-model tensor count: 339
Authoritative quant mix: F32=141, Q4_K=169, Q6_K=29.

The fixed known tensors imply the 56 target tensors contain exactly 28 Q4_K + 28 Q6_K. This is only a consistency gate and must not be used to infer layer ownership.

## Method

Read GGUF header, metadata and tensor descriptors with the existing GgufReader. No tensor payload evaluation. No Vulkan runtime. No inference, decode, timestamp, timer, counter, bandwidth test or performance measurement.

For each layer emit tensor name, type, dimensions, row bytes and tensor bytes, plus a patch_manifest mapping to Lxx.v_proj and Lxx.ffn_down in the central hardware ledger.

## PASS

All 56 tensors are present; dimensions are exact; every target is Q4_K or Q6_K; the whole-model quant mix matches; and target mix is exactly 28 Q4_K + 28 Q6_K.

## After PASS

Independent adjudication may patch only quant-dependent fields of ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL. M1 remains unauthorized until that patch is audited.
