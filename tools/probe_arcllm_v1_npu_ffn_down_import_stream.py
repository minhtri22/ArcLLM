import io, json, sys, time
sys.path.insert(0, r"D:\WORK\RESEARCH\_npu_probe_py")
import numpy as np
import openvino as openvino_pkg
import openvino.runtime as ov
from openvino.runtime import opset11 as ops

K=18944; R=3584
rng=np.random.default_rng(20260928)
w=np.empty((K,R),dtype=np.float16)
for i in range(0,K,64):
    n=min(64,K-i); w[i:i+n]=(rng.standard_normal((n,R),dtype=np.float32)*0.01).astype(np.float16)
x=(rng.standard_normal((1,K),dtype=np.float32)*0.01).astype(np.float16)
param=ops.parameter([1,K],ov.Type.f16,name="x")
model=ov.Model([ops.matmul(param,ops.constant(w),False,False)],[param],"ARCLLM_FFN_DOWN_F16_IMPORT_STREAM")
core=ov.Core()
print("stage=compile",flush=True)
t0=time.perf_counter(); compiled=core.compile_model(model,"NPU",{"PERFORMANCE_HINT":"LATENCY"}); compile_ms=(time.perf_counter()-t0)*1000
print("stage=export",flush=True)
stream=io.BytesIO()
t0=time.perf_counter(); compiled.export_model(stream); export_ms=(time.perf_counter()-t0)*1000
blob_bytes=len(stream.getvalue())
print("stage=import",flush=True)
core2=ov.Core()
stream.seek(0)
t0=time.perf_counter(); imported=core2.import_model(stream,"NPU"); import_ms=(time.perf_counter()-t0)*1000
print("stage=infer",flush=True)
req=imported.create_infer_request(); req.infer({0:x})
times=[]
for _ in range(5):
    t0=time.perf_counter(); req.infer({0:x}); times.append((time.perf_counter()-t0)*1000)
print(json.dumps({
 "openvino_version":openvino_pkg.__version__,
 "compile_ms":compile_ms,"export_ms":export_ms,"compiled_blob_bytes":blob_bytes,
 "import_ms":import_ms,"execution_devices":imported.get_property("EXECUTION_DEVICES"),
 "infer_ms":times,"infer_median_ms":float(np.median(times)),
 "status":"PASS_NPU_COMPILED_GRAPH_EXPORT_IMPORT_STREAM"
},indent=2),flush=True)
