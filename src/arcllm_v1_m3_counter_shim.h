#pragma once
#include <cstddef>
#include <cstdint>

struct M3PerfCounterMeta {
    uint32_t index;
    uint32_t storage;
    uint32_t unit;
    uint32_t scope;
    uint32_t flags;
    char name[256];
    char category[256];
    char description[256];
    char uuid_hex[33];
};

struct M3PerfCounterValue {
    uint32_t storage;
    int64_t i64;
    uint64_t u64;
    double f64;
};

extern "C" int m3_perf_create_device(
    void* physical_device,
    uint32_t queue_family,
    float priority,
    void** out_device,
    char* error,
    size_t error_size);

extern "C" int m3_perf_create_query_pool(
    void* instance,
    void* physical_device,
    void* device,
    uint32_t queue_family,
    const uint32_t* counter_indices,
    uint32_t counter_count,
    uint32_t query_count,
    void** out_query_pool,
    uint32_t* out_pass_count,
    M3PerfCounterMeta* out_meta,
    char* error,
    size_t error_size);

extern "C" int m3_perf_acquire_lock(void* device, uint64_t timeout_ns, char* error, size_t error_size);
extern "C" void m3_perf_release_lock(void* device);
extern "C" void m3_perf_cmd_begin_query(void* command_buffer, void* query_pool, uint32_t query);
extern "C" void m3_perf_cmd_end_query(void* command_buffer, void* query_pool, uint32_t query);
extern "C" int m3_perf_submit_pass(
    void* queue, void* command_buffer, void* fence, uint32_t pass_index, char* error, size_t error_size);
extern "C" int m3_perf_get_results(
    void* device,
    void* query_pool,
    uint32_t query_count,
    uint32_t counter_count,
    const M3PerfCounterMeta* meta,
    M3PerfCounterValue* out_values,
    char* error,
    size_t error_size);
extern "C" void m3_perf_destroy_query_pool(void* device, void* query_pool);
