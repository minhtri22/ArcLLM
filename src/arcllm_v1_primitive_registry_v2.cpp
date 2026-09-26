#include "../include/arcllm/v1/primitive_registry_v2.h"

namespace arcllm::v1::registry_v2 {
namespace {

void set_error(RegistryError* e, RegistryStatus s, const char* c, std::uint64_t id=0) noexcept {
    if(e){e->status=s;e->category=c;e->id=id;}
}
bool ptr_ok(const void* p,std::size_t n) noexcept {return n==0||p!=nullptr;}

template<class D,class F>
RegistryStatus local_ids(const D* x,std::size_t n,F id,RegistryError* e,const char* cat) noexcept {
    for(std::size_t i=0;i<n;++i){
        auto a=id(x[i]);
        if(!a){set_error(e,RegistryStatus::INVALID_BUNDLE,cat);return RegistryStatus::INVALID_BUNDLE;}
        for(std::size_t j=i+1;j<n;++j) if(a==id(x[j])){
            set_error(e,RegistryStatus::DUPLICATE_ID,cat,a.value);return RegistryStatus::DUPLICATE_ID;
        }
    }
    return RegistryStatus::OK;
}
bool same_threshold(const AcquisitionThresholdDescriptor&a,const AcquisitionThresholdDescriptor&b) noexcept {
    return a.capability==b.capability&&a.profile==b.profile&&a.acquisition==b.acquisition;
}

} // namespace

const RegistrationBundle* PrimitiveRegistry::bundle_at(std::size_t i) const noexcept {
    return i<bundle_count_?bundles_[i]:nullptr;
}

#define FIND_IMPL(NAME,TYPE,IDTYPE,PTR,COUNT) const TYPE* PrimitiveRegistry::NAME(IDTYPE id) const noexcept {     if(!id) return nullptr;     for(std::size_t b=0;b<bundle_count_;++b){const auto&x=*bundles_[b];         for(std::size_t i=0;i<x.COUNT;++i) if(x.PTR[i].id==id) return &x.PTR[i];}     return nullptr; }
FIND_IMPL(find_domain,DomainDescriptor,DomainId,domains,domain_count)
FIND_IMPL(find_reuse_metric,ReuseMetricDescriptor,ReuseMetricId,reuse_metrics,reuse_metric_count)
FIND_IMPL(find_evidence_set,EvidenceSetDescriptor,EvidenceSetId,evidence_sets,evidence_set_count)
FIND_IMPL(find_profile,EvidenceProfileDescriptor,EvidenceProfileId,profiles,profile_count)
FIND_IMPL(find_family,PrimitiveFamilyDescriptor,PrimitiveFamilyId,families,family_count)
FIND_IMPL(find_capability,CapabilityDescriptor,CapabilityId,capabilities,capability_count)
FIND_IMPL(find_primitive,PrimitiveDescriptor,PrimitiveId,primitives,primitive_count)
FIND_IMPL(find_acquisition,AcquisitionPathDescriptor,AcquisitionPathId,acquisitions,acquisition_count)
#undef FIND_IMPL

const AcquisitionThresholdDescriptor* PrimitiveRegistry::find_threshold(
    CapabilityId c,EvidenceProfileId p,AcquisitionPathId a) const noexcept {
    for(std::size_t b=0;b<bundle_count_;++b){const auto&x=*bundles_[b];
        for(std::size_t i=0;i<x.threshold_count;++i){const auto&t=x.thresholds[i];
            if(t.capability==c&&t.profile==p&&t.acquisition==a) return &t;}}
    return nullptr;
}
const LifecycleDescriptor* PrimitiveRegistry::find_lifecycle(PrimitiveId p) const noexcept {
    for(std::size_t b=0;b<bundle_count_;++b){const auto&x=*bundles_[b];
        for(std::size_t i=0;i<x.lifecycle_count;++i) if(x.lifecycles[i].primitive==p) return &x.lifecycles[i];}
    return nullptr;
}

RegistryStatus PrimitiveRegistry::add_bundle(const RegistrationBundle& x, RegistryError* e) noexcept {
    set_error(e,RegistryStatus::OK,nullptr);
    if(bundle_count_>=kMaxBundles){set_error(e,RegistryStatus::CAPACITY_EXCEEDED,"bundle");return RegistryStatus::CAPACITY_EXCEEDED;}
    if(!x.bundle_name||!ptr_ok(x.domains,x.domain_count)||!ptr_ok(x.reuse_metrics,x.reuse_metric_count)||
       !ptr_ok(x.evidence_sets,x.evidence_set_count)||!ptr_ok(x.profiles,x.profile_count)||
       !ptr_ok(x.families,x.family_count)||!ptr_ok(x.capabilities,x.capability_count)||
       !ptr_ok(x.primitives,x.primitive_count)||!ptr_ok(x.acquisitions,x.acquisition_count)||
       !ptr_ok(x.thresholds,x.threshold_count)||!ptr_ok(x.lifecycles,x.lifecycle_count)){
        set_error(e,RegistryStatus::INVALID_BUNDLE,"bundle");return RegistryStatus::INVALID_BUNDLE;
    }

    RegistryStatus s;
    if((s=local_ids(x.domains,x.domain_count,[](const auto&v){return v.id;},e,"domain"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.reuse_metrics,x.reuse_metric_count,[](const auto&v){return v.id;},e,"reuse_metric"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.evidence_sets,x.evidence_set_count,[](const auto&v){return v.id;},e,"evidence_set"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.profiles,x.profile_count,[](const auto&v){return v.id;},e,"profile"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.families,x.family_count,[](const auto&v){return v.id;},e,"family"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.capabilities,x.capability_count,[](const auto&v){return v.id;},e,"capability"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.primitives,x.primitive_count,[](const auto&v){return v.id;},e,"primitive"))!=RegistryStatus::OK)return s;
    if((s=local_ids(x.acquisitions,x.acquisition_count,[](const auto&v){return v.id;},e,"acquisition"))!=RegistryStatus::OK)return s;

    for(std::size_t i=0;i<x.threshold_count;++i) for(std::size_t j=i+1;j<x.threshold_count;++j)
        if(same_threshold(x.thresholds[i],x.thresholds[j])){
            set_error(e,RegistryStatus::DUPLICATE_POLICY_KEY,"threshold");return RegistryStatus::DUPLICATE_POLICY_KEY;
        }
    for(std::size_t i=0;i<x.lifecycle_count;++i) for(std::size_t j=i+1;j<x.lifecycle_count;++j)
        if(x.lifecycles[i].primitive==x.lifecycles[j].primitive){
            set_error(e,RegistryStatus::DUPLICATE_POLICY_KEY,"lifecycle",x.lifecycles[i].primitive.value);
            return RegistryStatus::DUPLICATE_POLICY_KEY;
        }

    for(std::size_t i=0;i<x.domain_count;++i)if(find_domain(x.domains[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"domain",x.domains[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.reuse_metric_count;++i)if(find_reuse_metric(x.reuse_metrics[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"reuse_metric",x.reuse_metrics[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.evidence_set_count;++i)if(find_evidence_set(x.evidence_sets[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"evidence_set",x.evidence_sets[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.profile_count;++i)if(find_profile(x.profiles[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"profile",x.profiles[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.family_count;++i)if(find_family(x.families[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"family",x.families[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.capability_count;++i)if(find_capability(x.capabilities[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"capability",x.capabilities[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.primitive_count;++i)if(find_primitive(x.primitives[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"primitive",x.primitives[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t i=0;i<x.acquisition_count;++i)if(find_acquisition(x.acquisitions[i].id)){set_error(e,RegistryStatus::DUPLICATE_ID,"acquisition",x.acquisitions[i].id.value);return RegistryStatus::DUPLICATE_ID;}
    for(std::size_t b=0;b<bundle_count_;++b){const auto&old=*bundles_[b];
        for(std::size_t i=0;i<x.threshold_count;++i)for(std::size_t j=0;j<old.threshold_count;++j)
            if(same_threshold(x.thresholds[i],old.thresholds[j])){
                set_error(e,RegistryStatus::DUPLICATE_POLICY_KEY,"threshold");return RegistryStatus::DUPLICATE_POLICY_KEY;
            }
        for(std::size_t i=0;i<x.lifecycle_count;++i)for(std::size_t j=0;j<old.lifecycle_count;++j)
            if(x.lifecycles[i].primitive==old.lifecycles[j].primitive){
                set_error(e,RegistryStatus::DUPLICATE_POLICY_KEY,"lifecycle",x.lifecycles[i].primitive.value);
                return RegistryStatus::DUPLICATE_POLICY_KEY;
            }
    }

    bundles_[bundle_count_++]=&x;
    auto rollback=[&]() noexcept {--bundle_count_;bundles_[bundle_count_]=nullptr;};
    auto invalid=[&](RegistryStatus s2,const char*cat,std::uint64_t id=0) noexcept {
        rollback();set_error(e,s2,cat,id);return s2;
    };

    for(std::size_t i=0;i<x.profile_count;++i){
        const auto&p=x.profiles[i];
        if(!find_domain(p.domain)||!find_evidence_set(p.evidence_set))
            return invalid(RegistryStatus::INVALID_REFERENCE,"profile_reference",p.id.value);
        if(p.reuse_metric&&!find_reuse_metric(p.reuse_metric))
            return invalid(RegistryStatus::INVALID_REFERENCE,"profile_reuse_metric",p.id.value);
    }

    for(std::size_t i=0;i<x.family_count;++i){
        const auto&f=x.families[i];
        if(!find_domain(f.domain))
            return invalid(RegistryStatus::INVALID_REFERENCE,"family_domain",f.id.value);
    }

    for(std::size_t i=0;i<x.primitive_count;++i){
        const auto&p=x.primitives[i];
        const auto*f=find_family(p.family);
        const auto*c=find_capability(p.capability);
        if(!f||!c||f->domain!=c->domain)
            return invalid(RegistryStatus::INVALID_REFERENCE,"primitive_reference",p.id.value);
        const bool requires_residency=(p.flags&PRIMITIVE_REQUIRES_RESIDENCY)!=0;
        const bool no_extra=(p.flags&PRIMITIVE_NO_EXTRA_REPRESENTATION)!=0;
        if(requires_residency&&(!p.representation||p.residency_bytes==0))
            return invalid(RegistryStatus::INVALID_REFERENCE,"primitive_residency",p.id.value);
        if(no_extra&&(p.representation||p.residency_bytes!=0))
            return invalid(RegistryStatus::INVALID_REFERENCE,"primitive_no_extra_representation",p.id.value);
    }

    for(std::size_t i=0;i<x.capability_count;++i){
        const auto&c=x.capabilities[i];
        if(!find_domain(c.domain))
            return invalid(RegistryStatus::INVALID_REFERENCE,"capability_domain",c.id.value);
        const auto*preferred=find_primitive(c.preferred_primitive);
        if(!preferred||preferred->capability!=c.id)
            return invalid(RegistryStatus::INVALID_REFERENCE,"capability_preferred",c.id.value);
        if(c.fallback_primitive){
            const auto*fallback=find_primitive(c.fallback_primitive);
            if(!fallback||fallback->capability!=c.id)
                return invalid(RegistryStatus::INVALID_REFERENCE,"capability_fallback",c.id.value);
        }
    }

    for(std::size_t i=0;i<x.acquisition_count;++i){
        const auto&a=x.acquisitions[i];
        if(!find_primitive(a.target_primitive))
            return invalid(RegistryStatus::INVALID_REFERENCE,"acquisition_target",a.id.value);
        if(a.trigger==AcquisitionTrigger::MANDATORY_FOR_FEASIBILITY){
            for(std::size_t j=0;j<x.threshold_count;++j)
                if(x.thresholds[j].acquisition==a.id)
                    return invalid(RegistryStatus::INVALID_ACQUISITION_SEMANTICS,"mandatory_path_has_threshold",a.id.value);
            for(std::size_t b=0;b+1<bundle_count_;++b){const auto&old=*bundles_[b];
                for(std::size_t j=0;j<old.threshold_count;++j)
                    if(old.thresholds[j].acquisition==a.id)
                        return invalid(RegistryStatus::INVALID_ACQUISITION_SEMANTICS,"mandatory_path_has_threshold",a.id.value);
            }
        }
    }

    for(std::size_t i=0;i<x.threshold_count;++i){
        const auto&t=x.thresholds[i];
        const auto*c=find_capability(t.capability);
        const auto*p=find_profile(t.profile);
        const auto*a=find_acquisition(t.acquisition);
        const auto*m=find_reuse_metric(t.reuse_metric);
        if(!c||!p||!a||!m||p->domain!=c->domain||!p->reuse_metric||p->reuse_metric!=t.reuse_metric)
            return invalid(RegistryStatus::INVALID_REFERENCE,"threshold_reference");
        if(a->trigger!=AcquisitionTrigger::REUSE_AMORTIZED)
            return invalid(RegistryStatus::INVALID_ACQUISITION_SEMANTICS,"threshold_on_non_amortized_path",a->id.value);
        const auto*target=find_primitive(a->target_primitive);
        if(!target||target->capability!=c->id)
            return invalid(RegistryStatus::INVALID_REFERENCE,"threshold_target");
    }

    for(std::size_t i=0;i<x.lifecycle_count;++i)
        if(!find_primitive(x.lifecycles[i].primitive))
            return invalid(RegistryStatus::INVALID_REFERENCE,"lifecycle_primitive",x.lifecycles[i].primitive.value);

    return RegistryStatus::OK;
}

} // namespace arcllm::v1::registry_v2
