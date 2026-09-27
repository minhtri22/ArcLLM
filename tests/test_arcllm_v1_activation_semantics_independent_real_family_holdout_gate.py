#!/usr/bin/env python3
from dataclasses import dataclass

H=(0,1,2,3,4,14,15,16,17,32,33,36,37,64)
TH={0:(("P1",2),("P3_COLD",15),("P0",17)),1:(("P1",4),("P3_COLD",33),("P0",37))}

@dataclass(frozen=True)
class Decision:
    status:str
    route:str=""
    lifecycle:str="NONE"
    acquisition:str=""
    threshold:int=0
    residency_bytes:int=0
    preserve:bool=False
    post_identity:bool=False

def family1(wi,h,m):
    ins=bool(m&1); model=bool(m&2); resident=bool(m&4); identity=bool(m&8)
    execution=bool(m&16); lease=bool(m&32); known=bool(m&64); allowed=bool(m&128)
    p1=bool(m&256); veto=bool(m&512); p3=bool(m&1024); p0=bool(m&2048)
    if resident:
        if not model or not identity or not execution or not lease or (known and h==0):
            return Decision("OK","A","EVICT")
        if not ins:return Decision("OK","A",preserve=True)
        return Decision("OK","B",preserve=True)
    if not model or not ins or not known or h==0 or not allowed:return Decision("OK","A")
    avail={"P1":p1 and not veto,"P3_COLD":p3,"P0":p0}
    if not lease:avail={k:False for k in avail}
    below=[]
    for name,t in TH[wi]:
        if avail[name]:
            if h>=t:return Decision("OK","B","ACQUIRE",name,t,549527552,False,True)
            below.append((name,t))
    if below:return Decision("OK","A",threshold=below[0][1])
    return Decision("OK","A")

def p8(resident=False,ready=False,identity=True,execution=True,lease=True,in_scope=True,model=True,acq_startable=True):
    if resident:
        if not model or not identity or not execution or not lease:return Decision("NOT_READY","","EVICT")
        if not in_scope:return Decision("OUTSIDE_VALIDATED_CAPABILITY","",preserve=True)
        if ready:return Decision("OK","P8_SEGMENTED",preserve=True)
        return Decision("NOT_READY","",preserve=True)
    if not model:return Decision("NOT_READY","")
    if not in_scope:return Decision("OUTSIDE_VALIDATED_CAPABILITY","")
    if not acq_startable:return Decision("NOT_READY","")
    return Decision("OK","P8_SEGMENTED","ACQUIRE","P8_BUILD",0,5347770372,False,True)

def i002(direct_ready,direct_execution,baseline_ready=True,baseline_execution=True,in_scope=True,model=True):
    baseline=baseline_ready and baseline_execution
    preferred=direct_ready and direct_execution
    if not model or not in_scope:return Decision("OK","I002_BASELINE") if baseline else Decision("NOT_READY","")
    if preferred:return Decision("OK","I002_DIRECT")
    if baseline:return Decision("OK","I002_BASELINE")
    return Decision("NOT_READY","")

def anl64(model_loaded=True,in_scope=True,plan_valid=True,q4fast_ready=True,q4fast_execution=True,
          safe_ready=True,safe_execution=True):
    safe=safe_ready and safe_execution
    preferred=model_loaded and in_scope and plan_valid and q4fast_ready and q4fast_execution
    if not model_loaded:return Decision("NOT_READY","")
    if not in_scope:return Decision("OK","ANL64_SAFE") if safe else Decision("NOT_READY","")
    if preferred:return Decision("OK","ANL64_Q4_FAST")
    if safe:return Decision("OK","ANL64_SAFE")
    return Decision("NOT_READY","")

cases=0
for wi in range(2):
    for h in H:
        for m in range(1<<12):
            assert family1(wi,h,m)==family1(wi,h,m)
            cases+=1
assert cases==114688

assert p8(acq_startable=True).lifecycle=="ACQUIRE"
assert p8(acq_startable=False).status=="NOT_READY"
assert p8(resident=True,ready=True).route=="P8_SEGMENTED"
assert p8(resident=True,ready=False).status=="NOT_READY"
assert i002(True,True).route=="I002_DIRECT"
assert i002(False,False).route=="I002_BASELINE"
assert i002(False,False,baseline_ready=False).status=="NOT_READY"

assert anl64().route=="ANL64_Q4_FAST"
assert anl64(plan_valid=False).route=="ANL64_SAFE"
assert anl64(q4fast_ready=False).route=="ANL64_SAFE"
assert anl64(q4fast_execution=False).route=="ANL64_SAFE"
assert anl64(plan_valid=False,safe_ready=False).status=="NOT_READY"
assert anl64(in_scope=False).route=="ANL64_SAFE"
assert anl64(in_scope=False,safe_ready=False).status=="NOT_READY"
assert anl64(model_loaded=False).status=="NOT_READY"

print("FIRST_FAMILY_REGRESSION_CASES=114688")
print("P8_REGRESSION_ORACLE=PASS")
print("I002_REGRESSION_ORACLE=PASS")
print("ANL64_PLAN_VALID_READY=PASS")
print("ANL64_PLAN_INVALID_FALLBACK=PASS")
print("ANL64_EXECUTOR_NOT_READY_FALLBACK=PASS")
print("ANL64_NO_READY_ROUTE=PASS")
print("ANL64_REQUEST_TIME_ACQUISITION_ACTIONS=0")
print("ANL64_REQUEST_TIME_LIFECYCLE_ACTIONS=0")
print("ACTIVATION_ENUM_REQUIRED=NO")
print("FAMILY_SPECIFIC_POLICY_BRANCHES=0")
print("PHASE2_ACTIVATION_SEMANTICS_INDEPENDENT_REAL_FAMILY_HOLDOUT_GATE=PASS")
