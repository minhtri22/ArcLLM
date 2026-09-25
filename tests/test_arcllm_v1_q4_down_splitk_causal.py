from pathlib import Path
import json,subprocess,sys,tempfile

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"src/arcllm_v1_q4_down_splitk_causal.cpp"
ADJ=ROOT/"tools/adjudicate_arcllm_v1_q4_down_splitk_causal.py"
BUILD=ROOT/"tools/build_arcllm_v1_q4_down_splitk_causal.ps1"
SPEC=ROOT/"artifacts/ARCLLM_V1/Q4_DOWN_SPLITK_CAUSAL_PRELOCK_v0.1.json"

for p in [SRC,ADJ,BUILD,SPEC]:
    assert p.is_file(),p

s=SRC.read_text(encoding="utf-8")
for x in [
    "Q4_DOWN_LAYERS={3u,4u,6u,7u,8u,11u,12u,14u,15u,17u,18u,19u,21u,22u}",
    'mode="correctness"',
    'mode!="correctness"&&mode!="measure"',
    'p+"ffn_down","p7_q4k_gemm_2d.spv"',
    'p+"ffn_down","sa1_q4k_subgroup_splitk.spv"',
    'if(b.gx!=56u||a.gx!=896u)',
    'q4_down_dispatch_ids.size()!=14u',
    'profile_probe&&di==15u',
    'attempts.reserve(32)',
    "only_work_decomposition_differs",
    "extra_resident_bytes",
    "performance_counters_used",
    "materialization_used",
]:
    assert x in s,x

for forbidden in [
    "q4_down_exec148",
    "b_exec148",
    "materialize_tensor",
    "CPU_ONE_TIME_RAM_EXECUTION_IMAGE",
    "VK_KHR_performance_query",
]:
    assert forbidden not in s,forbidden

# Correctness mode returns before the measurement campaign's first wall clock use.
correct_pos=s.index('if(mode=="correctness")')
attempt_pos=s.index('struct Attempt{')
chrono_pos=s.index('std::chrono::steady_clock::now()',attempt_pos)
assert correct_pos < attempt_pos < chrono_pos
correct_block=s[correct_pos:attempt_pos]
assert "std::chrono" not in correct_block
assert "execute_profiled" not in correct_block
assert "wall_timing_executed" in correct_block
assert "timestamp_queries_executed" in correct_block

spec=json.loads(SPEC.read_text(encoding="utf-8"))
assert spec["target"]["q4_down_layers"]==[3,4,6,7,8,11,12,14,15,17,18,19,21,22]
assert spec["arms"]["0"]["workgroups_per_q4_down_dispatch"]==56
assert spec["arms"]["A"]["workgroups_per_q4_down_dispatch"]==896
assert spec["authorization"]["performance"] is False
assert spec["authorization"]["real_model_correctness"] is False

# Adjudicator syntax + deterministic positive synthetic regression.
atxt=ADJ.read_text(encoding="utf-8")
compile(atxt,str(ADJ),"exec")

def make(workload,component_ratio=0.5,carry_ratio=0.8):
    expected={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}[workload]
    attempts=[]
    for b in range(8):
        for arm in ["0","A"]:
            cr=1.0 if arm=="0" else carry_ratio
            qr=1.0 if arm=="0" else component_ratio
            attempts.append({
                "block":b,"arm":arm,"phase":"carry","success":True,
                "decode_ms":100.0*cr,"e2e_ms":110.0*cr,"q4_down_ticks":0,
                "q4_down_dispatches":0,"timestamp_valid_bits":0,
                "carry_timing_authoritative":True,"component_ticks_authoritative":False,
                "generated_hash_fnv1a64":expected
            })
            attempts.append({
                "block":b,"arm":arm,"phase":"component","success":True,
                "decode_ms":120.0,"e2e_ms":130.0,"q4_down_ticks":100000*qr,
                "q4_down_dispatches":14,"timestamp_valid_bits":64,
                "carry_timing_authoritative":False,"component_ticks_authoritative":True,
                "generated_hash_fnv1a64":expected
            })
    return {
        "schema":"arcllm.v1.q4_down_splitk_causal.collection.v0.1",
        "status":"PASS_COLLECTION","workload":workload,
        "expected_generated_hash_fnv1a64":expected,
        "component_correctness":{"pass":True,"max_abs":0.001,"rmse":0.0001},
        "isolation":{
            "q4_down_layers":[3,4,6,7,8,11,12,14,15,17,18,19,21,22],
            "q4_down_dispatches_per_decode":14,
            "q6_down_unchanged":True,"prefill_unchanged":True,"extra_resident_bytes":0
        },
        "attempts":attempts,
    }

with tempfile.TemporaryDirectory() as td0:
    td=Path(td0)
    ws=td/"ws.json";wc=td/"wc.json";out=td/"adj.json"
    ws.write_text(json.dumps(make("W-S")),encoding="utf-8")
    wc.write_text(json.dumps(make("W-C")),encoding="utf-8")
    cp=subprocess.run([sys.executable,str(ADJ),"--w-s",str(ws),"--w-c",str(wc),"--out",str(out)],
                      capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    d=json.loads(out.read_text(encoding="utf-8"))
    assert d["verdict"]=="PASS_CAUSAL_Q4_DOWN_WORK_DECOMPOSITION_WITH_E2E_CARRY"
    assert d["cross_workload"]["component_supported_both"] is True
    assert d["cross_workload"]["e2e_carry_supported_both"] is True

print("Q4_DOWN_SPLITK_CAUSAL_STATIC_QA=PASS")
