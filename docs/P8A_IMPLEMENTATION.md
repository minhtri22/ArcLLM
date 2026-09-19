# P8-A Implementation

P8-A reuses the repository GGUF parser but intentionally does not use TensorStore because TensorStore maps the entire file. The planner needs only metadata and tensor descriptors.

The executable checks exact file size, parses GGUF, derives model dimensions, computes tensor bytes for F32/Q4_K/Q6_K, validates offsets, simulates inherited <=256 MiB arenas, computes production KV and P7-L-equivalent working buffers, and writes deterministic JSON evidence.

run_p8a.ps1 performs the authoritative SHA256 check before invoking the planner.

tools/fetch_p8_target.ps1 downloads only the frozen revision/file into .models/ and verifies size + SHA. .models/ remains untracked.

P8-A performs no Vulkan allocation and no inference.
