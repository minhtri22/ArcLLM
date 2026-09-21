#!/usr/bin/env python3
from __future__ import annotations
import json, math, statistics, sys
from pathlib import Path
CELLS=["Q4_H3584_R3584_BIAS","Q4_H3584_R3584_NOBIAS","Q4_H3584_R512_BIAS","Q4_H3584_R18944_NOBIAS","Q4_H18944_R3584_NOBIAS"]
def load(p):
    x=json.loads(Path(p).read_text(encoding="utf-8"))
    if x.get("status")!="MEASUREMENT_COMPLETE_NOT_ADJUDICATED": raise SystemExit(f"invalid result {p}")
    return x
def one(x):
    out={}
    for c in x["cells"]:
        b=statistics.median(c["baseline_ns"]);q=statistics.median(c["candidate_ns"])
        out[c["id"]]={"baseline_median_ns":b,"candidate_median_ns":q,"speedup":b/q}
    if set(out)!=set(CELLS): raise SystemExit("cell census mismatch")
    gm=math.prod(out[k]["speedup"] for k in CELLS)**(1/len(CELLS))
    return out,gm
def main():
    if len(sys.argv)!=4: raise SystemExit("usage: adjudicate A.json B.json out.json")
    A,B=load(sys.argv[1]),load(sys.argv[2])
    if A.get("process")!="A" or B.get("process")!="B": raise SystemExit("process identity mismatch")
    ar,ag=one(A);br,bg=one(B)
    floor=min([v["speedup"] for v in ar.values()]+[v["speedup"] for v in br.values()])
    passed=ag>=1.5 and bg>=1.5 and floor>=1.1
    result={"schema":"arcllm.sa1.k1.q4.adjudication_candidate.v0.1","A":{"cells":ar,"geomean_speedup":ag},"B":{"cells":br,"geomean_speedup":bg},"minimum_cell_speedup":floor,"frozen_gate":{"geomean_min":1.5,"every_cell_min":1.1},"classification":"Q4_STAGE_PASS" if passed else "Q4_STAGE_FAIL","q6_implementation_permitted":False,"requires_independent_adjudication":True}
    Path(sys.argv[3]).write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
if __name__=="__main__": main()
