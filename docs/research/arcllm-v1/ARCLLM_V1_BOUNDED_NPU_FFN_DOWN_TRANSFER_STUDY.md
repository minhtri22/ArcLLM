# ArcLLM v1 — Bounded NPU FFN-down Transfer Study

This study is the only active ArcLLM-v1 research study after the NPU capability/transfer/Amdahl gate.

## Single scientific question

Using exact real FFN-down weights from the current model, does a model-load-only NPU representation preserve numerical semantics and retain a positive warm transfer budget after activation movement and graph lifecycle are accounted for?

The study does not modify the canonical runtime and does not implement an NPU backend.

## Frozen sample

The 28 FFN-down layers are split structurally into the already-established 14 Q4_K and 14 Q6_K sets. Before observing any real-weight NPU outcome, the first, upper-middle, and last layer of each quant set are selected:

- Q4_K: L03, L14, L22
- Q6_K: L00, L16, L27

Each layer is evaluated with three frozen deterministic activation vectors.

## Semantics

The exact GGUF quantized tensor is dequantized to FP32 as the source reference. The NPU representation is produced once by converting the dequantized weight to FP16 constant weights. The activation crosses the provider boundary as FP16. End-to-end NPU output is compared with the exact dequantized FP32 source reference.

Every one of the 18 layer × activation cases must be finite and satisfy max_abs <= 0.02 and RMSE <= 0.005.

## Transfer and lifecycle

For each sampled layer, dequantization/conversion, compile, export, and import are timed separately. Warm timing begins only after graph import and three warmups. The timed infer call includes the provider API boundary for the FP16 input and output materialization.

The primary warm metric is the p95 of the 18 case medians, each case median formed from 9 synchronous imported-graph inferences.

Positive budget requires:

`28 * p95_case_median < min(160.5974302, 118.1383591) ms/token`.

A >=10% family-level margin on the tighter W-C budget is reported as the material secondary criterion.

No full-model execution is required or allowed by this study.
