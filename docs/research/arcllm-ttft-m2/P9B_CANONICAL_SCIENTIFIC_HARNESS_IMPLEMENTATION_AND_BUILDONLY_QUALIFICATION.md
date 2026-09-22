# TTFT_M2 — P9B Canonical Scientific Harness Implementation and BuildOnly Qualification

**Status:** `IMPLEMENTED_BUILDONLY_AUTHORIZED_LOCAL_EXECUTION_PENDING`

P9B creates a fresh M2-native scientific execution package bound to the P9A canonical design. It does not reuse the M1 implementation lock or M1 BuildOnly runner as authority.

## Canonical implementation

```text
diagnostic source
14eb11690b76c1ada102ab2d1ced9607ff522c34

native build tool
46e733a2a7948f0182825fd1a1d069f029ba2050

shader build tool
4a217a8911c2b9b657f0177ab55f1ae45c4ff571

static package QA
94d5640cfcf47b02f29446c783e647c03b0ddb28

future science runner
4326d69f058ad8266e88f20272b3ca17f1b21293

implementation lock
d3df46210f81b113e346b0cb50faffcdb3a7eaaa

BuildOnly runner
05f4bddbbe697e39ba4968a54bc4594a5435541c

BuildOnly authorization
0e5ec30d0a1d302c898cbc6343df6b7fe13c186a
```

The M2 diagnostic source preserves the P9A causal design but uses fresh M2 program/schema identity. Static QA rejects stale M1 program authority.

## BuildOnly behavior

The exact runner:

`run_ttft_m2_p9b_buildonly.ps1`

performs:

1. tracked-worktree cleanliness check;
2. exact authorization/lock/runner provenance checks;
3. exact Git-blob verification for all design, source, build, shader and orchestration members;
4. static package QA;
5. shader compilation only;
6. native executable compilation only;
7. manifest/hash verification;
8. evidence bundle packaging.

It **does not launch** the diagnostic executable.

## H-ART boundary

P9B compiles and hashes the shader payload but deliberately sets:

`h_art_mechanism_adjudication_performed = false`

H-ART static scientific adjudication remains in the future hard-gated science runner after a separate execution authorization.

This keeps P9B a zero-science BuildOnly qualification.

## Future science runner

`scripts/ttft_m2/run_p9_science.ps1`

is committed and provenance-bound, but it hard-fails unless this future file exists:

`config/arcllm_ttft_m2_p9c_execution_authorization_v0.1.json`

with exact decision:

`M2_P9C_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_AUTHORIZED`

Therefore committing P9B cannot accidentally launch model/GPU/timing science.

## Scientific boundary

Still forbidden during P9B:

```text
diagnostic executable launch  false
target model load             false
GPU dispatch                  false
performance measurement       false
fresh TTFT observations       0
H-ART mechanism adjudication  false
mechanism result              NONE
```

## Qualification state

P9B implementation is frozen and BuildOnly execution is authorized on the qualified local Windows substrate.

P9B is **not yet PASS**.

The exact BuildOnly runner must execute successfully on local Windows and its returned bundle must be independently adjudicated before any scientific execution authorization can be considered.
