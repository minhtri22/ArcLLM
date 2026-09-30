# runs/

This directory contains run-specific operational PowerShell entrypoints that previously lived at repository root.

## Layout

- `runs/anl64/` — ANL64 operational runners/recovery/inspection
- `runs/arcllm_v1/` — ArcLLM-v1 operational runners/recovery
- `runs/p7/` — P7 runners
- `runs/p8/` — P8 runners
- `runs/q/` — Q-series runners and Q3 packager
- `runs/sa/` — SA runners
- `runs/ttft/` — TTFT runners

## Relocation invariant

Moved scripts no longer assume that their own directory is repository root. Their bootstrap resolves repo root as `../..` from `runs/<study>/`, so existing references to `config/`, `inputs/`, `results/`, `artifacts/`, `tools/`, `tests/`, `src/`, `shaders/`, and `compiled_shaders/` continue to resolve against the same canonical repository locations.

Cross-run references were rewritten to canonical `runs/<study>/...` paths. No scientific `results/`, frozen `artifacts/`, preregistration/execution `config/`, docs, runtime/kernel/shader source, or lineage is moved by this cleanup.

## Historical reproducibility

This cleanup intentionally does not rewrite frozen historical execution locks. If a historical study requires the exact pre-cleanup runner blob identity, checkout parent commit `c37521311e13398960b4d01ecb90a2804c2f5d3d` and run it there. The mapping from every old root path/blob to its relocated path/blob is recorded in `MIGRATION_MANIFEST_2026-10-01.json`.
