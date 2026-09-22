# TTFT_M2 — P9C-A Execution Identity Binding Correction and Zero-Science Revalidation

**Result:** `PASS_M2_P9C_A_EXECUTION_IDENTITY_BINDING_CORRECTION_AND_ZERO_SCIENCE_REVALIDATION`

P9C-A corrects only the pre-execution provenance/runtime-binding defects found by the first P9C execution authorization gate.

No scientific execution occurred.

## Corrected authority

New execution-binding contract:

```text
config/arcllm_ttft_m2_p9c_a_execution_binding_v0.1.json
90dc9d87fb987a3892d0a64c27e89baa2cbd3a32
```

Corrected future science runner:

```text
scripts/ttft_m2/run_p9_science.ps1
f0a627de05c509286cdb51a070d409b297c7dbaa
```

It prospectively supersedes only the old future runner:

`4326d69f058ad8266e88f20272b3ca17f1b21293`

The P9B lock remains immutable historical BuildOnly authority.

## Why a new execution-binding contract is required

P9B lock v0.2 includes the historical future science-runner blob as one of its exact members.

Mutating that lock after P9B PASS would falsify the BuildOnly audit trail.

P9C-A therefore preserves P9B exactly and introduces a narrow new execution authority:

- P9B lock remains authority for the qualified scientific payload/build inputs;
- the corrected P9C-A runner becomes the prospective execution runner;
- at runtime all P9B exact Git members are reverified except the single superseded historical runner path;
- a future P9C authorization must bind both the corrected runner blob and the P9C-A execution-binding blob.

## Resolution of P9C blockers

### G1 — stale lock reference

Resolved.

The runner now requires:

`config/arcllm_ttft_m2_p9b_implementation_lock_v0.2.json`

and contains no v0.1 lock reference.

### G2 — qualified executable identity

Resolved.

Before any executable launch, F0 requires:

```text
SHA256
9E0C0A7CCCBB2DA767B4DE90354EC7ADF4463286264187843521419113C019C7

bytes
441856
```

### G3 — qualified build-manifest identity

Resolved.

F0 requires exact byte hashes:

```text
NATIVE_BUILD.json
0E51867BE03AFD60E90B823D2A6FE19F22ACBF6B628E9CE37E30E673C6AF91E4

SHADER_BUILD.json
9298D5EE2838B3FE5D39FC968DC937DC4183F168517CEE1F30F54E9CA86093FA
```

### G4 — runtime SPIR-V identity

Resolved.

The runner now hashes every actual SPIR-V file in the runtime shader directory and requires equality with the already-qualified exact shader manifest.

This includes all 16 common artifacts and Q4FAST.

### G5 — frozen gate order

Resolved.

The runner is now structurally ordered:

```text
F0 provenance / payload / environment / freshness
        ↓
F1 H-ART static identity
        ↓
F2–F7 fresh collection + later independent adjudication
```

No executable launch occurs before F0 and F1 complete.

## Zero-science revalidation

An independent source-level audit of the corrected exact runner confirms all five blockers are resolved.

A reproducibility test is also committed:

```text
tests/test_ttft_m2_p9c_a_runner.py
1dba3b2305a608b5f6fbe81d31a2dd1ab17baac8
```

The Python test is committed for future/local reproduction; it was not represented as executed in this gate.

Scientific accounting remains:

```text
diagnostic executable launches  0
target model loads              0
GPU dispatches                  0
performance measurements       0
fresh TTFT observations         0
mechanism adjudication          false
scientific result               NONE
```

No P9C execution authorization file is created by P9C-A.

## Repair budget

P9B BuildOnly PASS remains valid and unchanged.

The correction is entirely pre-science and occurs before any science-runner invocation.

Package repair budget consumed remains:

`0/1`

## Next admissible step

`M2_P9C_EXPLICIT_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_AUTHORIZATION_GATE_REVIEW_2`

Only that separate gate may decide whether model load, GPU dispatch, timing and the 80 fresh TTFT observations become authorized.
