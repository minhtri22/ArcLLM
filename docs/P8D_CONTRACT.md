# P8-D Contract — segmented access integration into the 7B graph

## Parent evidence

P8-C is a genuine PASS on the frozen 7B target.

Authoritative evidence SHA256:
- shader_provenance.json: 25D7E7D01F033B85692E83C102D269A37E21988D229191BCA5FF51DBC0E118E3
- p8c_access_results.json: 9773C48A7D895EB3D22B993132854E58BC0668288725E5186E80D3462D4D5340
- p8c_summary.json: 8620A9089CF9066088F5E30543864D7A17B87F4DC55CEFE1CD10320C01574CD4

P8-C proved both layers of the segmented-access contract independently:
- global-row -> segment/local-row mapping equivalence passes for embedding and LM-head probes;
- GPU numerical access passes across both exact segment boundaries.

Embedding compared 28,672 values with max_abs=0 and RMSE=0. LM-head compared eight selected logits with max_abs=2.38418579102e-07 and RMSE=1.11027394095e-07. Exactly two dispatches were submitted once. Full inference remained forbidden.

## Scientific question

Can the frozen P7-L production graph representation consume the exact P8-A2 7B segmented tensor map through one graph-level tensor-binding resolver, while preserving byte identity and existing non-segmented tensor semantics, before any decoder-layer execution is attempted?

## Single integration hypothesis

Replace graph assumptions that a logical tensor always has one contiguous physical binding with a deterministic tensor-binding descriptor:

- ordinary tensor -> exactly one arena slice;
- token_embd.weight -> exactly two row-aligned slices from P8-A2;
- output.weight -> exactly two row-aligned slices from P8-A2.

No quantization format, tensor bytes, layer math, context, KV layout, P7-L kernel choice, or P8 memory budget may change.

## Frozen scope

P8-D is a graph-wiring qualification, not a 28-layer inference run.

It must:

1. Load the exact target SHA256 and exact P8-A2 19-arena / 341-piece plan.
2. Build the 28-layer Qwen2 graph metadata for layers=28, hidden=3584, q_heads=28, kv_heads=4, head_dim=128, ffn=18944 and vocab=152064.
3. Resolve every graph-referenced weight tensor to physical arena slice(s).
4. Require exactly two segmented logical tensors: token_embd.weight and output.weight.
5. Preserve one-piece resolution for every non-segmented tensor.
6. Prove every resolved byte span equals the original GGUF tensor span, with no overlap, omission or duplicated ownership.
7. Route the P8-C endpoint probes through the graph-level resolver/executor rather than direct hard-coded tensor offsets.
8. Execute only the two endpoint correctness operations:
   - segmented embedding probe;
   - selected-row segmented LM-head probe.
9. Keep all decoder-layer, KV-attention, prefill, decode and generation execution disabled.

## PASS gate

P8-D PASS requires all of:

- exact target SHA/size and parent P8-C evidence match;
- graph metadata matches the frozen 7B architecture;
- exact 339 GGUF tensor census is retained;
- exact 19 arenas and 341 physical pieces are retained;
- graph-required tensor resolution has zero missing or ambiguous bindings;
- exactly two logical tensors resolve to multiple physical pieces;
- every other graph-referenced tensor resolves to one piece;
- exhaustive resolved-byte-span equivalence passes;
- endpoint probe rows are unchanged from P8-C;
- P8-C numerical thresholds remain max_abs <= 0.02 and RMSE <= 0.005;
- mapping equivalence remains independently required;
- exact two endpoint dispatches in one submit;
- no decoder layer executes;
- no performance gate.

P8-D must not claim model inference correctness. It qualifies only graph-level binding and endpoint execution.

## Failure interpretation

- Package/build/compiler failure -> repair infrastructure only; no scientific verdict.
- Resolver census/span failure -> graph-binding architecture FAIL; do not bypass segmentation or enlarge the arena cap.
- Endpoint numerical failure after resolver/span PASS -> adjudicate graph binding/descriptor propagation before any layer execution.

## Next

PASS -> P8-E bounded single-layer 7B graph correctness bring-up.

FAIL -> adjudicate the exact P8-D graph-binding obstruction.

Full 28-layer prefill/decode remains forbidden after P8-D PASS.
