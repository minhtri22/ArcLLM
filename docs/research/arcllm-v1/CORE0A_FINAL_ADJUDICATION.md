# CORE-0A — Final ArcLLM Adjudication

Date: 2026-09-29  
Branch: `research/current-main-baseline-xray`

## Question

Can the frozen Token-XRay compatibility release observe the current canonical ArcLLM execution surface without losing current decode/prefill geometry, semantic phase, Q4V4 lifecycle separation, or timing-authority boundaries?

## Frozen systems

ArcLLM canonical parent:

`7a5672112dc22de15f0e9bb6445508fbb3099b15`

Token-XRay final compatibility freeze:

`freeze/token-xray-core-v0.1-arcllm-current-compatible@35f86ac68f98ffe60fc441a790274cd1f1269dfe`

Token-XRay validated code payload:

`984908d8ab457328fd82b74090d4ab29383acc2d`

Independent Git comparison confirmed that all six commits after the validated code payload change only documentation, lineage, or QA/freeze artifacts.

## Independent ArcLLM revalidation

ArcLLM reran its own deterministic compatibility auditor against a clean checkout of the exact Token-XRay freeze.

Result:

```text
CORE0A_VERDICT=PASS_CORE0A_TOKEN_XRAY_COMPATIBLE
OPEN_FINDINGS=0
DECODE_GEOMETRY=469/469
PREFILL_GEOMETRY=441/441
```

Additional evidence:

- Token-XRay Python compileall: PASS
- Token-XRay pytest: 63 passed, 1 skipped, 0 failed
- decode semantic suffix coverage: complete
- decode unmapped shader dispatches: 0
- prefill unmapped shader dispatches: 0
- prefill semantic issues: none
- Q4V4 P1 materialization shader mapping: present in Python and C++ adapters
- separate `RUNTIME_LIFECYCLE_TRACE`: detected with acquire/materialize/validate/resident/reuse/evict/release contract
- observer contract fields: measurement_mode, timing_authority, instrumentation_overhead all present
- Token-XRay validated code payload is an ancestor of the final freeze
- post-code freeze changes contain no code/schema/test modifications

## Verdict

**PASS_CORE0A_TOKEN_XRAY_COMPATIBLE**

CORE-0A is formally closed.

This is a zero-science tooling/system compatibility PASS. It is **not** an ArcLLM performance result and is not appended to the scientific lineage.

## Consequence

CORE-0B is now unblocked.

CORE-0B must remain uninstrumented performance authority. Token-XRay instrumented collection remains CORE-0C diagnostic/localization evidence and cannot replace CORE-0B timing.

## Preserved boundaries

- science measured inferences in CORE-0A: 0
- ArcLLM runtime modified: false
- mechanism selected: false
- performance claim created: false
- NPU execution added: false
