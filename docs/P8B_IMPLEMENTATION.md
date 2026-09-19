# P8-B Implementation

P8-B uses the repository GgufReader + TensorStore for a read-only mapped source and a minimal SDK-less Vulkan loader derived from the already validated P7 runtime ABI definitions.

It recreates the exact P8-A2 row-segmented plan and compares it against the frozen parent arena boundaries before allocating memory.

Vulkan buffers use the same P7 memory requirement: DEVICE_LOCAL + HOST_VISIBLE + HOST_COHERENT, preferring HOST_CACHED when available. Each allocation records requested bytes and Vulkan memory-requirement allocation bytes.

All 19 weight arenas are initialized directly from the mapped GGUF payload and fully compared byte-for-byte while resident. K/V and every P7-equivalent working buffer are then allocated and kept alive concurrently through the final gate.

No SPIR-V is read, no shader/pipeline/descriptor/command object is created, and no dispatch occurs.
