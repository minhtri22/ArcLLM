import json, sys, time
sys.path.insert(0, r"D:\WORK\RESEARCH\_npu_probe_py")
import numpy as np
import openvino as openvino_pkg
import openvino.runtime as ov
from openvino.runtime import opset11 as ops

K=3584
R=18944
rng=np.random.default_rng(20260928)
wg=np.empty((K,R),dtype=np.float16)
wu=np.empty((K,R),dtype=np.float16)
chunk=64
for i in range(0,K,chunk):
    n=min(chunk,K-i)
    wg[i:i+n]=(rng.standard_normal((n,R),dtype=np.float32)*0.01).astype(np.float16)
    wu[i:i+n]=(rng.standard_normal((n,R),dtype=np.float32)*0.01).astype(np.float16)
x=(rng.standard_normal((1,K),dtype=np.float32)*0.01).astype(np.float16)

param=ops.parameter([1,K],ov.Type.f16,name="x")
gate=ops.matmul(param,ops.constant(wg),False,False)
up=ops.matmul(param,ops.constant(wu),False,False)
sig=ops.sigmoid(gate)
silu=ops.multiply(gate,sig)
out=ops.multiply(silu,up)
model=ov.Model([out],[param],"ARCLLM_GATE_UP_SWIGLU_F16_EXACT_SHAPE")
core=ov.Core()
result={
  "openvino_version":openvino_pkg.__version__,
  "available_devices":list(core.available_devices),
  "shape":{"input":[1,K],"gate_weight":[K,R],"up_weight":[K,R],"output":[1,R]},
  "dtype":"FP16",
  "weight_bytes":int(wg.nbytes+wu.nbytes),
  "input_bytes":int(x.nbytes),
  "output_bytes":int(R*2)
}
if "NPU" not in core.available_devices:
    result["status"]="FAIL_NPU_NOT_ENUMERATED"
    print(json.dumps(result,indent=2)); raise SystemExit(2)
t0=time.perf_counter()
compiled=core.compile_model(model,"NPU",{"PERFORMANCE_HINT":"LATENCY"})
result["compile_ms"]=(time.perf_counter()-t0)*1000
try: result["execution_devices"]=compiled.get_property("EXECUTION_DEVICES")
except Exception as e: result["execution_devices_error"]=repr(e)
req=compiled.create_infer_request()
for _ in range(2): req.infer({0:x})
times=[]
last=None
for _ in range(9):
    t0=time.perf_counter()
    res=req.infer({0:x})
    times.append((time.perf_counter()-t0)*1000)
    last=np.array(res[compiled.output(0)],copy=True)
result["infer_ms"]=times
result["infer_median_ms"]=float(np.median(times))
result["infer_min_ms"]=float(np.min(times))
result["infer_max_ms"]=float(np.max(times))
result["finite"]=bool(np.isfinite(last).all())
result["output_shape"]=list(last.shape)
result["status"]="PASS_EXACT_GATE_UP_SWIGLU_NPU_COMPILE_EXECUTE"
print(json.dumps(result,indent=2))
