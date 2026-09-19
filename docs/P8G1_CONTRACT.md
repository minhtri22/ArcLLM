# P8-G1 Contract — L3 FFN-down causal decomposition

## Parent evidence

P8-G is a genuine FAIL under its frozen gate on the exact 7B target.

Authoritative P8-G SHA256:
- shader provenance: C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9
- four-layer results: BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129
- summary: 0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE

P8-G executed exactly layers [0,1,2,3], 60 dispatches in one submit, direct GPU handoff, valid 19-arena/341-piece binding, and all K/V rows passed. The first failing checkpoint was L3.ffn_down.

Observed parent facts:
- L0, L1 and L2: all 17 checkpoints PASS.
- L3 through swiglu: PASS.
- L3.swiglu: max_abs 0.00838851928711, RMSE 7.10637175468e-05.
- L3.ffn_down: max_abs 0.0283279418945, RMSE 0.000364843778882, FAIL by max_abs only.
- L3.final_layer_output: max_abs 0.0276565551758, RMSE 0.000364470381402, FAIL.
- L0-L2 ffn_down format: Q6_K.
- L3 ffn_down format: Q4_K.
- L3 V projection is also Q4_K and still PASS.

P8-G remains FAIL. P8-G1 is a diagnostic experiment and cannot retroactively convert P8-G to PASS.

## Scientific question

What causes the first P8-G failure at L3.ffn_down?

Distinguish at least these hypotheses:

H-KERNEL: the optimized P7-G Q4_K tiled16 down-projection implementation is incorrect for the exact L3 FFN-down shape/weights/input.

H-AMPLIFICATION: the Q4_K down projection is correct on a fixed input, but the small upstream GPU-vs-CPU difference at L3.swiglu is amplified enough by the down operator to cross the frozen max_abs gate.

H-COMMON-Q4: both P7-G tiled16 and the independent P7-C Q4_K tiled path disagree with CPU on the same input, indicating a common Q4_K decode/reference/binding issue rather than a tiled16-specific error.

H-INTERACTION: neither kernel-only nor input-only effect crosses the gate alone, but their combination does.

No hypothesis is selected in advance.

## Frozen target

Use the same exact model SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Focus tensor:
blk.3.ffn_down.weight

Required format:
Q4_K

Exact matrix geometry:
- n = 18944
- rows = 3584
- batch = 4
- no bias

The tensor must resolve to exactly one P8-D/P8-G physical slice. Before numerical tests, prove the arena binding maps to the exact original GGUF byte span.

## Controlled inputs

Reproduce the four-layer prefix only through L3.swiglu and retain both vectors:

X_CPU = independent CPU L3.swiglu result.

X_GPU = actual GPU L3.swiglu result produced through direct L0->L1->L2->L3 composition.

The observed X_GPU vs X_CPU metrics must reproduce the P8-G L3.swiglu checkpoint within deterministic floating-point equality/tolerance.

## Required outputs

Using the exact same blk.3.ffn_down.weight:

Y_CC = CPU_down(X_CPU)

Y_CG = CPU_down(X_GPU)

Y_GC_P7G = GPU P7-G Q4_K tiled16 down(X_CPU)

Y_GG_P7G = GPU P7-G Q4_K tiled16 down(X_GPU)

Y_GC_P7C = GPU P7-C Q4_K tiled8 down(X_CPU)

Y_GG_P7C = GPU P7-C Q4_K tiled8 down(X_GPU)

No quantization conversion, re-encoding or replacement weight is permitted.

Teacher-forced X_CPU GPU calls are diagnostic interventions only; they are not part of P8-G execution and cannot be used to claim P8-G PASS.

## Required comparisons

C0 — parent reproduction:
Y_GG_P7G vs Y_CC must reproduce the P8-G L3.ffn_down failure.

C1 — optimized kernel, fixed CPU input:
Y_GC_P7G vs Y_CC.

C2 — independent GPU Q4 control, fixed CPU input:
Y_GC_P7C vs Y_CC.

C3 — input amplification under CPU reference:
Y_CG vs Y_CC.

C4 — optimized kernel on actual GPU input:
Y_GG_P7G vs Y_CG.

C5 — independent GPU Q4 control on actual GPU input:
Y_GG_P7C vs Y_CG.

C6 — tiled16 vs tiled8 direct comparison:
Y_GC_P7G vs Y_GC_P7C and Y_GG_P7G vs Y_GG_P7C.

For all CPU-vs-GPU correctness comparisons, retain the frozen numerical gates:
- finite values required;
- max_abs <= 0.02;
- RMSE <= 0.005.

Do not relax these gates after evidence.

## Adjudication

Support H-KERNEL when C1 fails while C2 passes. This localizes the defect to the P7-G tiled16 specialization rather than the common Q4_K decode path.

Support H-COMMON-Q4 when C1 and C2 both fail materially on X_CPU. Stop and audit Q4_K decode/reference/binding before changing prefix depth.

Support H-AMPLIFICATION when C1 and C2 pass, while C3 alone crosses the frozen max_abs gate and the actual-path C0 failure is reproduced.

Support H-INTERACTION when C1, C2 and C3 each pass individually, but C0 fails and same-input/kernel comparisons show the combined perturbations are required.

If C1 and C2 disagree only on X_GPU but both pass on X_CPU, classify as input-dependent tiled-kernel sensitivity and do not merge it into H-KERNEL without a further controlled test.

## Exclusions

P8-G1 does not:
- change any shader;
- change numerical gates;
- change model weights;
- execute layer 4+;
- test performance;
- permit full inference, decode or generation.

## Next

After P8-G1 adjudication:
- tiled16-specific defect -> design a bounded P7-G Q4 tiled16 repair/validation before re-running P8-G;
- amplification -> design an error-budget/scale-aware correctness study without changing the frozen P8-G verdict;
- common Q4 issue -> return to Q4_K decode/binding validation;
- interaction -> design one additional controlled decomposition.

P8-H is blocked until the P8-G failure is causally resolved and P8-G is rerun under its original frozen gates.
