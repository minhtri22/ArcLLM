# ArcLLM v1 — Historical-Evidence Headroom Map

**Derived from:** ArcLLM v1 Architecture Learning Methodology + Architecture Reframe  
**Methodology blob:** `bd943fe99e0a6758f8e0aa8060f9540a0b74a942`  
**Architecture-reframe blob:** `5f06a648d200366066e79c9355ae3bb448e1c8fb`  
**Status:** INITIAL HEADROOM MAP / QA-AMENDED HISTORICAL EVIDENCE ONLY  
**No new performance execution in this artifact**

## 1. Purpose

This document converts historical ArcLLM PASS/FAIL evidence into an initial map of:

- current end-to-end cost concentration;
- demonstrated recoverable headroom;
- suspected maturity debt;
- possible structural limits;
- unresolved measurement gaps.

The map is used to select ArcLLM v1 Intervention-001.

It does not rewrite historical verdicts.

## 1A. Headroom vocabulary and confidence rubric

This document uses the methodology evidence classes: `MEASURED`, `DERIVED`, `BOUND`, and `HYPOTHESIS`.

A Headroom Map entry separates three different ideas that must not be collapsed:

- **system headroom envelope:** an Amdahl-style upper bound if a broad cost region improved;
- **attributed recoverable headroom:** the portion linked to a specific, evidenced mechanism;
- **unattributed gap:** measured difference for which no causal recovery mechanism is yet established.

Ordinal confidence means:

- **HIGH:** exact provenance plus replicated or independently corroborated evidence, with narrow claim scope;
- **MEDIUM:** exact evidence exists but causal attribution or generalization is incomplete;
- **LOW:** primarily hypothesis-generating or dependent on weak/incomplete instrumentation.

`VERY HIGH/MEDIUM/LOW headroom` is not a statistical scale. When used, it must specify whether it refers to a **system envelope** or **attributed recoverability**.

## 2. Evidence anchors

Historical source artifacts:

| Evidence | Git blob | Role |
|---|---|---|
| Q2 matched characterization | `295f87e70bfe9fe409eb4b5f16f779933530ea31` | matched ArcLLM vs llama.cpp cost gap |
| Q3 formal adjudication | `d1719bbc9e8d9dc89b67b25360944c447fa1fa3e` | fresh reproduction of no demonstrated advantage |
| P7 production closeout | `1b5b81646575384c44630baf1cce8588ac8d939d` | prefill/decode dispatch census and kernel shares |
| SA1 closeout | `be070085183df7cd120a77081dfa255846de69b5` | Q4 component headroom and Q6 boundary |
| ANL64 final adjudication | `dd0ab0389251e77ccc2d654e77a897781426be34` | successor decode/E2E gain with TTFT harm |
| TTFT-M2 P9D | `228e33410fd5e67c67a08771c5ed0c896e0e51e1` | preregistered TTFT mechanism non-replication |

## 3. Matched Q2 system gap

Q2 frozen descriptive medians:

| Workload | System | TTFT ms | Decode tok/s | E2E ms | Working set |
|---|---|---:|---:|---:|---:|
| W-S | ArcLLM | 918.265 | 0.3184 | 98,285.231 | 10,017,341,440 |
| W-S | llama.cpp | 100.318 | 12.7449 | 2,535.868 | 5,467,303,936 |
| W-C | ArcLLM | 14,706.425 | 0.3990 | 91,043.729 | 10,006,102,016 |
| W-C | llama.cpp | 1,423.482 | 13.0354 | 3,803.029 | 5,481,332,736 |

Historical ratios:
- W-S TTFT: 9.154× slower;
- W-S decode throughput: 0.02499× baseline;
- W-S E2E: 38.758× slower;
- W-C TTFT: 10.331× slower;
- W-C decode throughput: 0.03061× baseline;
- W-C E2E: 23.940× slower;
- working set: approximately 1.83× baseline in both workloads.

Private bytes were about 3% lower than baseline, but Q2/Q3 correctly treated that as insufficient for practical advantage.

## 4. E2E cost concentration

Using Q2 medians, define a coarse post-TTFT remainder:

[
T_{post} = T_{E2E} - T_{TTFT}
]

This is not assumed to equal pure GPU decode time. It contains the complete post-TTFT generation path and is used only as an E2E headroom envelope.

### W-S

```text
E2E        98,285.231 ms
TTFT          918.265 ms
post-TTFT  97,366.966 ms
post share      99.066%
TTFT share       0.934%
```

### W-C

```text
E2E        91,043.729 ms
TTFT       14,706.425 ms
post-TTFT  76,337.304 ms
post share      83.847%
TTFT share      16.153%
```

The corresponding post-TTFT ArcLLM/baseline envelopes are approximately:

```text
W-S  39.98×
W-C  32.08×
```

This makes the post-TTFT/decode execution plane the dominant system-level gap under the frozen 32-token workloads.

## 5. Amdahl ceilings

### TTFT-only intervention ceiling

Using the Q2 measured share (f_{TTFT}), an idealized intervention that removes TTFT completely has ceiling:

[
S_{E2E,TTFT\to0}=\frac{1}{1-f_{TTFT}}
]

Under the historical Q2 partition:

```text
W-S maximum E2E speedup ≈ 1.009×
W-C maximum E2E speedup ≈ 1.193×
```

Therefore TTFT-only optimization cannot be Intervention-001 for the existing 32-token E2E regimes. This conclusion is **regime-specific** and does not apply automatically to short-output workloads.

### Post-TTFT/decode-plane intervention

For post-TTFT share (f_{post}) and hypothetical improvement (s):

[
S_{E2E}(f_{post},s)=\frac{1}{(1-f_{post})+f_{post}/s}
]

| Hypothetical post-TTFT speedup | W-S E2E envelope | W-C E2E envelope |
|---:|---:|---:|
| 2× | 1.981× | 1.722× |
| 3× | 2.945× | 2.267× |
| 5× | 4.820× | 3.037× |
| 10× | 9.224× | 4.075× |

Idealized zero-cost ceilings are approximately:
- W-S: 107.0×;
- W-C: 6.19×.

These are **system headroom envelopes**, not evidence that the corresponding share is recoverable.

## 6. Decode execution maturity evidence

P7 reports:
- prefill dispatches = 441;
- cached decode dispatches = 469 per step;
- decode remained much slower than the external R8-VK reference;
- decode did not receive the same optimization depth as prefill.

This is evidence of incomplete decode maturity.

It does not prove that dispatch count itself is the sole cause.

### ANL64 successor evidence

Against the exact safe ArcLLM reference, ANL64 fresh decode ratios were:

```text
A / W-S   2.5318×
A / W-C   1.2228×
B / W-S   2.1635×
B / W-C   3.2002×
```

Fresh E2E latency ratios candidate/reference:

```text
A / W-S   0.4044  → ~2.47× E2E speedup
A / W-C   0.8765  → ~1.14×
B / W-S   0.4709  → ~2.12×
B / W-C   0.3984  → ~2.51×
```

ANL64 demonstrates that an **integrated ArcLLM-family successor can move decode and E2E by multi-x amounts** relative to the safe ArcLLM reference. Because ANL64 changed a bundled successor execution path, it does not identify which individual mechanism produced each part of the gain.

Its overall claim failed because TTFT worsened in three of four comparisons.

Interpretation for v1:
- **integrated decode-plane recoverability is supported at the system level**;
- attribution to dispatch granularity, kernel compute, locality, lifecycle, or another maturity-debt subclass remains unresolved;
- integration coupling can create a new startup penalty;
- future decode architecture must preserve/measure TTFT separately.

## 7. Prefill maturity evidence

P7 demonstrated large real component improvements:

- P7-C packed FFN tiling: 5.1829× pp512 speedup versus its frozen baseline;
- P7-E attention-projection tiling: 1.8488×;
- P7-G FFN token tile16: 1.2246×;
- P7-L fused Q4 gate+up: 1.2429×.

P7-L final profile shares:

```text
gate/up      44.4034%
FFN-down     30.4215%
attention     9.3385%
attn_qkv      7.5939%
attn_output   5.8623%
```

P7 correctly noted that eliminating the untouched attention family entirely would yield only about 1.103× on that prefill profile.

Interpretation:
- significant prefill maturity debt was already harvested;
- individual remaining prefill kernel families now have lower system ceiling than the decode plane;
- the remaining 9–10× external TTFT gap suggests architecture/integration debt remains, but old TTFT-M2 mechanisms do not provide a stable explanation.

## 8. TTFT mechanism evidence

TTFT-M2 valid fresh collection:

```text
H-ART               FALSIFIED
H-DPIPE              NOT SUPPORTED
H-PRECOND            NOT SUPPORTED
H-STATE-INTERACTION  NOT SUPPORTED
H-NULL               SUPPORTED
```

This does not imply no TTFT mechanism exists.

It means the preregistered candidates should **not** be promoted into v1 as if they were established causes.

Initial Headroom Map therefore marks TTFT cause as:

`MEASUREMENT_GAP / NEW_MECHANISM_REQUIRED`

not as Intervention-001.

## 9. Quant-specific evidence

SA1:

```text
Q4 split-K component speedup  ~3.14×  PASS
Q6 same mechanism             correctness FAIL
```

Interpretation:
- strong Q4 maturity headroom exists at component level;
- cross-quant generality is falsified for the unchanged mechanism;
- Q4 component gain has not been shown to dominate current E2E gap;
- Q6 requires its own execution/numerical design.

Quant specialization remains a v1 architecture principle, but it is not the first system intervention.

## 10. Memory headroom

Matched working-set ratio:

```text
W-S ~1.832× baseline
W-C ~1.825× baseline
```

Private bytes are slightly lower than baseline.

This mixed signal means the memory story is not yet causally decomposed.

Possible debt/concerns:
- residency representation;
- mapped/visible memory behavior;
- duplicate working state;
- executable/runtime overhead;
- difference between process working set and private committed bytes.

Initial classification:

`MEASUREMENT_GAP with STRUCTURAL_CONCERN`

Memory may become a dedicated regime-advantage track, especially for integrated/shared-memory hardware, but historical evidence does not support ranking it above decode for current E2E performance.

## 11. Initial Headroom Map

| Area | Current historical evidence | E2E share / gap | Recoverable headroom | Structural concern | Confidence | Initial priority |
|---|---|---|---|---|---|---|
| Decode/post-TTFT execution plane | ~32–40× post envelope vs baseline; 469 dispatches/step; ANL64 integrated multi-x movement | 99.1% W-S, 83.8% W-C | system envelope VERY HIGH; attributable recoverable share UNKNOWN | UNKNOWN | HIGH for gap / MEDIUM for causal attribution | **1** |
| Prefill/TTFT architecture | ~9–10× TTFT gap; many component wins already harvested | 0.9% W-S, 16.2% W-C in current E2E | MEDIUM | MEDIUM | HIGH for gap, LOW for cause | 2 |
| Memory/working-set topology | ~1.83× working set, ~3% lower private bytes | indirect | UNKNOWN | MEDIUM/HIGH | MEDIUM | 3 |
| Q4 specialized kernels | ~3.14× component result in SA1 | local only | HIGH locally | LOW for Q4 | HIGH | subordinate |
| Q6 specialized kernels | unchanged Q4 mechanism fails correctness | local only | UNKNOWN | HIGH | HIGH boundary evidence | separate study |
| Existing TTFT-M2 mechanisms | preregistered mechanisms do not replicate | TTFT only | LOW for old mechanisms | — | HIGH | do not prioritize |

## 12. Maturity debt vs structural gap classification

### Supported integrated recoverability; maturity-debt subclass unresolved

**Decode execution plane**
- less optimized than prefill;
- enormous external gap;
- ANL64 demonstrates multi-x integrated movement;
- the specific recoverable share attributable to dispatch, synchronization, locality, lifecycle, or kernels is not yet identified.

### Possible structural gap

**Working-set / memory topology**
- substantial matched working-set disadvantage;
- insufficient decomposition to call it structural.

### Unresolved

**TTFT residual gap**
- large matched gap;
- old causal candidates not supported;
- new attribution needed later.

### Supported local specialization

**Q4 split-K**
- real component value;
- E2E carry-through not established.

## 12A. Prior-art constraints on interpreting the map

The Headroom Map incorporates these literature lessons:

- **Amdahl**: large local speedups matter only in proportion to affected system share.
- **Roofline**: a large runtime gap is not enough to identify compute versus data-movement limitation; a bound model is needed.
- **FlashAttention / FlashAttention-2**: IO and work partitioning can dominate realized performance even when semantic math is unchanged.
- **vLLM/PagedAttention**: memory fragmentation/layout can materially affect system behavior.
- **Orca/Sarathi/NanoFlow/DeepSpeed-FastGen**: iteration composition and operation scheduling can move end-to-end metrics.
- **FlashInfer**: graph compatibility, KV format, scheduling, and specialized kernels interact.

Therefore the `~32–40× post-TTFT gap` is a **measurement target**, not a pre-labeled dispatch or memory bottleneck.

## 13. First-intervention decision boundary

Intervention-001 must target:

```text
DECODE EXECUTION PLANE
```

because it uniquely combines:
- the largest current E2E share;
- the largest matched external gap;
- historical evidence of recoverable multi-x movement;
- incomplete optimization maturity;
- direct relevance to the system-level failure of ArcLLM v0.

The next artifact must choose a specific architectural mechanism inside this plane and state its falsification conditions.

## 14. What is not yet claimed

This Headroom Map does not claim:
- 469 dispatches are the sole decode bottleneck;
- persistent execution is guaranteed to win;
- ANL64's complete successor architecture should be copied;
- external baseline gap is entirely maturity debt;
- TTFT no longer matters;
- memory is not important.

Those are hypotheses for future discrimination.

## 15. Next

Create:

`ARCLLM_V1_INTERVENTION_001_SELECTION.md`

The selected intervention must maximize credible E2E headroom **without pretending the cause is already known**.


## 16. Headroom Map update after I001 historical cost discriminator — 2026-09-22

The decode plane remains the highest-priority system area, but its internal mechanism ranking changes.

Historical proxy decomposition shows:

```text
P7-H outside-submit bucket      0.4339%
P7-M outside-submit bucket      3.6454%

P7-H barrier/unattributed       0.3455%
P7-M barrier/unattributed       0.3534%

six major device families       ~99.2% device-chain time
```

ANL64 preserves a 469-node candidate plan and nevertheless produces fresh 7B decode gains of approximately 1.22–3.20×.

Updated interpretation:

| Decode mechanism class | Updated status |
|---|---|
| launch/API/lifecycle removal alone | LOW HEADROOM |
| barrier removal alone | NOT A DOMINANT HISTORICAL EXPLANATION |
| graph compression based only on reducing 469 dispatch count | NOT JUSTIFIED |
| region locality/state reuse | UNRESOLVED |
| kernel/device-work efficiency | HIGHEST NEXT DISCRIMINATION PRIORITY |
| mixed kernel + locality architecture | POSSIBLE, NOT YET QUANTIFIED |

Therefore `I001-PDEP` is falsified as the first implementation, while the broader **decode device-work plane remains priority #1**.

Exact 7B per-family timestamp evidence is still missing, so no replacement implementation is selected yet.

Next:

`ARCLLM_V1_I001R_EXACT_7B_DECODE_DEVICE_WORK_PROFILE.md`


## 17. Exact 7B I001R update — dominant gate/up family

I001R exact 7B collection is valid and formally adjudicated:

`PASS_VALID_COLLECTION_DOMINANT_DEVICE_FAMILY_FFN_GATE_UP`

Independent raw-tick recomputation:

```text
ffn_gate_up        59.6727%
lm_head            21.4557%
ffn_down           15.8168%
attn_qkv            2.5975%
attn_output         1.0218%
attention           0.2093%

barrier/unattributed 0.04596%
lifecycle outside-submit 0.08170%
```

The gate/up result replicates across all four cells:

```text
A/W-S 64.7356%
A/W-C 57.8115%
B/W-C 59.6859%
B/W-S 58.8697%
```

It also remains above 52% in every one of the 60 individual profile probes.

This closes the previous attribution gap:

- launch/lifecycle is not a first-order decode target;
- explicit barrier/unattributed cost is not a first-order decode target;
- `ffn_gate_up` is the dominant exact-7B device-work family.

Exact source mapping identifies 56 Q4_K batch-1 gate/up matvec dispatches per token, all using `p7_q4k_gemm_2d.spv` with geometry 3584→18,944.

Historical SA1 had already validated a subgroup32 split-K mechanism on that exact geometry with ~5.33–5.50× component speedup and correctness PASS.

Updated priority:

| Area | Exact evidence | Recoverable evidence | Priority |
|---|---|---|---|
| Q4 FFN gate/up decode | 59.67% exact-7B device-chain | SA1 exact-shape 5.33–5.50× component PASS | **1** |
| LM-head | 21.46% | no equally strong exact-shape mechanism carried here | 2 |
| FFN-down | 15.82% | SA1 Q4 down supported locally, Q6 extension failed correctness | 3 / quant-dependent |
| QKV/O | ~3.62% combined | ANL64/SA1 Q4 mechanism evidence exists | subordinate after gate/up |
| lifecycle/barrier | <0.1% each | negligible first-order ceiling | deprioritized |

The new selected intervention is:

`I002-Q4-GU-SG32 — Q4 FFN Gate/Up Subgroup32 Split-K Decode Executor`

No production implementation is authorized until its real-model transfer/carry-through specification is frozen.


## 18. I002 final carry-through update — PASS

I002 `Q4 FFN Gate/Up Subgroup32 Split-K Decode Executor` is formally closed:

`PASS_I002_REAL_MODEL_CARRY_THROUGH`

Primary T3 result under the original frozen v0.3 contract:

| Cell | Decode speedup | TTFT ratio | E2E speedup |
|---|---:|---:|---:|
| A/W-S | 2.3338× | 0.8841 | 2.3067× |
| A/W-C | 2.0383× | 1.0680 | 1.7020× |
| B/W-C | 2.0607× | 1.0071 | 1.7404× |
| B/W-S | 2.3890× | 1.0344 | 2.3539× |

Global decode geomean speedup: `2.1998×`.

Global geomean of cell-median E2E speedups: `2.0026×`.

All 20 measured pairs preserve generated-token semantics and improve decode and E2E latency.

Knowledge update:
- `ffn_gate_up` was a real dominant maturity-debt source;
- SA1 subgroup32 split-K transferred from component fixture → real weights/activations → full decode → E2E;
- carry-through is strong rather than being absorbed elsewhere;
- generic launch/PDEP remains deprioritized.

Claim boundary:
this is an internal candidate/reference ArcLLM result. A fresh matched external llama.cpp comparison is still required before any practical external-advantage claim.

Next priority:
fresh matched external-baseline study first; only then profile the post-I002 candidate to select the next bottleneck.
