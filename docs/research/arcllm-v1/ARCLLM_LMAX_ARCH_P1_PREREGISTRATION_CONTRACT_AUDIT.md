# ARCLLM_LMAX_ARCH_P1 — Preregistration Contract Audit

Status: **P REVISION REQUIRED BEFORE E**

The frozen v0.1 preregistration correctly repairs the P0 evidence-schema omissions and makes the 1,000,000-event control-path subtest mandatory inside fresh E. During internal readiness review, one scientific ambiguity was found before any P1 outcome was opened.

## Blocking ambiguity

The inference arms are fully specified, but the zero-model control-path subtest says Arm A is the direct reference without freezing the exact direct-reference dispatcher mechanics.

That leaves outcome-bearing degrees of freedom in:
- whether Arm A is synchronous inline dispatch, a one-slot handoff, or a bounded queue;
- whether Arm A uses a mutex/condition variable, atomics, or another mechanism;
- whether Arm A has the same capacity 64 as Arm B;
- how Arm A full/backpressure observations are defined;
- consequently, what denominator makes the preregistered rule "Arm B full/backpressure observation rate is not worse than A" well-defined.

Choosing any of these during R would change the scientific comparator and could materially change CONTROL_PATH_ONLY.

## Required P amendment

Before R may implement the control-path subtest, a new preregistration revision must freeze the exact Arm-A control dispatcher.

Recommended fair comparator:

`DIRECT_LOCKED_SPSC64`

- exactly one producer and one consumer;
- fixed preallocated array of 64 event slots;
- FIFO head/tail/count protected by one `std::mutex`;
- producer blocks on `not_full` `std::condition_variable`;
- consumer blocks on `not_empty` `std::condition_variable`;
- no dynamic allocation after queue initialization;
- the same event payload, capacity, service delay, event count and timestamp positions as `LMAX_RING_P1`;
- full/backpressure observation is incremented once per publish attempt that finds count==64 before waiting;
- exact consumed order and raw latency streams are serialized identically for both arms.

This comparator makes the control-path question a bounded comparison of a conventional blocking SPSC handoff against the P1 sequenced-ring/single-writer handoff, without giving either arm an allocation or capacity advantage by construction.

No P1 fresh outcome, model inference, Vulkan execution or control-path performance measurement has occurred during this audit.
