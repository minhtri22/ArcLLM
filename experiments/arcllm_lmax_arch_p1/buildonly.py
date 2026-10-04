#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, os, pathlib, subprocess, sys

def run(argv, *, cwd, env=None, check=True):
    cp=subprocess.run([str(x) for x in argv],cwd=str(cwd),env=env,text=True,
                      stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
    if check and cp.returncode!=0:
        raise RuntimeError("command failed rc=%d: %r\n%s"%(cp.returncode,argv,cp.stdout))
    return cp

def sha256(p):
    h=hashlib.sha256()
    with open(p,"rb") as f:
        for b in iter(lambda:f.read(1024*1024),b""): h.update(b)
    return h.hexdigest().upper()

def git_blob(root,rel):
    return run(["git","hash-object",rel],cwd=root).stdout.strip()

def vc_environment():
    pf86=os.environ.get("ProgramFiles(x86)",r"C:\Program Files (x86)")
    vswhere=pathlib.Path(pf86)/"Microsoft Visual Studio"/"Installer"/"vswhere.exe"
    if not vswhere.is_file(): raise RuntimeError("vswhere.exe not found")
    cp=subprocess.run([str(vswhere),"-latest","-products","*",
        "-requires","Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property","installationPath"],text=True,stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,shell=False)
    if cp.returncode!=0 or not cp.stdout.strip(): raise RuntimeError("VS Build Tools not found")
    install=pathlib.Path(cp.stdout.strip())
    devcmd=install/"VC"/"Auxiliary"/"Build"/"vcvars64.bat"
    cp=subprocess.run(f'call "{devcmd}" >nul && set',text=True,stdout=subprocess.PIPE,
                      stderr=subprocess.STDOUT,shell=True)
    if cp.returncode!=0: raise RuntimeError("vcvars64 environment capture failed")
    env=dict(os.environ)
    for line in cp.stdout.splitlines():
        if "=" in line:
            k,v=line.split("=",1); env[k]=v
    return env,str(install)

def main():
    here=pathlib.Path(__file__).resolve().parent
    root=here.parents[1]
    evidence=root/"results"/"arcllm_lmax_arch_p1_readiness"
    build=evidence/"build"
    build.mkdir(parents=True,exist_ok=True)

    qa=run([sys.executable,here/"static_qa.py"],cwd=root)
    sys.stdout.write(qa.stdout)

    generated=build/"arcllm_v1_runtime_p1_instrumented.cpp"
    transform=evidence/"RUNTIME_TRANSFORM.json"
    gen=run([sys.executable,here/"generate_instrumented_runtime.py",
             "--root",root,"--out",generated,"--manifest",transform],cwd=root)
    sys.stdout.write(gen.stdout)

    env,vs=vc_environment()
    runner=build/"arcllm_lmax_p1_runner.exe"
    control=build/"arcllm_lmax_p1_control_path.exe"

    sources=[
        generated,
        here/"p1_runner.cpp",
        root/"src"/"gguf.cpp",
        root/"src"/"tensor_store.cpp",
        root/"src"/"arcllm_v1_primitive_registry_v2.cpp",
        root/"src"/"arcllm_v1_generic_policy_engine_v4.cpp",
        root/"src"/"arcllm_v1_generic_backend_binding_v4.cpp",
        root/"src"/"registrations"/"arcllm_v1_q4k_down_reference_registration_v2.cpp",
    ]
    cmd=["cl.exe","/nologo","/std:c++17","/O2","/EHsc","/W4","/bigobj","/Brepro",
         f"/I{root/'src'}",f"/I{root/'include'}",f"/I{here}",
         f"/Fe:{runner}",*[str(p) for p in sources]]
    cp=run(cmd,cwd=build,env=env); sys.stdout.write(cp.stdout)
    if not runner.is_file(): raise RuntimeError("P1 runner missing")

    cmd2=["cl.exe","/nologo","/std:c++17","/O2","/EHsc","/W4","/Brepro",
          f"/I{here}",f"/Fe:{control}",str(here/"p1_control_path.cpp")]
    cp=run(cmd2,cwd=build,env=env); sys.stdout.write(cp.stdout)
    if not control.is_file(): raise RuntimeError("P1 control harness missing")

    rd=run([runner,"--describe"],cwd=build,env=env)
    cd=run([control,"--describe"],cwd=build,env=env)
    (evidence/"RUNNER_DESCRIBE.txt").write_text(rd.stdout,encoding="utf-8")
    (evidence/"CONTROL_DESCRIBE.txt").write_text(cd.stdout,encoding="utf-8")
    for text,kind in ((rd.stdout,"runner"),(cd.stdout,"control")):
        if "MODEL_EXECUTED=false" not in text or "VULKAN_INITIALIZED=false" not in text:
            raise RuntimeError(kind+" describe did not prove zero-science")
    if "RAW_PRIMITIVES_SERIALIZED=true" not in rd.stdout:
        raise RuntimeError("runner describe missing raw primitive claim")
    if "SERVICE_DELAY=MONOTONIC_BUSY_DELAY" not in cd.stdout:
        raise RuntimeError("control describe missing frozen service delay")

    rg=run([runner],cwd=build,env=env,check=False)
    cg=run([control],cwd=build,env=env,check=False)
    (evidence/"RUNNER_AUTH_GUARD.txt").write_text(rg.stdout,encoding="utf-8")
    (evidence/"CONTROL_AUTH_GUARD.txt").write_text(cg.stdout,encoding="utf-8")
    if rg.returncode!=2 or "outcome execution blocked" not in rg.stdout:
        raise RuntimeError("runner authorization guard did not fail closed")
    if cg.returncode!=2 or "outcome execution blocked" not in cg.stdout:
        raise RuntimeError("control authorization guard did not fail closed")

    compiler=run(["cl.exe","/Bv"],cwd=build,env=env,check=False)
    tracked=[
      "experiments/arcllm_lmax_arch_p1/p1_ring.h",
      "experiments/arcllm_lmax_arch_p1/p1_runner.cpp",
      "experiments/arcllm_lmax_arch_p1/p1_control_path.cpp",
      "experiments/arcllm_lmax_arch_p1/generate_instrumented_runtime.py",
      "experiments/arcllm_lmax_arch_p1/static_qa.py",
      "experiments/arcllm_lmax_arch_p1/buildonly.py",
      "config/arcllm_lmax_arch_p1_preregistration_v0.4.json",
      "docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P1_PREREGISTRATION_REVISION_V0_4.md",
    ]
    ev={
      "schema":"arcllm.lmax_arch_p1.readiness_buildonly.v0.4",
      "status":"PASS_IMPLEMENTATION_STATIC_PREFLIGHT_BUILDONLY",
      "git_head":run(["git","rev-parse","HEAD"],cwd=root).stdout.strip(),
      "canonical_runtime_git_blob":git_blob(root,"src/arcllm_v1_runtime.cpp"),
      "canonical_api_git_blob":git_blob(root,"include/arcllm/v1/runtime.h"),
      "source_sha256":{p:sha256(root/p) for p in tracked},
      "source_git_blobs":{p:git_blob(root,p) for p in tracked},
      "generated_runtime_sha256":sha256(generated),
      "transform_manifest_sha256":sha256(transform),
      "runner_exe_sha256":sha256(runner),
      "runner_exe_bytes":runner.stat().st_size,
      "control_exe_sha256":sha256(control),
      "control_exe_bytes":control.stat().st_size,
      "compiler_info":compiler.stdout.strip(),
      "visual_studio_installation":vs,
      "static_qa":"PASS",
      "reversible_runtime_transform":"PASS",
      "runner_authorization_guard":"PASS_FAIL_CLOSED",
      "control_authorization_guard":"PASS_FAIL_CLOSED",
      "model_executed":False,
      "vulkan_initialized":False,
      "measured_inference_runs":0,
      "control_path_1m_runs":0,
      "lineage_modified":False
    }
    (evidence/"BUILDONLY_EVIDENCE.json").write_text(json.dumps(ev,indent=2)+"\n",encoding="utf-8")
    for p in build.glob("*.obj"):
        try:p.unlink()
        except OSError:pass
    print("ARCLLM_LMAX_ARCH_P1_BUILDONLY=PASS")
    print("RUNNER_SHA256="+ev["runner_exe_sha256"])
    print("CONTROL_SHA256="+ev["control_exe_sha256"])
    print("MODEL_EXECUTED=false")
    print("MEASURED_INFERENCE_RUNS=0")
    print("CONTROL_PATH_1M_RUNS=0")
    return 0

if __name__=="__main__":
    try: raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_BUILDONLY=FAIL: "+str(exc),file=sys.stderr)
        raise
