# P8-C Implementation

Two dedicated correctness-probe shaders isolate segmented vocab access from the rest of the model.

`p8c_embedding_q4k_segmented_probe.comp` binds embedding segment 0, embedding segment 1, token IDs and output. It performs direct Q4_K dequantization and chooses the segment with `row < 133152`.

`p8c_lmhead_q6k_segmented_probe.comp` binds output segment 0, output segment 1, requested global row IDs, a deterministic FP32 hidden vector and result output. It performs the existing Q6_K scalar dot-product path with `row < 91304` segment selection.

The native harness reuses the validated SDK-less Vulkan runtime and TensorStore, allocates only the four vocab segments plus small probe buffers, computes CPU references directly from the original GGUF mapping, executes exactly two dispatches and compares results.

Shaders are compiled with pinned glslang 16.5.0 and provenance is written to results/shader_provenance.json. P8-C is correctness-only.