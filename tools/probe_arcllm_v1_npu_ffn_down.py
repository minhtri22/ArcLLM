import json, sys, time
sys.path.insert(0, r"D:\WORK\RESEARCH\_npu_probe_py")
import numpy as np
import openvino as openvino_pkg
import openvino.runtime as ov
from openvino.runtime import opset11 as ops

K=18944
R=3584
rng=np.random.default_rng(20260928)
w=np.empty((K,R),dtype=np.float16)
chunk=64
for i in range(0,K,chunk):
    n=min(chunk,K-i)
    w[i:i+n]=(rng.standard_normal((n,R),dtype=np.float32)*0.01).astype(np.float16)
x=(rng.standard_normal((1,K),dtype=np.float32)*0.01).astype(np.float16)

param=ops.parameter([1,K],ov.Type.f16,name="x")
out=ops.matmul(param,ops.constant(w),False,False)
model=ov.Model([out],[param],"ARCLLM_FFN_DOWN_F16_EXACT_SHAPE")
core=ov.Core()
result={
  "openvino_version":openvino_pkg.__version__,
  "available_devices":list(core.available_devices),
  "shape":{"input":[1,K],"weight":[K,R],"output":[1,R]},
  "dtype":"FP16","weight_bytes":int(w.nbytes),"input_bytes":int(x.nbytes),"output_bytes":int(R*2)
}
t0=time.perf_counter()
compiled=core.compile_model(model,"NPU",{"PERFORMANCE_HINT":"LATENCY"})
result["compile_ms"]=(time.perf_counter()-t0)*1000
result["execution_devices"]=compiled.get_property("EXECUTION_DEVICES")
req=compiled.create_infer_request()
for _ in range(2): req.infer({0:x})
times=[]
last=None
for _ in range(9):
    t0=time.perf_counter(); res=req.infer({0:x}); times.append((time.perf_counter()-t0)*1000)
    last=np.array(res[compiled.output(0)],copy=True)
result["infer_ms"]=times
result["infer_median_ms"]=float(np.median(times))
result["infer_min_ms"]=float(np.min(times))
result["infer_max_ms"]=float(np.max(times))
result["finite"]=bool(np.isfinite(last).all())
result["output_shape"]=list(last.shape)
result["status"]="PASS_EXACT_FFN_DOWN_NPU_COMPILE_EXECUTE"
print(json.dumps(result,indent=2))
