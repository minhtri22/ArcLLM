# ArcLLM Q1 Adjudication — real-model end-to-end feasibility

Date: 2026-09-20

## Verdict

**Q1_FEASIBILITY_ESTABLISHED**

This closes Q1 under `docs/P8_Q1_END_TO_END_CONTRACT.md`.

The exact frozen 7B model completed end-to-end prefill plus autoregressive cached decode on the target Intel Arc runtime with the frozen P8 residency architecture.

This verdict establishes feasibility only. It does **not** establish performance or resource advantage.

## Authoritative execution

Implementation commit:
`ec83bf42727f799e31d3900a7545e2642b3b90eb`

Exact target:
- SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`
- bytes: `4,683,074,048`
- qwen2 / 28 layers / hidden 3584 / FFN 18944 / vocab 152064.

Environment:
- Windows 11 build 26200;
- Intel Core Ultra 7 258V, 8 cores / 8 logical processors;
- Intel Arc 140V GPU (16GB family), driver 32.0.101.8860.

The standalone `vulkaninfo --summary` environment probe emitted a Windows loader registry warning, but the actual Vulkan Q1 runtime initialized and completed the full frozen execution. This warning therefore does not invalidate Q1.

## F0 — PASS A/B

Both independent executions satisfy the frozen structural/runtime gate.

Observed:
- 19 weight arenas;
- 341 physical tensor pieces;
- exactly two segmented logical tensors;
- span equivalence PASS;
- global coverage PASS;
- required Vulkan memory flags PASS;
- requested total residency 5,149,055,000 bytes <= frozen P8-B envelope 5,347,770,372 bytes and <= usable budget 16,374,562,816 bytes;
- exact 441 prefill dispatches;
- exact 469 dispatches for every cached decode step;
- no CPU model-math fallback;
- no CPU teacher forcing.

## F1 — PASS A/B

For prefill and all four cached decode steps:
- full logit count = 152064;
- logits finite;
- greedy argmax valid;
- feedback token valid;
- positions advance correctly.

Each execution generates exactly five token IDs:
`[128275,128301,128275,128301,128275]`.

## F2 — PASS

Executions A and B produce exactly the same generated-token sequence.

The observed repeat is stronger than the minimum frozen gate:
- per-step top-1 and top-2 IDs are identical;
- per-step logits FNV1a64 hashes are identical;
- per-step final-normalized-hidden FNV1a64 hashes are identical;
- recorded logits and margins are identical.

These stronger observations are descriptive corroboration; the frozen F2 decision remains exact token-sequence equality.

## F3 — PASS

The evidence manifest is complete and byte hashes of the uploaded evidence were independently rechecked.

Authoritative SHA256:
- `q1_end_to_end_results.json`: `FAD892B25C3A82F62C0CC8060F413792B8B0B73F78A7C3B4CF969E19753E63FA`
- `q1_shader_provenance.json`: `21CDDC88E086EAA8EC916379F518B2E750B2164010F468446223C4691B2BC496`
- `q1_environment.json`: `EB0E7B09A65ED50D67AC047918CC460843DD363773EE562689DE17FE3260DA20`
- `q1_console.log`: `A36ECA73DABC05D23DFE808487BAD561464ED77B7A36ED991477ACBC40C4D301`
- `q1_summary.json`: `36BFB413BCE31A1A6E2B77F96071162809B48FC7F566D61B34C779D90388AC95`
- `q1_evidence_manifest.json`: `89B22986B6BF5F4776413BD13D0F3CDAEF8B13B9EAA1B11000D9027F0F8F9548`
- returned ZIP: `DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43`

The ZIP contains the six returned Q1 artifacts and each contained file hash matches the separately uploaded artifact.

Shader provenance:
- glslang 16.5.0;
- pinned compiler asset SHA256 `06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE`;
- Vulkan 1.2 target;
- 16 compiled shaders.

## Governance

Q1 is CLOSED.

Historical P8-G through P8-G6 outcomes remain frozen and are not rewritten.

Q1 timing values are explicitly descriptive and must not be reused as Q2/Q3 performance claims.

Next scientific step:
**Q2 — frozen matched performance/resource characterization.**

No new subsystem correctness study is authorized by this Q1 PASS.
