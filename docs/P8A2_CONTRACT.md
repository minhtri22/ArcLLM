# P8-A2 Contract — segmented large-tensor memory plan

## Parent evidence

P8-A-R1 is a genuine memory-contract FAIL, not a capacity failure. Exact target identity, qwen2 metadata, context, supported F32/Q4_K/Q6_K types, non-overlap and total capacity all PASS. The only failed gates are inherited single-tensor <=256 MiB placement and arena packing, caused by exactly token_embd.weight and output.weight.

Authoritative P8-A raw SHA256:
- p8a_memory_plan.json: 6495545700E4104B05ED7D913ACC728C64329CB92AC894342A690946EDECE31E
- p8a_summary.json: C191DC70AEBAEEF733847AFE8B15ECECC95BE112F1041178DD307FE68D981EA9

## Single architecture hypothesis

Keep the 256 MiB physical arena cap and all P8-A memory assumptions unchanged, but allow an oversize logical tensor to be represented by multiple row-aligned physical segments. This should remove the exact P8-A placement obstruction without changing tensor bytes, quantization semantics, KV, working buffers or total planned memory.

## Frozen segmentation rules

1. Only tensors whose exact packed bytes exceed 256 MiB may be segmented.
2. Non-oversize tensors remain one physical piece.
3. A segment boundary must be a complete row boundary using GGUF dims[0] as row width and product(dims[1:]) as row count.
4. No Q4_K/Q6_K row or quant block may be split.
5. Each segment byte count must be <=256 MiB.
6. Segments for a tensor must cover rows [0,total_rows) exactly once, contiguously and in increasing order.
7. Physical file spans must remain non-overlapping and preserve original byte order.
8. Deterministic arena packing may cut at segment boundaries but every physical arena remains <=256 MiB.
9. Total weight payload, FP32 KV@4096, pp512 working buffers, observed 17.25 GiB budget and 2 GiB reserve are unchanged.

## Addressability qualification

P8-A2 is not allowed to declare success from byte packing alone.
- token_embd.weight must be a 2-D vocab-row matrix and its segment table must support global token row -> (segment, local row).
- output.weight must be a 2-D vocab-row matrix and its segment table must support global vocab row -> (segment, local row).
- Both tensors must use supported packed types and have row count equal to model vocab.
- The planner records exact type, dims, row_bytes, packed bytes and every segment range.

These mappings are a contract for P8-B/P8-C; P8-A2 does not implement or execute new shaders.

## PASS gate

P8-A2 PASS requires:
- parent obstruction matches exactly token_embd.weight + output.weight;
- both oversize tensors are row-segmentable vocab matrices;
- all physical pieces <=256 MiB;
- exact row coverage for every segmented tensor;
- deterministic arena packing succeeds with every arena <=256 MiB;
- every physical piece is contained by exactly one arena;
- total planned GPU-visible bytes remain <=15.25 GiB usable budget;
- no Vulkan allocation, shader compile or inference occurs.

No performance gate. Full inference remains forbidden after P8-A2 PASS.

## Next

PASS -> P8-B segmented residency allocation/copy bring-up.
FAIL -> adjudicate the exact remaining addressability/packing obstruction; do not relax the arena cap post hoc.
