#!/usr/bin/env python3
from __future__ import annotations

import argparse, math

MARKERS=[
 "child_science_window_start_ns","prefill_wall_start_ns","prefill_wall_end_ns",
 "decode_wall_start_ns","decode_wall_end_ns","child_science_window_end_ns",
]

def phase_decompose(markers:dict, wall_ns:int, g_prefill_ns:int, g_decode_ns:int)->dict:
    vals=[]
    for k in MARKERS:
        v=markers.get(k)
        if not isinstance(v,(int,float)) or not math.isfinite(float(v)):
            raise ValueError(f"invalid marker {k}")
        vals.append(int(v))
    if any(b<=a for a,b in zip(vals,vals[1:])):
        raise ValueError("phase markers not strictly ordered")
    if wall_ns<=0 or g_prefill_ns<=0 or g_decode_ns<=0:
        raise ValueError("wall/G durations must be positive")

    start,pfs,pfe,ds,de,end=vals
    window=end-start
    prefill=pfe-pfs
    decode_transition=ds-pfe
    decode_wall=de-ds
    decode_phase=de-pfe

    p0=wall_ns-window
    p1=pfs-start
    p2=prefill-g_prefill_ns
    p3=decode_phase-g_decode_ns
    p4=end-de

    g=g_prefill_ns+g_decode_ns
    h=wall_ns-g
    phases={
      "P0_PROCESS_ENVELOPE":p0,
      "P1_PRE_TOKEN":p1,
      "P2_PREFILL_HOST":p2,
      "P3_DECODE_HOST":p3,
      "P4_POST_TOKEN":p4,
    }

    if min([window,p0,p1,p2,p3,p4,decode_transition,decode_wall])<0:
        raise ValueError("negative qualified phase duration")

    closure_window=window-(p1+prefill+decode_phase+p4)
    closure_h=h-sum(phases.values())

    return {
      "child_science_window_ns":window,
      "prefill_wall_ns":prefill,
      "decode_transition_ns":decode_transition,
      "decode_wall_ns":decode_wall,
      "decode_phase_ns":decode_phase,
      "G_prefill_ns":g_prefill_ns,
      "G_decode_ns":g_decode_ns,
      "G_total_ns":g,
      "H_ns":h,
      "phases_ns":phases,
      "closure_window_ns":closure_window,
      "closure_H_ns":closure_h,
    }

def self_test()->int:
    m={
      "child_science_window_start_ns":1000,
      "prefill_wall_start_ns":2000,
      "prefill_wall_end_ns":5000,
      "decode_wall_start_ns":6000,
      "decode_wall_end_ns":16000,
      "child_science_window_end_ns":18000,
    }
    r=phase_decompose(m,20000,1000,4000)
    assert r["decode_transition_ns"]==1000
    assert r["phases_ns"]=={
      "P0_PROCESS_ENVELOPE":3000,
      "P1_PRE_TOKEN":1000,
      "P2_PREFILL_HOST":2000,
      "P3_DECODE_HOST":7000,
      "P4_POST_TOKEN":2000,
    }
    assert r["H_ns"]==15000
    assert r["closure_window_ns"]==0
    assert r["closure_H_ns"]==0

    bad=dict(m);bad["decode_wall_start_ns"]=5000
    try: phase_decompose(bad,20000,1000,4000)
    except ValueError: pass
    else: raise SystemExit("CORE0E marker-order negative fixture failed")

    try: phase_decompose(m,20000,3500,9000)
    except ValueError: pass
    else: raise SystemExit("CORE0E negative phase residual fixture failed")

    print("CORE0E_PHASE_CONTRACT_SELF_TEST=PASS")
    return 0

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:return self_test()
    return 0

if __name__=="__main__":
    raise SystemExit(main())
