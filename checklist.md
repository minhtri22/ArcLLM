# ArcLLM Main Product Handoff Checklist

Date: 2026-09-29  
Repository: `minhtri22/ArcLLM`  
Canonical product branch: `main`  
Validated runtime commit: `3363b5a2f146f9840cae3a1700151f1f5417871b`

> This checklist is the handoff record for the final ArcLLM product state validated on the target Windows machine. The checklist commit itself is documentation-only; the runtime/build/inference evidence below was produced from the exact validated runtime commit above.

## A. Repository convergence and branch role

- [x] Default branch is `main`.
- [x] Historical pre-v1 `main` is preserved by tag `archive/arcllm-pre-v1` -> `3352dbc841a06f1cab8e70f9d8353d683c411ccc`.
- [x] `research/arcllm-v1` was fast-forwarded into `main` with no history rewrite.
- [x] `research/arcllm-v1` was deleted after verifying it was identical to `main`.
- [x] All remaining CLOSED legacy `research/*` refs were archived to immutable `archive/*-closed` tags and then deleted.
- [x] Current `research/*` branch count = 0; future research branches are created only for an active single-question study.
- [x] Obsolete `ebook/inside-arcllm-from-zero` was deleted after confirming the separate public `minhtri22/Inside-ArcLLM` repository supersedes it.
- [x] M3-C terminal scientific result was converged into `programs/arcllm_v1/lineage.md` as `STOP_M3C_UNRESOLVED_COUNTER_ADEQUACY`.
- [x] Q4-down split-K side branch closure was not misrepresented as science: it ended before outcome-bearing correctness/performance science and therefore adds no PASS/FAIL claim to scientific lineage.
- [x] Active runtime binding now names `main`.
- [x] Product/research lifecycle is documented in `docs/ARCLLM_PRODUCT_RESEARCH_GOVERNANCE.md`.
- [x] README identifies `main` as the canonical product line and separates current product state from historical research chronology.

## B. Active product/runtime boundary

- [x] Public C++ API exists: `arcllm::v1::runtime::generate(const RunRequest&)`.
- [x] CLI exists: `src/arcllm_v1_runtime_cli.cpp`.
- [x] Caller-supplied token IDs are accepted.
- [x] Caller-supplied `max_new_tokens` is accepted.
- [x] `PROFILE_0` and `PROFILE_1` are supported.
- [x] Greedy generation is the declared generation mode.
- [x] I002 Gate/Up fast path is used only inside the validated evidence domain.
- [x] Generic policy/binding v4 is in the active runtime.
- [x] `Q4VulkanBackendV4` FFN-down is in the active runtime.
- [x] Outside the validated evidence domain, the runtime uses the safe fallback path and does not silently acquire represented-B state.
- [x] Active runtime contains no W-S/W-C fixture logic, frozen expected token hashes, experiment PASS/FAIL adjudication, historical P8C main, or backend-QA main in the active compilation path.

## C. Build from final product commit

Command class: `tools/build_arcllm_v1_runtime.ps1`

- [x] Canonical runtime extraction static QA: PASS.
- [x] Q4 Vulkan backend shader-byte validation: PASS.
- [x] Full ArcLLM runtime shader build: PASS.
- [x] Native C++ runtime build: PASS.
- [x] Build artifact schema reports `PASS_STATIC_SHADER_NATIVE_BUILD`.
- [x] Executable size: 489,984 bytes.
- [x] Executable SHA256 for this final validation build: `9AA55411B4E1BFF45690775593DFFD7EBBEBFC5E9B374108C48ED96A35D03020`.
- [x] Build artifact records `git_head=3363b5a2f146f9840cae3a1700151f1f5417871b`.
- [x] Build does not create a new performance claim.

## D. Exact target model resolution

- [x] Local Ollama resolver found exactly one manifest matching the frozen target layer.
- [x] Exact target model SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`.
- [x] Exact target model size: 4,683,074,048 bytes.
- [x] Real model, not a synthetic fixture, was used for the handoff inference runs.

## E. Real canonical regression — PROFILE_0

Result: PASS

- [x] Generated token count = 32.
- [x] Generated hash = `f31d4bb9fe5eb9c3`.
- [x] Frozen hash matches.
- [x] Prefill dispatches = 441.
- [x] Prefill submits = 1.
- [x] Decode dispatches per step = 469.
- [x] Decode submits per step = 1.
- [x] Decode steps = 31.
- [x] Route-A steps = 0.
- [x] Route-B steps = 31.
- [x] Policy acquire event = 1.
- [x] Policy evict event = 1.
- [x] Backend allocation/materialization/validation = 1/1/1.
- [x] Backend release = 1.
- [x] P1 acquisition calls = 1.
- [x] P3/P0 fallback acquisition calls = 0/0.
- [x] Final numerical state is finite.

## F. Real canonical regression — PROFILE_1

Result: PASS

- [x] Generated token count = 32.
- [x] Generated hash = `471519ddc45b232e`.
- [x] Frozen hash matches.
- [x] Prefill dispatches = 441.
- [x] Prefill submits = 1.
- [x] Decode dispatches per step = 469.
- [x] Decode submits per step = 1.
- [x] Decode steps = 31.
- [x] Route-A steps = 0.
- [x] Route-B steps = 31.
- [x] Policy acquire event = 1.
- [x] Policy evict event = 1.
- [x] Backend allocation/materialization/validation = 1/1/1.
- [x] Backend release = 1.
- [x] P1 acquisition calls = 1.
- [x] P3/P0 fallback acquisition calls = 0/0.
- [x] Final numerical state is finite.

Canonical regression summary: `PASS_EXACT_FROZEN_ORACLE_PRESERVED`.

## G. Real non-fixture product smoke

Input:
- token IDs: `[1, 42, 314, 2718]`
- `max_new_tokens=2`
- profile: `PROFILE_0`
- `request_within_validated_domain=false`

Observed:
- [x] Runtime exited successfully.
- [x] Generated token IDs = `[2718, 2718]`.
- [x] Prefill dispatches/submits = 441/1.
- [x] Decode dispatches/submits per step = 469/1.
- [x] Decode steps = 1.
- [x] Route-A steps = 1.
- [x] Route-B steps = 0.
- [x] Acquire/evict events = 0/0.
- [x] B allocation/materialization/validation/release = 0/0/0/0.
- [x] P1/P3/P0 acquisition calls = 0/0/0.
- [x] Final numerical state is finite.

Interpretation: caller-provided input and generation length execute outside the frozen fixtures, while the runtime respects the safe fallback boundary and does not perform hidden represented-B acquisition.

## H. CLI/API guard checks

- [x] Invalid profile `--profile 9` is rejected.
  - exit code: 2
  - error: `--profile must be 0 or 1`
- [x] `--max-new 0` is rejected before inference.
  - exit code: 2
  - error: `ArcLLM runtime requires max_new_tokens >= 1`
- [x] Runtime source has explicit prefill-token guard at 256 tokens.
- [x] Runtime source has explicit KV context guard at 4096 tokens.
- [x] Runtime source rejects out-of-vocabulary token IDs.
- [x] Runtime source rejects empty input and missing model/shader configuration.

## I. Explicit non-capabilities / claim boundaries

These are not handoff failures; they are current declared product boundaries.

- [x] Persistent model session: NOT IMPLEMENTED / NOT CLAIMED.
- [x] Canonical NPU execution: NOT INTEGRATED / NOT CLAIMED.
- [x] Arbitrary prompt semantic-quality validation: NOT CLAIMED.
- [x] External llama.cpp performance advantage for the current product revision: NOT CLAIMED.
- [x] This handoff run is functional/correctness validation, not a new scientific performance experiment.

## J. Repository hygiene note

A fresh Windows checkout reports 10 historical P7 PowerShell files as modified only because of CRLF/LF normalization. Verification with `git diff --ignore-space-at-eol --exit-code` returns 0.

- [x] No content difference remains after ignoring end-of-line representation.
- [x] None of those historical scripts is in the active canonical runtime compilation path.
- [x] No runtime/product finding is opened from this normalization behavior.

## Final acceptance

- [x] Repository convergence: PASS.
- [x] Canonical product branch migration: PASS.
- [x] Closed research-branch archival cleanup: PASS.
- [x] Active runtime binding: PASS.
- [x] Build: PASS.
- [x] Shader compilation: PASS.
- [x] Native executable: PASS.
- [x] Exact real-model resolution: PASS.
- [x] PROFILE_0 real inference: PASS.
- [x] PROFILE_1 real inference: PASS.
- [x] Non-fixture safe fallback inference: PASS.
- [x] CLI guard behavior: PASS.
- [x] Scientific claim boundaries preserved: PASS.

```text
HANDOFF_STATUS = PASS
OPEN_FINDINGS = 0
PERFORMANCE_CLAIM_CREATED = false
NEW_SCIENTIFIC_RESULT_CREATED = false
```
