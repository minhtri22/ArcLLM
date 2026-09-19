# P8-A-R1 Implementation

Default model resolution is local and offline.

tools/resolve_p8_target.ps1:
- reads config/p8_target.json;
- uses OLLAMA_MODELS when set, otherwise %USERPROFILE%/.ollama/models;
- recursively scans manifests/registry.ollama.ai/library/qwen2.5-coder;
- selects only application/vnd.ollama.image.model layers matching the frozen digest and size;
- resolves the frozen content-addressed blob path;
- checks existence and byte size;
- outputs the blob path.

run_p8a.ps1 then independently computes SHA256 on the resolved blob before static QA, build and planner execution.

tools/fetch_p8_target.ps1 is retained only as a compatibility entry point; it performs no network download and delegates to the local resolver.

The native planner remains metadata-only: no Vulkan instance, GPU allocation, shader compile or inference.
