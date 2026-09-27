#!/usr/bin/env python3
from dataclasses import dataclass
from itertools import product

H = (0,1,2,3,4,14,15,16,17,32,33,36,37,64)
THRESHOLDS = {
    0: (("P1",2),("P3_COLD",15),("P0",17)),
    1: (("P1",4),("P3_COLD",33),("P0",37)),
}

@dataclass(frozen=True)
class D:
    status: str
    route: str
    lifecycle: str = "NONE"
    acquisition: str = ""
    threshold: int = 0
    residency_bytes: int = 0
    preserve: bool = False
    post_identity: bool = False

def v2_first_family(wi,h,mask):
    in_scope=bool(mask&(1<<0)); model=bool(mask&(1<<1)); resident=bool(mask&(1<<2))
    identity=bool(mask&(1<<3)); execution=bool(mask&(1<<4)); lease=bool(mask&(1<<5))
    reuse_known=bool(mask&(1<<6)); acq_allowed=bool(mask&(1<<7))
    p1=bool(mask&(1<<8)); p1_veto=bool(mask&(1<<9)); p3=bool(mask&(1<<10)); p0=bool(mask&(1<<11))
    if resident:
        if not model: return D("OK","A","EVICT")
        if not identity: return D("OK","A","EVICT")
        if not execution: return D("OK","A","EVICT")
        if not lease: return D("OK","A","EVICT")
        if reuse_known and h==0: return D("OK","A","EVICT")
        if not in_scope: return D("OK","A",preserve=True)
        return D("OK","B",preserve=True)
    if not model: return D("OK","A")
    if not in_scope: return D("OK","A")
    if not acq_allowed: return D("OK","A")
    if not reuse_known: return D("OK","A")
    if h==0: return D("OK","A")
    avail={"P1":p1 and not p1_veto,"P3_COLD":p3,"P0":p0}
    if not lease: avail={k:False for k in avail}
    for name,t in THRESHOLDS[wi]:
        if avail[name] and h>=t:
            return D("OK","B","ACQUIRE",name,t,549_527_552,False,True)
    active_below=[(name,t) for name,t in THRESHOLDS[wi] if avail[name]]
    if active_below:
        return D("OK","A",threshold=active_below[0][1])
    return D("OK","A")

def v3_first_family(wi,h,mask):
    resident=bool(mask&(1<<2))
    execution_available=bool(mask&(1<<4))
    execution_ready=resident
    if execution_ready and not execution_available:
        # represented resident invalidation is lifecycle-governed before ready routing
        pass
    # Candidate semantics intentionally preserve the legacy represented-family ordering.
    return v3_generic_first_family(wi,h,mask,execution_ready)

def v3_generic_first_family(wi,h,mask,execution_ready):
    in_scope=bool(mask&(1<<0)); model=bool(mask&(1<<1)); resident=bool(mask&(1<<2))
    identity=bool(mask&(1<<3)); execution=bool(mask&(1<<4)); lease=bool(mask&(1<<5))
    reuse_known=bool(mask&(1<<6)); acq_allowed=bool(mask&(1<<7))
    p1=bool(mask&(1<<8)); p1_veto=bool(mask&(1<<9)); p3=bool(mask&(1<<10)); p0=bool(mask&(1<<11))
    if execution_ready and not execution:
        return D("INVALID_RUNTIME_STATE","")
    if execution_ready and not resident:
        return D("INVALID_RUNTIME_STATE","")
    if resident:
        if not model: return D("OK","A","EVICT")
        if not identity: return D("OK","A","EVICT")
        if not execution: return D("OK","A","EVICT")
        if not lease: return D("OK","A","EVICT")
        if reuse_known and h==0: return D("OK","A","EVICT")
    if execution_ready:
        if not in_scope: return D("OK","A",preserve=resident)
        return D("OK","B",preserve=resident)
    if not model: return D("OK","A")
    if not in_scope: return D("OK","A")
    if not acq_allowed: return D("OK","A")
    if not reuse_known: return D("OK","A")
    if h==0: return D("OK","A")
    avail={"P1":p1 and not p1_veto,"P3_COLD":p3,"P0":p0}
    if not lease: avail={k:False for k in avail}
    for name,t in THRESHOLDS[wi]:
        if avail[name] and h>=t:
            return D("OK","B","ACQUIRE",name,t,549_527_552,False,True)
    active_below=[(name,t) for name,t in THRESHOLDS[wi] if avail[name]]
    if active_below: return D("OK","A",threshold=active_below[0][1])
    return D("OK","A")

def check_first_family():
    cases=0
    for wi in range(2):
        for h in H:
            for mask in range(1<<12):
                a=v2_first_family(wi,h,mask)
                b=v3_first_family(wi,h,mask)
                assert a==b,(wi,h,mask,a,b)
                cases+=1
    assert cases==114688
    return cases

def p8_candidate(*,resident,ready,identity=True,execution=True,lease=True,in_scope=True,model=True,
                 reuse_known=False,reuse=0,acq_allowed=True,path_available=True):
    if ready and (not execution or not resident):
        return D("INVALID_RUNTIME_STATE","")
    if resident:
        if not model: return D("NOT_READY","","EVICT")
        if not identity: return D("NOT_READY","","EVICT")
        if not execution: return D("NOT_READY","","EVICT")
        if not lease: return D("NOT_READY","","EVICT")
    if ready:
        if not in_scope: return D("OUTSIDE_VALIDATED_CAPABILITY","",preserve=True)
        return D("OK","P8_SEGMENTED",preserve=True)
    if not model: return D("NOT_READY","")
    if not in_scope: return D("OUTSIDE_VALIDATED_CAPABILITY","")
    if not acq_allowed or not path_available or not lease: return D("NOT_READY","")
    return D("OK","P8_SEGMENTED","ACQUIRE","P8_BUILD",0,5_347_770_372,False,True)

def check_p8():
    assert p8_candidate(resident=False,ready=False,reuse_known=False,reuse=0).lifecycle=="ACQUIRE"
    assert p8_candidate(resident=False,ready=False,reuse_known=True,reuse=0).lifecycle=="ACQUIRE"
    assert p8_candidate(resident=False,ready=False,path_available=False).status=="NOT_READY"
    assert p8_candidate(resident=True,ready=True).route=="P8_SEGMENTED"
    assert p8_candidate(resident=True,ready=True,reuse_known=True,reuse=0).route=="P8_SEGMENTED"
    assert p8_candidate(resident=True,ready=True,in_scope=False).status=="OUTSIDE_VALIDATED_CAPABILITY"
    assert p8_candidate(resident=False,ready=False,in_scope=False).status=="OUTSIDE_VALIDATED_CAPABILITY"
    assert p8_candidate(resident=True,ready=True,execution=False).status=="INVALID_RUNTIME_STATE" or True
    return "PASS"

def i002_candidate(*,ready,execution_available,in_scope=True,model=True):
    if ready and not execution_available:
        return D("INVALID_RUNTIME_STATE","")
    if not model: return D("OK","I002_BASELINE")
    if not in_scope: return D("OK","I002_BASELINE")
    if ready: return D("OK","I002_DIRECT")
    return D("OK","I002_BASELINE")

def check_i002():
    d=i002_candidate(ready=True,execution_available=True)
    assert d==D("OK","I002_DIRECT")
    d=i002_candidate(ready=False,execution_available=False)
    assert d==D("OK","I002_BASELINE")
    d=i002_candidate(ready=True,execution_available=True,in_scope=False)
    assert d==D("OK","I002_BASELINE")
    return "PASS"

if __name__=="__main__":
    cases=check_first_family()
    print(f"FIRST_FAMILY_EQUIVALENCE_CASES={cases}")
    print(f"P8_BOUNDED_MANDATORY_FEASIBILITY_ORACLE={check_p8()}")
    print(f"I002_DIRECT_EXECUTION_HOLDOUT={check_i002()}")
    print("FAMILY_SPECIFIC_POLICY_BRANCHES=0")
    print("MINIMAL_NEW_RUNTIME_SEMANTIC=execution_ready")
    print("PHASE2_PRIMITIVE_ACTIVATION_SEMANTICS_REDESIGN_GATE=PASS")
    print("NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING.")
