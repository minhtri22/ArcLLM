#include "../include/arcllm/v1/generic_policy_engine_v2.h"
#include <cstdlib>
#include <iostream>

namespace reg=arcllm::v1::registry_v2;
namespace gp=arcllm::v1::policy_v2;
namespace base=arcllm::v1::registry;

static void req(bool c,const char*m){if(!c){std::cerr<<"DIRECT_HOLDOUT_HARNESS_FAIL: "<<m<<"\n";std::exit(2);}}

namespace i002 {
constexpr base::DomainId D{0xD9000001ull};
constexpr base::EvidenceSetId E{0xE9000001ull};
constexpr base::EvidenceProfileId P{0x59000001ull};
constexpr base::PrimitiveFamilyId F{0xF9000001ull};
constexpr base::CapabilityId C{0x19000001ull};
constexpr base::PrimitiveId BASELINE{0xA9000001ull};
constexpr base::PrimitiveId DIRECT{0xB9000001ull};
constexpr base::ProvenanceRef PROV{
 "artifacts/ARCLLM_V1/ARCLLM_V1_I002_FINAL_ADJUDICATION_v0.1.json",
 "5d3d853247900bf0ea19fbf04f8e7a89412b9ebb"
};

constexpr base::DomainDescriptor domains[]={{D,base::ValidationState::VALIDATED,"I002_FROZEN_DIRECT_EXECUTION_DOMAIN",PROV}};
constexpr base::EvidenceSetDescriptor evidence[]={{E,base::ValidationState::VALIDATED,"I002_REAL_MODEL_CARRY_THROUGH",PROV}};
constexpr reg::EvidenceProfileDescriptor profiles[]={{P,D,{},E,base::ValidationState::VALIDATED,"I002_DIRECT_PROFILE",PROV}};
constexpr base::PrimitiveFamilyDescriptor families[]={{F,D,base::ValidationState::VALIDATED,"I002_DIRECT_EXECUTION_FAMILY",PROV}};
constexpr reg::CapabilityDescriptor caps[]={{C,D,DIRECT,BASELINE,base::ValidationState::VALIDATED,"I002_GATE_UP_DIRECT_CAPABILITY",PROV}};
constexpr base::PrimitiveDescriptor primitives[]={
 {BASELINE,F,C,{},0,base::PRIMITIVE_FALLBACK|base::PRIMITIVE_NO_EXTRA_REPRESENTATION,base::ValidationState::VALIDATED,"I002_BASELINE_GATE_UP",PROV},
 {DIRECT,F,C,{},0,base::PRIMITIVE_NO_EXTRA_REPRESENTATION,base::ValidationState::VALIDATED,"I002_SUBGROUP32_SPLITK_DIRECT",PROV}
};
constexpr reg::RegistrationBundle bundle{
 "I002_DIRECT_EXECUTION_HOLDOUT",
 domains,1,nullptr,0,evidence,1,profiles,1,families,1,caps,1,
 primitives,2,nullptr,0,nullptr,0,nullptr,0
};
}

static gp::PolicyRequest make_request(
 gp::PrimitiveRuntimeState* ps,std::size_t n,bool in_scope=true){
 gp::PolicyRequest r{};
 r.capability=i002::C;r.profile=i002::P;
 r.request_within_capability_evidence_scope=in_scope;
 r.model_loaded=true;
 r.future_reuse_known=false;
 r.future_reuse_units=0;
 r.acquisition_allowed=true;
 r.primitive_states=ps;r.primitive_state_count=n;
 return r;
}

int main(){
 reg::PrimitiveRegistry registry;reg::RegistryError e{};
 req(registry.add_bundle(i002::bundle,&e)==reg::RegistryStatus::OK,
     "evidence-faithful direct bundle must be structurally valid");

 // Preferred direct implementation is available, but has no residency/acquisition.
 gp::PrimitiveRuntimeState direct_ready[]={{i002::DIRECT,false,true,true,false}};
 auto r=make_request(direct_ready,1,true);
 auto d=gp::evaluate(registry,r);
 req(d.status==gp::PolicyStatus::OK &&
     d.route==i002::BASELINE &&
     d.lifecycle==gp::LifecycleAction::NONE &&
     !d.acquisition,
     "frozen v2 must expose fallback bias for nonresident direct preferred primitive");
 std::cout<<"COUNTEREXAMPLE_DIRECT_READY=FALLBACK_SELECTED\n";

 // If unavailable, fallback is correct.
 direct_ready[0].execution_available=false;
 r=make_request(direct_ready,1,true);
 d=gp::evaluate(registry,r);
 req(d.status==gp::PolicyStatus::OK && d.route==i002::BASELINE,
     "preferred unavailable must route validated baseline");
 std::cout<<"HOLDOUT_FALLBACK_WHEN_UNAVAILABLE=PASS\n";

 // Residency overloading makes v2 select preferred, proving the escape hatch is semantic abuse.
 gp::PrimitiveRuntimeState abused[]={{i002::DIRECT,true,true,true,true}};
 r=make_request(abused,1,true);
 d=gp::evaluate(registry,r);
 req(d.status==gp::PolicyStatus::OK &&
     d.route==i002::DIRECT &&
     d.lifecycle==gp::LifecycleAction::NONE,
     "resident overload should expose how preferred can only be reached");
 std::cout<<"WORKAROUND_REQUIRES_RESIDENCY_OVERLOAD=YES\n";

 // Outside the validated I002 scope, baseline fallback remains the valid route.
 r=make_request(direct_ready,1,false);
 d=gp::evaluate(registry,r);
 req(d.status==gp::PolicyStatus::OK && d.route==i002::BASELINE,
     "outside scope must use baseline");
 std::cout<<"OUTSIDE_SCOPE_FALLBACK=PASS\n";

 std::cout<<"PHASE2_DIRECT_EXECUTION_PRIMITIVE_HOLDOUT_GENERALIZATION_GATE=FAIL_ABSTRACTION_INCOMPLETE\n";
 std::cout<<"MISSING_SEMANTIC=DIRECT_READY_EXECUTION_WITHOUT_RESIDENCY_OR_ACQUISITION\n";
 std::cout<<"FROZEN_V2_GENERIC_FILES_MODIFIED=NO\n";
 std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO NEW PERFORMANCE STUDY.\n";
 return 0;
}
