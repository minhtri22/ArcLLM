# CORE-0C — Zero-Science Preflight Final Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0C_ZERO_SCIENCE_PREFLIGHT**

Open findings: **0**.

This closes only the implementation/static-preflight gate. No model inference or CORE-0C measured request was executed during this gate.

## Frozen implementation

- implementation HEAD: `8113d909ab31f42198485cb13972b90d414dab89`
- final preflight evidence commit: `74c5e859f59a2a529decb30c1bcda08e6437c17f`
- Token-XRay: `freeze/token-xray-core-v0.1-arcllm-current-compatible@35f86ac68f98ffe60fc441a790274cd1f1269dfe`
- trace executable SHA256: `22CBBA8A3168EF0F1480994890C05F72D2061642B497E8B9AEBA2A5BD5CCAED4`

The diagnostic executable is a deterministic benchmark-only derivative from exact canonical ArcLLM blobs. Canonical runtime, public API, Q4V4 backend and shader source remain unchanged.

## Static/fixture qualification

```text
Token-XRay selected tests   38 passed, 1 skipped, 0 failed
decode geometry             469/469
prefill geometry            441/441
decode semantics            451/451
prefill semantics           451/451
Q4V4 lifecycle separation   PASS
prefill/decode mismatch     REJECTED as required
observer-runner fixture     PASS
science runner fail-closed  PASS
```

Host Vulkan timestamp capability was queried directly:

```text
device                    Intel(R) Arc(TM) 140V GPU (16GB)
timestampPeriod           52.0833 ns
timestampValidBits        64
```

## Integration boundary

ArcLLM's canonical Vulkan layer uses a manually declared ABI rather than the official Vulkan header surface consumed by Token-XRay's native Vulkan runtime tracer. CORE-0C therefore uses a benchmark-only bridge: exact Token-XRay semantic/lifecycle contracts plus a deterministic timestamp-query shim generated from canonical ArcLLM source.

This does not authorize that bridge as product runtime code.

## Authority boundary

CORE-0B remains the product-performance authority.

CORE-0C trace timing is permanently:

```text
measurement_mode = TOKEN_XRAY_TRACE
timing_authority = DIAGNOSTIC_ONLY
```

The adjacent control runs exist only to quantify observer overhead.

## Consequence

The exact 12-request paired diagnostic collection may now be authorized:

```text
W-S: 3 blocks × (CONTROL, TRACE)
W-C: 3 blocks × (CONTROL, TRACE)
total = 12 requests
```

Order is frozen with block 1 reversed. No selective rerun, hardware counters, profiler, resource sampler, NPU path, mechanism selection or CORE-0D execution is authorized.

This zero-science PASS is not appended to the scientific lineage.
