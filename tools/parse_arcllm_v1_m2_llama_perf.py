import argparse,json,re,statistics
from pathlib import Path

LINE_RE=re.compile(r"^(.*?):\s+(\d+)\s+x\s+([0-9.eE+-]+)\s+us\s+=\s+([0-9.eE+-]+)\s+us")
TOTAL_RE=re.compile(r"^Total time:\s+([0-9.eE+-]+)\s+us")
MM_RE=re.compile(r"MUL_MAT(?:_VEC)?\s+([qQ][46]_K)\s+m=(\d+)\s+n=(\d+)\s+k=(\d+)")

def classify(name):
    m=MM_RE.search(name)
    if not m: return None
    typ=m.group(1).upper(); M=int(m.group(2)); N=int(m.group(3)); K=int(m.group(4))
    if N!=1: return None
    if typ=="Q4_K" and M==18944 and K==3584: return "ffn_gate_up_q4_k"
    if typ=="Q4_K" and M==3584 and K==18944: return "ffn_down_q4_k"
    if typ=="Q6_K" and M==3584 and K==18944: return "ffn_down_q6_k"
    if typ=="Q6_K" and M==152064 and K==3584: return "lm_head_q6_k"
    if typ=="Q4_K" and M==3584 and K==3584: return "q_o_q4_k_combined"
    if typ=="Q4_K" and M==512 and K==3584: return "k_v_q4_k_combined"
    if typ=="Q6_K" and M==512 and K==3584: return "v_q6_k"
    return None

EXPECTED_COUNTS={
 "ffn_gate_up_q4_k":56,
 "ffn_down_q4_k":14,
 "ffn_down_q6_k":14,
 "lm_head_q6_k":1,
 "q_o_q4_k_combined":56,
 "k_v_q4_k_combined":42,
 "v_q6_k":14,
}

def blocks(path):
    lines=Path(path).read_text(encoding="utf-8",errors="replace").splitlines()
    out=[]; cur=None
    for line in lines:
        if line.strip()=="Vulkan Timings:":
            cur={"entries":[],"total_us":None}
            continue
        if cur is None: continue
        m=LINE_RE.match(line.strip())
        if m:
            cur["entries"].append({"name":m.group(1),"count":int(m.group(2)),"avg_us":float(m.group(3)),"total_us":float(m.group(4))})
            continue
        mt=TOTAL_RE.match(line.strip())
        if mt:
            cur["total_us"]=float(mt.group(1)); out.append(cur); cur=None
    return out

def decode_block(b):
    for e in b["entries"]:
        if classify(e["name"])=="lm_head_q6_k" and e["count"]==1:
            return True
    return False

def map_block(b):
    fam={}
    raw=[]
    for e in b["entries"]:
        f=classify(e["name"])
        if f:
            raw.append({"family":f,**e})
            if f in fam: raise ValueError("duplicate semantic anchor "+f)
            fam[f]={"count":e["count"],"total_us":e["total_us"],"share":e["total_us"]/b["total_us"] if b["total_us"] else None,"raw_name":e["name"]}
    missing=[f for f in EXPECTED_COUNTS if f not in fam]
    if missing: raise ValueError("missing semantic families: "+",".join(missing))
    bad={f:(fam[f]["count"],n) for f,n in EXPECTED_COUNTS.items() if fam[f]["count"]!=n}
    if bad: raise ValueError("semantic call-count mismatch: "+repr(bad))
    return {"total_us":b["total_us"],"families":fam,"raw_semantic_entries":raw}

def median(xs): return statistics.median(xs)

def process(path,workload):
    bs=blocks(path)
    dec=[b for b in bs if decode_block(b)]
    if len(dec)<62: raise ValueError(f"{workload}: expected at least 62 decode timing blocks from warmup+measured, got {len(dec)}")
    measured=dec[-31:]
    probes=[0,15,30]
    selected=[map_block(measured[i]) for i in probes]
    fams={}
    for f in EXPECTED_COUNTS:
        fams[f]={
          "median_us":median([x["families"][f]["total_us"] for x in selected]),
          "median_share":median([x["families"][f]["share"] for x in selected]),
          "expected_call_count":EXPECTED_COUNTS[f]
        }
    return {
      "workload":workload,
      "timing_blocks_total":len(bs),
      "decode_blocks_total":len(dec),
      "measured_decode_blocks_used":31,
      "probe_decode_indices":probes,
      "probes":selected,
      "median_profiled_total_us":median([x["total_us"] for x in selected]),
      "family_metrics":fams
    }

ap=argparse.ArgumentParser()
ap.add_argument("--ws-log",required=True);ap.add_argument("--wc-log",required=True);ap.add_argument("--out",required=True)
a=ap.parse_args()
try:
    ws=process(a.ws_log,"W-S"); wc=process(a.wc_log,"W-C")
    glob={}
    for f in EXPECTED_COUNTS:
        vals=[];shares=[]
        for w in [ws,wc]:
            for p in w["probes"]:
                vals.append(p["families"][f]["total_us"]);shares.append(p["families"][f]["share"])
        glob[f]={"median_us":median(vals),"median_share":median(shares),"samples":len(vals),"expected_call_count":EXPECTED_COUNTS[f]}
    out={
      "schema":"arcllm.v1.m2.pinned_llama_same_semantic_map.v0.1",
      "status":"PASS",
      "baseline":{"release":"v0.4.1","commit":"b29c606e28a01b1bc8c1351026a0fa6e616bf6c4","backend":"Vulkan","perf_logger":"GGML_VK_PERF_LOGGER=1"},
      "timing_semantics":"Vulkan query timestamps converted by llama backend using VkPhysicalDeviceLimits.timestampPeriod; logger instrumentation enabled but kernels/model/schedule source are unchanged.",
      "workloads":{"W-S":ws,"W-C":wc},
      "global_family_metrics":glob,
      "next":"BUILD_ARC_VS_LLAMA_EXCESS_COST_MAP"
    }
except Exception as e:
    out={"schema":"arcllm.v1.m2.pinned_llama_same_semantic_map.v0.1","status":"FAIL","error":str(e)}
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(out["status"])
if out["status"]!="PASS": raise SystemExit(2)
