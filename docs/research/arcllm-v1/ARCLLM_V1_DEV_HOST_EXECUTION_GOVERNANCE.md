# ArcLLM v1 — Dev-Host Execution Governance

**Status:** PROSPECTIVE AMENDMENT BEFORE I002 T3 OUTCOME EXPOSURE  
**Scope:** ArcLLM v1 executions performed on a shared development machine

## 1. Principle

Governance protects scientific interpretation, provenance, and anti-cherry-pick behavior. It must not require a dedicated idle workstation when the actual research substrate is a shared development machine.

Ambient CPU/RAM/GPU/process load is therefore an observed covariate, not a hard execution blocker.

Hard blockers remain limited to identity/correctness conditions such as:
- wrong model/build/candidate;
- wrong hardware/driver when the study explicitly binds them;
- corrupted/missing executable or shader;
- semantic guard failure where the study defines it as a correctness boundary.

## 2. DEV_HOST execution class

A DEV_HOST run may coexist with unrelated development workloads.

The runner should record, when practical:
- approximate CPU load;
- free/used physical memory;
- top memory/CPU processes;
- active power scheme;
- AC/battery state.

These observations describe the execution context. They do not automatically invalidate or block a run.

Claims from DEV_HOST runs are scoped to a mixed-load development host unless separately replicated under a controlled idle host.

## 3. Rerun policy

Rerun is allowed under the same frozen payload.

Rules:

1. Every invocation receives a unique attempt ID.
2. Every attempt is append-only; prior evidence is never deleted or overwritten.
3. No selective cell rerun. A rerun is the complete frozen collection.
4. No parameter/kernel/threshold/workload changes between attempts.
5. Infrastructure failure, OS interruption, OOM, driver reset, competing dev workload, or user interruption do not permanently consume the study.
6. A completed valid collection remains evidence even if later replications are run.

## 4. Confirmatory interpretation

For a confirmatory study:

- the **first complete valid collection** is the primary confirmatory dataset;
- later complete collections are replication/robustness evidence;
- later runs may reveal instability or environment sensitivity but do not erase or replace the primary result;
- partial/failed operational attempts remain provenance but are not promoted to complete confirmatory datasets.

This prevents both governance lockout and rerun-until-PASS behavior.

## 5. Forbidden rescue remains forbidden

Even on DEV_HOST, the following remain prohibited after outcome exposure:
- changing the candidate;
- tuning subgroup/local-size/tile;
- changing gates or primary endpoints;
- dropping a failed cell;
- deleting an unfavorable complete run;
- choosing only favorable attempts for the final report.

## 6. I002 T3 application

I002 T3 uses DEV_HOST governance prospectively before any T3 outcome exists.

Therefore:
- ambient machine load is not a blocker;
- the old permanent science-start marker is replaced by per-attempt append-only markers;
- manual full-collection rerun is allowed;
- automatic/self rerun remains disabled;
- first complete valid T3 collection is primary;
- later complete T3 collections are replication evidence;
- T2 semantic guard remains a correctness gate.


## Chronology correction — primary I002 T3

The first complete I002 T3 bundle was generated under the earlier authorization `v0.3` / HEAD `d5f144ce...` before this DEV_HOST amendment was adopted.

Therefore:
- that primary T3 collection is adjudicated under its original v0.3 one-shot contract;
- this DEV_HOST governance is not applied retroactively to change its validity or gates;
- DEV_HOST rules apply only to future replication/re-execution programs opened after the amendment.

This correction preserves the no-post-outcome-mutation invariant.
