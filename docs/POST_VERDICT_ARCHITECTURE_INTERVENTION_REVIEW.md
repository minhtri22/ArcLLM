# ArcLLM — Post-Verdict Architecture Intervention Review

**Ngày:** 2026-09-20  
**Loại:** specification-only / evidence synthesis  
**Parent verdict:** `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`  
**Current architecture:** CLOSED  
**Implementation/target execution:** FORBIDDEN

## 1. Mục đích

Review này thực hiện đúng boundary sau Q3:

> Chỉ xem xét một successor architecture khi tồn tại causal mechanism cụ thể nối evidence hiện có của ArcLLM với một intervention có khả năng tác động trực tiếp lên bottleneck đã được chứng minh.

Review được phép dùng:
1. evidence nội bộ ArcLLM;
2. finding đã được NEXUS chứng minh hoặc adjudicate;
3. finding provisional/unresolved của NEXUS chỉ như cảnh báo hoặc hypothesis, không như positive evidence;
4. paper/publication bên ngoài như independent support.

Không được chuyển PASS của NEXUS hoặc paper result thành ArcLLM evidence.

## 2. ArcLLM bottleneck đã được chứng minh

Q3 fresh reproduction, 40/40 attempts, cho thấy:

| Regime | TTFT Arc/Base | Decode Arc/Base | E2E Arc/Base | Working-set Arc/Base |
|---|---:|---:|---:|---:|
| A / W-S | 12.422x | 0.02473x | 39.209x | 1.836x |
| A / W-C | 9.198x | 0.02172x | 31.355x | 1.829x |
| B / W-S | 14.801x | 0.01761x | 55.009x | 1.836x |
| B / W-C | 9.605x | 0.02800x | 26.451x | 1.829x |

Private bytes thấp hơn khoảng 3% và process CPU mean thấp hơn mạnh, nhưng không tạo practical advantage.

### 2.1 Decode topology hiện tại

Frozen Q2/Q3 source có:

```text
decode per token = 469 dispatches
                = 28 layers × 16 layer ops
                + token embedding
                + output norm
                + 19 LM-head chunk dispatches
```

Mỗi layer decode gồm:

```text
RMSNorm
Q projection
K projection
V projection
Q RoPE
K RoPE
KV store
cached GQA
O projection
attention residual
FFN RMSNorm
gate projection
up projection
SwiGLU
down projection
FFN residual
```

Trong đó 7 projection/GEMM ops mỗi layer là Q/K/V/O/gate/up/down:

```text
7 × 28 = 196 GEMM dispatches / decode token
```

Q2 runner đã kiểm `submit_count == 1` cho mỗi decode token. Vì vậy ArcLLM **không** đang trả 469 CPU queue submissions/token; 469 là GPU dispatch graph nằm trong một prepared-chain submission.

### 2.2 Decode path khác prefill

Prefill đã có:
- tiled Q/K/V/O paths;
- fused gate+up path;
- tiled FFN-down path.

Decode vẫn sử dụng generic batch-1 Q4_K/Q6_K GEMM paths cho Q/K/V/O/gate/up/down, và gate/up vẫn là hai kernels riêng.

P7 closeout cũng ghi rõ decode chưa nhận cùng mức optimization depth như prefill.

## 3. NEXUS evidence review

### N1 — Stable full-N scan: overhead chỉ amortize khi payload đủ lớn

Canonical R4 cho thấy sparse stable-scan không tự nhiên nhanh chỉ vì active ratio thấp.

- tăng N từ 20k → 1.6M làm sparse/dense gap thu hẹp nhưng không crossover;
- tăng degree làm ratio tiến gần 1;
- tăng state dimension D làm overhead được amortize mạnh;
- giảm active ratio ở baseline D=4 **không** tạo monotonic speedup;
- tăng interaction depth lặp lại scan/scatter overhead thay vì amortize.

Fresh R4-C chỉ có một qualifying point:

```text
N = 100000
D = 64
active = 5%
Sparse/Dense E2E:
A = 0.8262344559
B = 0.8980629007
```

**Transferable lesson:** một cơ chế sparse/dynamic chỉ hữu ích khi useful payload trên mỗi scheduled unit đủ lớn so với scheduling/representation overhead.

**ArcLLM mapping:** SUPPORTING PRINCIPLE ONLY. Dense Qwen decode không có active frontier để stable-scan loại bỏ.

### N2 — Event Ledger: direct frontier representation có thể loại full-N discovery

EL-L4, cùng Intel Arc/Vulkan family, xác nhận `global_queue_epoch`:

C1:

```text
N=100k, K=10k, D=4
ledger/Dense:
A = 0.7721864277
B = 0.7406188098
```

C2:

```text
N=1.6M, K=10k, D=4
ledger/Dense:
A = 0.0420408631
B = 0.0563318859
```

Event Ledger được NEXUS integration review promote thành architecture candidate vì nó thay đổi cost model từ discovery theo toàn N sang frontier trực tiếp.

**ArcLLM mapping:** NO DIRECT CAUSAL TRANSFER. ArcLLM current decode graph là fixed dense graph; nó không scan toàn model để tìm active operators và đã submit một prepared chain/token. Thay fixed dispatch list bằng event queue không tự loại được dense Q/K/V/O/FFN math.

**Transferable lesson:** representation phải trực tiếp biểu diễn useful work; không nên tạo/scan metadata có cardinality lớn hơn useful work.

### N3 — LD64 representation/layout co-design

LD64-0/1 chứng minh về cấu trúc/semantics rằng:
- region frontier + 64-bit active mask có thể giữ Event-Ledger activation authority;
- AoSoA D64 indexing có thể trực tiếp executable;
- không cần full-N scan, sort K, payload copy/repack hay bitmap→entity-list reconstruction;
- semantic equivalence PASS.

Nhưng LD64-2 performance hiện **UNRESOLVED** vì preregistered P5 measurement-sanity tail failure. Postmortem phân tách:
- host/queue/wakeup E2E jitter;
- GPU tail localize vào update stage;
- chưa chứng minh root cause hoặc valid repair.

**ArcLLM mapping:** dùng được như **representation/layout design principle**, không được dùng như performance evidence. Một successor ArcLLM không được thêm repack/copy chỉ để đạt layout đẹp.

### N4 — R5 learnability/calibration

R5/R5.1 findings về comparator-vs-competence-control và optimizer/architecture interaction là evidence có giá trị cho research governance, nhưng không có causal path trực tiếp tới ArcLLM inference bottleneck.

**Disposition:** không dùng để chọn runtime intervention.

## 4. External literature review

### P1 — FlashDecoding++ (Hong et al., 2023, public preprint)

Source: https://arxiv.org/abs/2311.01282

Paper xác định ba inference bottleneck, trong đó có **flat GEMM under-utilization** và static dataflow; đề xuất double buffering và hardware-adaptive dataflow. Báo cáo speedup end-to-end so với các inference engines trên NVIDIA/AMD.

**ArcLLM relevance: HIGH.** Decode batch=1 của ArcLLM chính là flat-GEMM regime và generic decode kernels là direct causal candidate.

### P2 — MARLIN (Frantar et al., 2024, public preprint)

Source: https://arxiv.org/abs/2408.11743

MARLIN thiết kế low-bit autoregressive matrix kernels bằng asynchronous memory access, task scheduling/pipelining và quantization-specific dataflow; báo cáo end-to-end gains khi tích hợp vào vLLM.

**ArcLLM relevance: HIGH, mechanism-level only.** CUDA/NVIDIA implementation không portable trực tiếp sang Intel Vulkan; transferable part là dataflow design for low-batch quantized GEMM.

### P3 — FlashAttention / FlashAttention-2 (Dao et al., 2022/2023)

Sources:
- https://arxiv.org/abs/2205.14135
- https://arxiv.org/abs/2307.08691

Core finding: IO-aware tiling, work partitioning và giảm shared/global memory movement có thể cải thiện attention đáng kể mà không đổi exact attention semantics.

**ArcLLM relevance: MEDIUM.** Attention là target hợp lệ, nhưng evidence ArcLLM hiện chưa cho thấy attention đủ lớn để giải thích 26–55x E2E gap. Không chọn làm first successor mechanism.

### P4 — DeepSpeed Inference (Aminabadi et al., 2022)

Source: https://arxiv.org/abs/2207.00032

System-level evidence cho transformer inference kernel/dataflow co-design, kernel fusion và heterogeneous memory execution.

**ArcLLM relevance: SUPPORTING.** Khẳng định hướng co-design, nhưng scale/hardware khác.

### P5 — PagedAttention / vLLM (Kwon et al., 2023)

Source: https://arxiv.org/abs/2309.06180

Paged KV management giảm fragmentation/redundant KV duplication và tăng serving throughput.

**ArcLLM relevance: CONDITIONAL.** Q3 working set ~1.83x baseline cần điều tra, nhưng frozen ArcLLM KV request chỉ ~448 MiB và private bytes lại gần baseline. Chưa có evidence rằng KV fragmentation là nguyên nhân của ~10 GiB working set.

### P6 — QServe (Lin et al., 2024)

Source: https://arxiv.org/abs/2405.04532

QServe cho thấy low-bit inference chỉ hiệu quả khi dequantization, weight layout, register parallelism và attention được co-design; naive low-bit có thể chịu 20–90% runtime dequant overhead.

**ArcLLM relevance: HIGH as warning/support.** Có direct relevance tới packed Q4 decode. Tuy nhiên W4A8/KV4 thay precision contract, nên không được đưa vào first exact-model successor intervention.

### P7 — MegaBlocks / Switch Transformer

Sources:
- https://proceedings.mlsys.org/paper/2023/hash/5a54f79333768effe7e8927bcccffe40-Abstract-mlsys2023.html
- https://www.jmlr.org/papers/v23/21-0998.html

Các hệ thống này cho thấy sparse routing chỉ đạt hardware efficiency khi sparse representation/kernels và routing được thiết kế cùng nhau.

**ArcLLM relevance: PRINCIPLE ONLY.** Current Qwen2.5-Coder target là dense Transformer; không có expert frontier để Event Ledger trực tiếp khai thác.

### P8 — PowerInfer (Song et al., 2023)

Source: https://arxiv.org/abs/2312.12456

PowerInfer khai thác neuron activation locality/sparsity bằng predictor và CPU/GPU hybrid execution.

**ArcLLM relevance: DEFER.** Đây là model/activation-dependent sparsity, không phải exact dense execution invariant của current ArcLLM.

### P9 — Dynamic Input Pruning / cache-aware masking (MLSys 2025)

Source: https://proceedings.mlsys.org/paper/2025/hash/afd6374c7f2839cba22f537f15f4f760-Abstract-Conference.html

Paper nêu rõ modern SwiGLU models có ít inherent sparsity hơn ReLU models và cần pruning/fine-tuning/predictor-free sparsification để tạo sparse execution.

**ArcLLM relevance: IMPORTANT NEGATIVE SUPPORT.** Nó củng cố quyết định không chuyển Event Ledger trực tiếp vào exact dense Qwen runtime.

### P10 — FlashInfer (MLSys 2025)

Source: https://proceedings.mlsys.org/paper/2025/hash/dbf02b21d77409a2db30e56866a8ab3a-Abstract-Conference.html

FlashInfer dùng workload-aware scheduling, block/composable KV formats và hardware-specific templates.

**ArcLLM relevance: MEDIUM.** Hỗ trợ nguyên tắc workload-specific kernels, đặc biệt cho attention; chưa đủ evidence để chọn attention làm first intervention.

## 5. Causal mapping matrix

| Candidate mechanism | Directly addresses Q3 gap? | Local evidence | NEXUS support | Paper support | Disposition |
|---|---|---|---|---|---|
| Event-ledger operator scheduler | Weak | fixed dense graph, 1 submit/token | strong in sparse frontier | sparse systems | DEFER_FURTHER |
| Sparse neuron/expert execution | Potential but changes model contract | no authoritative dense-Qwen sparsity | strong principle | Switch/MegaBlocks/PowerInfer | DEFER_FURTHER |
| Attention-only rewrite | Partial | no decode stage attribution proving dominance | none direct | strong | DEFER_FURTHER |
| KV paging/compression | Memory-specific | KV only a minority of requested residency; root cause unproven | none | strong serving evidence | DEFER_FURTHER |
| Single-copy weight residency | Memory-specific | working set 1.83x, private bytes ~0.97x | layout/no-copy principle | memory/offload literature | ENGINEERING/DIAGNOSTIC CANDIDATE |
| **Decode-specialized packed-quant fused dataflow** | **Direct** | **generic batch-1 GEMMs; 196 GEMM dispatches/token; decode under-optimized; 0.018–0.028x baseline TPS** | **amortization + representation co-design principles** | **FlashDecoding++, MARLIN, QServe, DeepSpeed** | **RESEARCH_REOPEN_CANDIDATE** |

## 6. Selected successor hypothesis

### SA-H1 — Decode-Specialized Packed-Quant Executor

> The dominant performance failure of the validated ArcLLM architecture is caused by a decode dataflow that applies generic batch-1 Q4_K/Q6_K kernels and separately materialized operator boundaries to flat autoregressive GEMMs. A successor architecture that co-designs packed-weight layout, batch-1 work partitioning, dequantization/pipelining and safe projection fusion can materially reduce decode GPU time without changing the exact GGUF model, greedy semantics, KV semantics or output-length contract.

Candidate intervention family is bounded to:

1. decode-only hardware-adaptive Q4_K/Q6_K GEMM dataflow;
2. async/double-buffered packed-weight load/dequant pipeline where supported by Vulkan/Intel capabilities;
3. Q/K/V projection co-design to reuse the same normalized input without changing arithmetic semantics;
4. gate/up projection fusion for decode;
5. LM-head launch/layout redesign only if exact segmentation semantics are preserved;
6. no sparse-neuron skip, no MoE conversion, no approximation, no activation pruning in SA-H1.

### Why SA-H1 is strong enough to justify a successor study

It passes the six historical architecture-experiment conditions:

1. **real model executed:** Q1 PASS;
2. **end-to-end bottleneck observed:** Q2/Q3 show severe reproducible decode/E2E deficit;
3. **blocks final goal:** Q3 closed with no demonstrated advantage;
4. **specific mechanism:** flat low-batch quant GEMM/dataflow + repeated projection boundaries;
5. **falsifiable:** successor must show preregistered decode-stage improvement and matched end-to-end movement;
6. **stoppable:** failure of capability/kernel gates or insufficient matched E2E movement closes SA-H1 without opening SA-H2 rescue tuning.

## 7. What SA-H1 does NOT claim

This review does not claim:
- that 469 dispatches alone caused the Q3 failure;
- that Event Ledger will speed up dense Qwen;
- that NVIDIA CUDA kernel speedups transfer quantitatively to Intel Arc Vulkan;
- that kernel fusion gains multiply;
- that ArcLLM can beat llama.cpp after one intervention;
- that current working-set excess has a proven root cause.

The only claim is that SA-H1 has the strongest presently evidenced causal path and is worth one bounded successor study.

## 8. Mandatory successor-study boundaries

A future SA-H1 preregistration must include, before implementation:

### SA0 — capability / causal qualification, zero target science

Freeze and verify:
- Intel Arc Vulkan subgroup/cooperative/data-layout capabilities actually available;
- exact baseline for batch-1 Q4_K/Q6_K projection kernels;
- decode operator census and byte/weight movement model;
- expected dispatch reductions from each allowed fusion;
- an upper-bound analysis showing whether even successful kernels could plausibly move end-to-end latency enough to justify target execution.

If the upper bound cannot plausibly close a material fraction of the Q3 gap, stop before implementation.

### SA1 — component qualification

One mechanism at a time:
- flat-GEMM dataflow;
- QKV fusion;
- gate/up fusion;
- optional LM-head dispatch/layout.

Every component must preserve exact frozen semantics/tolerance and have an independently frozen performance gate.

Microbenchmarks may qualify components but cannot establish successor advantage.

### SA2 — real-model matched confirmation

Only after SA1:
- same exact 7B GGUF;
- same Intel Arc 140V target;
- same W-S/W-C first;
- same llama.cpp v0.4.1 baseline initially;
- fresh matched A/B sessions;
- no threshold mutation.

The successor must first demonstrate that the intervention produces material end-to-end movement. A later advantage claim still requires a separately frozen practical threshold and fresh reproduction.

## 9. Memory issue disposition

The 1.83x working-set gap is important but not selected as the first architecture intervention because:
- private bytes are already slightly below baseline;
- the root cause of working-set inflation has not been isolated;
- reducing RAM does not address the 26–55x E2E latency deficit.

Before any memory architecture work, perform a zero-science residency attribution:
- mapped GGUF pages;
- Vulkan host-visible/device-visible allocations;
- duplicated packed weights;
- page residency/shared working-set accounting;
- KV/working buffers.

Only a demonstrated duplicate-residency mechanism may promote a memory successor hypothesis.

## 10. NEXUS boundary

NEXUS contributes evidence and design lessons, not code/claim transfer.

Allowed:
- use R4 amortization findings when defining scheduler/dataflow cost models;
- use EL-L4 as evidence that eliminating unnecessary global discovery can matter greatly;
- use LD64-0/1 as a representation/layout co-design lesson;
- inspect future NEXUS findings before SA-H1 preregistration.

Forbidden:
- copy an NEXUS PASS label into ArcLLM;
- claim Event Ledger works for dense Qwen without new evidence;
- use unresolved LD64-2 performance as positive support;
- import a NEXUS workload/threshold after seeing ArcLLM successor data.

## 11. Review decision

```text
CURRENT ARCLLM ARCHITECTURE     CLOSED
Q3                              TERMINAL
POST-VERDICT REVIEW             COMPLETE
SA-H1                           RESEARCH_REOPEN_CANDIDATE
SA-H1 IMPLEMENTATION            NOT AUTHORIZED
SA-H1 TARGET EXECUTION          NOT AUTHORIZED
NEXUS CODE MERGE                NOT AUTHORIZED
NEXUS EVIDENCE TRANSFER         NOT AUTHORIZED
```

The review provides enough causal justification to **design/preregister** one successor architecture study. It does not authorize code or target measurements.

## 12. Tiếp theo đúng khoa học

Create **SA0 — Decode Architecture Causal/Capability Qualification**, specification-only.

SA0 must answer before any new kernel is written:

> Is the Q3 gap large because the current batch-1 packed-quant decode dataflow is fundamentally under-utilizing the Intel Arc execution/memory pipeline, and is there a capability-backed upper bound showing that the proposed fusion/dataflow family can plausibly move real-model E2E enough to justify implementation?

If SA0 cannot establish that plausibility from source/capability/cost-model evidence, SA-H1 closes without implementation.
