#include "../src/core0c_token_xray_bridge.h"
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
    if (!arcllm_core0c::enabled()) {
        std::cerr << "CORE0C fixture requires trace env\n";
        return 2;
    }
    const double period = arcllm_core0c::timestamp_period_ns();
    const uint32_t bits = arcllm_core0c::expected_timestamp_valid_bits();

    std::vector<arcllm_core0c::DispatchObservation> prefill;
    prefill.push_back({0u,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",56u,1u,1u,10u,20u,100u});
    prefill.push_back({1u,"L00.causal_gqa","p7_attention_prefill_online.spv",112u,1u,1u,21u,31u,120u});
    prefill.push_back({2u,"L00.ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv",2368u,1u,1u,32u,42u,140u});
    arcllm_core0c::write_token_trace("prefill",0u,4u,bits,period,500u,prefill);

    std::vector<arcllm_core0c::DispatchObservation> decode;
    decode.push_back({0u,"token_embedding","p8c_embedding_q4k_segmented_probe.spv",14u,1u,1u,50u,60u,100u});
    decode.push_back({1u,"L00.cached_gqa","p7_attention_kv_online.spv",28u,1u,1u,61u,71u,120u});
    decode.push_back({2u,"L03.ffn_down","q4_down_exec148_serial.spv",56u,1u,1u,72u,82u,140u});
    arcllm_core0c::write_token_trace("decode",0u,0u,bits,period,500u,decode);

    arcllm_core0c::record_acquire();
    arcllm_core0c::record_p1_materialization({"Q4V4.P1.L3","Q4V4.P1.L4"},{1000u,1100u});
    arcllm_core0c::record_validate();
    arcllm_core0c::record_resident();
    arcllm_core0c::record_reuse();
    arcllm_core0c::record_evict();
    arcllm_core0c::record_release();
    arcllm_core0c::lifecycle().flush();

    std::cout << "CORE0C_FIXTURE_SMOKE=PASS\n";
    return 0;
}
