import argparse,json,re
from pathlib import Path

NORMALIZERS=[
 ("dram_read_bytes", [r"\bdram\b.*read.*byte",r"system memory.*read.*byte",r"memory read.*byte"]),
 ("dram_write_bytes",[r"\bdram\b.*write.*byte",r"system memory.*write.*byte",r"memory write.*byte"]),
 ("occupancy_percent",[r"occupancy"]),
 ("compute_active_percent",[r"(xve|eu|vector).*active",r"compute.*active"]),
 ("xve_stalled_percent",[r"(xve|eu|vector).*stall"]),
 ("xve_idle_percent",[r"(xve|eu|vector).*idle"]),
 ("l3_read_bytes",[r"\bl3\b.*read.*byte"]),
 ("l3_write_bytes",[r"\bl3\b.*write.*byte"]),
 ("l3_miss_percent",[r"\bl3\b.*miss.*(percent|ratio|rate)"]),
 ("memory_stall_percent",[r"memory.*stall",r"send.*stall"]),
]
def norm(c):
    s=" ".join([str(c.get("name","")),str(c.get("category","")),str(c.get("description",""))]).lower()
    for name,pats in NORMALIZERS:
        if any(re.search(p,s,re.I) for p in pats): return name
    return None

ap=argparse.ArgumentParser()
ap.add_argument("--raw",required=True)
ap.add_argument("--vtune-json",required=True)
ap.add_argument("--out",required=True)
a=ap.parse_args()
raw=json.loads(Path(a.raw).read_text(encoding="utf-8"))
vt=json.loads(Path(a.vtune_json).read_text(encoding="utf-8"))
mapped=[]
for c in raw.get("counters",[]):
    x=dict(c);x["normalized_candidate"]=norm(c);mapped.append(x)
k=raw["vk_khr_performance_query"]
khr_ok=bool(k["extension_present"] and k["performance_counter_query_pools_feature"] and k["counter_count"]>0)
mapped_names=sorted({x["normalized_candidate"] for x in mapped if x["normalized_candidate"]})
vtune_ok=bool(vt.get("available"))
if khr_ok:
    preferred="VULKAN_KHR_PERFORMANCE_QUERY"
    reason="Driver exposes VK_KHR_performance_query with usable counter enumeration; prefer in-runtime scoped evidence."
elif vtune_ok:
    preferred="INTEL_VTUNE"
    reason="Vulkan performance-query counters unavailable; VTune detected as vendor counter provider."
else:
    preferred=None
    reason="No counter provider qualified by M3-A; install/enable a provider before M3-B."

out={
 "schema_version":"0.1",
 "artifact_type":"COUNTER_CAPABILITY",
 "probe_id":"arcllm-v1-m3a-arc140v",
 "hardware_profile_id":"intel.core_ultra_7_258v.arc_140v.devhost_32gib.v0.1",
 "providers":[
  {"kind":"VULKAN_KHR_PERFORMANCE_QUERY","available":khr_ok,"version":None,
   "supports":{"extension_present":k["extension_present"],"feature":k["performance_counter_query_pools_feature"],
    "multiple_query_pools":k["performance_counter_multiple_query_pools_feature"],
    "allow_command_buffer_query_copies":k["allow_command_buffer_query_copies"],
    "queue_family":raw["queue_family"],"passes_for_all_counters":k["passes_for_all_counters"],
    "normalized_candidates":mapped_names},
   "counters":mapped},
  {"kind":"INTEL_VTUNE","available":vtune_ok,"version":vt.get("version"),
   "supports":{"path":vt.get("path"),"scope_note":"vendor sampled/token-run scope unless Vulkan task correlation is independently proven"},
   "counters":[]},
  {"kind":"INTEL_GPA","available":False,"version":None,
   "supports":{"status":"NOT_FOUNDATIONAL_PROVIDER","reason":"Intel GPA 2025.1 is EOL/discontinued in 2026"},"counters":[]}
 ],
 "selection":{"preferred_provider":preferred,"reason":reason,"quiet_host_required_for_collection":True},
 "notes":[
  "M3-A is capability-only and consumes no model inference/counter science.",
  "Counter normalization is candidate mapping only until M3-B validates units/scope semantics.",
  "Provider-native names remain authoritative raw evidence."
 ]
}
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print("M3_A_SUMMARY=PASS")
print("PREFERRED_PROVIDER="+str(preferred))
print("NORMALIZED_CANDIDATES="+",".join(mapped_names))
