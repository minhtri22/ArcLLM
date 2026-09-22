# TTFT_M2 — ORIGIN

**Date:** 2026-09-22  
**Program:** `ARCLLM_TTFT_M2`  
**Branch:** `research/arcllm-ttft-m2`  
**Git history base:** `86ec48ce0cfff28fa43c23c1d8b57b8f881d249e`

## Independent-program declaration

TTFT_M2 is a new independent research program. It is not ANL64 rescue, TTFT_M1 rescue, TTFT_M1 continuation, TTFT_M1 lock v0.3, a renamed P7, or a mechanism for bypassing the TTFT_M1 terminal stop.

The Git history base is the TTFT_M1 terminal commit only so source history remains available. Scientific lineage is fresh and lives at:

`programs/arcllm_ttft_m2/lineage.md`

The closed TTFT_M1 lineage must never be appended.

## Immutable parent A — ANL64

- branch: `research/arcllm-nexus-ledger64`
- terminal HEAD: `0e40b3affe4f6add9ce23921687b2659017c95d3`
- terminal status: `ANL64_PROGRAM_CLOSED_VALID_NEGATIVE_TTFT_BLOCKED`
- P7 adjudication: `artifacts/ANL64/ANL64_P7_FINAL_PROGRAM_ADJUDICATION_v0.1.json`
- P7 adjudication blob: `79df1758f23f724890eb302907c0b2d3045df735`
- P7 document blob: `dd0ab0389251e77ccc2d654e77a897781426be34`
- P6 formal adjudication blob: `56dd01850238e4131d357303d3888fd1826f6eb9`

ANL64's fresh P6 evidence demonstrated semantic preservation and material decode/E2E gains, while the preregistered TTFT blocking guard failed in 3/4 matched comparisons. Those timings are historical parent evidence only and cannot be used as TTFT_M2 confirmatory observations.

## Immutable parent B — TTFT_M1

- branch: `research/arcllm-ttft-mechanism`
- terminal HEAD: `86ec48ce0cfff28fa43c23c1d8b57b8f881d249e`
- terminal commit message: `research: finalize TTFT M1 terminal provenance`
- terminal status: `TTFT_M1_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`
- P10 adjudication blob: `3fd5ed7909f409e3b7419da8ec50cfc6041e1f48`
- P10 document blob: `219724e91a8b228dc11197600def4abd19e3d0c4`
- P6 terminal infrastructure adjudication blob: `de89ae36950a9bee2262c7f055cad95a7d9e3106`
- terminal governance blob: `64d6faffdd20d131938d7178b13acd928f0b5c10`
- closed lineage blob: `6ab8d3aa4c36c58f2ae898bbbc9f2efc443f7980`

TTFT_M1 produced no empirical mechanism result:

- BuildOnly runner executions: 0
- diagnostic executable launches: 0
- target model loads: 0
- GPU dispatches: 0
- fresh TTFT observations: 0
- parent P6 timings reused as fresh data: 0

Therefore `H-ART`, `H-DPIPE`, `H-PRECOND`, `H-STATE-INTERACTION`, `H-NULL`, and any TTFT-removal intervention remain unadjudicated.

## Why TTFT_M2 exists

TTFT_M1 exposed an infrastructure-governance failure before science: a repaired implementation lock v0.2 was correctly used by preflight, but the runner's result/package section still referenced v0.1. Because the execution-stage repair allowance had already been consumed by an earlier H-ART operationalization defect, the second defect forced a terminal stop.

TTFT_M2 therefore has an infrastructure validity question that precedes its scientific question:

> Can the complete mechanism-identification execution package be proven internally self-consistent — runner, lock, schemas, provenance and packaging — before any execution-stage repair budget or scientific observation is at risk?

Only after that question is answered by a zero-science atomic-package QA may BuildOnly authorization even be considered.

## Current authorization

At program creation:

- diagnostic-science implementation: false
- BuildOnly execution: false
- target model load: false
- GPU dispatch: false
- performance measurement: false
- fresh TTFT observation: false
- execution-stage repair budget active: false

## No-drift rule

If a proposed action reopens either parent, mutates their closed lineage, imports a TTFT_M1 mechanism verdict, treats ANL64 timing as fresh TTFT_M2 evidence, activates the execution-stage repair budget before atomic-package QA, or authorizes target execution before governance permits it, classify the action as:

`STOP_DRIFT`
