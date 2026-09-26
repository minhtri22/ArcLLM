#include "../include/arcllm/v1/package_api.h"
#include "../include/arcllm/v1/backend_api.h"
#include <cstdlib>
#include <iostream>

using namespace arcllm::v1::package;

static void req(bool c,const char*m){if(!c){std::cerr<<"PHASE2_FAIL: "<<m<<"\n";std::exit(2);}}

class FakeBackend final : public BackendAdapter {
public:
    BackendStatus acquire_status=BackendStatus::OK;
    BackendStatus validate_status=BackendStatus::OK;
    BackendStatus release_status=BackendStatus::OK;
    BackendStatus resolve_a_status=BackendStatus::OK;
    BackendStatus resolve_b_status=BackendStatus::OK;
    int acquire_calls=0,validate_calls=0,release_calls=0,resolve_a_calls=0,resolve_b_calls=0;
    AcquisitionId last_acquisition=AcquisitionId::NONE;
    std::uint64_t last_bytes=0;

    BackendStatus acquire(AcquisitionId p,std::uint64_t b,RepresentationHandle&out) noexcept override{
        ++acquire_calls;last_acquisition=p;last_bytes=b;
        if(acquire_status==BackendStatus::OK)out.opaque=0xBEEFull;
        return acquire_status;
    }
    BackendStatus validate(RepresentationHandle h,CapabilityId c) noexcept override{
        ++validate_calls;
        req(bool(h),"validate must receive representation");
        req(c==CapabilityId::Q4K_DECODE_FFN_DOWN,"validate capability drift");
        return validate_status;
    }
    BackendStatus release(RepresentationHandle h) noexcept override{
        ++release_calls;req(bool(h),"release must receive representation");return release_status;
    }
    BackendStatus resolve_primitive(PrimitiveId p,PrimitiveHandle&out) noexcept override{
        if(p==PrimitiveId::A_SPLIT_K32){++resolve_a_calls;if(resolve_a_status==BackendStatus::OK)out.opaque=0xA001;return resolve_a_status;}
        ++resolve_b_calls;if(resolve_b_status==BackendStatus::OK)out.opaque=0xB001;return resolve_b_status;
    }
};

static PlanRequest base(EvidenceProfileId profile,std::uint64_t h){
    PlanRequest r{};
    r.profile=profile;
    r.runtime.current_request_in_validated_domain=true;
    r.runtime.model_loaded=true;
    r.runtime.residency_lease_granted=true;
    r.runtime.future_reuse={true,h};
    r.runtime.primary_acquisition_available=true;
    r.runtime.secondary_acquisition_available=true;
    r.runtime.tertiary_acquisition_available=true;
    return r;
}

int main(){
    const auto& d=capability_descriptor();
    req(d.api_version.major==1 && d.api_version.minor==0,"API version");
    req(d.represented_residency_bytes==549527552ull,"residency");
    req(d.profile_count==2,"profile count");
    req(d.profiles[0].primary_create_threshold_tokens==2,"profile0 primary");
    req(d.profiles[1].primary_create_threshold_tokens==4,"profile1 primary");

    auto r=base(EvidenceProfileId::PROFILE_0,1);
    req(plan(r).route==PrimitiveId::A_SPLIT_K32,"profile0 h1 A");
    r.runtime.future_reuse.tokens=2;
    auto primary=plan(r);
    req(primary.route==PrimitiveId::B_EXEC148 && primary.lifecycle==LifecycleAction::ACQUIRE && primary.acquisition==AcquisitionId::GPU_IN_PLACE,"profile0 h2 primary");

    r=base(EvidenceProfileId::PROFILE_1,3);
    req(plan(r).route==PrimitiveId::A_SPLIT_K32,"profile1 h3 A");
    r.runtime.future_reuse.tokens=4;
    req(plan(r).acquisition==AcquisitionId::GPU_IN_PLACE,"profile1 h4 primary");

    r=base(EvidenceProfileId::PROFILE_0,15);
    r.runtime.primary_acquisition_available=false;
    req(plan(r).acquisition==AcquisitionId::COLD_UNBUFFERED_SIDECAR,"secondary mapping");

    r=base(EvidenceProfileId::PROFILE_1,37);
    r.runtime.primary_acquisition_available=false;
    r.runtime.secondary_acquisition_available=false;
    req(plan(r).acquisition==AcquisitionId::CPU_DIRECT,"tertiary mapping");

    r=base(EvidenceProfileId::PROFILE_0,1);
    r.runtime.b_resident=true;r.runtime.b_identity_valid=true;
    req(plan(r).route==PrimitiveId::B_EXEC148 && plan(r).lifecycle==LifecycleAction::NONE,"resident hysteresis");

    r.runtime.current_request_in_validated_domain=false;
    auto outside=plan(r);
    req(outside.route==PrimitiveId::A_SPLIT_K32 && outside.preserve_existing_b && outside.lifecycle==LifecycleAction::NONE,"route/residency separation");

    PlanRequest bad=base(static_cast<EvidenceProfileId>(0xDEADu),100);
    req(plan(bad).status==PlanStatus::UNSUPPORTED_PROFILE && plan(bad).route==PrimitiveId::A_SPLIT_K32,"unsupported profile fail closed");

    FakeBackend b; PackageState st{};
    auto ok=apply_plan(primary,b,st);
    req(ok.ready && !ok.fell_back_to_a && bool(st.b_representation),"acquire success");
    req(b.acquire_calls==1 && b.validate_calls==1 && b.resolve_b_calls==1,"acquire call sequence");
    req(b.last_bytes==549527552ull,"lease bytes");

    FakeBackend vf; PackageState st2{};
    vf.validate_status=BackendStatus::VALIDATION_FAILED;
    auto vr=apply_plan(primary,vf,st2);
    req(vr.fell_back_to_a && vr.ready && !st2.b_representation,"validation failure fallback");
    req(vr.backend_status==BackendStatus::VALIDATION_FAILED,"preserve validation failure");
    req(vf.acquire_calls==1 && vf.validate_calls==1 && vf.release_calls==1 && vf.resolve_a_calls==1,"no hidden retry after validation fail");

    FakeBackend af; PackageState st3{};
    af.acquire_status=BackendStatus::ACQUISITION_FAILED;
    auto ar=apply_plan(primary,af,st3);
    req(ar.fell_back_to_a && af.acquire_calls==1 && af.validate_calls==0 && af.resolve_a_calls==1,"acquire fail no retry");
    req(ar.backend_status==BackendStatus::ACQUISITION_FAILED,"preserve acquisition failure");

    FakeBackend ev; PackageState st4{}; st4.b_representation.opaque=0xBEEF;
    Plan ep{};ep.route=PrimitiveId::A_SPLIT_K32;ep.lifecycle=LifecycleAction::EVICT;
    auto er=apply_plan(ep,ev,st4);
    req(er.fell_back_to_a && !st4.b_representation && ev.release_calls==1,"eviction");

    std::cout<<"PHASE2_PACKAGE_API_ZERO_SCIENCE_QA=PASS\n";
    std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO TIMING. NO PLACEMENT BENCHMARK.\n";
    return 0;
}
