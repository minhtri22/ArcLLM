# Q6CB-1 Causal Harness Implementation

**Status:** IMPLEMENTATION-ONLY; SCIENTIFIC EXECUTION CLOSED  
**Branch:** `research/q6-correctness-boundary`

## Authorization boundary

Implementation was explicitly authorized after Q6CB-0 specification QA and termination-governance QA passed.

The authorization permits source/tooling/unit/static work and BuildOnly evidence. It does not permit fresh fixture execution, CPU scientific execution, GPU dispatch, timing, target-model loading, SA1 rerun, threshold selection, or creation of an execution authorization.

## Exact implementation

The implementation is isolated from SA1.

### CPU/reference path

- `src/q6cb_q6_reference.hpp`
  - independent canonical 210-byte Q6_K decoder;
  - `R64`: canonical expanded values with binary64 accumulation;
  - `S32`: the same canonical values with serial binary32 accumulation;
  - `T32-CPU`: the same canonical values partitioned across 32 lanes with a frozen five-stage pairwise tree.

The T32-CPU tree is a deterministic canonical topology proxy. It does not assume Vulkan `subgroupAdd` exposes or guarantees the same internal tree. Any future GPU-vs-CPU discrepancy therefore belongs to the preregistered device/subgroup implementation contrast rather than being silently attributed to arithmetic precision alone.

### Deterministic fixture generator

- `src/q6cb_fixture_generator.hpp`

It supports exactly the five preregistered conditioning strata and directly rejects:

- SA1 cell `n=3584, rows=512`;
- SA1 cell `n=18944, rows=3584`;
- all four SA1 fixture seeds.

The generator has no adaptive search API. Future exact seeds, fixture counts, identification partition and confirmatory partition remain intentionally unfrozen until the later execution-contract stage.

### GPU arms

- `shaders/q6cb_t32_gpu_packed.comp`
  - subgroup-32 split-K;
  - direct packed Q6_K decode;
  - local size 128 / four rows per workgroup.

- `shaders/q6cb_t32_gpu_expanded.comp`
  - identical subgroup-32 split-K geometry;
  - reads canonical pre-expanded FP32 weights;
  - removes packed-dequant access from the causal contrast.

Neither shader contains timing, shared-memory staging, cooperative matrix logic or optimization search.

### Harness

- `src/q6cb_causal_harness.cpp`

The harness can produce all five arms and raw causal contrasts, but it is fail-closed.

Before fixture generation, CPU arm evaluation, Vulkan initialization or GPU dispatch, it requires a future authorization file containing:

`Q6CB1_SCIENTIFIC_EXECUTION_AUTHORIZED`

No such authorization file is created or permitted during Q6CB-1.

The harness performs no scientific classification and contains no threshold logic. It emits raw arm outputs and descriptive difference metrics only.

### BuildOnly tooling

- `tools/compile_q6cb1.ps1`
- `tools/build_q6cb1.ps1`

These compile shaders and link the native harness, record exact source/blob/SHA provenance and explicitly record:

`scientific_execution=false`

The native BuildOnly script does not launch the produced executable.

### Unit/static QA

- `tests/test_q6cb1_package.py`

The test checks the authorization boundary, SA1 immutability, five-arm census, canonical Q6 layout, conditioning-strata/exclusion rules, GPU topology, absence of timing/model paths, fail-closed ordering and a tiny non-scientific packed-code round-trip invariant.

## Current boundary

No fresh fixture has been generated or executed by this implementation work. No GPU scientific dispatch has been authorized. No Q6CB outcome has been observed.

Target-local shader compile/native BuildOnly evidence remains a separate non-scientific implementation validation step before final static-equivalent adjudication and exact implementation lock.
