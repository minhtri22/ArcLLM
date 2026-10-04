#!/usr/bin/env python3
"""Generate the P1 instrumented ArcLLM runtime from the exact canonical source.

This script is zero-science. It does not run the model or Vulkan. It performs a
deterministic, reversible source transformation that only inserts token-ready
observer hooks.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess

EXPECTED_RUNTIME_BLOB = "0c613f6f740931a88ddd3ee5b904533a58001a06"

DECL_MARKER = "struct RuntimeArena { uint64_t start=0,end=0; };"
DECL_BLOCK = r'''
// ARCLLM_LMAX_P1_INSTRUMENTATION_BEGIN observer_api
extern "C" {
using ArcllmLmaxP1TokenObserver = void(*)(std::uint32_t, std::uint32_t, void*);
static thread_local ArcllmLmaxP1TokenObserver g_arcllm_lmax_p1_observer = nullptr;
static thread_local void* g_arcllm_lmax_p1_observer_user = nullptr;
void arcllm_lmax_p1_set_token_observer(ArcllmLmaxP1TokenObserver observer, void* user) {
    g_arcllm_lmax_p1_observer = observer;
    g_arcllm_lmax_p1_observer_user = user;
}
}
static inline void arcllm_lmax_p1_emit_token(std::uint32_t index, std::uint32_t token_id) {
    if (g_arcllm_lmax_p1_observer) {
        g_arcllm_lmax_p1_observer(index, token_id, g_arcllm_lmax_p1_observer_user);
    }
}
// ARCLLM_LMAX_P1_INSTRUMENTATION_END observer_api
'''

PREFILL_MARKER = "        result.generated_token_ids.push_back(top.top1);"
PREFILL_BLOCK = r'''
        // ARCLLM_LMAX_P1_INSTRUMENTATION_BEGIN prefill_token_ready
        arcllm_lmax_p1_emit_token(0u, top.top1);
        // ARCLLM_LMAX_P1_INSTRUMENTATION_END prefill_token_ready
'''

DECODE_MARKER = "            result.generated_token_ids.push_back(next);"
DECODE_BLOCK = r'''
            // ARCLLM_LMAX_P1_INSTRUMENTATION_BEGIN decode_token_ready
            arcllm_lmax_p1_emit_token(
                static_cast<std::uint32_t>(result.generated_token_ids.size() - 1u), next);
            // ARCLLM_LMAX_P1_INSTRUMENTATION_END decode_token_ready
'''


def git_blob(repo: pathlib.Path, path: pathlib.Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(repo), "hash-object", str(path)],
        text=True,
    ).strip()


def normalized_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8").replace("\r\n", "\n")


def transform(source: str) -> str:
    for marker in (DECL_MARKER, PREFILL_MARKER, DECODE_MARKER):
        if source.count(marker) != 1:
            raise RuntimeError(f"expected exactly one marker: {marker!r}")

    out = source.replace(DECL_MARKER, DECL_BLOCK + "\n" + DECL_MARKER, 1)
    out = out.replace(PREFILL_MARKER, PREFILL_MARKER + PREFILL_BLOCK, 1)
    out = out.replace(DECODE_MARKER, DECODE_MARKER + DECODE_BLOCK, 1)
    return out


def reverse_transform(generated: str) -> str:
    out = generated
    out = out.replace(DECL_BLOCK + "\n", "", 1)
    out = out.replace(PREFILL_BLOCK, "", 1)
    out = out.replace(DECODE_BLOCK, "", 1)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--manifest", required=True)
    args = ap.parse_args()

    root = pathlib.Path(args.root).resolve()
    source_path = root / "src" / "arcllm_v1_runtime.cpp"
    out_path = pathlib.Path(args.out).resolve()
    manifest_path = pathlib.Path(args.manifest).resolve()

    blob = git_blob(root, source_path)
    if blob != EXPECTED_RUNTIME_BLOB:
        raise RuntimeError(
            f"canonical runtime blob drift: expected {EXPECTED_RUNTIME_BLOB}, got {blob}"
        )

    source = normalized_text(source_path)
    generated = transform(source)
    if reverse_transform(generated) != source:
        raise RuntimeError("instrumentation transformation is not exactly reversible")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(generated, encoding="utf-8", newline="\n")

    manifest = {
        "schema": "arcllm.lmax_arch_p1.instrumented_runtime_transform.v0.4",
        "status": "PASS_REVERSIBLE_TRANSFORM",
        "canonical_runtime_path": "src/arcllm_v1_runtime.cpp",
        "canonical_runtime_git_blob": blob,
        "observer_hook_count": generated.count("arcllm_lmax_p1_emit_token(") - 1,
        "canonical_function_name_unchanged": True,
        "transform_reversible_to_normalized_canonical_source": True,
        "model_executed": False,
        "vulkan_initialized": False,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("ARCLLM_LMAX_P1_RUNTIME_TRANSFORM=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
