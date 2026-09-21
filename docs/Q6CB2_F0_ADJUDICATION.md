# Q6CB-2 F0 Adjudication — Terminal Infrastructure Stop

**Decision:** `STOP_INFRASTRUCTURE_UNSTABLE`

Q6CB-2 did not reach scientific execution.

## Evidence sequence

The first authorized runner invocation failed closed during environment preflight because `vulkaninfo.exe` was unavailable. No Q6CB fixture was generated and the Q6CB causal harness was not launched.

Governance allowed one infrastructure-only repair for the stage. That repair replaced the `vulkaninfo.exe` assumption with the already-frozen SA0 zero-science Vulkan capability probe.

The second invocation again failed before any scientific fixture:

```text
SA0-CAP BuildOnly PASS
scientific workload=NOT RUN
SA0-CAP error: requested Vulkan device substring not found
Q6CB2_FAIL_CLOSED: SA0 capability probe execution failed
```

Therefore the second event is a second pre-science infrastructure F0 in the same Q6CB-2 stage.

## Technical diagnosis

The repaired runner invokes the probe with:

```text
--device-substring "Arc 140V"
```

The previously qualified Vulkan target name is:

```text
Intel(R) Arc(TM) 140V GPU
```

The probe uses a contiguous substring match. Therefore `Arc 140V` does not match `Arc(TM) 140V`. This is a plausible static explanation of the failure.

It is intentionally **not repaired**. The termination contract permits at most one infrastructure repair per stage and specifies that a second F0 terminates as `STOP_INFRASTRUCTURE_UNSTABLE`.

## Scientific interpretation

This is not:

- `BOUNDARY_NOT_REPRODUCED`;
- `UNRESOLVED_MECHANISM`;
- support or rejection of H-RTCI, H-PDI, H-DSA or H-SEM.

No valid Q6CB-2 identification collection exists, so F1-F6 are not adjudicated.

The frozen scientific design remains intact and unmodified. The failure is a terminal execution-infrastructure result, not a causal result.

## Closure

Q6CB-2 execution authorization is revoked. Q6CB-3 remains closed. No second runner repair, direct harness invocation, fixture substitution, threshold change or renamed continuation is permitted within Q6CB.

The next scientifically valid action is a **program-level ArcLLM decision** that records Q6CB as infrastructure-terminated with no causal result; it is not another Q6CB experiment.
