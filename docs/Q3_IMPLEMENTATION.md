# ArcLLM Q3 — Implementation Lock Package

**Status:** IMPLEMENTATION STATIC LOCKED / EXECUTION CLOSED  
**Scientific contract:** `docs/Q3_NO_PRACTICAL_ADVANTAGE_CONFIRMATORY_DESIGN.md`

## Execution shape

Q3 deliberately reuses the exact Q2 inference paths rather than creating a new model runtime:

- ArcLLM executable source path remains `src/q2_benchmark.cpp`;
- baseline remains the exact llama.cpp v0.4.1 adapter;
- the 16 production shaders remain unchanged;
- Q2 resource sampler remains the measurement instrument.

`run_q3_preflight.ps1` verifies every Q2 implementation-critical worktree SHA against the committed Q2 execution authorization before it builds anything. This makes an architecture or instrumentation change fail closed.

## Zero-measurement preflight

The preflight:

1. requires Q3 execution authorization to be absent;
2. requires no Q3 session directory to exist;
3. checks CPU/GPU/driver/power scheme/AC first;
4. verifies the frozen Q3 design and parent Q2 formal result;
5. verifies all Q2 frozen runtime/instrumentation hashes;
6. runs Q3 static QA;
7. verifies the exact target GGUF;
8. recompiles the unchanged 16 shaders and unchanged ArcLLM executable;
9. rebuilds the exact pinned llama.cpp baseline;
10. performs W-S and W-C baseline `--qualify-only` runtime qualification;
11. requires full Vulkan offload and zero `llama_decode` / zero measured attempts;
12. writes `results/q3_preflight_lock.json`;
13. packages `results/q3_preflight_return_to_chatgpt.zip`.

Successful preflight still does **not** authorize Q3 execution.

## Fresh-session enforcement

`run_q3.ps1` requires `-Session A` or `-Session B` and uses fixed directories:

- `results/q3_session_A`
- `results/q3_session_B`

If the requested session directory already exists, the runner refuses to overwrite or silently rerun it. Session A and B therefore require separate PowerShell invocations. Each session records its runner PID and fixed execution order.

Each session is exactly 4 cells × 5 measured attempts = 20 measured attempts, plus one warmup per cell. The two sessions total 40 fresh measured attempts.

The runner never compiles or rebuilds measured artifacts. It verifies the preflight lock, a later committed Q3 authorization, all Q3 critical hashes, all frozen Q2 runtime hashes, model bytes, executable hashes and compiled shader hashes before any measurement.

## Candidate adjudicator

`tools/adjudicate_q3.py` is deterministic and reads thresholds only from the frozen Q3 design. It requires:

- five measured attempts and five mandatory resource summaries per cell;
- exact prompt hashes;
- valid timing and mandatory RAM/CPU metrics;
- distinct session runner process IDs;
- matched OS/power scheme/GPU driver;
- the fixed A/B cell orders.

It evaluates only TTFT, decode throughput, E2E latency and working-set peak as primary advantage dimensions. Private bytes, CPU utilization and GPU counters are supporting-only.

A machine-generated candidate verdict is **not** the final independent governance adjudication.

## Evidence packaging

After both fresh sessions exist, `package_q3.ps1`:

1. runs the deterministic candidate adjudicator;
2. hashes all Session A/B artifacts and frozen binding files;
3. writes a manifest;
4. produces `results/q3_return_to_chatgpt.zip`.

## Authorization boundary

Current state remains closed. A successful returned preflight must be independently adjudicated before a governance-only commit may add:

`config/q3_execution_authorization.json`

Only that later commit may set Q3 execution permitted. No implementation-critical file may change between preflight and authorization.
