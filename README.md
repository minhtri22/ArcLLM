# ArcLLM

**ArcLLM is a native C++17/Vulkan Compute runtime for GGUF LLM inference on Windows.**  
The project is built from first principles around direct packed-quantized execution, explicit GPU memory residency, and a research process in which scientific claims are frozen, measured, and preserved separately from the canonical runtime.

The canonical product/runtime line is the default `main` branch. The current validated target is the frozen **Qwen2.5-Coder 7B** model layer on **Intel Arc 140V UMA**.

> **Start here**
>
> - 📘 **Book / project journey:** [Inside ArcLLM](https://github.com/minhtri22/Inside-ArcLLM)
> - 🧪 **Scientific history and PASS/FAIL record:** [lineage.md](./lineage.md)
> - 🔒 **Canonical runtime binding:** [config/arcllm_v1_runtime_active_v0.2.json](./config/arcllm_v1_runtime_active_v0.2.json)
> - 📐 **Product + research governance:** [docs/ARCLLM_PRODUCT_RESEARCH_GOVERNANCE.md](./docs/ARCLLM_PRODUCT_RESEARCH_GOVERNANCE.md)

---

## Architecture at a glance

```text
Caller / application
        │
        │  model path
        │  shader path
        │  token IDs
        │  max_new_tokens
        │  evidence boundary
        ▼
┌──────────────────────────────────────────────┐
│ Public runtime boundary                     │
│ arcllm::v1::runtime::generate(RunRequest)    │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│ ArcLLM-v1 runtime orchestration              │
│                                              │
│ • GGUF packed tensor access                  │
│ • request-scoped Vulkan residency            │
│ • explicit scratch / memory planning         │
│ • GPU-resident KV across decode submissions  │
│ • greedy autoregressive generation           │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│ Runtime policy + backend binding             │
│                                              │
│ generic policy / binding v4                  │
│                                              │
│ validated domain ──► I002 Gate/Up fast path  │
│ outside domain   ──► safe fallback route     │
│                                              │
│ Q4VulkanBackendV4 for validated Q4 paths     │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│ Vulkan execution graph                      │
│                                              │
│ RMSNorm                                      │
│ packed Q4_K / Q6_K projections              │
│ RoPE                                         │
│ attention + softmax                         │
│ SwiGLU / FFN                                 │
│ final norm + LM head                        │
└──────────────────────────────────────────────┘
        │
        ▼
┌──────────────────────────────────────────────┐
│ Intel Arc GPU / Vulkan Compute              │
└──────────────────────────────────────────────┘
        │
        ▼
generated token IDs + RuntimeStats
```

### Core design ideas

ArcLLM intentionally keeps the runtime boundary small and explicit:

1. **Packed weights stay packed.** Q4_K/Q6_K tensors are consumed directly instead of expanding the full model into floating-point weights.
2. **Residency is explicit.** Weight arenas, scratch memory, and KV state are deliberately managed rather than delegated to an opaque framework.
3. **Decode state stays on the GPU.** The KV cache remains GPU-resident across decode submissions; host interaction is kept at defined orchestration boundaries.
4. **Optimizations are evidence-gated.** A fast path is only enabled where the current runtime binding says its evidence domain applies; otherwise the runtime uses a safe fallback route.
5. **Research code is not automatically product code.** Experimental PASS/FAIL results are preserved in the scientific record, while only accepted mechanisms are allowed to converge into `main`.

---

## Canonical runtime surface

The active binding is:

```text
config/arcllm_v1_runtime_active_v0.2.json
```

The public C++ entry point is:

```cpp
arcllm::v1::runtime::generate(const RunRequest&)
```

Key files:

| Area | Path |
|---|---|
| Public C++ API | [include/arcllm/v1/runtime.h](./include/arcllm/v1/runtime.h) |
| Canonical runtime | [src/arcllm_v1_runtime.cpp](./src/arcllm_v1_runtime.cpp) |
| CLI | [src/arcllm_v1_runtime_cli.cpp](./src/arcllm_v1_runtime_cli.cpp) |
| Vulkan runtime support | [src/arcllm_v1_vulkan_runtime_support.h](./src/arcllm_v1_vulkan_runtime_support.h) |
| Q4 Vulkan backend | [src/arcllm_v1_q4_vulkan_backend_v4_runtime.h](./src/arcllm_v1_q4_vulkan_backend_v4_runtime.h) |
| Vulkan shaders | [shaders/](./shaders/) |
| Runtime/config locks | [config/](./config/) |
| Research documents | [docs/](./docs/) |
| Scientific lineage | [lineage.md](./lineage.md) |

---

## Current validated boundary

The canonical runtime is intentionally narrower than a general-purpose LLM framework.

| Boundary | Current state |
|---|---|
| Runtime | Native C++17 + Vulkan Compute |
| OS focus | Windows |
| Validated hardware | Intel Arc 140V UMA |
| Frozen model SHA256 | `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463` |
| Max prefill tokens | 256 |
| Max context tokens | 4096 |
| Evidence profiles | `PROFILE_0`, `PROFILE_1` |
| Input boundary | caller-provided token IDs |
| Generation | greedy |
| Runtime lifetime | request-scoped |
| Persistent model session | not currently supported |
| Canonical NPU backend | not currently supported |
| Current external llama.cpp performance-advantage claim | **none** |

The `request_within_validated_domain` field is an **evidence boundary**, not a prompt classifier. Callers should only set it to `true` for requests covered by the currently validated runtime domain.

---

## Scientific status

ArcLLM has established that the frozen 7B model can execute end-to-end on the target Intel Arc/Vulkan runtime. The matched performance program did **not** establish a current general performance advantage over the frozen external llama.cpp baseline.

That distinction is intentional:

```text
feasibility evidence ≠ performance advantage claim
component speedup    ≠ end-to-end speedup
research PASS        ≠ automatic product promotion
```

The repository contains both positive and negative results. They are part of the project, not discarded history.

For the chronological scientific record, including the exact questions, evidence, PASS/FAIL decisions, quantitative findings, and successor studies, read:

**➡️ [ArcLLM scientific lineage](./lineage.md)**

---

## Repository model

```text
main
  │
  ├── canonical runtime / accepted mechanisms
  │
  └── canonical scientific knowledge
        ▲
        │ converge only after formal adjudication
        │
research/<single-question>
        │
        ├── preregistration
        ├── implementation
        ├── bounded execution
        ├── independent QA / adjudication
        └── PASS / FAIL / UNRESOLVED
```

`lineage.md` is append-only for scientific results. Build repair, transport, runner maintenance, and other infrastructure work do not belong in scientific lineage.

See [Product and Research Governance](./docs/ARCLLM_PRODUCT_RESEARCH_GOVERNANCE.md) for the full rules.

---

## Read the book

If you want the project explained as a learning journey rather than as a research repository, start with:

### [Inside ArcLLM — Xây dựng một runtime LLM từ các nguyên lý đầu tiên](https://github.com/minhtri22/Inside-ArcLLM)

The book walks through the ideas behind GGUF, packed quantization, Vulkan compute, memory residency, decoder execution, KV cache, measurement, and the decisions that shaped ArcLLM.

---

## Project philosophy

ArcLLM is not an attempt to win by adding more layers of framework abstraction.

The project asks a narrower question:

> **How much of an LLM runtime can be understood, measured, and deliberately controlled when the path from packed model weights to physical GPU execution is made explicit?**

That means correctness before optimization, frozen gates before measurements, negative results kept as evidence, and a clear separation between what the runtime **can do**, what experiments **suggest**, and what the project is actually prepared to **claim**.
