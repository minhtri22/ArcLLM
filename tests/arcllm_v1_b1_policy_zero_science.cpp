#include "../src/arcllm_v1_b1_policy_runtime_hook.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace arcllm::v1::b1;

static void req(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "B1_POLICY_ZERO_SCIENCE_FAIL: " << msg << "\n";
        std::exit(2);
    }
}

static RuntimeSignals base(Workload w) {
    RuntimeSignals s{};
    s.workload = w;
    s.current_request_in_validated_domain = true;
    s.model_loaded = true;
    s.b_resident = false;
    s.b_identity_valid = false;
    s.b_execution_available = true;
    s.residency_lease_granted = true;
    s.future_reuse_known = true;
    s.future_reuse_tokens = 1;
    s.acquisition_allowed = true;
    s.p1_available = true;
    s.p1_vetoed = false;
    s.p3_cold_available = true;
    s.p0_cpu_available = true;
    return s;
}

static void boundary_tests() {
    SelectionLifetimeHook hook{};

    {
        auto s=base(Workload::WS); s.future_reuse_tokens=1;
        auto p=hook.plan(s);
        req(p.route_a && !p.acquire_p1, "WS H=1 must remain A");
    }
    {
        auto s=base(Workload::WS); s.future_reuse_tokens=2;
        auto p=hook.plan(s);
        req(p.acquire_p1 && p.route_b_after_success && p.decision.threshold_tokens==2,
            "WS H=2 must acquire P1");
    }
    {
        auto s=base(Workload::WC); s.future_reuse_tokens=3;
        req(hook.plan(s).route_a, "WC H=3 must remain A");
        s.future_reuse_tokens=4;
        req(hook.plan(s).acquire_p1, "WC H=4 must acquire P1");
    }
    {
        auto s=base(Workload::WS); s.p1_available=false; s.future_reuse_tokens=14;
        req(hook.plan(s).route_a, "P3 WS H=14 must remain A");
        s.future_reuse_tokens=15;
        req(hook.plan(s).acquire_p3_cold, "P3 WS H=15 must acquire");
    }
    {
        auto s=base(Workload::WC); s.p1_available=false; s.future_reuse_tokens=32;
        req(hook.plan(s).route_a, "P3 WC H=32 must remain A");
        s.future_reuse_tokens=33;
        req(hook.plan(s).acquire_p3_cold, "P3 WC H=33 must acquire");
    }
    {
        auto s=base(Workload::WS); s.p1_available=false; s.p3_cold_available=false;
        s.future_reuse_tokens=16;
        req(hook.plan(s).route_a, "P0 WS H=16 must remain A");
        s.future_reuse_tokens=17;
        req(hook.plan(s).acquire_p0_cpu, "P0 WS H=17 must acquire");
    }
    {
        auto s=base(Workload::WC); s.p1_available=false; s.p3_cold_available=false;
        s.future_reuse_tokens=36;
        req(hook.plan(s).route_a, "P0 WC H=36 must remain A");
        s.future_reuse_tokens=37;
        req(hook.plan(s).acquire_p0_cpu, "P0 WC H=37 must acquire");
    }
    {
        auto s=base(Workload::WC);
        s.b_resident=true; s.b_identity_valid=true; s.future_reuse_tokens=1;
        auto p=hook.plan(s);
        req(p.route_b_after_success && !p.acquire_p1 && !p.evict_b,
            "resident B must ignore creation threshold");
    }
    {
        auto s=base(Workload::WS);
        s.b_resident=true; s.b_identity_valid=true; s.future_reuse_known=false;
        auto p=hook.plan(s);
        req(p.route_b_after_success && p.decision.preserve_existing_b,
            "resident B with unknown H must be retained");
    }
    {
        auto s=base(Workload::WS);
        s.b_resident=true; s.b_identity_valid=true; s.current_request_in_validated_domain=false;
        auto p=hook.plan(s);
        req(p.route_a && !p.evict_b && p.decision.preserve_existing_b,
            "outside-domain request must use A without destroying valid B");
    }
    {
        auto s=base(Workload::WS);
        s.b_resident=true; s.b_identity_valid=true; s.residency_lease_granted=false;
        req(hook.plan(s).evict_b, "lease revocation must evict resident B");
    }
    {
        auto s=base(Workload::WS);
        s.b_resident=true; s.b_identity_valid=false;
        req(hook.plan(s).evict_b, "identity invalidation must evict resident B");
    }
    {
        auto s=base(Workload::WS);
        s.b_resident=true; s.b_identity_valid=true; s.future_reuse_tokens=0;
        req(hook.plan(s).evict_b, "known zero future reuse must evict resident B");
    }
    {
        auto s=base(Workload::WS);
        s.future_reuse_known=false;
        req(hook.plan(s).route_a, "absent B with unknown H must not speculate");
    }
    {
        auto s=base(Workload::WS);
        s.residency_lease_granted=false; s.future_reuse_tokens=100;
        req(hook.plan(s).route_a, "lease denied must block acquisition");
    }
    {
        auto s=base(Workload::WS);
        s.acquisition_allowed=false; s.future_reuse_tokens=100;
        req(hook.plan(s).route_a, "global acquisition veto must block acquisition");
    }
    req(kExec148ResidentBytes==549527552ull, "resident byte constant drift");
}

static void exhaustive_invariant_tests() {
    SelectionLifetimeHook hook{};
    constexpr std::array<std::uint64_t,14> H{
        0,1,2,3,4,14,15,16,17,32,33,36,37,64
    };
    std::uint64_t cases=0, acquisitions=0, evictions=0, resident_routes=0;

    // 11 independent booleans -> 2048 masks, two workloads, representative H partition.
    for (int wi=0; wi<2; ++wi) {
        for (auto h:H) {
            for (std::uint32_t mask=0; mask<(1u<<11); ++mask) {
                RuntimeSignals s{};
                s.workload=wi==0?Workload::WS:Workload::WC;
                s.current_request_in_validated_domain=(mask&(1u<<0))!=0;
                s.model_loaded=(mask&(1u<<1))!=0;
                s.b_resident=(mask&(1u<<2))!=0;
                s.b_identity_valid=(mask&(1u<<3))!=0;
                s.b_execution_available=(mask&(1u<<4))!=0;
                s.residency_lease_granted=(mask&(1u<<5))!=0;
                s.future_reuse_known=(mask&(1u<<6))!=0;
                s.future_reuse_tokens=h;
                s.acquisition_allowed=(mask&(1u<<7))!=0;
                s.p1_available=(mask&(1u<<8))!=0;
                s.p1_vetoed=(mask&(1u<<9))!=0;
                s.p3_cold_available=(mask&(1u<<10))!=0;
                s.p0_cpu_available=true;

                const auto p1=hook.plan(s);
                const auto p2=hook.plan(s);
                req(same_decision(p1.decision,p2.decision), "determinism violated");
                req(p1.requested_residency_bytes==549527552ull, "lease byte drift");

                const int acquire_count=int(p1.acquire_p1)+int(p1.acquire_p3_cold)+int(p1.acquire_p0_cpu);
                req(acquire_count<=1, "more than one acquisition action");
                req(!(p1.evict_b && acquire_count), "eviction and acquisition cannot coexist");
                req(!(p1.route_a && p1.route_b_after_success), "route A/B must be exclusive");

                if (acquire_count) {
                    ++acquisitions;
                    req(!s.b_resident, "must not reacquire resident B");
                    req(s.current_request_in_validated_domain, "must not acquire outside domain");
                    req(s.model_loaded, "must not acquire without model");
                    req(s.future_reuse_known && h>0, "must not acquire without positive known H");
                    req(s.residency_lease_granted, "must not acquire without lease");
                    req(s.acquisition_allowed, "must not acquire under global veto");
                    req(p1.require_post_acquisition_identity_validation,
                        "acquisition must require exact post-validation");
                }
                if (p1.evict_b) {
                    ++evictions;
                    req(s.b_resident, "eviction requires resident B");
                    req(p1.route_a, "eviction must route A");
                }
                if (s.b_resident && s.b_identity_valid && s.b_execution_available &&
                    s.residency_lease_granted &&
                    !(s.future_reuse_known && h==0) &&
                    s.current_request_in_validated_domain && s.model_loaded) {
                    ++resident_routes;
                    req(p1.route_b_after_success && !p1.evict_b,
                        "valid resident B must route B inside domain");
                }
                if (s.b_resident && s.b_identity_valid && s.b_execution_available &&
                    s.residency_lease_granted &&
                    !(s.future_reuse_known && h==0) &&
                    !s.current_request_in_validated_domain && s.model_loaded) {
                    req(p1.route_a && !p1.evict_b && p1.decision.preserve_existing_b,
                        "outside domain must preserve valid resident B");
                }
                ++cases;
            }
        }
    }
    std::cout<<"cases="<<cases
             <<" acquisitions="<<acquisitions
             <<" evictions="<<evictions
             <<" resident_routes="<<resident_routes<<"\n";
}

int main() {
    boundary_tests();
    exhaustive_invariant_tests();
    std::cout<<"B1_POLICY_ZERO_SCIENCE_QA=PASS\n";
    std::cout<<"NO MODEL LOAD. NO GPU. NO VULKAN. NO PERFORMANCE TIMING. NO PLACEMENT BENCHMARK.\n";
    return 0;
}
