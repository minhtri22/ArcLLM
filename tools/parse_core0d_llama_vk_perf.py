#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

HEADER = "Vulkan Timings:"
TOTAL_RE = re.compile(r"^Total time:\s*([0-9]+(?:\.[0-9]+)?)\s*us\.\s*$", re.MULTILINE)


def parse_text(text: str) -> dict:
    parts = text.split(HEADER)
    blocks = []
    for part in parts[1:]:
        m = TOTAL_RE.search(part)
        if not m:
            continue
        total_us = float(m.group(1))
        if not math.isfinite(total_us) or total_us <= 0:
            raise ValueError("non-positive/non-finite Vulkan timing block")
        blocks.append(total_us)
    if len(blocks) != 32:
        raise ValueError(f"expected exactly 32 Vulkan timing blocks, got {len(blocks)}")
    return {
        "schema": "arcllm.core0d.llama_vk_perf_parse.v0.1",
        "timing_authority": "DIAGNOSTIC_ONLY",
        "block_count": 32,
        "prefill_gpu_us": blocks[0],
        "decode_gpu_us": blocks[1:],
        "decode_step_count": 31,
        "decode_gpu_total_us": sum(blocks[1:]),
        "token_gpu_total_us": sum(blocks),
    }


def fixture_text() -> str:
    out = []
    for i in range(32):
        total = 1000.0 + i
        out.append("----------------\nVulkan Timings:\n")
        out.append(f"fixture_op: 1 x {total:.3f} us = {total:.3f} us\n")
        out.append(f"Total time: {total:.3f} us.\n")
    return "".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input")
    ap.add_argument("--out")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        r = parse_text(fixture_text())
        expected = sum(1000.0 + i for i in range(32))
        if r["block_count"] != 32 or r["decode_step_count"] != 31:
            raise SystemExit("CORE0D parser self-test cardinality failure")
        if not math.isclose(r["token_gpu_total_us"], expected, rel_tol=0, abs_tol=1e-9):
            raise SystemExit("CORE0D parser self-test arithmetic failure")
        print("CORE0D_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS")
        return 0

    if not args.input or not args.out:
        raise SystemExit("--input and --out are required")
    text = Path(args.input).read_text(encoding="utf-8", errors="replace")
    result = parse_text(text)
    Path(args.out).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("CORE0D_LLAMA_VK_PERF_PARSE=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
