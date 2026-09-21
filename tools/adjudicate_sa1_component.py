#!/usr/bin/env python3
from __future__ import annotations
import json, math, statistics, sys
from pathlib import Path
Q4_CELLS=["Q4_H3584_R3584_BIAS","Q4_H3584_R3584_NOBIAS","Q4_H3584_R512_BIAS","Q4_H3584_R18944_NOBIAS","Q4_H18944_R3584_NOBIAS"]
Q6_CELLS=["Q6_H3584_R512_BIAS","Q6_H18944_R3584_NOBIAS"]
def load(p):
    x=json.loads(Path(p).read_text(encoding="utf-8"))
    if x.get("status")!="MEASUREMENT_COMPLETE_NOT_ADJUDICATED": raise SystemExit(f"invalid result {p}")
    return x
def one(x,cells):
    out={}
    for c in x["cells"]:
        b=statistics.median(c["baseline_ns"]);q=statistics.median(c["candidate_ns"])
        out[c["id"]]={"baseline_median_ns":b,"candidate_median_ns":q,"speedup":b/q}
    if set(out)!=set(cells): raise SystemExit("cell census mismatch")
    gm=math.prod(out[k]["speedup"] for k in cells)**(1/len(cells))
    return out,gm
def main():
    if len(sys.argv)!=4: raise SystemExit("usage: adjudicate A.json B.json out.json")
    A,B=load(sys.argv[1]),load(sys.argv[2])
    if A.get("process")!="A" or B.get("process")!="B": raise SystemExit("process identity mismatch")
    quant=A.get("quant","Q4_K")
    if B.get("quant","Q4_K")!=quant: raise SystemExit("quant identity mismatch")
    cells=Q6_CELLS if quant=="Q6_K" else Q4_CELLS
    ar,ag=one(A,cells);br,bg=one(B,cells)
    floor=min([v["speedup"] for v in ar.values()]+[v["speedup"] for v in br.values()])
    passed=ag>=1.5 and bg>=1.5 and floor>=1.1
    stage="Q6" if quant=="Q6_K" else "Q4"
    result={"schema":f"arcllm.sa1.{stage.lower()}.adjudication_candidate.v0.1","quant":quant,"A":{"cells":ar,"geomean_speedup":ag},"B":{"cells":br,"geomean_speedup":bg},"minimum_cell_speedup":floor,"frozen_gate":{"geomean_min":1.5,"every_cell_min":1.1},"classification":f"{stage}_STAGE_PASS" if passed else f"{stage}_STAGE_FAIL","requires_independent_adjudication":True}
    Path(sys.argv[3]).write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
if __name__=="__main__": main()
