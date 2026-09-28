from pathlib import Path
import json, subprocess, sys

root=Path(__file__).resolve().parents[1]
exe=root/"arcllm_v1_runtime.exe"
model=Path(sys.argv[1])
shader_dir=root/"compiled_shaders"

fixtures=[
    {
      "id":"PROFILE_0_FROZEN_WS",
      "profile":"0",
      "tokens":[1,133151,133152,152062],
      "expected_hash":"f31d4bb9fe5eb9c3"
    },
    {
      "id":"PROFILE_1_FROZEN_WC",
      "profile":"1",
      "tokens":[1,133151,133152,152062]+[1+((104729+7919*i)%152063) for i in range(4,256)],
      "expected_hash":"471519ddc45b232e"
    }
]

def fnv1a64_u32(xs):
    h=1469598103934665603
    for x in xs:
        for b in int(x).to_bytes(4,"little"):
            h ^= b
            h = (h * 1099511628211) & ((1<<64)-1)
    return f"{h:016x}"

results=[]
for f in fixtures:
    out=root/"results"/("RUNTIME_EXTRACTION_"+f["id"]+".json")
    cmd=[
      str(exe),"--model",str(model),"--shader-dir",str(shader_dir),
      "--tokens",",".join(map(str,f["tokens"])),"--max-new","32",
      "--profile",f["profile"],"--within-validated-domain","1","--out",str(out)
    ]
    cp=subprocess.run(cmd,cwd=root)
    if cp.returncode!=0:
        raise SystemExit(f"runtime fixture failed: {f['id']} rc={cp.returncode}")
    o=json.loads(out.read_text())
    got=fnv1a64_u32(o["generated_token_ids"])
    s=o["stats"]
    checks={
      "generated_count":len(o["generated_token_ids"])==32,
      "hash_match":got==f["expected_hash"],
      "prefill_dispatches":s["prefill_dispatches"]==441,
      "prefill_submits":s["prefill_submits"]==1,
      "decode_dispatches":s["decode_dispatches_per_step"]==469,
      "decode_submits":s["decode_submits_per_step"]==1,
      "decode_steps":s["decode_steps"]==31,
      "route_A_steps":s["route_a_steps"]==0,
      "route_B_steps":s["route_b_steps"]==31,
      "acquire_once":s["acquire_events"]==1,
      "evict_once":s["evict_events"]==1,
      "backend_acquire_once":s["b_allocations"]==1 and s["b_materializations"]==1 and s["b_validations"]==1,
      "backend_release_once":s["b_releases"]==1,
      "p1_once":s["p1_calls"]==1 and s["p3_calls"]==0 and s["p0_calls"]==0,
      "finite":s["finite"] is True
    }
    if not all(checks.values()):
        raise SystemExit(f"regression failed {f['id']}: {checks}")
    results.append({"id":f["id"],"generated_hash":got,"checks":checks})

summary={
  "schema":"arcllm.v1.canonical_runtime_extraction.regression.v0.1",
  "status":"PASS_EXACT_FROZEN_ORACLE_PRESERVED",
  "fixtures":results,
  "performance_adjudicated":False
}
(root/"results"/"ARCLLM_V1_CANONICAL_RUNTIME_EXTRACTION_REGRESSION.json").write_text(json.dumps(summary,indent=2)+"\n")
print("ARCLLM_V1_CANONICAL_RUNTIME_EXTRACTION_REGRESSION=PASS")
