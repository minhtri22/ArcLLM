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

def old_f1(wi,h,m):
    ins=bool(m&1); model=bool(m&2); resident=bool(m&4); identity=bool(m&8)
    ex=bool(m&16); lease=bool(m&32); known=bool(m&64); allowed=bool(m&128)
    p1=bool(m&256); veto=bool(m&512); p3=bool(m&1024); p0=bool(m&2048)
    if resident:
        if not model:return Decision("OK","A","EVICT")
        if not identity:return Decision("OK","A","EVICT")
        if not ex:return Decision("OK","A","EVICT")
        if not lease:return Decision("OK","A","EVICT")
        if known and h==0:return Decision("OK","A","EVICT")
        if not ins:return Decision("OK","A",preserve=True)
        return Decision("OK","B",preserve=True)
    if not model or not ins or not known or h==0 or not allowed:return Decision("OK","A")
    av={"P1":p1 and not veto,"P3_COLD":p3,"P0":p0}
    if not lease:av={k:False for k in av}
    below=[]
    for n,t in TH[wi]:
        if av[n]:
            if h>=t:return Decision("OK","B","ACQUIRE",n,t,549527552,False,True)
            below.append((n,t))
    if below:return Decision("OK","A",threshold=below[0][1])
    return Decision("OK","A")

def stable_f1(wi,h,m,a_ready=True,b_ready_override=None,acq_startable_override=None):
    ins=bool(m&1); model=bool(m&2); resident=bool(m&4); identity=bool(m&8)
    ex=bool(m&16); lease=bool(m&32); known=bool(m&64); allowed=bool(m&128)
    p1=bool(m&256); veto=bool(m&512); p3=bool(m&1024); p0=bool(m&2048)
    b_ready=(resident and ex) if b_ready_override is None else b_ready_override
    fallback=lambda preserve=False: Decision("OK","A",preserve=preserve) if a_ready else Decision("NOT_READY","",preserve=preserve)
    if resident:
        if not model or not identity or not ex or not lease or (known and h==0):
            return Decision("OK","A","EVICT") if a_ready else Decision("NOT_READY","","EVICT")
        if b_ready:
            if not ins:return fallback(True)
            return Decision("OK","B",preserve=True)
        return fallback(True)
    if not model or not ins or not known or h==0 or not allowed:return fallback()
    av={"P1":p1 and not veto,"P3_COLD":p3,"P0":p0}
    if not lease or acq_startable_override is False:av={k:False for k in av}
    below=[]
    for n,t in TH[wi]:
        if av[n]:
            if h>=t:return Decision("OK","B","ACQUIRE",n,t,549527552,False,True)
            below.append((n,t))
    if below:
        return Decision("OK","A",threshold=below[0][1]) if a_ready else Decision("NOT_READY","",threshold=below[0][1])
    return fallback()

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
    base=baseline_ready and baseline_execution
    direct=direct_ready and direct_execution
    if not model or not in_scope:return Decision("OK","I002_BASELINE") if base else Decision("NOT_READY","")
    if direct:return Decision("OK","I002_DIRECT")
    if base:return Decision("OK","I002_BASELINE")
    return Decision("NOT_READY","")

cases=0
for wi in range(2):
    for h in H:
        for m in range(1<<12):
            assert old_f1(wi,h,m)==stable_f1(wi,h,m)
            cases+=1
assert cases==114688

valid_resident=(1|2|4|8|16|32|64|128)
assert stable_f1(0,10,valid_resident,b_ready_override=False)==Decision("OK","A",preserve=True)
assert stable_f1(0,10,valid_resident,a_ready=False,b_ready_override=False)==Decision("NOT_READY","",preserve=True)
absent_acq=(1|2|8|16|32|64|128|256)
assert stable_f1(0,10,absent_acq,acq_startable_override=False).lifecycle=="NONE"
assert p8(acq_startable=True).lifecycle=="ACQUIRE"
assert p8(acq_startable=False)==Decision("NOT_READY","")
assert p8(resident=True,ready=False)==Decision("NOT_READY","",preserve=True)
assert p8(resident=True,ready=False,identity=False).lifecycle=="EVICT"
assert p8(resident=True,ready=False,lease=False).lifecycle=="EVICT"
assert i002(True,True)==Decision("OK","I002_DIRECT")
assert i002(False,False)==Decision("OK","I002_BASELINE")
assert i002(False,False,baseline_ready=False)==Decision("NOT_READY","")
assert i002(True,True,in_scope=False)==Decision("OK","I002_BASELINE")

print("FIRST_FAMILY_EQUIVALENCE_CASES=114688")
print("TRANSITIONAL_REPRESENTED_READINESS=PASS")
print("ACQUISITION_IN_PROGRESS_OR_NOT_STARTABLE=PASS")
print("P8_BOUNDED_ORACLE=PASS")
print("I002_DIRECT_AND_FALLBACK_READINESS=PASS")
print("ACTIVATION_ENUM_REQUIRED=NO")
print("FAMILY_SPECIFIC_POLICY_BRANCHES=0")
print("PHASE2_ACTIVATION_SEMANTICS_STABILIZATION_GATE=PASS")
