#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

HEADER = "Vulkan Timings:"
NUMERIC = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
TOTAL_RE = re.compile(rf"^Total time:\s*({NUMERIC})\s*us\.\s*$", re.MULTILINE)
Q_SHAPE_RE = re.compile(r"q\(128,([0-9]+),28,1\)")


def _split_blocks(text: str) -> list[str]:
    blocks = text.split(HEADER)[1:]
    if len(blocks) != 32:
        raise ValueError(f"expected exactly 32 Vulkan timing groups, got {len(blocks)}")
    return blocks


def inspect_structure(text: str) -> dict:
    """Validate only group/phase structure; do not parse or return timing magnitudes."""
    blocks = _split_blocks(text)
    q_sequence_lengths: list[int] = []
    lexical_classes: list[str] = []

    for index, block in enumerate(blocks):
        q = Q_SHAPE_RE.search(block)
        if not q:
            raise ValueError(f"Vulkan timing group {index} missing qualified q-shape")
        q_sequence_lengths.append(int(q.group(1)))

        matches = TOTAL_RE.findall(block)
        if len(matches) != 1:
            raise ValueError(
                f"Vulkan timing group {index} expected exactly one finite numeric Total time record, got {len(matches)}"
            )
        token = matches[0]
        lexical_classes.append("SCIENTIFIC" if "e" in token.lower() else "PLAIN")

    if q_sequence_lengths[0] not in (4, 256):
        raise ValueError(
            f"Vulkan timing group 0 is not a frozen CORE-0D prefill shape: q-seq={q_sequence_lengths[0]}"
        )
    if any(v != 1 for v in q_sequence_lengths[1:]):
        raise ValueError(
            "Vulkan timing groups 1-31 are not all cached-decode q-seq=1 shapes"
        )

    return {
        "group_count": 32,
        "prefill_group_index": 0,
        "prefill_q_sequence_length": q_sequence_lengths[0],
        "decode_group_indices": list(range(1, 32)),
        "decode_q_sequence_lengths_unique": sorted(set(q_sequence_lengths[1:])),
        "numeric_lexical_classes": lexical_classes,
        "timing_magnitudes_returned": False,
    }


def parse_text(text: str) -> dict:
    structure = inspect_structure(text)
    blocks = _split_blocks(text)
    totals: list[float] = []

    for index, block in enumerate(blocks):
        matches = TOTAL_RE.findall(block)
        if len(matches) != 1:
            raise ValueError(
                f"Vulkan timing group {index} expected exactly one Total time record, got {len(matches)}"
            )
        total_us = float(matches[0])
        if not math.isfinite(total_us) or total_us <= 0:
            raise ValueError(
                f"Vulkan timing group {index} has non-positive/non-finite Total time"
            )
        totals.append(total_us)

    if len(totals) != 32:
        raise ValueError(f"expected exactly 32 Vulkan timing blocks, got {len(totals)}")

    return {
        "schema": "arcllm.core0d_r1.llama_vk_perf_parse.v0.1",
        "timing_authority": "DIAGNOSTIC_ONLY",
        "numeric_grammar": NUMERIC,
        "block_count": 32,
        "prefill_group_index": structure["prefill_group_index"],
        "prefill_q_sequence_length": structure["prefill_q_sequence_length"],
        "prefill_gpu_us": totals[0],
        "decode_group_indices": structure["decode_group_indices"],
        "decode_gpu_us": totals[1:],
        "decode_step_count": 31,
        "decode_gpu_total_us": sum(totals[1:]),
        "token_gpu_total_us": sum(totals),
    }


def fixture_text(
    *,
    prefill_total: str = "1.2345e+05",
    decode_total: str = "12345.5",
    prefill_q: int = 4,
    groups: int = 32,
    decode_q: int = 1,
) -> str:
    out: list[str] = []
    for i in range(groups):
        qn = prefill_q if i == 0 else decode_q
        token = prefill_total if i == 0 else decode_total
        out.append("----------------\nVulkan Timings:\n")
        out.append(
            f"FLASH_ATTN_EXT dst(128,28,{qn},1),  q(128,{qn},28,1),  "
            f"k(128,256,4,1),  v(128,256,4,1),  m(256,{qn},1,1): "
            f"1 x 1 us = 1 us\n"
        )
        out.append(f"Total time: {token} us.\n")
    return "".join(out)


def _expect_reject(text: str, label: str) -> None:
    try:
        parse_text(text)
    except ValueError:
        return
    raise SystemExit(f"CORE0D-R1 parser self-test expected rejection: {label}")


def self_test() -> int:
    positive = [
        "12345",
        "12345.5",
        "1.2345e+05",
        "1.2345E+05",
        "1.2345e-05",
    ]
    for token in positive:
        result = parse_text(fixture_text(prefill_total=token))
        if result["block_count"] != 32 or result["decode_step_count"] != 31:
            raise SystemExit(f"CORE0D-R1 parser positive fixture cardinality failure: {token}")
        if result["prefill_q_sequence_length"] != 4:
            raise SystemExit(f"CORE0D-R1 parser positive fixture prefill-shape failure: {token}")

    wc = parse_text(fixture_text(prefill_total="1.2345e+05", prefill_q=256))
    if wc["prefill_q_sequence_length"] != 256:
        raise SystemExit("CORE0D-R1 parser W-C prefill-shape fixture failure")

    for token in ["nan", "inf", "-inf"]:
        _expect_reject(fixture_text(prefill_total=token), f"non-finite {token}")

    _expect_reject(fixture_text(groups=31), "31 groups")
    _expect_reject(fixture_text(groups=33), "33 groups")
    _expect_reject(fixture_text(prefill_q=1), "decode-shaped group 0")
    _expect_reject(fixture_text(decode_q=4), "prefill-shaped decode groups")
    _expect_reject(fixture_text(prefill_total="0"), "zero total")
    _expect_reject(fixture_text(prefill_total="-1.0"), "negative total")

    print("CORE0D_R1_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input")
    ap.add_argument("--out")
    ap.add_argument("--inspect-structure", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return self_test()

    if not args.input or not args.out:
        raise SystemExit("--input and --out are required")

    text = Path(args.input).read_text(encoding="utf-8", errors="replace")
    if args.inspect_structure:
        result = inspect_structure(text)
        result = {
            "schema": "arcllm.core0d_r1.llama_vk_perf_structure.v0.1",
            "classification": "STRUCTURE_ONLY_ZERO_SCIENCE_REGRESSION",
            **result,
        }
        print("CORE0D_R1_LLAMA_VK_PERF_STRUCTURE=PASS")
    else:
        result = parse_text(text)
        print("CORE0D_R1_LLAMA_VK_PERF_PARSE=PASS")

    Path(args.out).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
