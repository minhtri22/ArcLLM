# ArcLLM — Final Validation Verdict

**Date:** 2026-09-20  
**Verdict:** `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`

## What was established

ArcLLM established real-model end-to-end feasibility for the frozen 7B target on the Intel Arc 140V system. Q2 then completed a matched llama.cpp characterization, and Q3 completed 40 fresh measured attempts across two independently launched sessions.

The current architecture therefore has strong evidence for **feasibility and reproducibility of execution**.

## What was not established

The current architecture did not demonstrate a preregistered practical regime advantage in either W-S or W-C.

Across both fresh sessions, ArcLLM remained substantially behind the matched llama.cpp baseline on TTFT, decode throughput, E2E latency and peak working set. Lower private bytes and CPU utilization were supporting observations only and were not sufficient under the frozen Q3 gate.

## Final claim

> With the frozen 7B GGUF, 4096 context envelope and W-S/W-C workloads on the target Intel Core Ultra 7 258V + Arc 140V system, ArcLLM can execute the model end-to-end with its native Vulkan runtime, but the validated current architecture has **no demonstrated practical regime advantage** over the matched llama.cpp v0.4.1 Vulkan baseline.

This claim is bounded to the validated architecture, hardware, model and workloads.

## Stop decision

The current ArcLLM architecture line is closed.

Further iterative optimization of this exact line is not authorized by the completed validation program. Any future architecture work must be a separately justified successor study rather than continuation of Q3.

The only scientifically valid reopening route is a post-verdict Architecture Intervention Review that first establishes a mechanistic rationale for a bounded successor architecture hypothesis.
