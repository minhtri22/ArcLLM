import argparse,json,math,statistics
from pathlib import Path

WORKLOADS=["W-S","W-C"]
GROUPS=["memory_cache","execution_occupancy","stall_cause"]
FAMILIES=["lm_head_q6","ffn_down_q4","ffn_down_q6","split_k_q4_control"]
SEMANTIC_BYTES_PER_NODE={
    "lm_head_q6":447082496,
    "ffn_down_q4":38266880,
    "ffn_down_q6":55771136,
    "split_k_q4_control":38205440,
}
SEMANTIC_NODE_COUNTS={
    "lm_head_q6":1,
    "ffn_down_q4":14,
    "ffn_down_q6":14,
    "split_k_q4_control":56,
}
DENSE_FLOPS_PER_NODE={
    "lm_head_q6":1089994752,
    "ffn_down_q4":135790592,
    "ffn_down_q6":135790592,
    "split_k_q4_control":135790592,
}
COUNTER_GROUP={
    "AvgGpuCoreFrequencyMHz":"execution_occupancy",
    "GPGPU_THREADGROUP_COUNT":"execution_occupancy",
    "XVE_ACTIVE":"execution_occupancy",
    "XVE_THREADS_OCCUPANCY_ALL":"execution_occupancy",
    "XVE_STALL":"execution_occupancy",
    "THREADGROUP_DISPATCH_QUEUE0_RESOURCE_STALL":"execution_occupancy",
    "THREADGROUP_DISPATCH_QUEUE1_RESOURCE_STALL":"execution_occupancy",
    "XVE_INST_EXECUTED_ALU0_ALL":"execution_occupancy",
    "XVE_INST_EXECUTED_ALU1_ALL":"execution_occupancy",
    "XVE_INST_EXECUTED_SEND_ALL":"execution_occupancy",
    "GPU_MEMORY_BYTE_READ":"memory_cache",
    "GPU_MEMORY_BYTE_WRITE":"memory_cache",
    "GPU_MEMORY_ACTIVE":"memory_cache",
    "GPU_MEMORY_REQUEST_QUEUE_FULL":"memory_cache",
    "L3_HIT":"memory_cache",
    "L3_MISS":"memory_cache",
    "L3_STALL":"memory_cache",
    "LOAD_STORE_CACHE_HIT":"memory_cache",
    "LOAD_STORE_CACHE_ACCESS":"memory_cache",
    "XVE_STALL_ALUWR":"stall_cause",
    "XVE_STALL_BARRIER":"stall_cause",
    "XVE_STALL_CONTROL":"stall_cause",
    "XVE_STALL_INSTFETCH":"stall_cause",
    "XVE_STALL_OTHER":"stall_cause",
    "XVE_STALL_PIPESTALL":"stall_cause",
    "XVE_STALL_SBID":"stall_cause",
    "XVE_STALL_SENDWR":"stall_cause",
}
RATIO_HIGH=1.5
RATIO_LOW=0.67
PERCENT_DELTA=10.0
CLOCK_LOW=0.80
CLOCK_HIGH=1.25

def finite(x):
    return isinstance(x,(int,float)) and math.isfinite(float(x))

def med(xs):
    xs=[float(x) for x in xs if finite(x)]
    return statistics.median(xs) if xs else None

def safe_ratio(a,b):
    if a is None or b is None or float(b)==0.0:
        return None
    return float(a)/float(b)

def load_runs(root):
    runs={}
    for workload,stem in [("W-S","W_S"),("W-C","W_C")]:
        runs[workload]={}
        for group in GROUPS:
            p=root/f"M3C_{stem}_{group}.json"
            d=json.loads(p.read_text(encoding="utf-8"))
            if d.get("status")!="PASS":
                raise SystemExit(f"M3-C raw run not PASS: {p}")
            runs[workload][group]=d
    return runs

def family_dispatches(runs,workload,group,family):
    return [d for d in runs[workload][group]["dispatches"] if d.get("target_family")==family]

def counter_values(runs,workload,family,name):
    group=COUNTER_GROUP[name]
    out=[]
    for d in family_dispatches(runs,workload,group,family):
        by={x["name"]:x["value"] for x in d.get("counters",[])}
        if name in by and finite(by[name]):
            out.append(float(by[name]))
    return out

def counter_sum(runs,workload,family,name):
    xs=counter_values(runs,workload,family,name)
    return sum(xs) if xs else None

def counter_median(runs,workload,family,name):
    return med(counter_values(runs,workload,family,name))

def total_semantic_bytes(family):
    return SEMANTIC_BYTES_PER_NODE[family]*SEMANTIC_NODE_COUNTS[family]

def total_dense_flops(family):
    return DENSE_FLOPS_PER_NODE[family]*SEMANTIC_NODE_COUNTS[family]

def family_metrics(runs,workload,family):
    dram=counter_sum(runs,workload,family,"GPU_MEMORY_BYTE_READ")
    l3h=counter_sum(runs,workload,family,"L3_HIT")
    l3m=counter_sum(runs,workload,family,"L3_MISS")
    lsch=counter_sum(runs,workload,family,"LOAD_STORE_CACHE_HIT")
    lsca=counter_sum(runs,workload,family,"LOAD_STORE_CACHE_ACCESS")
    alu0=counter_sum(runs,workload,family,"XVE_INST_EXECUTED_ALU0_ALL")
    alu1=counter_sum(runs,workload,family,"XVE_INST_EXECUTED_ALU1_ALL")
    send=counter_sum(runs,workload,family,"XVE_INST_EXECUTED_SEND_ALL")
    instruction_total=None if None in (alu0,alu1,send) else alu0+alu1+send
    return {
        "dram_read_amplification": None if dram is None else dram/total_semantic_bytes(family),
        "dram_read_bytes_total":dram,
        "l3_miss_fraction":None if l3h is None or l3m is None or (l3h+l3m)==0 else l3m/(l3h+l3m),
        "lsc_hit_fraction":None if lsch is None or lsca is None or lsca==0 else lsch/lsca,
        "gpu_memory_active_percent":counter_median(runs,workload,family,"GPU_MEMORY_ACTIVE"),
        "memory_request_queue_full_percent":counter_median(runs,workload,family,"GPU_MEMORY_REQUEST_QUEUE_FULL"),
        "l3_stall_percent":counter_median(runs,workload,family,"L3_STALL"),
        "occupancy_percent":counter_median(runs,workload,family,"XVE_THREADS_OCCUPANCY_ALL"),
        "xve_active_percent":counter_median(runs,workload,family,"XVE_ACTIVE"),
        "xve_stall_percent":counter_median(runs,workload,family,"XVE_STALL"),
        "threadgroup_count_median":counter_median(runs,workload,family,"GPGPU_THREADGROUP_COUNT"),
        "resource_stall_percent":med(
            counter_values(runs,workload,family,"THREADGROUP_DISPATCH_QUEUE0_RESOURCE_STALL")+
            counter_values(runs,workload,family,"THREADGROUP_DISPATCH_QUEUE1_RESOURCE_STALL")
        ),
        "instruction_density_per_semantic_byte":None if instruction_total is None else instruction_total/total_semantic_bytes(family),
        "instruction_density_per_dense_flop":None if instruction_total is None else instruction_total/total_dense_flops(family),
        "avg_gpu_core_frequency_mhz":counter_median(runs,workload,family,"AvgGpuCoreFrequencyMHz"),
        "stall_causes":{
            k:counter_median(runs,workload,family,k) for k in [
                "XVE_STALL_ALUWR","XVE_STALL_BARRIER","XVE_STALL_CONTROL","XVE_STALL_INSTFETCH",
                "XVE_STALL_OTHER","XVE_STALL_PIPESTALL","XVE_STALL_SBID","XVE_STALL_SENDWR"
            ]
        }
    }

def all_high(pair_metrics,key):
    vals=[]
    for workload in WORKLOADS:
        a=pair_metrics[workload]["target"].get(key)
        b=pair_metrics[workload]["control"].get(key)
        r=safe_ratio(a,b); vals.append(r)
    return all(r is not None and r>=RATIO_HIGH for r in vals),vals

def all_low(pair_metrics,key):
    vals=[]
    for workload in WORKLOADS:
        a=pair_metrics[workload]["target"].get(key)
        b=pair_metrics[workload]["control"].get(key)
        r=safe_ratio(a,b); vals.append(r)
    return all(r is not None and r<=RATIO_LOW for r in vals),vals

def all_delta_high(pair_metrics,key):
    vals=[]
    for workload in WORKLOADS:
        a=pair_metrics[workload]["target"].get(key)
        b=pair_metrics[workload]["control"].get(key)
        d=None if a is None or b is None else float(a)-float(b); vals.append(d)
    return all(d is not None and d>=PERCENT_DELTA for d in vals),vals

def all_delta_low(pair_metrics,key):
    vals=[]
    for workload in WORKLOADS:
        a=pair_metrics[workload]["target"].get(key)
        b=pair_metrics[workload]["control"].get(key)
        d=None if a is None or b is None else float(a)-float(b); vals.append(d)
    return all(d is not None and d<=-PERCENT_DELTA for d in vals),vals

def clock_confound(pair_metrics):
    ratios=[]
    for workload in WORKLOADS:
        ratios.append(safe_ratio(
            pair_metrics[workload]["target"].get("avg_gpu_core_frequency_mhz"),
            pair_metrics[workload]["control"].get("avg_gpu_core_frequency_mhz")))
    return any(r is None or r<CLOCK_LOW or r>CLOCK_HIGH for r in ratios),ratios

def adjudicate_pair(name,target,control,all_metrics,geometry_rule=False):
    pm={w:{"target":all_metrics[w][target],"control":all_metrics[w][control]} for w in WORKLOADS}
    confounded,clock_ratios=clock_confound(pm)
    mem_amp,mem_amp_ratios=all_high(pm,"dram_read_amplification")
    mem_active,mem_active_delta=all_delta_high(pm,"gpu_memory_active_percent")
    mem_queue,mem_queue_delta=all_delta_high(pm,"memory_request_queue_full_percent")
    cache_miss,cache_miss_ratios=all_high(pm,"l3_miss_fraction")
    cache_hit_low,cache_hit_ratios=all_low(pm,"lsc_hit_fraction")
    occ_low,occ_ratios=all_low(pm,"occupancy_percent")
    active_low,active_ratios=all_low(pm,"xve_active_percent")
    resource_high,resource_delta=all_delta_high(pm,"resource_stall_percent")
    instruction_high,instruction_ratios=all_high(pm,"instruction_density_per_semantic_byte")
    tg_low,tg_ratios=all_low(pm,"threadgroup_count_median")

    def status(flag):
        if confounded:
            return "UNRESOLVED_CLOCK_CONFOUND"
        return "SUPPORTED_DESCRIPTIVE" if flag else "NOT_SUPPORTED_DESCRIPTIVE"

    geometry_flag=geometry_rule and tg_low and (occ_low or active_low or resource_high)
    memory_flag=mem_amp or (mem_active and mem_queue)
    cache_flag=cache_miss or cache_hit_low
    instruction_flag=instruction_high and not mem_amp
    result={
        "comparison":name,"target":target,"control":control,
        "clock_confound":confounded,"clock_ratio_target_over_control":clock_ratios,
        "mechanisms":{
            "EXECUTION_GEOMETRY_DEFICIT":{"status":status(geometry_flag),"signals":{"threadgroup_ratio":tg_ratios,"occupancy_ratio":occ_ratios,"xve_active_ratio":active_ratios,"resource_stall_delta_pp":resource_delta}},
            "MEMORY_TRAFFIC_EXCESS":{"status":status(memory_flag),"signals":{"dram_read_amplification_ratio":mem_amp_ratios,"gpu_memory_active_delta_pp":mem_active_delta,"memory_queue_full_delta_pp":mem_queue_delta}},
            "CACHE_REUSE_DEFICIT":{"status":status(cache_flag),"signals":{"l3_miss_fraction_ratio":cache_miss_ratios,"lsc_hit_fraction_ratio":cache_hit_ratios}},
            "INSTRUCTION_OR_DEQUANT_COST":{"status":status(instruction_flag),"signals":{"instruction_density_ratio":instruction_ratios,"dram_read_amplification_high":mem_amp}},
        },
        "metrics":pm,
        "interpretation_rule":"Descriptive hardware evidence only. No mechanism becomes causal without a later discriminating intervention."
    }
    return result

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--out",required=True)
a=ap.parse_args()
root=Path(a.results_dir)
runs=load_runs(root)
all_metrics={w:{f:family_metrics(runs,w,f) for f in FAMILIES} for w in WORKLOADS}
result={
    "schema":"arcllm.v1.m3c.mechanism_discrimination.v0.1",
    "status":"PASS",
    "study_role":"FROZEN_DESCRIPTIVE_MECHANISM_DISCRIMINATION",
    "thresholds":{"ratio_high":RATIO_HIGH,"ratio_low":RATIO_LOW,"percent_delta":PERCENT_DELTA,"clock_ratio_bounds":[CLOCK_LOW,CLOCK_HIGH]},
    "phase2_targets":{"primary":"lm_head_q6","secondary":["ffn_down_q4","ffn_down_q6"],"positive_control":"split_k_q4_control"},
    "comparisons":[
        adjudicate_pair("Q4_DOWN_VS_Q4_SPLIT_K","ffn_down_q4","split_k_q4_control",all_metrics,geometry_rule=True),
        adjudicate_pair("Q6_LM_HEAD_VS_Q6_DOWN","lm_head_q6","ffn_down_q6",all_metrics,geometry_rule=False),
        adjudicate_pair("Q6_DOWN_VS_Q4_SPLIT_K_GENERALIZATION","ffn_down_q6","split_k_q4_control",all_metrics,geometry_rule=False),
    ],
    "global_rules":[
        "Counter-run GpuTime is diagnostic only and is not used as the performance baseline.",
        "A descriptive mechanism status is not causal proof.",
        "Q4-down versus split-K Q4 is the primary geometry comparison because quantization is matched.",
        "LM-head Q6 versus Q6-down is the primary same-quantization comparison for memory/cache/instruction patterns.",
        "A later causal intervention is required before changing the ArcLLM architecture."
    ],
    "next":"IF_A_PATTERN_IS_SUPPORTED_DESCRIPTIVE_PREREGISTER_ONE_MINIMAL_CAUSAL_INTERVENTION_ELSE_STOP_UNRESOLVED"
}
Path(a.out).write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print("M3_C_MECHANISM_DISCRIMINATION=PASS")
