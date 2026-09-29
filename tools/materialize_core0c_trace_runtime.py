#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CANONICAL = {
    "support": ROOT / "src" / "arcllm_v1_vulkan_runtime_support.h",
    "q4": ROOT / "src" / "arcllm_v1_q4_vulkan_backend_v4_runtime.h",
    "runtime": ROOT / "src" / "arcllm_v1_runtime.cpp",
}

EXPECTED_BLOBS = {
    "support": "1b3a2a935134a3afa680ee4880377a20bc8a466f",
    "q4": "3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
    "runtime": "0c613f6f740931a88ddd3ee5b904533a58001a06",
}


def git_blob(path: Path) -> str:
    import subprocess
    return subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", f"HEAD:{path.relative_to(ROOT).as_posix()}"],
        text=True,
    ).strip()


def sha256_text(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8")).hexdigest().upper()


def one_replace(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one anchor, found {count}")
    return text.replace(old, new, 1)


def materialize_support(src: str) -> str:
    s = src
    s = one_replace(
        s,
        '#include "tensor_store.h"\n',
        '#include "tensor_store.h"\n#include "core0c_token_xray_bridge.h"\n',
        "support bridge include",
    )
    s = one_replace(
        s,
        "using VkDependencyFlags = uint32_t;\n",
        "using VkDependencyFlags = uint32_t;\n"
        "using VkQueryPoolCreateFlags = uint32_t;\n"
        "using VkQueryType = int32_t;\n"
        "using VkQueryResultFlags = uint32_t;\n",
        "query typedefs",
    )
    s = one_replace(
        s,
        "struct VkCommandBuffer_T; using VkCommandBuffer = VkCommandBuffer_T*;\n",
        "struct VkCommandBuffer_T; using VkCommandBuffer = VkCommandBuffer_T*;\n"
        "struct VkQueryPool_T; using VkQueryPool = VkQueryPool_T*;\n",
        "query pool type",
    )
    s = one_replace(
        s,
        "static constexpr VkStructureType VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8;\n",
        "static constexpr VkStructureType VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8;\n"
        "static constexpr VkStructureType VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO = 11;\n",
        "query structure type",
    )
    s = one_replace(
        s,
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800;\n"
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_HOST_BIT = 0x00004000;\n",
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001;\n"
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800;\n"
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000;\n"
        "static constexpr VkPipelineStageFlags VK_PIPELINE_STAGE_HOST_BIT = 0x00004000;\n"
        "static constexpr VkQueryType VK_QUERY_TYPE_TIMESTAMP = 2;\n"
        "static constexpr VkQueryResultFlags VK_QUERY_RESULT_64_BIT = 0x00000001;\n"
        "static constexpr VkQueryResultFlags VK_QUERY_RESULT_WAIT_BIT = 0x00000002;\n",
        "query constants",
    )
    s = one_replace(
        s,
        "struct VkMemoryBarrier {\n"
        "    VkStructureType sType;\n"
        "    const void* pNext;\n"
        "    VkAccessFlags srcAccessMask;\n"
        "    VkAccessFlags dstAccessMask;\n"
        "};\n",
        "struct VkMemoryBarrier {\n"
        "    VkStructureType sType;\n"
        "    const void* pNext;\n"
        "    VkAccessFlags srcAccessMask;\n"
        "    VkAccessFlags dstAccessMask;\n"
        "};\n\n"
        "struct VkQueryPoolCreateInfo {\n"
        "    VkStructureType sType;\n"
        "    const void* pNext;\n"
        "    VkQueryPoolCreateFlags flags;\n"
        "    VkQueryType queryType;\n"
        "    uint32_t queryCount;\n"
        "    VkFlags pipelineStatistics;\n"
        "};\n",
        "query create struct",
    )
    s = one_replace(
        s,
        "using PFN_vkCmdPipelineBarrier = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const void*, uint32_t, const void*);\n",
        "using PFN_vkCmdPipelineBarrier = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const void*, uint32_t, const void*);\n"
        "using PFN_vkCreateQueryPool = VkResult (WINAPI*)(VkDevice, const VkQueryPoolCreateInfo*, const void*, VkQueryPool*);\n"
        "using PFN_vkDestroyQueryPool = void (WINAPI*)(VkDevice, VkQueryPool, const void*);\n"
        "using PFN_vkCmdResetQueryPool = void (WINAPI*)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);\n"
        "using PFN_vkCmdWriteTimestamp = void (WINAPI*)(VkCommandBuffer, VkPipelineStageFlags, VkQueryPool, uint32_t);\n"
        "using PFN_vkGetQueryPoolResults = VkResult (WINAPI*)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void*, VkDeviceSize, VkQueryResultFlags);\n",
        "query PFNs",
    )
    s = one_replace(
        s,
        "                queue_family_ = i;\n                found = true;\n",
        "                queue_family_ = i;\n"
        "                timestamp_valid_bits_ = qprops[i].timestampValidBits;\n"
        "                found = true;\n",
        "timestamp valid bits capture",
    )
    s = one_replace(
        s,
        '        cmd_pipeline_barrier_ = reqd<PFN_vkCmdPipelineBarrier>(gdpa_, device_, "vkCmdPipelineBarrier");\n',
        '        cmd_pipeline_barrier_ = reqd<PFN_vkCmdPipelineBarrier>(gdpa_, device_, "vkCmdPipelineBarrier");\n'
        '        create_query_pool_ = reqd<PFN_vkCreateQueryPool>(gdpa_, device_, "vkCreateQueryPool");\n'
        '        destroy_query_pool_ = reqd<PFN_vkDestroyQueryPool>(gdpa_, device_, "vkDestroyQueryPool");\n'
        '        cmd_reset_query_pool_ = reqd<PFN_vkCmdResetQueryPool>(gdpa_, device_, "vkCmdResetQueryPool");\n'
        '        cmd_write_timestamp_ = reqd<PFN_vkCmdWriteTimestamp>(gdpa_, device_, "vkCmdWriteTimestamp");\n'
        '        get_query_pool_results_ = reqd<PFN_vkGetQueryPoolResults>(gdpa_, device_, "vkGetQueryPoolResults");\n',
        "query function load",
    )

    start = s.index("    ChainStats execute_prepared(")
    end = s.index("\n    void destroy_prepared", start)
    old_fn = s[start:end]
    new_fn = r'''    ChainStats execute_prepared(const PreparedChain& chain,const std::vector<DispatchOp>& ops,bool cross_submit_compute_barrier=false){
        if(ops.size()!=chain.prepared.size()||ops.empty())throw std::runtime_error("prepared chain shape mismatch");
        for(size_t i=0;i<ops.size();++i){const auto& p=chain.prepared[i];if(ops[i].spv_path!=p.spv_path||ops[i].push.size()!=p.push_size||ops[i].buffers.size()!=p.buffers.size())throw std::runtime_error("prepared op signature mismatch: "+ops[i].name);for(size_t j=0;j<ops[i].buffers.size();++j)if(ops[i].buffers[j]!=p.buffers[j])throw std::runtime_error("prepared op buffer mismatch: "+ops[i].name);}

        const bool trace_enabled=arcllm_core0c::enabled();
        const bool lifecycle_ops=std::all_of(ops.begin(),ops.end(),[](const DispatchOp& op){return op.name.rfind("Q4V4.P1.L",0)==0;});
        bool prefill_ops=false;
        for(const auto& op:ops)if(op.name.find("causal_gqa")!=std::string::npos||op.name.find("ffn_gate_up_fused")!=std::string::npos)prefill_ops=true;
        const std::string trace_mode=prefill_ops?"prefill":"decode";

        VkQueryPool qp=nullptr;
        uint32_t qcount=0;
        if(trace_enabled){
            if(timestamp_valid_bits_==0u)throw std::runtime_error("CORE0C compute queue has timestampValidBits=0");
            if(timestamp_valid_bits_!=arcllm_core0c::expected_timestamp_valid_bits())throw std::runtime_error("CORE0C timestampValidBits differs from frozen preflight");
            qcount=2u+2u*uint32_t(ops.size());
            VkQueryPoolCreateInfo qci{};qci.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;qci.queryType=VK_QUERY_TYPE_TIMESTAMP;qci.queryCount=qcount;
            if(create_query_pool_(device_,&qci,nullptr,&qp)!=VK_SUCCESS||!qp)throw std::runtime_error("CORE0C vkCreateQueryPool failed");
        }

        auto tall0=std::chrono::steady_clock::now();
        VkCommandPoolCreateInfo pci{};pci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;pci.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;pci.queueFamilyIndex=queue_family_;
        VkCommandPool pool=nullptr;VkResult vr=create_command_pool_(device_,&pci,nullptr,&pool);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateCommandPool failed ArcLLM runtime");
        VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
        VkCommandBuffer cb=nullptr;vr=allocate_command_buffers_(device_,&ai,&cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkAllocateCommandBuffers failed ArcLLM runtime");
        VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vr=begin_command_buffer_(cb,&bi);if(vr!=VK_SUCCESS)throw std::runtime_error("vkBeginCommandBuffer failed ArcLLM runtime");
        ChainStats st{};

        if(trace_enabled){cmd_reset_query_pool_(cb,qp,0,qcount);cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,qp,0);}
        if(cross_submit_compute_barrier){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);st.initial_compute_barrier_count=1;}

        for(size_t oi=0;oi<ops.size();++oi){
            const auto& op=ops[oi];const auto& p=chain.prepared[oi];
            cmd_bind_pipeline_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);
            cmd_bind_descriptor_sets_(cb,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline_layout,0,1,&p.descriptor_set,0,nullptr);
            if(!op.push.empty())cmd_push_constants_(cb,p.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,uint32_t(op.push.size()),op.push.data());
            if(trace_enabled)cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,qp,2u+2u*uint32_t(oi));
            cmd_dispatch_(cb,op.gx,op.gy,op.gz);
            if(trace_enabled)cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,qp,3u+2u*uint32_t(oi));
            ++st.dispatch_count;
            if(oi+1<ops.size()){VkMemoryBarrier mb{};mb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;mb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&mb,0,nullptr,0,nullptr);++st.internal_barrier_count;}
        }

        VkMemoryBarrier hb{};hb.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;hb.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;hb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        cmd_pipeline_barrier_(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hb,0,nullptr,0,nullptr);st.final_host_barrier_count=1;
        if(trace_enabled)cmd_write_timestamp_(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,qp,1);
        vr=end_command_buffer_(cb);if(vr!=VK_SUCCESS)throw std::runtime_error("vkEndCommandBuffer failed ArcLLM runtime");

        VkFenceCreateInfo fci{};fci.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence=nullptr;vr=create_fence_(device_,&fci,nullptr,&fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkCreateFence failed ArcLLM runtime");
        VkSubmitInfo si{};si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;si.commandBufferCount=1;si.pCommandBuffers=&cb;
        auto ts0=std::chrono::steady_clock::now();vr=queue_submit_(queue_,1,&si,fence);if(vr!=VK_SUCCESS)throw std::runtime_error("vkQueueSubmit failed ArcLLM runtime");
        st.submit_count=1;vr=wait_for_fences_(device_,1,&fence,VK_TRUE,(std::numeric_limits<uint64_t>::max)());if(vr!=VK_SUCCESS)throw std::runtime_error("vkWaitForFences failed ArcLLM runtime");st.fence_wait_count=1;
        auto ts1=std::chrono::steady_clock::now();st.submit_wait_ms=std::chrono::duration<double,std::milli>(ts1-ts0).count();

        if(trace_enabled){
            std::vector<uint64_t> ticks(qcount,0);
            vr=get_query_pool_results_(device_,qp,0,qcount,ticks.size()*sizeof(uint64_t),ticks.data(),sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT);
            if(vr!=VK_SUCCESS)throw std::runtime_error("CORE0C vkGetQueryPoolResults failed");
            auto delta=[&](uint64_t a,uint64_t b){if(timestamp_valid_bits_==64u)return b-a;const uint64_t mask=(uint64_t{1}<<timestamp_valid_bits_)-1u;return (b-a)&mask;};
            const double period=arcllm_core0c::timestamp_period_ns();
            auto to_ns=[&](uint64_t t)->uint64_t{return uint64_t(std::llround(double(t)*period));};
            std::vector<arcllm_core0c::DispatchObservation> obs;
            std::vector<uint64_t> lifecycle_durations;
            std::vector<std::string> lifecycle_names;
            obs.reserve(ops.size());lifecycle_durations.reserve(ops.size());lifecycle_names.reserve(ops.size());
            for(size_t oi=0;oi<ops.size();++oi){
                const uint64_t d=to_ns(delta(ticks[2u+2u*oi],ticks[3u+2u*oi]));
                if(lifecycle_ops){lifecycle_names.push_back(ops[oi].name);lifecycle_durations.push_back(d);}
                else{
                    arcllm_core0c::DispatchObservation x; x.dispatch_id=uint32_t(oi);x.runtime_name=ops[oi].name;x.shader_path=ops[oi].spv_path;x.group_x=ops[oi].gx;x.group_y=ops[oi].gy;x.group_z=ops[oi].gz;x.start_tick=ticks[2u+2u*oi];x.end_tick=ticks[3u+2u*oi];x.duration_ns=d;obs.push_back(std::move(x));
                }
            }
            if(lifecycle_ops){
                arcllm_core0c::record_p1_materialization(lifecycle_names,lifecycle_durations);
            }else{
                uint32_t context_length=0;
                if(prefill_ops){for(const auto& op:ops)if(op.name.find("causal_gqa")!=std::string::npos&&op.gx%28u==0u){context_length=op.gx/28u;break;}}
                const uint32_t idx=prefill_ops?core0c_prefill_trace_index_++:core0c_decode_trace_index_++;
                arcllm_core0c::write_token_trace(trace_mode,idx,context_length,timestamp_valid_bits_,period,to_ns(delta(ticks[0],ticks[1])),obs);
            }
            destroy_query_pool_(device_,qp,nullptr);qp=nullptr;
        }

        destroy_fence_(device_,fence,nullptr);destroy_command_pool_(device_,pool,nullptr);
        st.record_submit_wait_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tall0).count();
        return st;
    }
'''
    s = s[:start] + new_fn + s[end:]
    s = one_replace(
        s,
        "    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier_ = nullptr;\n",
        "    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier_ = nullptr;\n"
        "    PFN_vkCreateQueryPool create_query_pool_ = nullptr;\n"
        "    PFN_vkDestroyQueryPool destroy_query_pool_ = nullptr;\n"
        "    PFN_vkCmdResetQueryPool cmd_reset_query_pool_ = nullptr;\n"
        "    PFN_vkCmdWriteTimestamp cmd_write_timestamp_ = nullptr;\n"
        "    PFN_vkGetQueryPoolResults get_query_pool_results_ = nullptr;\n"
        "    uint32_t timestamp_valid_bits_ = 0;\n"
        "    uint32_t core0c_prefill_trace_index_ = 0;\n"
        "    uint32_t core0c_decode_trace_index_ = 0;\n",
        "query members",
    )
    return s


def materialize_q4(src: str) -> str:
    s = src
    s = one_replace(
        s,
        '#include "../include/arcllm/v1/generic_policy_engine_v4.h"',
        '#include "arcllm/v1/generic_policy_engine_v4.h"',
        "q4 policy include",
    )
    s = one_replace(
        s,
        '#include "../include/arcllm/v1/generic_backend_binding_v4.h"',
        '#include "arcllm/v1/generic_backend_binding_v4.h"',
        "q4 binding include",
    )
    s = one_replace(
        s,
        '#include "arcllm_v1_vulkan_runtime_support.h"',
        '#include "core0c_arcllm_v1_vulkan_runtime_support.h"',
        "q4 support include",
    )
    s = one_replace(
        s,
        "            exec_=vk_.make_buffer(kExecFamilyBytes,nullptr);\n            ++counters_.b_allocations;\n",
        "            exec_=vk_.make_buffer(kExecFamilyBytes,nullptr);\n"
        "            ++counters_.b_allocations;\n"
        "            arcllm_core0c::record_acquire();\n",
        "q4 acquire event",
    )
    s = one_replace(
        s,
        "            validated_=true;\n            return bind::BackendStatus::OK;\n",
        "            validated_=true;\n"
        "            arcllm_core0c::record_validate();\n"
        "            arcllm_core0c::record_resident();\n"
        "            return bind::BackendStatus::OK;\n",
        "q4 validate resident events",
    )
    s = one_replace(
        s,
        "            ++counters_.b_releases;\n            return bind::BackendStatus::OK;\n",
        "            ++counters_.b_releases;\n"
        "            arcllm_core0c::record_release();\n"
        "            return bind::BackendStatus::OK;\n",
        "q4 release event",
    )
    return s


def materialize_runtime(src: str) -> str:
    s = src
    s = one_replace(
        s,
        '#include "arcllm_v1_q4_vulkan_backend_v4_runtime.h"',
        '#include "core0c_arcllm_v1_q4_vulkan_backend_v4_runtime.h"',
        "runtime q4 include",
    )
    s = one_replace(
        s,
        '#include "../include/arcllm/v1/runtime.h"',
        '#include "arcllm/v1/runtime.h"',
        "runtime public include",
    )
    s = one_replace(
        s,
        "            if(decision.lifecycle==gp::LifecycleAction::ACQUIRE)++result.stats.acquire_events;\n"
        "            if(decision.lifecycle==gp::LifecycleAction::EVICT)++result.stats.evict_events;\n",
        "            if(decision.lifecycle==gp::LifecycleAction::ACQUIRE)++result.stats.acquire_events;\n"
        "            if(decision.lifecycle==gp::LifecycleAction::EVICT){++result.stats.evict_events;arcllm_core0c::record_evict();}\n",
        "runtime decode lifecycle event",
    )
    s = one_replace(
        s,
        "            if(applied.primitive.opaque==q4reg::kPrimitiveA.value)++result.stats.route_a_steps;\n"
        "            else if(applied.primitive.opaque==q4reg::kPrimitiveB.value)++result.stats.route_b_steps;\n",
        "            if(applied.primitive.opaque==q4reg::kPrimitiveA.value)++result.stats.route_a_steps;\n"
        "            else if(applied.primitive.opaque==q4reg::kPrimitiveB.value){++result.stats.route_b_steps;if(decision.lifecycle!=gp::LifecycleAction::ACQUIRE)arcllm_core0c::record_reuse();}\n",
        "runtime reuse event",
    )
    s = one_replace(
        s,
        "        if(close_decision.lifecycle==gp::LifecycleAction::EVICT)++result.stats.evict_events;\n",
        "        if(close_decision.lifecycle==gp::LifecycleAction::EVICT){++result.stats.evict_events;arcllm_core0c::record_evict();}\n",
        "runtime close evict event",
    )
    s = one_replace(
        s,
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n        return result;\n",
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n"
        "        arcllm_core0c::lifecycle().flush();\n"
        "        return result;\n",
        "runtime lifecycle flush",
    )
    return s


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", required=True)
    args = ap.parse_args()
    out = Path(args.out_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)

    actual = {k: git_blob(v) for k, v in CANONICAL.items()}
    if actual != EXPECTED_BLOBS:
        raise SystemExit(f"canonical blob drift: {actual}")

    raw = {k: p.read_text(encoding="utf-8") for k, p in CANONICAL.items()}
    generated = {
        "core0c_arcllm_v1_vulkan_runtime_support.h": materialize_support(raw["support"]),
        "core0c_arcllm_v1_q4_vulkan_backend_v4_runtime.h": materialize_q4(raw["q4"]),
        "core0c_arcllm_v1_runtime.cpp": materialize_runtime(raw["runtime"]),
    }
    for name, content in generated.items():
        (out / name).write_text(content, encoding="utf-8", newline="\n")

    manifest = {
        "schema": "arcllm.core0c.generated_runtime.v0.1",
        "classification": "BENCHMARK_ONLY_DETERMINISTIC_DERIVATIVE",
        "canonical_blobs": actual,
        "generated_sha256": {name: sha256_text(content) for name, content in generated.items()},
        "canonical_runtime_modified": False,
        "kernel_source_modified": False,
        "patch_scope": [
            "Vulkan timestamp-query ABI declarations",
            "Token-XRay semantic/schema bridge",
            "Q4V4 lifecycle evidence hooks",
            "diagnostic trace emission only",
        ],
    }
    (out / "CORE0C_GENERATED_RUNTIME_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print("CORE0C_MATERIALIZE=PASS")
    for name, value in manifest["generated_sha256"].items():
        print(f"{name}={value}")


if __name__ == "__main__":
    main()
