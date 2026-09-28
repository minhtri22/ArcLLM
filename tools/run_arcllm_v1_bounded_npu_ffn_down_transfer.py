import gc, hashlib, io, json, math, os, pathlib, statistics, sys, time

OPENVINO_SITE = pathlib.Path(r"D:\WORK\RESEARCH\_npu_probe_py")
if not OPENVINO_SITE.exists():
    raise SystemExit("isolated OpenVINO probe environment missing")
sys.path.insert(0, str(OPENVINO_SITE))

import numpy as np
import gguf
import openvino.runtime as ov
from openvino.runtime import opset11 as ops

MODEL_SHA256 = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
K = 18944
R = 3584
SELECTED = [
    {"layer":3, "qtype":"Q4_K", "qtype_id":12},
    {"layer":14,"qtype":"Q4_K", "qtype_id":12},
    {"layer":22,"qtype":"Q4_K", "qtype_id":12},
    {"layer":0, "qtype":"Q6_K", "qtype_id":14},
    {"layer":16,"qtype":"Q6_K", "qtype_id":14},
    {"layer":27,"qtype":"Q6_K", "qtype_id":14},
]
MAX_ABS_MAX = 0.02
RMSE_MAX = 0.005
GPU_BUDGET = {"W-S":160.59743019334533,"W-C":118.13835909692924}
WARMUPS = 3
REPEATS = 9

def percentile95(xs):
    ys=sorted(float(x) for x in xs)
    if not ys: return float("nan")
    idx=max(0, min(len(ys)-1, math.ceil(0.95*len(ys))-1))
    return ys[idx]

def err(a,b):
    af=np.asarray(a,dtype=np.float32).reshape(-1)
    bf=np.asarray(b,dtype=np.float32).reshape(-1)
    d=af-bf
    return {
      "max_abs":float(np.max(np.abs(d))),
      "rmse":float(np.sqrt(np.mean(d*d))),
      "rel_l2":float(np.linalg.norm(d)/(np.linalg.norm(bf)+1e-30))
    }

def raw_sha256(a):
    h=hashlib.sha256()
    mv=memoryview(np.asarray(a)).cast("B")
    step=8*1024*1024
    for i in range(0,len(mv),step):
        h.update(mv[i:i+step])
    return h.hexdigest().upper()

def activations():
    a=(np.random.default_rng(20260928).standard_normal(K)*0.02).astype(np.float32)
    b=(np.random.default_rng(20260929).standard_normal(K)*0.10).astype(np.float32)
    i=np.arange(K,dtype=np.float64)
    c=(0.05*np.sin((i+1.0)*0.017)+0.02*np.cos((i+1.0)*0.013)).astype(np.float32)
    return [("A",a),("B",b),("C",c)]

def main():
    if len(sys.argv)!=3:
        raise SystemExit("usage: study.py MODEL OUT_JSON")
    model_path=pathlib.Path(sys.argv[1])
    out_path=pathlib.Path(sys.argv[2])
    if out_path.exists():
        raise SystemExit("output already exists; no overwrite")
    g=gguf.GGUFReader(str(model_path))
    if len(g.tensors)!=339:
        raise SystemExit(f"integrity: tensor census {len(g.tensors)} != 339")
    byname={t.name:t for t in g.tensors}
    core=ov.Core()
    devices=list(core.available_devices)
    if "NPU" not in devices:
        raise SystemExit("provider: NPU not enumerated")
    acts=activations()
    cases=[]
    layers=[]
    case_medians=[]
    all_numerics=True

    for spec in SELECTED:
        lname=f"blk.{spec['layer']}.ffn_down.weight"
        t=byname.get(lname)
        if t is None:
            raise SystemExit("integrity: missing "+lname)
        if int(t.tensor_type)!=spec["qtype_id"]:
            raise SystemExit(f"integrity: {lname} type {int(t.tensor_type)} != {spec['qtype_id']}")
        if list(t.shape)!=[K,R]:
            raise SystemExit(f"integrity: {lname} shape {list(t.shape)} != {[K,R]}")
        source_sha=raw_sha256(t.data)

        t0=time.perf_counter()
        w32=gguf.dequantize(t.data,t.tensor_type)
        dequant_ms=(time.perf_counter()-t0)*1000.0
        if w32.shape!=(R,K) or w32.dtype!=np.float32:
            raise SystemExit(f"integrity: dequant shape/dtype {w32.shape}/{w32.dtype}")
        t0=time.perf_counter()
        w16=w32.astype(np.float16)
        fp16_cast_ms=(time.perf_counter()-t0)*1000.0

        x32=np.stack([x for _,x in acts],axis=0)
        x16=x32.astype(np.float16)
        # Source semantics and representation-only decomposition.
        t0=time.perf_counter()
        source_ref=x32 @ w32.T
        source_ref_ms=(time.perf_counter()-t0)*1000.0
        w16f32=w16.astype(np.float32)
        t0=time.perf_counter()
        representation_ref=x16.astype(np.float32) @ w16f32.T
        representation_ref_ms=(time.perf_counter()-t0)*1000.0

        param=ops.parameter([1,K],ov.Type.f16,name="x")
        const=ops.constant(w16)
        node=ops.matmul(param,const,False,True)
        model=ov.Model([node],[param],f"ARCLLM_REAL_FFN_DOWN_L{spec['layer']:02d}_{spec['qtype']}")
        t0=time.perf_counter()
        compiled=core.compile_model(model,"NPU",{"PERFORMANCE_HINT":"LATENCY"})
        compile_ms=(time.perf_counter()-t0)*1000.0
        if compiled.get_property("EXECUTION_DEVICES")!="NPU":
            raise SystemExit("provider: execution device is not NPU")
        stream=io.BytesIO()
        t0=time.perf_counter()
        compiled.export_model(stream)
        export_ms=(time.perf_counter()-t0)*1000.0
        blob_bytes=len(stream.getvalue())
        del compiled, model, node, const, param
        gc.collect()

        import_core=ov.Core()
        stream.seek(0)
        t0=time.perf_counter()
        imported=import_core.import_model(stream,"NPU")
        import_ms=(time.perf_counter()-t0)*1000.0
        del stream
        req=imported.create_infer_request()
        outport=imported.output(0)

        layer_case_medians=[]
        for ai,(aid,_) in enumerate(acts):
            xin=x16[ai:ai+1]
            for _ in range(WARMUPS):
                rr=req.infer({0:xin})
                _=np.array(rr[outport],copy=True)
            times=[]
            y=None
            for _ in range(REPEATS):
                t0=time.perf_counter()
                rr=req.infer({0:xin})
                y=np.array(rr[outport],copy=True)
                times.append((time.perf_counter()-t0)*1000.0)
            cm=float(statistics.median(times))
            layer_case_medians.append(cm)
            case_medians.append(cm)
            e2e=err(y,source_ref[ai])
            rep=err(representation_ref[ai],source_ref[ai])
            provider=err(y,representation_ref[ai])
            passed=(np.isfinite(y).all() and e2e["max_abs"]<=MAX_ABS_MAX and e2e["rmse"]<=RMSE_MAX)
            all_numerics=all_numerics and bool(passed)
            cases.append({
              "layer":spec["layer"],"qtype":spec["qtype"],"activation":aid,
              "infer_ms":times,"infer_median_ms":cm,
              "end_to_end_error_vs_exact_quant_reference":e2e,
              "representation_error_only":rep,
              "provider_error_vs_fp16_representation_reference":provider,
              "finite":bool(np.isfinite(y).all()),"numerical_pass":bool(passed)
            })

        layers.append({
          "layer":spec["layer"],"qtype":spec["qtype"],"tensor":lname,
          "source_tensor_sha256":source_sha,
          "source_tensor_bytes":int(t.data.nbytes),
          "fp16_weight_bytes":int(w16.nbytes),
          "dequant_ms":dequant_ms,"fp16_cast_ms":fp16_cast_ms,
          "dequant_plus_fp16_ms":dequant_ms+fp16_cast_ms,
          "source_reference_ms_for_3_vectors":source_ref_ms,
          "representation_reference_ms_for_3_vectors":representation_ref_ms,
          "compile_ms":compile_ms,"export_ms":export_ms,
          "compiled_blob_bytes":blob_bytes,"import_ms":import_ms,
          "case_median_ms":layer_case_medians,
          "layer_median_ms":float(statistics.median(layer_case_medians))
        })
        del req, imported, import_core, w16f32, representation_ref, source_ref, x16, x32, w16, w32
        gc.collect()

    primary_median=float(statistics.median(case_medians))
    p95=float(percentile95(case_medians))
    family_p95_ms=28.0*p95
    positive=family_p95_ms < min(GPU_BUDGET.values())
    material=family_p95_ms <= 0.90*min(GPU_BUDGET.values())
    import_med=float(statistics.median([x["import_ms"] for x in layers]))
    compile_med=float(statistics.median([x["compile_ms"] for x in layers]))
    conversion_med=float(statistics.median([x["dequant_plus_fp16_ms"] for x in layers]))
    setup_import=28.0*import_med
    setup_compile=28.0*compile_med
    setup_conversion=28.0*conversion_med
    warm_savings={k:v-family_p95_ms for k,v in GPU_BUDGET.items()}
    break_even_import={k:(setup_import/s if s>0 else None) for k,s in warm_savings.items()}
    break_even_import_plus_conversion={k:((setup_import+setup_conversion)/s if s>0 else None) for k,s in warm_savings.items()}
    if not all_numerics:
        verdict="FAIL_NUMERICAL_SEMANTICS"
    elif not positive:
        verdict="FAIL_WARM_TRANSFER_BUDGET"
    elif material:
        verdict="PASS_REAL_WEIGHT_WARM_TRANSFER_MATERIAL"
    else:
        verdict="PASS_REAL_WEIGHT_WARM_TRANSFER"

    result={
      "schema":"arcllm.v1.bounded_npu_ffn_down_transfer.execution.v0.1",
      "status":"PASS_COMPLETE_BOUNDED_EXECUTION",
      "study":"ARCLLM_V1_BOUNDED_NPU_FFN_DOWN_TRANSFER_STUDY",
      "model_path":str(model_path),
      "model_sha256_expected":MODEL_SHA256,
      "openvino_version":ov.get_version(),
      "provider_devices":devices,
      "selected_layers":SELECTED,
      "cases":cases,
      "layers":layers,
      "aggregate":{
        "all_18_numerical_cases_pass":all_numerics,
        "case_median_ms_primary":primary_median,
        "case_median_ms_p95_guard":p95,
        "family_28_layer_p95_ms_per_token":family_p95_ms,
        "GPU_family_budget_ms_per_token":GPU_BUDGET,
        "warm_savings_ms_per_token":warm_savings,
        "positive_budget_rule_pass":positive,
        "material_10pct_rule_pass":material,
        "median_import_ms_per_layer":import_med,
        "median_compile_ms_per_layer":compile_med,
        "median_conversion_ms_per_layer":conversion_med,
        "projected_28_layer_import_ms":setup_import,
        "projected_28_layer_compile_ms":setup_compile,
        "projected_28_layer_conversion_ms":setup_conversion,
        "break_even_tokens_import_only":break_even_import,
        "break_even_tokens_import_plus_conversion":break_even_import_plus_conversion
      },
      "formal_execution_verdict":verdict,
      "claim_boundary":{
        "full_model_run":False,
        "full_runtime_speedup":False,
        "canonical_integration":False,
        "cold_short_request_advantage":False
      }
    }
    out_path.parent.mkdir(parents=True,exist_ok=True)
    out_path.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"status":result["status"],"verdict":verdict,"aggregate":result["aggregate"]},indent=2))

if __name__=="__main__":
    main()
