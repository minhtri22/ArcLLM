# ArcLLM v1 — NPU Capability / Transfer / Amdahl Gate

**Gate:** `ARCLLM_V1_NPU_CAPABILITY_TRANSFER_AMDAHL_GATE`  
**Type:** scientific feasibility gate before implementation  
**Production NPU executor:** forbidden until the gate authorizes one.

## Scientific question

On the clean canonical ArcLLM-v1 runtime, is there a current operator/workload surface that the host NPU can actually execute with a valid representation and transfer path, and whose removable share is large enough under Amdahl's law to justify a bounded NPU implementation study?

The gate separates three questions that must not be conflated:

1. **Capability** — an NPU device and callable execution provider exist.
2. **Transfer/representation** — ArcLLM's current Q4_K/Q6_K weights and F32 activations can reach that provider without an invalid or per-token full-weight conversion path.
3. **Amdahl headroom** — the candidate occupies enough exact-target runtime share that even an optimistic NPU acceleration could matter.

NPU peak TOPS alone cannot pass this gate.

## Frozen rules

- No NPU production executor is written.
- No driver or firmware change.
- Initial provider inventory occurs before any package installation.
- One-time model-load representation conversion is admissible; per-token full-weight repacking is not.
- Shared physical memory is not treated as proof of zero transfer cost.
- Cross-domain activation movement must be accounted explicitly.
- The minimum ideal zero-cost Amdahl ceiling is 1.10×.
- Existing historical timing may locate candidate families, but current canonical differences must be respected.
- Any provider microprobe, if needed, is a successor bounded capability experiment, not an ArcLLM runtime implementation.

## Candidate surfaces

Initial candidate families are `lm_head`, `ffn_down`, `ffn_gate_up`, and `attn_qkv`. This list is based on existing exact-target device-work evidence and does not itself authorize offload.

## Outcome

Only `PASS_OPEN_BOUNDED_NPU_TRANSFER_STUDY` authorizes a new NPU transfer experiment. All other outcomes either close the current path or require a narrower provider-level capability probe first.
