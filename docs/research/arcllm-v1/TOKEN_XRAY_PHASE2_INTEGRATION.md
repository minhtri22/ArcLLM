# ArcLLM × Token X-Ray Phase 2 Integration

Integration branch: `integration/token-xray-phase2`.

This branch is diagnostic only and leaves `research/arcllm-v1` unchanged.

## Purpose

```text
semantic NodeID
→ shader
→ dispatch geometry
→ barrier topology
→ Vulkan GPU timestamp
→ TOKEN_TRACE.json
→ NODE_LEDGER_TRACED.json
```

No performance counters are collected.

## One-command local acceptance

Use `tools/run_token_xray_phase2.ps1` with the exact GGUF and local Token X-Ray repo.

The runner:

1. resolves Vulkan timestampPeriod from `vulkaninfo` unless explicitly provided;
2. computes exact model SHA256;
3. runs ModelLens;
4. builds the diagnostic candidate;
5. traces exactly one selected decode step;
6. joins TOKEN_TRACE into NODE_LEDGER;
7. requires exactly 469 dispatches and zero unknown/unmapped semantic IDs.

Output bundle:

```text
.local/token_xray_phase2/<timestamp>/
├── static/MODEL_GRAPH.json
├── static/NODE_LEDGER.json
├── ARCLLM_TRACE_ATTEMPT.json
├── TOKEN_TRACE.json
├── NODE_LEDGER_TRACED.json
└── RUNTIME_TRACE_SUMMARY.json
```

The instrumented timing is diagnostic and must not be mixed back into I003 benchmark evidence.
