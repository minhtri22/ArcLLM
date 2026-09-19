# P7 Closeout — Q4_K_M production path

Date: 2026-09-19

## Decision

P7 is CLOSED with **P7-L** as the frozen production prefill winner.

This is a production-path closeout, not a claim of globally optimal GPU performance. The repository had no separate global P7 throughput gate; the >=1.10x thresholds were pre-registered per optimization option. P7 therefore closes on architecture/correctness readiness plus an evidence-based stopping rule rather than inventing a new post-hoc throughput target.

## Frozen production composition

- Direct packed Q4_K / Q6_K weights; no full-model expansion before shader execution.
- Whole decoder resident in GPU-visible memory.
- GPU-resident KV cache across decode submissions.
- Attention Q/K/V/O projections: frozen tiled projection path from P7-E.
- FFN gate+up: frozen fused Q4_K path from P7-L.
- FFN down: frozen P7-G row8 x token16 x K32 Q4_K/Q6_K path.
- SwiGLU remains separate.
- Online prefill attention and cached decode attention remain unchanged.
- Decode remains the validated P7 decode graph.

## Winner evidence

P7-L:
- correctness PASS;
- exact top1/logits agreement in all A/B trials;
- pp512 same-run speedup = 1.242926134x versus its frozen baseline;
- 56 separate gate/up GEMMs reduced to 28 fused dispatches.

P7-M re-profile of the exact P7-L winner:
- prefill dispatches = 441;
- decode dispatches = 469;
- all inherited and fused regressions PASS;
- timestamp_valid_bits = 64;
- prefill shares: gate/up 44.4034%, FFN-down 30.4215%, attention 9.3385%, attn_qkv 7.5939%, attn_output 5.8623%;
- barrier/unattributed ~= 0.0243%.

## Rejected post-winner options

- P7-I token tile16 -> 32: 1.003404444x — FAIL.
- P7-J block-aware vec4 dequant: 0.9821774944x — FAIL.
- P7-K row tile8 -> 16: 1.068446103x — FAIL.
- P7-N fuse SwiGLU into gate+up: 1.051612109x — FAIL.
- P7-O FFN-down K32 -> K64: 0.8836691312x — FAIL.

All above correctness-preserving negatives remain evidence and are not promoted.

P7-O R1 authoritative evidence SHA256:
- p7o_ab_results: 814D72E96F082575BF160CF8DE02102BA69DDAA79494A07E9EEDE67F39DE6624
- p7o_summary: 9C5D3CC11EAE52CC7BF7C55BD2F479F22B0456C113E3BE07E476356700C894B6
- p7o_shader_provenance: 59AD51C2DA5D68D01C387592635D675D674490CCC8BCE4B060DB93984B898EC2

## Why stop here

P7-M shows the largest remaining untouched non-FFN family is the attention kernel at 9.3385% of chain time. Even eliminating that family entirely would yield only about 1.1030x theoretical end-to-end speedup on that profile; meeting a 1.10x option gate would therefore require removing almost all attention cost. attn_qkv and attn_output individually have theoretical maxima below 1.10x.

The two largest FFN families were already challenged after P7-L:
- gate/up adjacent work: P7-N improved only 1.0516x and failed the gate;
- FFN-down K64: P7-O regressed to 0.8837x.

Continuing P7 with increasingly broad multi-family changes would break the one-option-at-a-time attribution rule. Remaining optimization work belongs in a future performance track, not as a blocker for the production Q4 path.

## Known limitations carried forward

- Absolute throughput varies substantially across target-machine runs; scientific decisions rely on same-run interleaved A/B and device attribution, not cross-run absolute tok/s.
- Decode remains much slower than the external R8-VK reference and has not received the same optimization depth as prefill.
- Attention subgroup/block redesign remains plausible future work.
- P7 closeout does not validate 7B memory fit; that is P8.

## Next

P8 — 7B memory-planned runtime. Freeze the exact 7B GGUF target and SHA256 before implementation.
