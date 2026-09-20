#!/usr/bin/env python3
import argparse, hashlib, json, zipfile
from pathlib import Path

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--root",required=True);ap.add_argument("--session-a",required=True);ap.add_argument("--session-b",required=True)
    ap.add_argument("--candidate",required=True);ap.add_argument("--out",required=True)
    a=ap.parse_args();root=Path(a.root);out=Path(a.out)
    sources=[]
    for label,d in (("session_A",Path(a.session_a)),("session_B",Path(a.session_b))):
        for p in sorted(d.rglob("*")):
            if p.is_file(): sources.append((f"{label}/{p.relative_to(d).as_posix()}",p))
    top=[
      ("q3_adjudication_candidate.json",Path(a.candidate)),
      ("q3_confirmatory_design.json",root/"config/q3_confirmatory_design.json"),
      ("q3_preflight_lock.json",root/"results/q3_preflight_lock.json"),
      ("q3_execution_authorization.json",root/"config/q3_execution_authorization.json"),
      ("q2_formal_result.authoritative.json",root/"inputs/q2_formal_result.authoritative.json"),
      ("q2_execution_authorization.json",root/"config/q2_execution_authorization.json"),
      ("q2_baseline_qualification.json",root/"results/q2_baseline_qualification.json"),
      ("q3_baseline_runtime_qualification_W_S.json",root/"results/q3_baseline_runtime_qualification_W_S.json"),
      ("q3_baseline_runtime_qualification_W_C.json",root/"results/q3_baseline_runtime_qualification_W_C.json"),
      ("q2_arcllm_shader_provenance.json",root/"results/q2_arcllm_shader_provenance.json"),
    ]
    sources += top
    missing=[str(p) for _,p in sources if not p.is_file()]
    if missing: raise SystemExit("Missing evidence files: "+"; ".join(missing))
    manifest={"schema":"arcllm.q3.evidence_manifest.v1","expected_measured_attempts":40,"sessions":["A","B"],"files":[]}
    for arc,p in sources:
        manifest["files"].append({"path":arc,"bytes":p.stat().st_size,"sha256":sha(p)})
    manifest_bytes=(json.dumps(manifest,indent=2)+"\n").encode("utf-8")
    out.parent.mkdir(parents=True,exist_ok=True)
    if out.exists(): out.unlink()
    with zipfile.ZipFile(out,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for arc,p in sources: z.write(p,arc)
        z.writestr("q3_evidence_manifest.json",manifest_bytes)
    print(json.dumps({"bundle":str(out),"bytes":out.stat().st_size,"sha256":sha(out),"files":len(sources)+1}))
if __name__=="__main__": main()
