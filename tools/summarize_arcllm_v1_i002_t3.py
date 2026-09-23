import argparse,json,statistics,math
from pathlib import Path
CELLS={"A_WS":("A","W-S"),"A_WC":("A","W-C"),"B_WC":("B","W-C"),"B_WS":("B","W-S")}
ap=argparse.ArgumentParser();ap.add_argument("--results-dir",required=True);ap.add_argument("--out",required=True)
a=ap.parse_args();root=Path(a.results_dir)
errors=[];data={}
for fn,(sess,wl) in CELLS.items():
    p=root/f"i002_t3_{fn}.json"
    if not p.exists():errors.append(f"missing {p.name}");continue
    d=json.loads(p.read_text(encoding="utf-8-sig"));data[fn]=d
    if d.get("schema")!="arcllm.v1.i002.t3.paired_cell.v0.1":errors.append(f"{fn}: schema")
    if d.get("session")!=sess or d.get("workload")!=wl:errors.append(f"{fn}: identity")
    if d.get("t2_semantic_guard_pass") is not True:errors.append(f"{fn}: T2 semantic guard")
    pairs=d.get("pairs",[])
    if len(pairs)!=5:errors.append(f"{fn}: pair count")
    for i,pair in enumerate(pairs):
        b=pair.get("baseline",{});c=pair.get("candidate",{})
        if not b.get("success") or not c.get("success"):errors.append(f"{fn}/{i}: execution")
        if not b.get("final_logits_finite") or not c.get("final_logits_finite"):errors.append(f"{fn}/{i}: finite")
        if not b.get("dispatch_census_pass") or not c.get("dispatch_census_pass"):errors.append(f"{fn}/{i}: census")
        if not pair.get("semantic_equal"):errors.append(f"{fn}/{i}: token semantic guard")
        if b.get("generated_token_ids")!=c.get("generated_token_ids"):errors.append(f"{fn}/{i}: token mismatch")

out={"schema":"arcllm.v1.i002.t3.adjudication_input.v0.1","validity":"FAIL" if errors else "PASS","errors":errors}
if not errors:
    cs={};speedups=[]
    for k,d in data.items():
        dr=[];tt=[];er=[];ds=[]
        for pair in d["pairs"]:
            b=pair["baseline"];c=pair["candidate"]
            dr.append(c["decode_ms"]/b["decode_ms"])
            ds.append(b["decode_ms"]/c["decode_ms"])
            tt.append(c["ttft_ms"]/b["ttft_ms"])
            er.append(c["e2e_ms"]/b["e2e_ms"])
        md=statistics.median(dr);ms=statistics.median(ds);mt=statistics.median(tt);me=statistics.median(er)
        speedups.append(ms)
        cs[k]={
          "decode_latency_ratio_median_paired":md,
          "decode_speedup_median_paired":ms,
          "ttft_ratio_median_paired":mt,
          "e2e_ratio_median_paired":me,
          "g2_cell_decode_pass":md<=0.90,
          "g3_ttft_pass":mt<=1.10,
          "g4_e2e_pass":me<1.00,
        }
    geo=math.exp(sum(math.log(x) for x in speedups)/len(speedups))
    g2=all(x["g2_cell_decode_pass"] for x in cs.values()) and geo>=1.25
    g3=all(x["g3_ttft_pass"] for x in cs.values())
    g4=all(x["g4_e2e_pass"] for x in cs.values())
    out["cells"]=cs;out["global_decode_geomean_speedup"]=geo
    out["g2_decode_pass"]=g2;out["g3_ttft_pass"]=g3;out["g4_e2e_pass"]=g4
    if not g2:dec="FAIL_I002_CARRY_THROUGH"
    elif not g3:dec="FAIL_I002_TTFT_COUPLING"
    elif not g4:dec="FAIL_I002_E2E_INTEGRATION"
    else:dec="PASS_I002_T3_CARRY_THROUGH"
    out["decision"]=dec
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(out.get("decision","INVALID"))
raise SystemExit(0 if not errors else 2)
