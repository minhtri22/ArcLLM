# P8-A-R1 Contract — frozen local Ollama 7B memory-plan-only bring-up

## Pre-execution target amendment

No P8-A scientific run was executed against the earlier Hugging Face candidate. Before execution, an existing local Ollama model layer was identified. It is not byte-identical to the earlier candidate, so P8-A-R1 freezes the local blob as a new exact target rather than treating the two artifacts as interchangeable.

Superseded candidate (never executed):
- Hugging Face Qwen/Qwen2.5-Coder-7B-Instruct-GGUF
- qwen2.5-coder-7b-instruct-q4_k_m.gguf
- 4,683,073,536 bytes
- SHA256 509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C

Frozen P8-A-R1 target:
- source: local Ollama content-addressed model layer
- repository namespace: registry.ollama.ai/library/qwen2.5-coder
- media type: application/vnd.ollama.image.model
- digest/SHA256: 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
- size: 4,683,074,048 bytes
- expected blob: blobs/sha256-60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463
- initial max context: 4096
- initial prefill chunk: 512
- inherited arena cap: 256 MiB

## Resolver contract

The default runner must not guess a filename or tag and must not download a substitute. It scans all manifest files under the qwen2.5-coder Ollama manifest directory, accepts only a model layer whose mediaType, digest and byte size exactly match config/p8_target.json, resolves the content-addressed blob path, checks that the blob exists and has the frozen size, then the runner independently computes SHA256 over the actual blob before invoking the planner.

An explicit -ModelPath is allowed only if the actual file independently matches the same frozen size and SHA256.

## Scientific question

Can this exact local 7B artifact inherit the P7 packed-residency architecture without allocating Vulkan memory or running inference?

## P8-A gate

The planner and memory gate are unchanged from P8-A: qwen2 metadata, P7-supported F32/Q4_K/Q6_K tensors only, valid non-overlapping spans, deterministic <=256 MiB arenas, no single tensor >256 MiB, and total planned GPU-visible bytes <=15.25 GiB after the frozen 2 GiB reserve.

P8-A has no performance gate. Full inference remains forbidden until PASS.
