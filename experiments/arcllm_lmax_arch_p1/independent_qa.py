#!/usr/bin/env python3
from __future__ import annotations
import array, hashlib, json, math, pathlib, re, statistics, sys
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[2]
E = ROOT / "results" / "arcllm_lmax_arch_p1_e_fresh_exploratory_one_shot"
OUT = ROOT / "results" / "arcllm_lmax_arch_p1_independent_qa"
PREREG = ROOT / "config" / "arcllm_lmax_arch_p1_preregistration_v0.4.json"
LOCK = ROOT / "config" / "arcllm_lmax_arch_p1_execution_lock_v0.4.json"
AUTH = ROOT / "config" / "arcllm_lmax_arch_p1_e_execution_authorization_v0.4.json"
TERMINAL = OUT / "E_TERMINAL_RECEIPT.json"

CELLS = ["W1","W2","W3","W4","W5","W6"]
PAIR_ORDER = {
    0: ["CURRENT_DIRECT","LMAX_RING_P1"],
    1: ["LMAX_RING_P1","CURRENT_DIRECT"],
    2: ["CURRENT_DIRECT","LMAX_RING_P1"],
    3: ["LMAX_RING_P1","CURRENT_DIRECT"],
}
EXPECTED_CONTROL = [
    ("0ns_direct_locked_spsc64", 0, "DIRECT_LOCKED_SPSC64"),
    ("0ns_lmax_ring_p1", 0, "LMAX_RING_P1"),
    ("10000ns_lmax_ring_p1", 10000, "LMAX_RING_P1"),
    ("10000ns_direct_locked_spsc64", 10000, "DIRECT_LOCKED_SPSC64"),
    ("100000ns_direct_locked_spsc64", 100000, "DIRECT_LOCKED_SPSC64"),
    ("100000ns_lmax_ring_p1", 100000, "LMAX_RING_P1"),
]

def sha256(path: pathlib.Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(8*1024*1024), b""): h.update(b)
    return h.hexdigest().upper()

def q7(values, p):
    xs=sorted(values)
    if not xs: return None
    if len(xs)==1: return float(xs[0])
    h=(len(xs)-1)*p
    lo=math.floor(h); hi=math.ceil(h)
    if lo==hi: return float(xs[lo])
    return float(xs[lo] + (h-lo)*(xs[hi]-xs[lo]))

def pct_lower_better(a,b):
    return None if not a else (a-b)/a*100.0

def pct_higher_better(a,b):
    return None if not a else (b-a)/a*100.0

def pct_regression_lower_better(a,b):
    return None if not a else (b-a)/a*100.0

def pct_regression_higher_better(a,b):
    return None if not a else (a-b)/a*100.0

def require(cond,msg,failures):
    if not cond: failures.append(msg)

def recompute_record(d):
    rp=d["raw_primitives"]
    ready=[int(x) for x in d["token_ready_ns"]]
    start=int(rp["request_start_ns"]); end=int(rp["request_end_ns"])
    first=int(rp["first_token_ready_ns"])
    wall=end-start
    ttft=first-start if ready else 0
    itl=[ready[i]-ready[i-1] for i in range(1,len(ready))]
    if len(ready)>1 and ready[-1]>ready[0]:
        tps=(len(ready)-1)/((ready[-1]-ready[0])/1e9)
    else:
        tps=0.0
    cpu100=int(rp["cpu_end_100ns"])-int(rp["cpu_start_100ns"])
    lpc=int(rp["logical_processor_count"])
    cpu=(cpu100*100.0)/(wall*lpc)*100.0 if wall and lpc else 0.0
    alloc_req=int(rp["request_allocation_counter_end"])-int(rp["request_allocation_counter_start"])
    alloc_dec=int(rp["decode_allocation_counter_end"])-int(rp["decode_allocation_counter_start"])
    return {
        "ttft_ns":ttft,"e2e_ns":wall,"itl_ns":itl,
        "decode_tokens_per_second":tps,"cpu_utilization_percent":cpu,
        "allocation_count_request":alloc_req,"allocation_count_decode_window":alloc_dec,
    }

def parse_malformed_manifest(text):
    def num(name, cast=int):
        m=re.search(r'"'+re.escape(name)+r'"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?)',text)
        if not m: return None
        return cast(m.group(1))
    def string(name):
        m=re.search(r'"'+re.escape(name)+r'"\s*:\s*"([^"\r\n]*)"',text)
        return m.group(1) if m else None
    return {
      "schema":string("schema"),"arm":string("arm"),
      "service_delay_ns":num("service_delay_ns"),
      "event_count":num("event_count"),
      "steady_state_allocation_count":num("steady_state_allocation_count"),
      "max_occupancy":num("max_occupancy"),
      "full_backpressure_observations":num("full_backpressure_observations"),
      "full_backpressure_observation_rate":num("full_backpressure_observation_rate",float),
      "spin_count":num("spin_count"),
      "switch_to_thread_count":num("switch_to_thread_count"),
      "wait_on_address_count":num("wait_on_address_count"),
      "producer_condition_variable_wait_count":num("producer_condition_variable_wait_count"),
      "consumer_condition_variable_wait_count":num("consumer_condition_variable_wait_count"),
      "wake_count":num("wake_count"),
      "service_delay_loop_iterations":num("service_delay_loop_iterations"),
      "first_ordering_mismatch":num("first_ordering_mismatch"),
    }

def load_u64le(path):
    a=array.array("Q")
    with path.open("rb") as f:
        a.fromfile(f, path.stat().st_size//8)
    if sys.byteorder!="little": a.byteswap()
    return a

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    failures=[]
    prereg=json.loads(PREREG.read_text(encoding="utf-8"))
    lock=json.loads(LOCK.read_text(encoding="utf-8"))
    auth=json.loads(AUTH.read_text(encoding="utf-8"))
    terminal=json.loads(TERMINAL.read_text(encoding="utf-8"))

    require(prereg["schema"]=="arcllm.lmax_arch_p1.preregistration.v0.4","prereg schema drift",failures)
    require(lock["status"]=="EXECUTION_LOCKED","execution lock invalid",failures)
    require(auth["decision"]=="ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED","authorization invalid",failures)
    require(terminal["job_state"]=="FAILED","terminal job state not FAILED",failures)
    require(terminal["proxy_job_id"]=="rjob_d1b02647d2ec23874710ce343af1d981","unexpected E job id",failures)
    require("JSONDecodeError" in terminal["stderr_signature"],"terminal failure signature mismatch",failures)

    pre=json.loads((E/"PRELAUNCH_IMMEDIATE.json").read_text(encoding="utf-8"))
    started=json.loads((E/"E_STARTED.json").read_text(encoding="utf-8"))
    progress=json.loads((E/"E_PROGRESS.json").read_text(encoding="utf-8"))
    require(pre.get("pass") is True,"prelaunch did not pass",failures)
    require(started.get("rerun_allowed") is False,"E_STARTED rerun policy drift",failures)
    require(progress.get("warmups_completed")==4,"warmup count !=4",failures)
    require(progress.get("measured_completed")==48,"measured count !=48",failures)

    auth_sha=sha256(AUTH)
    expected_identity={
      "runner_sha256":lock["runner"]["sha256"],
      "model_sha256":lock["model"]["sha256"],
      "shader_manifest_sha256":lock["shader_manifest"]["sha256"],
      "authorization_sha256":auth_sha,
      "evidence_contract_sha256":lock["evidence_contract_sha256"],
    }

    files=sorted((E/"inference"/"measured").glob("W*/pair_*_*.json"))
    require(len(files)==48,f"measured file count {len(files)} !=48",failures)
    records=[]
    metric_mismatch=[]
    schema_missing=[]
    identity_mismatch=[]
    for p in files:
        d=json.loads(p.read_text(encoding="utf-8"))
        r=recompute_record(d)
        if d.get("schema")!="arcllm.lmax_arch_p1.inference_observation.v0.4":
            schema_missing.append(str(p.relative_to(E)))
        if d.get("success") is not True or d.get("error")!="":
            failures.append(f"unsuccessful inference record {p.name}")
        if d.get("runtime_stats",{}).get("finite") is not True:
            failures.append(f"non-finite runtime record {p.name}")
        if len(d.get("generated_token_ids",[]))!=d.get("generated_token_count"):
            failures.append(f"generated token count mismatch {p.name}")
        if d.get("raw_primitives",{}).get("first_token_ready_ns") != (d.get("token_ready_ns") or [0])[0]:
            failures.append(f"first-token primitive mismatch {p.name}")
        calc_itl=r["itl_ns"]
        if calc_itl!=d.get("itl_ns"):
            failures.append(f"ITL raw recomputation mismatch {p.name}")
        for k in ("ttft_ns","e2e_ns","allocation_count_request","allocation_count_decode_window"):
            if int(d["metrics"][k])!=int(r[k]):
                metric_mismatch.append({"file":p.name,"metric":k,"stored":d["metrics"][k],"recomputed":r[k]})
        for k in ("decode_tokens_per_second","cpu_utilization_percent"):
            a=float(d["metrics"][k]); b=float(r[k])
            if abs(a-b)>max(1e-9,abs(b)*1e-12):
                metric_mismatch.append({"file":p.name,"metric":k,"stored":a,"recomputed":b})
        for k,v in expected_identity.items():
            if str(d.get("identity",{}).get(k,"")).upper()!=str(v).upper():
                identity_mismatch.append({"file":p.name,"field":k,"observed":d.get("identity",{}).get(k),"expected":v})
        arm=d["arm"]
        if arm=="CURRENT_DIRECT":
            if d.get("ring") is not None or d.get("completion_wait") is not None or d.get("consumer_ready_wait") is not None:
                failures.append(f"Arm A P1 wait fields not null {p.name}")
        elif arm=="LMAX_RING_P1":
            for sec in ("ring","completion_wait","consumer_ready_wait"):
                if not isinstance(d.get(sec),dict): failures.append(f"Arm B missing {sec} {p.name}")
        else:
            failures.append(f"unknown arm {arm} {p.name}")
        records.append((p,d,r))

    require(not schema_missing,f"schema mismatches: {schema_missing}",failures)
    require(not metric_mismatch,f"stored/recomputed metric mismatches: {len(metric_mismatch)}",failures)
    require(not identity_mismatch,f"identity mismatches: {len(identity_mismatch)}",failures)

    bykey={(d["cell"],int(d["pair_index"]),d["arm"]):(p,d,r) for p,d,r in records}
    semantic_mismatches=[]
    schedule_mismatches=[]
    for cell in CELLS:
        for pair in range(4):
            expected=PAIR_ORDER[pair]
            pair_records=[x for x in records if x[1]["cell"]==cell and int(x[1]["pair_index"])==pair]
            require(len(pair_records)==2,f"{cell} pair {pair} count !=2",failures)
            ordered=sorted(pair_records,key=lambda x:int(x[1]["pair_position"]))
            if [x[1]["arm"] for x in ordered] != expected:
                schedule_mismatches.append({"cell":cell,"pair":pair,"observed":[x[1]["arm"] for x in ordered],"expected":expected})
            a=bykey.get((cell,pair,"CURRENT_DIRECT"))
            b=bykey.get((cell,pair,"LMAX_RING_P1"))
            if not a or not b:
                semantic_mismatches.append({"cell":cell,"pair":pair,"reason":"missing arm"})
                continue
            ad=a[1]; bd=b[1]
            reasons=[]
            if ad["generated_token_ids"]!=bd["generated_token_ids"]: reasons.append("generated_token_ids")
            if ad["runtime_stats"]!=bd["runtime_stats"]: reasons.append("runtime_stats")
            if not ad.get("semantic_trace_consistent") or not bd.get("semantic_trace_consistent"): reasons.append("semantic_trace_consistent")
            if reasons: semantic_mismatches.append({"cell":cell,"pair":pair,"reasons":reasons})
    require(not schedule_mismatches,f"schedule mismatches: {schedule_mismatches}",failures)

    agg={}
    for cell in CELLS:
        agg[cell]={}
        for arm in ("CURRENT_DIRECT","LMAX_RING_P1"):
            rs=[r for _,d,r in records if d["cell"]==cell and d["arm"]==arm]
            itls=[x for r in rs for x in r["itl_ns"]]
            agg[cell][arm]={
              "ttft_ns":{"p50":q7([r["ttft_ns"] for r in rs],.5),"p95":q7([r["ttft_ns"] for r in rs],.95)},
              "e2e_ns":{"p50":q7([r["e2e_ns"] for r in rs],.5),"p95":q7([r["e2e_ns"] for r in rs],.95)},
              "decode_tokens_per_second":{"p50":q7([r["decode_tokens_per_second"] for r in rs],.5),"p95":q7([r["decode_tokens_per_second"] for r in rs],.95)},
              "cpu_utilization_percent":{"p50":q7([r["cpu_utilization_percent"] for r in rs],.5),"p95":q7([r["cpu_utilization_percent"] for r in rs],.95)},
              "itl_ns":{"p50":q7(itls,.5),"p95":q7(itls,.95)},
              "allocation_count_request":{"p50":q7([r["allocation_count_request"] for r in rs],.5),"p95":q7([r["allocation_count_request"] for r in rs],.95)},
              "allocation_count_decode_window":{"p50":q7([r["allocation_count_decode_window"] for r in rs],.5),"p95":q7([r["allocation_count_decode_window"] for r in rs],.95)},
              "run_count":len(rs),"itl_count":len(itls),
            }

    comparisons={}
    e2e_candidates=[]
    guard_violations=[]
    regression_cells=[]
    for cell in CELLS:
        A=agg[cell]["CURRENT_DIRECT"]; B=agg[cell]["LMAX_RING_P1"]
        c={}
        for endpoint in ("ttft_ns","e2e_ns"):
            c[endpoint]={
              "p50_improvement_percent":pct_lower_better(A[endpoint]["p50"],B[endpoint]["p50"]),
              "p95_improvement_percent":pct_lower_better(A[endpoint]["p95"],B[endpoint]["p95"]),
            }
            if c[endpoint]["p50_improvement_percent"]>=5 and c[endpoint]["p95_improvement_percent"]>=5:
                e2e_candidates.append({"cell":cell,"endpoint":endpoint,**c[endpoint]})
        endpoint="decode_tokens_per_second"
        c[endpoint]={
          "p50_improvement_percent":pct_higher_better(A[endpoint]["p50"],B[endpoint]["p50"]),
          "p95_improvement_percent":pct_higher_better(A[endpoint]["p95"],B[endpoint]["p95"]),
        }
        if c[endpoint]["p50_improvement_percent"]>=5 and c[endpoint]["p95_improvement_percent"]>=5:
            e2e_candidates.append({"cell":cell,"endpoint":endpoint,**c[endpoint]})
        c["guards"]={
          "ttft_p95_regression_percent":pct_regression_lower_better(A["ttft_ns"]["p95"],B["ttft_ns"]["p95"]),
          "itl_p95_regression_percent":pct_regression_lower_better(A["itl_ns"]["p95"],B["itl_ns"]["p95"]),
          "decode_tps_p50_regression_percent":pct_regression_higher_better(A["decode_tokens_per_second"]["p50"],B["decode_tokens_per_second"]["p50"]),
          "cpu_p50_regression_percent":pct_regression_lower_better(A["cpu_utilization_percent"]["p50"],B["cpu_utilization_percent"]["p50"]),
          "cpu_p95_regression_percent":pct_regression_lower_better(A["cpu_utilization_percent"]["p95"],B["cpu_utilization_percent"]["p95"]),
        }
        for name,val in c["guards"].items():
            lim=5 if name.startswith("cpu_") else 3
            if val is not None and val>lim: guard_violations.append({"cell":cell,"guard":name,"value_percent":val,"limit_percent":lim})
        regression_hit=(
          c["guards"]["ttft_p95_regression_percent"]>10 or
          c["guards"]["itl_p95_regression_percent"]>10 or
          c["guards"]["decode_tps_p50_regression_percent"]>10
        )
        if regression_hit: regression_cells.append(cell)
        comparisons[cell]=c

    inference_regression_candidate=len(regression_cells)>=4
    inference_e2e_candidate=bool(e2e_candidates) and not guard_violations

    control={}
    present_control=[]
    missing_control=[]
    for dirname,delay,arm in EXPECTED_CONTROL:
        ddir=E/"control"/dirname
        if not ddir.is_dir():
            missing_control.append(dirname); continue
        present_control.append(dirname)
        manifest=ddir/"manifest.json"; seqp=ddir/"consumed_sequence_ids.u64le"; latp=ddir/"publish_to_consume_latency_ns.u64le"
        require(manifest.is_file(),f"missing manifest {dirname}",failures)
        require(seqp.is_file(),f"missing sequence stream {dirname}",failures)
        require(latp.is_file(),f"missing latency stream {dirname}",failures)
        if not (manifest.is_file() and seqp.is_file() and latp.is_file()): continue
        text=manifest.read_text(encoding="utf-8")
        standard_json_valid=True
        try: parsed=json.loads(text)
        except json.JSONDecodeError:
            standard_json_valid=False
            parsed=parse_malformed_manifest(text)
        seq_bytes=seqp.stat().st_size; lat_bytes=latp.stat().st_size
        require(seq_bytes==8000000,f"sequence byte length {dirname}={seq_bytes}",failures)
        require(lat_bytes==8000000,f"latency byte length {dirname}={lat_bytes}",failures)
        seq=load_u64le(seqp)
        first_bad=None
        if len(seq)!=1000000:
            first_bad=-2
        else:
            for i,v in enumerate(seq):
                if v!=i: first_bad=i; break
        lat=load_u64le(latp)
        control[dirname]={
          "expected_delay_ns":delay,"expected_arm":arm,
          "standard_json_valid":standard_json_valid,
          "manifest_sha256":sha256(manifest),
          "manifest_recovered_fields":parsed,
          "sequence_sha256":sha256(seqp),"sequence_bytes":seq_bytes,
          "latency_sha256":sha256(latp),"latency_bytes":lat_bytes,
          "sequence_entries":len(seq),"latency_entries":len(lat),
          "first_ordering_mismatch_recomputed":first_bad,
          "latency_ns_type7":{"p50":q7(lat,.5),"p95":q7(lat,.95)},
        }
        require(first_bad is None,f"control order mismatch {dirname}: {first_bad}",failures)
        require(parsed.get("event_count")==1000000,f"control event count mismatch {dirname}",failures)
        require(parsed.get("arm")==arm,f"control arm mismatch {dirname}",failures)
        require(parsed.get("service_delay_ns")==delay,f"control delay mismatch {dirname}",failures)

    control_complete=(len(present_control)==6 and not missing_control)
    e_complete=(E/"E_COMPLETE.json").exists()
    e_aborted=(E/"E_ABORTED.json").exists()

    evidence_valid_complete=(
      len(files)==48 and not semantic_mismatches and not schedule_mismatches
      and control_complete and e_complete and not failures
    )

    if semantic_mismatches:
        formal="SEMANTICS_FAIL"
        reason="Pairwise canonical inference semantics mismatch detected."
    elif not evidence_valid_complete:
        formal="UNRESOLVED"
        reason="Mandatory E evidence is incomplete/invalid: only %d/6 control runs materialized, E_COMPLETE absent, and the sole E job terminated FAILED after a JSON manifest parse error. Anti-rescue rules prohibit filling the missing five control runs post hoc." % len(present_control)
    elif inference_regression_candidate:
        formal="REGRESSION"
        reason="Frozen regression rule met."
    elif inference_e2e_candidate:
        formal="E2E_SUPPORTED"
        reason="Frozen E2E benefit rule met with all guards."
    else:
        # Control-path-only is only reachable with complete six-run control evidence.
        # If complete, evaluate all three delay cells pairwise here.
        qualifying=0
        for delay in (0,10000,100000):
            adir=f"{delay}ns_direct_locked_spsc64"; bdir=f"{delay}ns_lmax_ring_p1"
            A=control[adir]; B=control[bdir]
            am=A["manifest_recovered_fields"]; bm=B["manifest_recovered_fields"]
            allocA=am["steady_state_allocation_count"]; allocB=bm["steady_state_allocation_count"]
            alloc_imp=((allocA-allocB)/allocA*100.0) if allocA else (100.0 if allocB<allocA else 0.0)
            lat_imp=pct_lower_better(A["latency_ns_type7"]["p95"],B["latency_ns_type7"]["p95"])
            back_ok=bm["full_backpressure_observation_rate"]<=am["full_backpressure_observation_rate"]
            if (alloc_imp>=20 or lat_imp>=10) and back_ok: qualifying+=1
        if qualifying>=2:
            formal="CONTROL_PATH_ONLY"; reason="Frozen control-path-only rule met."
        else:
            formal="NO_SUPPORTED_BENEFIT"; reason="Complete valid E did not meet positive or regression rules."

    result={
      "schema":"arcllm.lmax_arch_p1.independent_qa_result.v0.4",
      "qa_execution_status":"PASS",
      "formal_verdict":formal,
      "formal_reason":reason,
      "evidence_valid_complete":evidence_valid_complete,
      "M_opened":formal in ("E2E_SUPPORTED","CONTROL_PATH_ONLY"),
      "C_opened":False,
      "anti_rescue_enforced":True,
      "terminal_receipt":terminal,
      "e_markers":{
        "prelaunch_sha256":sha256(E/"PRELAUNCH_IMMEDIATE.json"),
        "e_started_sha256":sha256(E/"E_STARTED.json"),
        "e_progress_sha256":sha256(E/"E_PROGRESS.json"),
        "e_complete_present":e_complete,
        "e_aborted_present":e_aborted,
      },
      "inference":{
        "measured_records":len(files),
        "semantic_mismatches":semantic_mismatches,
        "schedule_mismatches":schedule_mismatches,
        "stored_metric_mismatches":metric_mismatch,
        "identity_mismatches":identity_mismatch,
        "type7_aggregates":agg,
        "comparisons":comparisons,
        "diagnostic_e2e_candidates":e2e_candidates,
        "guard_violations":guard_violations,
        "regression_cells_gt10pct":regression_cells,
        "diagnostic_inference_regression_candidate":inference_regression_candidate,
        "diagnostic_inference_e2e_candidate":inference_e2e_candidate,
      },
      "control":{
        "required_runs":6,
        "present_runs":present_control,
        "missing_runs":missing_control,
        "complete":control_complete,
        "runs":control,
      },
      "qa_failures":failures,
      "contract":{
        "prereg_sha256":sha256(PREREG),
        "lock_sha256":sha256(LOCK),
        "authorization_sha256":auth_sha,
        "mandatory_control_inside_E":prereg["control_path_subtest"]["mandatory_inside_E"],
        "posthoc_control_subtest_allowed":prereg["anti_rescue"]["posthoc_control_subtest_allowed"],
        "selective_rerun_allowed":prereg["anti_rescue"]["selective_rerun_allowed"],
      }
    }
    out=OUT/"INDEPENDENT_QA_RESULT.json"
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("ARCLLM_LMAX_ARCH_P1_INDEPENDENT_QA=PASS")
    print("FORMAL_VERDICT="+formal)
    print("MEASURED_RECORDS="+str(len(files)))
    print("SEMANTIC_MISMATCHES="+str(len(semantic_mismatches)))
    print("CONTROL_PRESENT="+str(len(present_control))+"/6")
    print("EVIDENCE_VALID_COMPLETE="+str(evidence_valid_complete).lower())
    print("M_OPENED="+str(result["M_opened"]).lower())
    print("RESULT_SHA256="+sha256(out))
    return 0

if __name__=="__main__":
    try: raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_INDEPENDENT_QA=FAIL: "+repr(exc),file=sys.stderr)
        raise
