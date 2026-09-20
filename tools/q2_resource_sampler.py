#!/usr/bin/env python3
import argparse, ctypes, json, os, re, statistics, subprocess, sys, threading, time
from ctypes import wintypes
from pathlib import Path

MARKER=re.compile(r"^Q2_(WARMUP|ATTEMPT)_(BEGIN|END)\|([^|]+)\|([^|]+)\|(\d+)(?:\|(PASS|FAIL))?$")

class FILETIME(ctypes.Structure):
    _fields_=[("dwLowDateTime",wintypes.DWORD),("dwHighDateTime",wintypes.DWORD)]
class PMC_EX(ctypes.Structure):
    _fields_=[
        ("cb",wintypes.DWORD),("PageFaultCount",wintypes.DWORD),
        ("PeakWorkingSetSize",ctypes.c_size_t),("WorkingSetSize",ctypes.c_size_t),
        ("QuotaPeakPagedPoolUsage",ctypes.c_size_t),("QuotaPagedPoolUsage",ctypes.c_size_t),
        ("QuotaPeakNonPagedPoolUsage",ctypes.c_size_t),("QuotaNonPagedPoolUsage",ctypes.c_size_t),
        ("PagefileUsage",ctypes.c_size_t),("PeakPagefileUsage",ctypes.c_size_t),
        ("PrivateUsage",ctypes.c_size_t)
    ]

def ft_value(ft):
    return (ft.dwHighDateTime<<32)|ft.dwLowDateTime

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--system",required=True)
    ap.add_argument("--workload",required=True,choices=["W-S","W-C"])
    ap.add_argument("--trace",required=True)
    ap.add_argument("--log",required=True)
    ap.add_argument("--gpu-script",required=True)
    ap.add_argument("--sample-ms",type=int,default=100)
    ap.add_argument("cmd",nargs=argparse.REMAINDER)
    a=ap.parse_args()
    if a.cmd and a.cmd[0]=="--":a.cmd=a.cmd[1:]
    if not a.cmd:raise SystemExit("missing child command")
    if os.name!="nt":raise SystemExit("Q2 resource sampler requires Windows")

    trace_path=Path(a.trace);trace_path.parent.mkdir(parents=True,exist_ok=True)
    log_path=Path(a.log);log_path.parent.mkdir(parents=True,exist_ok=True)
    gpu_raw=trace_path.with_suffix(".gpu.jsonl")
    gpu_stop=trace_path.with_suffix(".gpu.stop")
    for p in (gpu_raw,gpu_stop):
        try:p.unlink()
        except FileNotFoundError:pass

    proc=subprocess.Popen(a.cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1,universal_newlines=True)
    active={"kind":None,"index":None}
    lock=threading.Lock()
    windows={}
    samples=[]
    reader_done=threading.Event()

    def reader():
        with log_path.open("w",encoding="utf-8",newline="\n") as lf:
            assert proc.stdout is not None
            for raw in proc.stdout:
                line=raw.rstrip("\r\n")
                ts=time.time()
                lf.write(line+"\n");lf.flush()
                print(line,flush=True)
                m=MARKER.match(line)
                if not m:continue
                kind,edge,system,workload,idx,status=m.groups()
                idx=int(idx);key=f"{kind}:{idx}"
                with lock:
                    if edge=="BEGIN":
                        windows[key]={"kind":kind,"index":idx,"system":system,"workload":workload,"start_epoch_s":ts,"status":None}
                        active["kind"]=kind;active["index"]=idx
                    else:
                        if key in windows:
                            windows[key]["end_epoch_s"]=ts;windows[key]["status"]=status
                        active["kind"]=None;active["index"]=None
            reader_done.set()

    t=threading.Thread(target=reader,daemon=True);t.start()

    gpu_proc=None
    try:
        gpu_proc=subprocess.Popen([
            "powershell.exe","-NoProfile","-ExecutionPolicy","Bypass","-File",str(Path(a.gpu_script).resolve()),
            "-ProcessId",str(proc.pid),"-OutputPath",str(gpu_raw.resolve()),"-StopPath",str(gpu_stop.resolve()),
            "-SampleMilliseconds",str(a.sample_ms)
        ],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    except Exception as e:
        gpu_launch_error=repr(e)
    else:
        gpu_launch_error=None

    PROCESS_QUERY_INFORMATION=0x0400
    PROCESS_VM_READ=0x0010
    kernel32=ctypes.WinDLL("kernel32",use_last_error=True)
    psapi=ctypes.WinDLL("psapi",use_last_error=True)
    kernel32.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]
    kernel32.OpenProcess.restype=wintypes.HANDLE
    kernel32.GetProcessTimes.argtypes=[wintypes.HANDLE,ctypes.POINTER(FILETIME),ctypes.POINTER(FILETIME),ctypes.POINTER(FILETIME),ctypes.POINTER(FILETIME)]
    kernel32.GetProcessTimes.restype=wintypes.BOOL
    kernel32.CloseHandle.argtypes=[wintypes.HANDLE]
    psapi.GetProcessMemoryInfo.argtypes=[wintypes.HANDLE,ctypes.POINTER(PMC_EX),wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype=wintypes.BOOL

    h=kernel32.OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,False,proc.pid)
    if not h:raise OSError(ctypes.get_last_error(),"OpenProcess")
    cpu_count=max(1,os.cpu_count() or 1)
    prev_cpu=None;prev_t=None

    try:
        while proc.poll() is None or not reader_done.is_set():
            now=time.time()
            pm=PMC_EX();pm.cb=ctypes.sizeof(PMC_EX)
            mem_ok=bool(psapi.GetProcessMemoryInfo(h,ctypes.byref(pm),pm.cb))
            c=FILETIME();e=FILETIME();k=FILETIME();u=FILETIME()
            cpu_ok=bool(kernel32.GetProcessTimes(h,ctypes.byref(c),ctypes.byref(e),ctypes.byref(k),ctypes.byref(u)))
            cpu_pct=None
            if cpu_ok:
                cur=(ft_value(k)+ft_value(u))/10_000_000.0
                if prev_cpu is not None and now>prev_t:
                    cpu_pct=max(0.0,(cur-prev_cpu)/(now-prev_t)/cpu_count*100.0)
                prev_cpu=cur;prev_t=now
            with lock:
                kind=active["kind"];idx=active["index"]
            if kind=="ATTEMPT":
                samples.append({
                    "timestamp_epoch_s":now,"attempt_index":idx,
                    "working_set_bytes":int(pm.WorkingSetSize) if mem_ok else None,
                    "private_bytes":int(pm.PrivateUsage) if mem_ok else None,
                    "cpu_percent_total_capacity":cpu_pct
                })
            time.sleep(max(0.02,a.sample_ms/1000.0))
    finally:
        kernel32.CloseHandle(h)
        gpu_stop.write_text("stop\n",encoding="ascii")
        if gpu_proc is not None:
            try:gpu_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:gpu_proc.terminate()
        t.join(timeout=5)

    rc=proc.wait()
    gpu_samples=[];gpu_error=gpu_launch_error
    if gpu_raw.exists():
        for line in gpu_raw.read_text(encoding="utf-8",errors="replace").splitlines():
            try:
                row=json.loads(line)
                if row.get("valid"):gpu_samples.append(row)
                elif row.get("error") and gpu_error is None:gpu_error=row["error"]
            except Exception:
                if gpu_error is None:gpu_error="invalid GPU counter JSONL"

    attempt_summaries=[]
    for idx in range(5):
        rows=[x for x in samples if x["attempt_index"]==idx]
        win=windows.get(f"ATTEMPT:{idx}",{})
        st=win.get("start_epoch_s");en=win.get("end_epoch_s")
        grows=[]
        if st is not None and en is not None:
            grows=[g for g in gpu_samples if st<=g.get("timestamp_epoch_s",0)<=en]
        cpu=[x["cpu_percent_total_capacity"] for x in rows if x["cpu_percent_total_capacity"] is not None]
        ws=[x["working_set_bytes"] for x in rows if x["working_set_bytes"] is not None]
        pb=[x["private_bytes"] for x in rows if x["private_bytes"] is not None]
        compute=[g.get("compute_percent") for g in grows if g.get("compute_percent") is not None]
        dedicated=[g.get("dedicated_bytes") for g in grows if g.get("dedicated_bytes") is not None]
        shared=[g.get("shared_bytes") for g in grows if g.get("shared_bytes") is not None]
        attempt_summaries.append({
            "index":idx,"marker_status":win.get("status"),"sample_count":len(rows),
            "working_set_peak_bytes":max(ws) if ws else None,
            "private_bytes_peak":max(pb) if pb else None,
            "cpu_mean_percent_total_capacity":statistics.fmean(cpu) if cpu else None,
            "cpu_peak_percent_total_capacity":max(cpu) if cpu else None,
            "gpu_sample_count":len(grows),
            "gpu_compute_mean_percent":statistics.fmean(compute) if compute else None,
            "gpu_compute_peak_percent":max(compute) if compute else None,
            "gpu_dedicated_peak_bytes":max(dedicated) if dedicated else None,
            "gpu_shared_peak_bytes":max(shared) if shared else None
        })

    out={
        "schema":"arcllm.q2.resource_trace.v1",
        "system":a.system,"workload":a.workload,"pid":proc.pid,
        "sample_interval_target_ms":a.sample_ms,"process_exit_code":rc,
        "mandatory_sampler":"Win32 GetProcessMemoryInfo + GetProcessTimes",
        "gpu_sampler":{"attempted":True,"available":bool(gpu_samples),"error":gpu_error},
        "attempt_windows":list(windows.values()),
        "attempt_summaries":attempt_summaries,
        "raw_process_samples":samples,
        "raw_gpu_samples":gpu_samples,
        "child_log":str(log_path)
    }
    trace_path.write_text(json.dumps(out,indent=2),encoding="utf-8")
    return rc

if __name__=="__main__":
    raise SystemExit(main())
