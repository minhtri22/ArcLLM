#!/usr/bin/env python3
from __future__ import annotations
import ctypes, hashlib, json, os, pathlib, shutil, subprocess, sys
try: import psutil
except Exception: psutil=None

ROOT=pathlib.Path(__file__).resolve().parents[2]
READINESS=ROOT/"results"/"arcllm_lmax_arch_p1_readiness"
MODEL=pathlib.Path(r"C:\Users\minht\.ollama\models\blobs\sha256-60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463")
SIDECAR=pathlib.Path(r"D:\WORK\RESEARCH\_arcllm_v4_gate_clean\results\canonical_exec148.sidecar")
SHADER_DIR=pathlib.Path(r"D:\WORK\RESEARCH\6.LTR\ArcLLM\compiled_shaders")
DEFAULT_OLLAMA=pathlib.Path(r"C:\Users\minht\AppData\Local\Programs\Ollama\ollama.exe")
U2_OLLAMA=pathlib.Path(r"D:\WORK\RESEARCH\2.CQG-UTS-U2\.local\ollama-v0.34.2\portable\ollama.exe")

def sha256(p):
    h=hashlib.sha256()
    with open(p,"rb") as f:
        for b in iter(lambda:f.read(8*1024*1024),b""): h.update(b)
    return h.hexdigest().upper()

def ollama_ps(exe,host=None):
    if not exe.is_file(): return {"available":False,"idle":True,"rows":[]}
    env=os.environ.copy()
    if host: env["OLLAMA_HOST"]=host
    cp=subprocess.run([str(exe),"ps"],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
    rows=[x.strip() for x in cp.stdout.splitlines() if x.strip()]
    return {"available":True,"rc":cp.returncode,"rows":rows,"idle":cp.returncode==0 and len(rows)<=1}

def memory():
    class M(ctypes.Structure):
        _fields_=[("dwLength",ctypes.c_ulong),("dwMemoryLoad",ctypes.c_ulong),
        ("ullTotalPhys",ctypes.c_ulonglong),("ullAvailPhys",ctypes.c_ulonglong),
        ("ullTotalPageFile",ctypes.c_ulonglong),("ullAvailPageFile",ctypes.c_ulonglong),
        ("ullTotalVirtual",ctypes.c_ulonglong),("ullAvailVirtual",ctypes.c_ulonglong),
        ("ullAvailExtendedVirtual",ctypes.c_ulonglong)]
    m=M();m.dwLength=ctypes.sizeof(M)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)): raise RuntimeError("GlobalMemoryStatusEx failed")
    return {"total_physical_bytes":m.ullTotalPhys,"available_physical_bytes":m.ullAvailPhys,"memory_load_percent":m.dwMemoryLoad}

def power():
    class P(ctypes.Structure):
        _fields_=[("ACLineStatus",ctypes.c_byte),("BatteryFlag",ctypes.c_byte),
        ("BatteryLifePercent",ctypes.c_byte),("SystemStatusFlag",ctypes.c_byte),
        ("BatteryLifeTime",ctypes.c_ulong),("BatteryFullLifeTime",ctypes.c_ulong)]
    p=P()
    if not ctypes.windll.kernel32.GetSystemPowerStatus(ctypes.byref(p)): raise RuntimeError("GetSystemPowerStatus failed")
    scheme=subprocess.run(["powercfg","/getactivescheme"],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout.strip()
    return {"ac_online":int(p.ACLineStatus)==1,"battery_percent":int(p.BatteryLifePercent),"scheme":scheme}

def processes(runner,control):
    out={"p1_runner":[],"p1_control":[],"other_arcllm":[],"loaded_model_servers":[]}
    if psutil is None: return {**out,"psutil_available":False}
    for p in psutil.process_iter(["pid","name","cmdline"]):
        try:
            n=(p.info["name"] or "").lower()
            cmd=" ".join(p.info["cmdline"] or [])
            row={"pid":p.info["pid"],"name":n,"cmd":cmd[:1200]}
            if runner.lower() in n: out["p1_runner"].append(row)
            if control.lower() in n: out["p1_control"].append(row)
            if n.endswith(".exe") and "arcllm" in n and runner.lower() not in n and control.lower() not in n: out["other_arcllm"].append(row)
            if "llama-server" in n and "-ngl 0" not in cmd: out["loaded_model_servers"].append(row)
        except Exception: pass
    return {**out,"psutil_available":True}

def main():
    ev=json.loads((READINESS/"BUILDONLY_EVIDENCE.json").read_text())
    assets=json.loads((ROOT/"config"/"arcllm_lmax_arch_p1_runtime_assets_v0.1.json").read_text())
    shaders=json.loads((ROOT/"config"/"arcllm_lmax_arch_p1_shader_manifest_v0.1.json").read_text())
    runner=READINESS/"build"/"arcllm_lmax_p1_runner.exe"
    control=READINESS/"build"/"arcllm_lmax_p1_control_path.exe"
    shader_results={}
    for name,expected in shaders["shaders"].items():
        p=SHADER_DIR/name
        shader_results[name]={"exists":p.is_file(),"sha256":sha256(p) if p.is_file() else None,"expected":expected}
    mem=memory(); pw=power(); proc=processes(runner.name,control.name)
    default=ollama_ps(DEFAULT_OLLAMA); u2=ollama_ps(U2_OLLAMA,"127.0.0.1:11435")
    gpu=subprocess.run(["powershell.exe","-NoProfile","-Command",
      "Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion | ConvertTo-Json -Compress"],
      text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    gpu_raw=gpu.stdout.strip()
    checks={
      "buildonly_pass":ev.get("status")=="PASS_IMPLEMENTATION_STATIC_PREFLIGHT_BUILDONLY",
      "runner_hash":runner.is_file() and sha256(runner)==ev["runner_exe_sha256"],
      "control_hash":control.is_file() and sha256(control)==ev["control_exe_sha256"],
      "model_hash":MODEL.is_file() and MODEL.stat().st_size==assets["model"]["bytes"] and sha256(MODEL)==assets["model"]["sha256"],
      "sidecar_hash":SIDECAR.is_file() and SIDECAR.stat().st_size==assets["sidecar"]["bytes"] and sha256(SIDECAR)==assets["sidecar"]["sha256"],
      "shader_hashes":all(x["exists"] and x["sha256"]==x["expected"] for x in shader_results.values()),
      "default_ollama_idle":default["idle"],
      "u2_ollama_idle":u2["idle"],
      "no_loaded_gpu_model_server":not proc["loaded_model_servers"],
      "no_p1_runner":not proc["p1_runner"],
      "no_p1_control":not proc["p1_control"],
      "no_other_arcllm":not proc["other_arcllm"],
      "arc140v":"Arc(TM) 140V" in gpu_raw or "Arc 140V" in gpu_raw,
      "ac_online":pw["ac_online"],
      "ram_at_least_8gib":mem["available_physical_bytes"]>=8*1024**3,
      "disk_at_least_5gib":shutil.disk_usage(str(ROOT)).free>=5*1024**3,
    }
    result={
      "schema":"arcllm.lmax_arch_p1.readiness_resource_audit.v0.4",
      "status":"PASS" if all(checks.values()) else "BLOCKED",
      "checks":checks,
      "memory":mem,"power":pw,
      "disk_free_bytes":shutil.disk_usage(str(ROOT)).free,
      "gpu_raw":gpu_raw,
      "default_ollama":default,"u2_ollama":u2,"processes":proc,
      "runner":{"path":str(runner),"sha256":sha256(runner) if runner.is_file() else None,"bytes":runner.stat().st_size if runner.is_file() else None},
      "control":{"path":str(control),"sha256":sha256(control) if control.is_file() else None,"bytes":control.stat().st_size if control.is_file() else None},
      "model":{"path":str(MODEL),"sha256":sha256(MODEL) if MODEL.is_file() else None,"bytes":MODEL.stat().st_size if MODEL.is_file() else None},
      "sidecar":{"path":str(SIDECAR),"sha256":sha256(SIDECAR) if SIDECAR.is_file() else None,"bytes":SIDECAR.stat().st_size if SIDECAR.is_file() else None},
      "shader_results":shader_results,
      "model_executed":False,"vulkan_initialized":False,"fresh_outcome_opened":False
    }
    out=READINESS/"RESOURCE_AUDIT.json"
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("ARCLLM_LMAX_ARCH_P1_RESOURCE_AUDIT="+result["status"])
    print("AVAILABLE_RAM_BYTES="+str(mem["available_physical_bytes"]))
    print("DISK_FREE_BYTES="+str(result["disk_free_bytes"]))
    print("MODEL_EXECUTED=false")
    print("FRESH_OUTCOME_OPENED=false")
    return 0 if result["status"]=="PASS" else 4

if __name__=="__main__":
    try: raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_RESOURCE_AUDIT=FAIL: "+repr(exc),file=sys.stderr)
        raise
