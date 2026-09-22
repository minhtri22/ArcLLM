# TTFT_M2 — P9C Explicit Fresh Mechanism-Identification Execution Authorization Gate

**Date:** 2026-09-22  
**Decision:** `DENY / LEAVE SCIENTIFIC EXECUTION BLOCKED`

## Gate result

`M2_P9C_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_NOT_AUTHORIZED`

Reason:

`EXECUTION_IDENTITY_AND_RUNTIME_PAYLOAD_BINDING_INCOMPLETE`

P9A and P9B both remain valid. The denial is caused by the future science-runner execution boundary, not by the scientific design or BuildOnly package.

## Prerequisites that passed

```text
P9A canonical design
abce0545cec33367f9b3a82f15d5ffd4fb026f64

P9B adjudication
d6a9947694cb35044251bac363236f69b06e4d9f

P9B active lock v0.2
7b06da63fe09597ebdcf641d1cd992d4a973075c

P9B active BuildOnly authorization v0.2
bcf7c04fbc09cdcbc39b57676945b808e66a2167

qualified executable SHA256
9E0C0A7CCCBB2DA767B4DE90354EC7ADF4463286264187843521419113C019C7
```

## Blocking findings

### G1 — stale implementation-lock reference

The current science runner:

`scripts/ttft_m2/run_p9_science.ps1`

blob:

`4326d69f058ad8266e88f20272b3ca17f1b21293`

still loads and Git-verifies:

`config/arcllm_ttft_m2_p9b_implementation_lock_v0.1.json`

The active qualified authority is **v0.2**, blob:

`7b06da63fe09597ebdcf641d1cd992d4a973075c`

This is a blocking provenance defect.

### G2 — qualified executable identity is not enforced

P9B qualified:

```text
SHA256  9E0C0A7CCCBB2DA767B4DE90354EC7ADF4463286264187843521419113C019C7
bytes   441856
```

The current runner checks that the executable exists, but does not verify this SHA256 or byte size before launch.

Therefore the gate cannot prove that the qualified executable is the executable that would actually run.

### G3 — qualified build-manifest identity is not enforced

Qualified manifests:

```text
NATIVE_BUILD.json
0E51867BE03AFD60E90B823D2A6FE19F22ACBF6B628E9CE37E30E673C6AF91E4

SHADER_BUILD.json
9298D5EE2838B3FE5D39FC968DC937DC4183F168517CEE1F30F54E9CA86093FA
```

The runner does not bind both exact returned manifest byte identities before execution.

### G4 — actual runtime SPIR-V payload is not rehashed

The runner reads hashes from `SHADER_BUILD.json` for H-ART comparison.

It does **not** independently hash the actual SPIR-V files in the runtime shader directory before the executable consumes them.

A BuildOnly-qualified manifest therefore does not yet prove that the runtime files are unchanged.

### G5 — frozen gate-order mismatch

The P9A falsification/STOP contract freezes:

```text
F0 provenance/environment/freshness
F1 H-ART static identity
...
```

The current future runner performs H-ART before F0.

Even though H-ART is static and pre-launch, changing the frozen adjudication order requires prospective correction before scientific execution.

## Scientific boundary

No science was executed by this gate.

```text
diagnostic executable launch  false
target model load             false
GPU dispatch                  false
performance measurement       false
fresh TTFT observations       0
mechanism result              NONE
```

No `p9c_execution_authorization` file is created.

P9B remains PASS and package repair budget remains 0/1.

## Next admissible step

`M2_P9C_A_EXECUTION_IDENTITY_BINDING_CORRECTION_AND_ZERO_SCIENCE_REVALIDATION`

That step is allowed to correct only the pre-execution runner/provenance boundary:

1. bind P9B lock v0.2;
2. bind exact P9B executable SHA256 + size;
3. bind exact native/shader build-manifest hashes;
4. rehash actual runtime SPIR-V payload before launch;
5. enforce canonical F0 → F1 order;
6. zero-science statically revalidate the corrected runner.

It may not launch the executable, load the model, dispatch GPU work, measure TTFT, or change hypotheses/arms/workloads/thresholds/endpoints.

After that revalidation, P9C authorization must be reviewed again.
