#!/usr/bin/env python3
import hashlib, json, sys

SEED=20260928
PREFIX="ANL64_CRT_P3"
CONDITIONS=[
  "A_CANONICAL/W-S",
  "B_PLAN_PREBOUND/W-S",
  "C_PLAN_PREBOUND_RESIDUAL/W-S",
  "A_CANONICAL/W-C",
  "B_PLAN_PREBOUND/W-C",
  "C_PLAN_PREBOUND_RESIDUAL/W-C"
]
BASE_ROWS=[
  [1,2,6,3,5,4],
  [2,3,1,4,6,5],
  [3,4,2,5,1,6],
  [4,5,3,6,2,1],
  [5,6,4,1,3,2],
  [6,1,5,2,4,3]
]

def H(kind,item):
    return hashlib.sha256(f"{PREFIX}|{SEED}|{kind}|{item}".encode("utf-8")).hexdigest()

cond_order=sorted(CONDITIONS,key=lambda c:H("condition",c))
mapping={i+1:c for i,c in enumerate(cond_order)}
row_order=sorted(range(6),key=lambda i:H("row",i))
blocks=[]
for b,ri in enumerate(row_order,1):
    blocks.append({"block":b,"source_williams_row_zero_based":ri,
                   "order":[mapping[x] for x in BASE_ROWS[ri]]})
out={
 "schema":"arcllm.anl64_crt.p3.blocked_randomized_schedule.v0.2",
 "seed":SEED,
 "algorithm":"lexicographic ascending SHA256 of UTF-8 strings PREFIX|SEED|kind|item; assign treatment IDs 1..6 by sorted condition hashes; order Williams rows by sorted row hashes",
 "prefix":PREFIX,
 "condition_hashes":{c:H("condition",c) for c in CONDITIONS},
 "row_hashes":{str(i):H("row",i) for i in range(6)},
 "treatment_label_mapping":{str(i):mapping[i] for i in range(1,7)},
 "williams_base_rows":BASE_ROWS,
 "frozen_row_order_zero_based":row_order,
 "blocks":blocks
}
print(json.dumps(out,indent=2))
