#!/usr/bin/env python3
from __future__ import annotations
import argparse, ctypes, datetime, hashlib, json, os, pathlib, shutil, subprocess, sys

try:
    import psutil
except Exception:
    psutil = None

CELLS=["W1","W2","W3","W4","W5","W6"]
PAIR_ORDERS=[["direct","ring"],["ring","direct"],["direct","ring"],["ring","direct"]]
CONTROL=[
    (0,"direct_locked_spsc64"),(0,"lmax_ring_p1"),
    (10000,"lmax_ring_p1"),(10000,"direct_locked_spsc64"),
    (100000,"direct_locked_spsc64"),(100000,"lmax_ring_p1"),
]
MODEL=pathlib.Path(r"C:\Users\minht\.ollama\models\blobs\sha256-60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463")
SHADER_DIR=pathlib.Path(r"D:\WORK\RESEARCH\6.LTR\ArcLLM\compiled_shaders")
SIDECAR=pathlib.Path(r"D:\WORK\RESEARCH\_arcllm_v4_gate_clean\results\canonical_exec148.sidecar")
DEFAULT_OLLAMA=pathlib.Path(r"C:\Users\minht\AppData\Local\Programs\Ollama\ollama.exe")
U2_OLLAMA=pathlib.Path(r"D:\WORK\RESEARCH\2.CQG-UTS-U2\.local\ollama-v0.34.2\portable\ollama.exe")

def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def sha256(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for b in iter(lambda:f.read(8*1024*1024),b""): h.update(b)
    return h.hexdigest().upper()

def atomic_json(path,obj):
    path.parent.mkdir(parents=True,exist_ok=True)
    tmp=path.with_suffix(path.suffix+".tmp")
    tmp.write_text(json.dumps(obj,indent=2)+"\n",encoding="utf-8")
    os.replace(tmp,path)

def ollama_ps(exe,host=None):
    env=os.environ.copy()
    if host: env["OLLAMA_HOST"]=host
    cp=subprocess.run([str(exe),"ps"],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
    rows=[x.strip() for x in cp.stdout.splitlines() if x.strip()]
    return {"rc":cp.returncode,"rows":rows,"idle":cp.returncode==0 and len(rows)<=1}

def memory_state():
    class M(ctypes.Structure):
        _fields_=[("dwLength",ctypes.c_ulong),("dwMemoryLoad",ctypes.c_ulong),
        ("ullTotalPhys",ctypes.c_ulonglong),("ullAvailPhys",ctypes.c_ulonglong),
        ("ullTotalPageFile",ctypes.c_ulonglong),("ullAvailPageFile",ctypes.c_ulonglong),
        ("ullTotalVirtual",ctypes.c_ulonglong),("ullAvailVirtual",ctypes.c_ulonglong),
        ("ullAvailExtendedVirtual",ctypes.c_ulonglong)]
    m=M();m.dwLength=ctypes.sizeof(M);ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return {"total_physical_bytes":m.ullTotalPhys,"available_physical_bytes":m.ullAvailPhys,"memory_load_percent":m.dwMemoryLoad}

def power_state():
    class P(ctypes.Structure):
        _fields_=[("ACLineStatus",ctypes.c_byte),("BatteryFlag",ctypes.c_byte),
        ("BatteryLifePercent",ctypes.c_byte),("SystemStatusFlag",ctypes.c_byte),
        ("BatteryLifeTime",ctypes.c_ulong),("BatteryFullLifeTime",ctypes.c_ulong)]
    p=P();ctypes.windll.kernel32.GetSystemPowerStatus(ctypes.byref(p))
    scheme=subprocess.run(["powercfg","/getactivescheme"],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout.strip()
    return {"ac":int(p.ACLineStatus),"battery":int(p.BatteryLifePercent),"scheme":scheme}

def process_state(runner_name,control_name):
    z={"p1_runner":[],"p1_control":[],"other_arcllm_executables":[],"loaded_model_servers":[],"current_priority":None}
    if psutil is None:
        z["psutil_unavailable"]=True
        return z
    try:z["current_priority"]=int(psutil.Process().nice())
    except Exception:pass
    for p in psutil.process_iter(["pid","name","cmdline"]):
        try:
            n=(p.info["name"] or "").lower()
            cmd=" ".join(p.info["cmdline"] or [])
            if runner_name.lower() in n:z["p1_runner"].append({"pid":p.info["pid"],"cmd":cmd[:1000]})
            if control_name.lower() in n:z["p1_control"].append({"pid":p.info["pid"],"cmd":cmd[:1000]})
            if n.endswith(".exe") and "arcllm" in n and runner_name.lower() not in n and control_name.lower() not in n:
                z["other_arcllm_executables"].append({"pid":p.info["pid"],"cmd":cmd[:1000]})
            if "llama-server" in n and "-ngl 0" not in cmd:
                z["loaded_model_servers"].append({"pid":p.info["pid"],"cmd":cmd[:1000]})
        except Exception:pass
    return z

def run_checked(argv,log_path):
    cp=subprocess.run([str(x) for x in argv],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
    log_path.parent.mkdir(parents=True,exist_ok=True)
    log_path.write_text(cp.stdout,encoding="utf-8")
    return cp

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--lock",required=True)
    ap.add_argument("--authorization",required=True)
    ap.add_argument("--out",required=True)
    args=ap.parse_args()

    root=pathlib.Path(__file__).resolve().parents[2]
    lock_path=pathlib.Path(args.lock).resolve()
    auth_path=pathlib.Path(args.authorization).resolve()
    out=pathlib.Path(args.out).resolve()
    lock=json.loads(lock_path.read_text(encoding="utf-8"))
    auth=json.loads(auth_path.read_text(encoding="utf-8"))

    if auth.get("decision")!="ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED" or auth.get("authorized") is not True:
        raise RuntimeError("P1 E authorization missing")
    if lock.get("status")!="EXECUTION_LOCKED" or lock.get("science_execution_authorized") is not False:
        raise RuntimeError("invalid P1 execution lock")
    if out.exists() and any(out.iterdir()):
        raise RuntimeError("fresh E output namespace is not pristine")
    out.mkdir(parents=True,exist_ok=True)

    runner=root/lock["runner"]["path"]
    control=root/lock["control"]["path"]
    runtime_assets_manifest=(root/lock["runtime_assets_manifest"]["path"]).resolve()
    shader_manifest=(root/lock["shader_manifest"]["path"]).resolve()
    runtime_assets=json.loads(runtime_assets_manifest.read_text(encoding="utf-8"))
    shaders=json.loads(shader_manifest.read_text(encoding="utf-8"))
    shader_hashes={}
    for name,expected in shaders["shaders"].items():
        p=SHADER_DIR/name
        shader_hashes[name]={"sha256":sha256(p) if p.is_file() else None,"expected":expected}

    checks={
      "lock_sha256":sha256(lock_path),
      "authorization_sha256":sha256(auth_path),
      "runtime_assets_manifest":{"path":str(runtime_assets_manifest),"sha256":sha256(runtime_assets_manifest)},
      "shader_manifest":{"path":str(shader_manifest),"sha256":sha256(shader_manifest)},
      "shader_hashes":shader_hashes,
      "runner":{"path":str(runner),"sha256":sha256(runner),"bytes":runner.stat().st_size},
      "control":{"path":str(control),"sha256":sha256(control),"bytes":control.stat().st_size},
      "model":{"path":str(MODEL),"sha256":sha256(MODEL),"bytes":MODEL.stat().st_size},
      "sidecar":{"path":str(SIDECAR),"sha256":sha256(SIDECAR),"bytes":SIDECAR.stat().st_size},
      "ollama_default":ollama_ps(DEFAULT_OLLAMA),
      "ollama_u2":ollama_ps(U2_OLLAMA,"127.0.0.1:11435"),
      "memory":memory_state(),
      "power":power_state(),
      "disk_free_bytes":shutil.disk_usage(str(root)).free,
    }
    checks["processes"]=process_state(runner.name,control.name)
    gpu=subprocess.run(["powershell.exe","-NoProfile","-Command",
      "Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion | ConvertTo-Json -Compress"],
      text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    checks["gpu"]={"raw":gpu.stdout.strip(),"arc140v":("Arc(TM) 140V" in gpu.stdout or "Arc 140V" in gpu.stdout)}
    conditions={
      "runner_hash":checks["runner"]["sha256"]==lock["runner"]["sha256"] and checks["runner"]["bytes"]==lock["runner"]["bytes"],
      "control_hash":checks["control"]["sha256"]==lock["control"]["sha256"] and checks["control"]["bytes"]==lock["control"]["bytes"],
      "model_hash":checks["model"]["sha256"]==lock["model"]["sha256"] and checks["model"]["bytes"]==lock["model"]["bytes"],
      "sidecar_hash":checks["sidecar"]["sha256"]==lock["sidecar"]["sha256"] and checks["sidecar"]["bytes"]==lock["sidecar"]["bytes"],
      "runtime_assets_manifest_hash":checks["runtime_assets_manifest"]["sha256"]==lock["runtime_assets_manifest"]["sha256"],
      "shader_manifest_hash":checks["shader_manifest"]["sha256"]==lock["shader_manifest"]["sha256"],
      "shader_hashes":all(v["sha256"]==v["expected"] for v in checks["shader_hashes"].values()),
      "default_ollama_idle":checks["ollama_default"]["idle"],
      "u2_ollama_idle":checks["ollama_u2"]["idle"],
      "no_competing_loaded_model_server":not checks["processes"]["loaded_model_servers"],
      "no_p1_runner":not checks["processes"]["p1_runner"],
      "no_p1_control":not checks["processes"]["p1_control"],
      "no_other_arcllm_executable":not checks["processes"]["other_arcllm_executables"],
      "arc140v":checks["gpu"]["arc140v"],
      "ac_online":checks["power"]["ac"]==1,
      "ram_floor":checks["memory"]["available_physical_bytes"]>=lock["resource_minimums"]["available_ram_bytes"],
      "disk_floor":checks["disk_free_bytes"]>=lock["resource_minimums"]["disk_free_bytes"],
    }
    atomic_json(out/"PRELAUNCH_IMMEDIATE.json",{"schema":"arcllm.lmax_arch_p1.prelaunch.v0.4","timestamp_utc":now(),"pass":all(conditions.values()),"conditions":conditions,"checks":checks,"fresh_outcome_started":False})
    if not all(conditions.values()):
        print("ARCLLM_LMAX_ARCH_P1_E=BLOCKED_PRELAUNCH",flush=True)
        return 4

    atomic_json(out/"E_STARTED.json",{
      "schema":"arcllm.lmax_arch_p1.e_started.v0.4","started_at_utc":now(),
      "rerun_allowed":False,"tuning_allowed":False,
      "planned_excluded_warmups":4,"planned_measured_inference_runs":48,
      "planned_control_runs":6,"control_events_per_run":1000000,
      "inference_pair_order":["A_B","B_A","A_B","B_A"],
      "control_order":[{"delay_ns":d,"arm":a} for d,a in CONTROL],
      "lock_sha256":checks["lock_sha256"],"authorization_sha256":checks["authorization_sha256"]
    })
    print("E_STARTED",flush=True)

    identity=[
      "--runner-sha256",lock["runner"]["sha256"],
      "--model-sha256",lock["model"]["sha256"],
      "--shader-manifest-sha256",lock["shader_manifest"]["sha256"],
      "--authorization-sha256",checks["authorization_sha256"],
      "--evidence-contract-sha256",lock["evidence_contract_sha256"],
    ]
    records=[]; warmups=0; measured=0; controls=0

    inference_plan=[]
    for i,a in enumerate(["direct","ring","direct","ring"]):
        inference_plan.append({"phase":"warmup","index":i,"cell":"W1","arm":a})
    for c in CELLS:
        for pair,arms in enumerate(PAIR_ORDERS):
            for pos,a in enumerate(arms):
                inference_plan.append({"phase":"measured","cell":c,"pair":pair,"pos":pos,"arm":a})

    for item in inference_plan:
        if item["phase"]=="warmup":
            rel=pathlib.Path("inference")/"warmups"/("%02d_%s_%s.json"%(item["index"],item["cell"],item["arm"]))
        else:
            rel=pathlib.Path("inference")/"measured"/item["cell"]/("pair_%d_%d_%s.json"%(item["pair"],item["pos"],item["arm"]))
        op=out/rel; lp=op.with_suffix(".log.txt")
        op.parent.mkdir(parents=True,exist_ok=True)
        pair_index=item.get("pair",-1)
        pair_position=item.get("pos",-1)
        argv=[runner,"--authorization",auth_path,"--arm",item["arm"],
              "--phase",item["phase"],"--pair-index",str(pair_index),
              "--pair-position",str(pair_position),"--cell",item["cell"],
              "--model",MODEL,"--shader-dir",SHADER_DIR,"--sidecar",SIDECAR,"--out",op,*identity]
        cp=run_checked(argv,lp)
        rec={**item,"rc":cp.returncode,"out":str(rel).replace("\\","/"),"log":str(lp.relative_to(out)).replace("\\","/"),"finished_at_utc":now()}
        if op.is_file():rec["sha256"]=sha256(op)
        records.append(rec)
        if cp.returncode!=0:
            atomic_json(out/"E_ABORTED.json",{"status":"UNRESOLVED","failed_record":rec,"warmups_completed":warmups,"measured_completed":measured,"control_runs_completed":controls,"rerun_forbidden":True})
            print("E_ABORTED_UNRESOLVED",flush=True);return 2
        if item["phase"]=="warmup":warmups+=1
        else:measured+=1
        atomic_json(out/"E_PROGRESS.json",{"warmups_completed":warmups,"measured_completed":measured,"control_runs_completed":controls,"records":records})
        print("INFERENCE_DONE warmups=%d measured=%d"%(warmups,measured),flush=True)

    for delay,arm in CONTROL:
        stem="%dns_%s"%(delay,arm)
        ddir=out/"control"/stem; ddir.mkdir(parents=True,exist_ok=True)
        manifest=ddir/"manifest.json"; seq=ddir/"consumed_sequence_ids.u64le"; lat=ddir/"publish_to_consume_latency_ns.u64le"; log=ddir/"run.log.txt"
        cp=run_checked([control,"--authorization",auth_path,"--arm",arm,"--delay-ns",str(delay),
          "--events","1000000","--manifest",manifest,"--sequence-out",seq,"--latency-out",lat],log)
        rec={"phase":"control","delay_ns":delay,"arm":arm,"rc":cp.returncode,"manifest":str(manifest.relative_to(out)).replace("\\","/"),"finished_at_utc":now()}
        if cp.returncode!=0:
            records.append(rec)
            atomic_json(out/"E_ABORTED.json",{"status":"UNRESOLVED","failed_record":rec,"warmups_completed":warmups,"measured_completed":measured,"control_runs_completed":controls,"rerun_forbidden":True})
            print("E_ABORTED_UNRESOLVED",flush=True);return 2
        m=json.loads(manifest.read_text(encoding="utf-8"))
        m["consumed_sequence_ids_sha256"]=sha256(seq)
        m["consumed_sequence_ids_bytes"]=seq.stat().st_size
        m["publish_to_consume_latency_ns_sha256"]=sha256(lat)
        m["publish_to_consume_latency_ns_bytes"]=lat.stat().st_size
        m["expected_u64_entries"]=1000000
        m["expected_u64_bytes"]=8000000
        atomic_json(manifest,m)
        rec["manifest_sha256"]=sha256(manifest);rec["sequence_sha256"]=m["consumed_sequence_ids_sha256"];rec["latency_sha256"]=m["publish_to_consume_latency_ns_sha256"]
        records.append(rec);controls+=1
        atomic_json(out/"E_PROGRESS.json",{"warmups_completed":warmups,"measured_completed":measured,"control_runs_completed":controls,"records":records})
        print("CONTROL_DONE runs=%d delay_ns=%d arm=%s"%(controls,delay,arm),flush=True)

    raw={"schema":"arcllm.lmax_arch_p1.raw_evidence.v0.4","records":[]}
    for rec in records:
        x={"record":rec}
        if rec["phase"] in ("warmup","measured"):
            x["result"]=json.loads((out/rec["out"]).read_text(encoding="utf-8"))
        else:
            x["result"]=json.loads((out/rec["manifest"]).read_text(encoding="utf-8"))
        raw["records"].append(x)
    atomic_json(out/"RAW_EVIDENCE.json",raw)
    atomic_json(out/"E_COMPLETE.json",{
      "schema":"arcllm.lmax_arch_p1.e_complete.v0.4","status":"PASS_E_COLLECTION_COMPLETE",
      "completed_at_utc":now(),"excluded_warmups":warmups,"measured_inference_runs":measured,
      "matched_pairs":measured//2,"control_runs":controls,"control_events_total":controls*1000000,
      "raw_evidence_sha256":sha256(out/"RAW_EVIDENCE.json"),"adjudication_performed":False,
      "rerun_allowed":False
    })
    print("PASS_E_COLLECTION_COMPLETE",flush=True)
    return 0

if __name__=="__main__":
    try:raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_E_DRIVER_ERROR="+repr(exc),file=sys.stderr)
        raise
