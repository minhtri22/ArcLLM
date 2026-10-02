from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from huggingface_hub import hf_hub_download
from tokenizers import Tokenizer

REPO_ID = "Qwen/Qwen2.5-Coder-7B-Instruct"
REVISION = "c03e6d358207e414f1eca0bb1891e29f1db0e242"
PROMPTS = {
    "P0": "The capital of France is",
    "P1": "2 + 2 =",
    "P2": "Hà Nội là thủ đô của",
    "P3": "A B C D E F G H",
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    tok_path = Path(
        hf_hub_download(
            repo_id=REPO_ID,
            filename="tokenizer.json",
            revision=REVISION,
        )
    )
    tokenizer = Tokenizer.from_file(str(tok_path))

    prompts = {}
    for pid, text in PROMPTS.items():
        enc = tokenizer.encode(text, add_special_tokens=False)
        prompts[pid] = {
            "utf8": text,
            "utf8_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
            "token_ids": enc.ids,
            "token_count": len(enc.ids),
        }

    result = {
        "schema": "arcllm.token_xray_r1.reference_tokenization.v0.1",
        "status": "PASS_UPSTREAM_REFERENCE_TOKENIZATION",
        "upstream_tokenizer": {
            "repo_id": REPO_ID,
            "revision": REVISION,
            "file": "tokenizer.json",
            "sha256": sha256_file(tok_path),
        },
        "add_special_tokens": False,
        "chat_template_used": False,
        "prompts": prompts,
        "target_gguf_crosscheck_required_before_model_execution": True,
        "science_execution": False,
    }
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
