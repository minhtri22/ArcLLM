from pathlib import Path

root = Path(__file__).resolve().parents[1]
api = (root / "include/arcllm/v1/runtime.h").read_text(encoding="utf-8")
rt = (root / "src/arcllm_v1_runtime.cpp").read_text(encoding="utf-8")
shader = (root / "shaders/token_xray_r1_capture_row.comp").read_text(encoding="utf-8")
compile_ps1 = (root / "tools/compile_arcllm_v1_full_runtime_q4_v4.ps1").read_text(encoding="utf-8")

assert "capture_representation_trajectory = false" in api
assert "struct RepresentationState" in api
assert "std::vector<RepresentationState> representation_states" in api
assert "representation_prefill_capture_dispatches" in api
assert "representation_decode_capture_dispatches" in api

assert "const uint32_t R1_CAPTURE_STATES=LAYERS+1u" in rt
assert "token_xray_r1_capture_row.spv" in rt
assert 'append_r1_capture(ops,&b_h0,0u,seq-1u);' in rt
assert 'append_r1_capture(ops,nxt,l+1u,seq-1u);' in rt
assert 'if(capture_this_step)append_r1_capture(ops,&b_h0,0u,0u);' in rt
assert 'if(capture_this_step)append_r1_capture(ops,nxt,l+1u,0u);' in rt
assert 'collect_r1_states("prefill",input_ids.back(),seq-1u);' in rt
assert 'collect_r1_states("decode",decode_input_token,pos);' in rt
assert "dchain_capture!=capture_this_step" in rt

# Hook-order guards: copy after embedding; block copy after FFN residual and before swap.
prefill_embedding = rt.index('addop(ops,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",{emb0,emb1,&b_ids,&b_h0}')
prefill_capture = rt.index('append_r1_capture(ops,&b_h0,0u,seq-1u);', prefill_embedding)
assert prefill_capture > prefill_embedding

prefill_residual = rt.index('addop(ops,p+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(PCN{seq*H})')
prefill_block_capture = rt.index('append_r1_capture(ops,nxt,l+1u,seq-1u);', prefill_residual)
prefill_swap = rt.index("std::swap(cur,nxt);", prefill_residual)
assert prefill_residual < prefill_block_capture < prefill_swap

decode_anchor = rt.index("auto build_decode=")
decode_residual = rt.index('addop(ops,p+"ffn_residual","p7_add.spv",{&b_r1,&b_d,nxt},push_bytes(PCN{H})', decode_anchor)
decode_block_capture = rt.index('if(capture_this_step)append_r1_capture(ops,nxt,l+1u,0u);', decode_residual)
decode_swap = rt.index("std::swap(cur,nxt);", decode_residual)
assert decode_residual < decode_block_capture < decode_swap

# Canonical model graph accounting remains distinct from observer dispatches.
assert "result.stats.prefill_dispatches=EXPECT_PREFILL" in rt
assert "result.stats.decode_dispatches_per_step=EXPECT_DECODE" in rt
assert "representation_prefill_capture_dispatches" in rt
assert "representation_decode_capture_dispatches" in rt

# Capture shader is a dead-end row copy: one read-only source, one write-only capture target.
assert "readonly buffer SrcBuffer" in shader
assert "writeonly buffer DstBuffer" in shader
assert "dst[i] = src[pc.row * pc.hidden + i];" in shader
for forbidden in ["atomic", "imageStore", "barrier()", "memoryBarrier"]:
    assert forbidden not in shader

assert '"token_xray_r1_capture_row.comp"' in compile_ps1

# R1 runtime implementation must not emit semantic/mechanistic interpretation.
for forbidden in [
    "semantic_meaning",
    "concept_name",
    "nearest_semantic_neighbors",
    "attention_source_attribution",
    "ffn_source_attribution",
    "causal_mechanism",
    "importance_score",
    "logit_attribution",
    "neuron_feature_interpretation",
]:
    assert forbidden not in rt

print("TOKEN_XRAY_R1_REPRESENTATION_CAPTURE_STATIC_QA=PASS")
