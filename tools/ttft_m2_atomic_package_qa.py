#!/usr/bin/env python3
import argparse
import copy
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

PACKAGE_ID = "ARCLLM_TTFT_M2_ATOMIC_PACKAGE"
PACKAGE_VERSION = "v0.1"
LOCK_PATH = "config/arcllm_ttft_m2_execution_lock_v0.1.json"
RUNNER_PATH = "run_ttft_m2_buildonly.ps1"
EVIDENCE_PATH = "config/arcllm_ttft_m2_evidence_manifest_v0.1.json"
PACKAGE_PATH = "config/arcllm_ttft_m2_package_manifest_v0.1.json"
FIXTURE_INDEX = "tests/fixtures/ttft_m2_atomic_package/index.json"

SCHEMA_PATHS = {
    "success_result": "schemas/ttft_m2/buildonly_result.schema.json",
    "failure_result": "schemas/ttft_m2/failure_result.schema.json",
    "evidence_manifest": "schemas/ttft_m2/evidence_manifest.schema.json",
    "package_manifest": "schemas/ttft_m2/package_manifest.schema.json",
    "adjudicator_input": "schemas/ttft_m2/adjudicator_input.schema.json",
}
SCHEMA_IDS = {
    "success_result": "arcllm.ttft_m2.buildonly_result.v0.1",
    "failure_result": "arcllm.ttft_m2.failure_result.v0.1",
    "evidence_manifest": "arcllm.ttft_m2.evidence_manifest.v0.1",
    "package_manifest": "arcllm.ttft_m2.package_manifest.v0.1",
    "adjudicator_input": "arcllm.ttft_m2.adjudicator_input.v0.1",
}

def load_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))

def git_blob(root, rel):
    p = subprocess.run(
        ["git", "-C", str(root), "rev-parse", f"HEAD:{rel}"],
        text=True, capture_output=True
    )
    if p.returncode != 0:
        raise RuntimeError(f"cannot resolve Git blob for {rel}: {p.stderr.strip()}")
    return p.stdout.strip()

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()

def runner_meta(root):
    text = (root / RUNNER_PATH).read_text(encoding="utf-8")
    first = text.splitlines()[0]
    prefix = "# M2_PACKAGE_META "
    if not first.startswith(prefix):
        raise AssertionError("RUNNER_META_MISSING")
    return json.loads(first[len(prefix):])

def duplicate_values(values):
    seen = set()
    dup = set()
    for x in values:
        if x in seen:
            dup.add(x)
        seen.add(x)
    return sorted(dup)

def actual_repo_checks(root, mode):
    errors = []
    lock = load_json(root / LOCK_PATH)
    package = load_json(root / PACKAGE_PATH)
    evidence = load_json(root / EVIDENCE_PATH)
    meta = runner_meta(root)

    if lock.get("package_id") != PACKAGE_ID or lock.get("package_version") != PACKAGE_VERSION:
        errors.append("LOCK_PACKAGE_ID_OR_VERSION_MISMATCH")
    if meta.get("package_id") != PACKAGE_ID or meta.get("package_version") != PACKAGE_VERSION:
        errors.append("RUNNER_PACKAGE_ID_OR_VERSION_MISMATCH")
    if meta.get("lock_path") != LOCK_PATH or meta.get("lock_version") != PACKAGE_VERSION:
        errors.append("RUNNER_LOCK_MISMATCH")

    for key, path in SCHEMA_PATHS.items():
        obj = load_json(root / path)
        if obj.get("$id") != SCHEMA_IDS[key]:
            errors.append(f"SCHEMA_ID_MISMATCH:{key}")
        expected_blob = lock.get("static_git_blobs", {}).get(path)
        if expected_blob != git_blob(root, path):
            errors.append(f"LOCK_SCHEMA_BLOB_MISMATCH:{key}")

    for path, expected in lock.get("static_git_blobs", {}).items():
        try:
            actual = git_blob(root, path)
        except Exception:
            errors.append(f"LOCK_STATIC_MEMBER_MISSING:{path}")
            continue
        if actual != expected:
            errors.append(f"LOCK_STATIC_BLOB_MISMATCH:{path}")

    if lock.get("runner", {}).get("path") != RUNNER_PATH:
        errors.append("LOCK_RUNNER_PATH_MISMATCH")
    if lock.get("runner", {}).get("git_blob") != git_blob(root, RUNNER_PATH):
        errors.append("LOCK_RUNNER_BLOB_MISMATCH")

    if package.get("canonical_lock", {}).get("source_path") != LOCK_PATH:
        errors.append("PACKAGE_LOCK_PATH_MISMATCH")
    if package.get("canonical_lock", {}).get("git_blob") != git_blob(root, LOCK_PATH):
        errors.append("PACKAGE_LOCK_BLOB_MISMATCH")
    if package.get("runner", {}).get("source_path") != RUNNER_PATH:
        errors.append("PACKAGE_RUNNER_PATH_MISMATCH")
    if package.get("runner", {}).get("git_blob") != git_blob(root, RUNNER_PATH):
        errors.append("PACKAGE_RUNNER_BLOB_MISMATCH")

    if evidence.get("lock", {}).get("source_path") != LOCK_PATH:
        errors.append("EVIDENCE_LOCK_PATH_MISMATCH")
    if evidence.get("lock", {}).get("git_blob") != git_blob(root, LOCK_PATH):
        errors.append("EVIDENCE_LOCK_BLOB_MISMATCH")
    if evidence.get("runner", {}).get("source_path") != RUNNER_PATH:
        errors.append("EVIDENCE_RUNNER_PATH_MISMATCH")
    if evidence.get("runner", {}).get("git_blob") != git_blob(root, RUNNER_PATH):
        errors.append("EVIDENCE_RUNNER_BLOB_MISMATCH")

    if meta.get("success_result_schema") != SCHEMA_IDS["success_result"]:
        errors.append("RUNNER_SUCCESS_SCHEMA_MISMATCH")
    if meta.get("failure_result_schema") != SCHEMA_IDS["failure_result"]:
        errors.append("RUNNER_FAILURE_SCHEMA_MISMATCH")
    if meta.get("evidence_manifest_schema") != SCHEMA_IDS["evidence_manifest"]:
        errors.append("RUNNER_EVIDENCE_SCHEMA_MISMATCH")
    if meta.get("package_manifest_schema") != SCHEMA_IDS["package_manifest"]:
        errors.append("RUNNER_PACKAGE_SCHEMA_MISMATCH")
    if meta.get("adjudicator_input_schema") != SCHEMA_IDS["adjudicator_input"]:
        errors.append("RUNNER_ADJUDICATOR_SCHEMA_MISMATCH")

    if meta.get("evidence_manifest_template") != EVIDENCE_PATH:
        errors.append("RUNNER_EVIDENCE_PATH_MISMATCH")
    if meta.get("package_manifest_path") != PACKAGE_PATH:
        errors.append("RUNNER_PACKAGE_PATH_MISMATCH")
    if meta.get("output_bundle_name") != package.get("output_bundle_name"):
        errors.append("OUTPUT_BUNDLE_NAME_MISMATCH")

    slot = package.get("p7_authorization_slot", {})
    for field, meta_field in [
        ("source_path","p7_authorization_path"),
        ("schema_id","p7_authorization_schema"),
        ("decision","p7_authorization_decision"),
    ]:
        if slot.get(field) != meta.get(meta_field):
            errors.append(f"P7_SLOT_RUNNER_MISMATCH:{field}")

    if slot.get("binding_mode") != "RUNTIME_REQUIRED_EXACT_GIT_BLOB":
        errors.append("P7_BINDING_MODE_MISMATCH")

    static_members = [m for m in package.get("members", []) if m.get("binding_mode") == "STATIC_COMMITTED"]
    ids = [m.get("id") for m in package.get("members", [])]
    dests = [m.get("destination_name") for m in package.get("members", [])]
    for d in duplicate_values(ids):
        errors.append(f"DUPLICATE_PACKAGE_MEMBER_ID:{d}")
    for d in duplicate_values(dests):
        errors.append(f"DUPLICATE_PACKAGE_DESTINATION:{d}")

    for m in static_members:
        path = m.get("source_path")
        blob = m.get("git_blob")
        if not path or not blob:
            errors.append(f"STATIC_MEMBER_BINDING_INCOMPLETE:{m.get('id')}")
            continue
        try:
            if git_blob(root, path) != blob:
                errors.append(f"STATIC_MEMBER_BLOB_MISMATCH:{m.get('id')}")
        except Exception:
            errors.append(f"STATIC_MEMBER_MISSING:{m.get('id')}")

    e_ids = [m.get("id") for m in evidence.get("members", [])]
    e_dests = [m.get("destination_name") for m in evidence.get("members", [])]
    for d in duplicate_values(e_ids):
        errors.append(f"DUPLICATE_EVIDENCE_MEMBER_ID:{d}")
    for d in duplicate_values(e_dests):
        errors.append(f"DUPLICATE_EVIDENCE_DESTINATION:{d}")

    package_by_id = {m.get("id"): m for m in package.get("members", [])}
    evidence_by_id = {m.get("id"): m for m in evidence.get("members", [])}
    for m in evidence.get("members", []):
        if m.get("binding_mode") == "STATIC_COMMITTED":
            path = m.get("source_path")
            blob = m.get("git_blob")
            if not path or not blob:
                errors.append(f"EVIDENCE_STATIC_BINDING_INCOMPLETE:{m.get('id')}")
                continue
            try:
                if git_blob(root, path) != blob:
                    errors.append(f"EVIDENCE_STATIC_BLOB_MISMATCH:{m.get('id')}")
            except Exception:
                errors.append(f"EVIDENCE_STATIC_MEMBER_MISSING:{m.get('id')}")
    for mid in sorted(set(package_by_id).intersection(evidence_by_id)):
        pm = package_by_id[mid]
        em = evidence_by_id[mid]
        for field in ("binding_mode","source_path","destination_name","git_blob","schema_id","required"):
            if pm.get(field) != em.get(field):
                errors.append(f"PACKAGE_EVIDENCE_MEMBER_MISMATCH:{mid}:{field}")

    self_members = [m for m in package.get("members", []) if m.get("id") == "package_manifest"]
    if len(self_members) != 1 or self_members[0].get("binding_mode") != "RUNTIME_REQUIRED_EXACT_GIT_BLOB" or self_members[0].get("source_path") != PACKAGE_PATH:
        errors.append("PACKAGE_SELF_BINDING_INVALID")
    p7_members = [m for m in package.get("members", []) if m.get("id") == "p7_authorization"]
    if len(p7_members) != 1 or p7_members[0].get("source_path") != slot.get("source_path") or p7_members[0].get("binding_mode") != "RUNTIME_REQUIRED_EXACT_GIT_BLOB":
        errors.append("PACKAGE_P7_BINDING_INVALID")

    schema_bindings = package.get("schema_bindings", {})
    for key, path in SCHEMA_PATHS.items():
        b = schema_bindings.get(key, {})
        if b.get("source_path") != path or b.get("git_blob") != git_blob(root, path):
            errors.append(f"PACKAGE_SCHEMA_BINDING_MISMATCH:{key}")

    stale_scan_paths = [RUNNER_PATH, LOCK_PATH, EVIDENCE_PATH, PACKAGE_PATH] + list(SCHEMA_PATHS.values())
    for rel in stale_scan_paths:
        text = (root / rel).read_text(encoding="utf-8")
        if "v0.0" in text or "execution_lock_v0.0" in text:
            errors.append(f"STALE_VERSION_TOKEN:{rel}")

    authority = lock.get("authority_operationalization", {})
    if not authority.get("frozen_authority_source") or not authority.get("candidate_reproduction_source"):
        errors.append("AUTHORITY_SOURCE_MISSING")
    elif authority["frozen_authority_source"] == authority["candidate_reproduction_source"]:
        errors.append("SAME_SOURCE_AUTHORITY_TAUTOLOGY")

    forbidden = lock.get("zero_science", {})
    if any(bool(forbidden.get(k)) for k in [
        "target_model_load_permitted","diagnostic_executable_launch_permitted",
        "gpu_dispatch_permitted","performance_measurement_permitted"
    ]):
        errors.append("ZERO_SCIENCE_BOUNDARY_VIOLATION")

    if mode == "runtime-preflight":
        p7_path = root / slot.get("source_path","")
        if not p7_path.is_file():
            errors.append("P7_AUTHORIZATION_MISSING")
        else:
            p7 = load_json(p7_path)
            if p7.get("schema") != slot.get("schema_id"):
                errors.append("P7_AUTHORIZATION_SCHEMA_MISMATCH")
            if p7.get("decision") != slot.get("decision"):
                errors.append("P7_AUTHORIZATION_DECISION_MISMATCH")
            auth = p7.get("authorization", {})
            if auth.get("target_model_execution") or auth.get("gpu_dispatch") or auth.get("performance_measurement"):
                errors.append("P7_ZERO_SCIENCE_BOUNDARY_VIOLATION")

    return errors

def get_path(obj, path):
    cur = obj
    for part in path.split("."):
        if isinstance(cur, list):
            cur = cur[int(part)]
        else:
            cur = cur[part]
    return cur

def set_path(obj, path, value):
    parts = path.split(".")
    cur = obj
    for part in parts[:-1]:
        cur = cur[int(part)] if isinstance(cur, list) else cur[part]
    last = parts[-1]
    if isinstance(cur, list):
        cur[int(last)] = value
    else:
        cur[last] = value

def delete_path(obj, path):
    parts = path.split(".")
    cur = obj
    for part in parts[:-1]:
        cur = cur[int(part)] if isinstance(cur, list) else cur[part]
    last = parts[-1]
    if isinstance(cur, list):
        del cur[int(last)]
    else:
        del cur[last]

def validate_synthetic(d):
    e = []
    lock = d["lock"]
    runner = d["runner"]
    sr = d["success_result"]
    fr = d["failure_result"]
    em = d["evidence_manifest"]
    pm = d["package_manifest"]
    ai = d["adjudicator_input"]

    if runner["lock_path"] != lock["path"] or runner["lock_version"] != lock["version"]:
        e.append("RUNNER_LOCK_MISMATCH")
    if sr["lock_path"] != lock["path"] or fr["lock_path"] != lock["path"]:
        e.append("RESULT_LOCK_PATH_MISMATCH")
    if sr["lock_blob"] != lock["blob"] or fr["lock_blob"] != lock["blob"]:
        e.append("RESULT_LOCK_BLOB_MISMATCH")
    if em["lock_source_path"] != lock["path"]:
        e.append("EVIDENCE_LOCK_PATH_MISMATCH")
    if pm["lock_source_path"] != lock["path"]:
        e.append("PACKAGE_LOCK_PATH_MISMATCH")
    canonical_dest = Path(lock["path"]).name
    if em["lock_destination_name"] != canonical_dest or pm["lock_destination_name"] != canonical_dest:
        e.append("WRONG_BUNDLE_DESTINATION_FILENAME")
    if sr["schema"] != runner["success_result_schema"]:
        e.append("RESULT_SCHEMA_VERSION_MISMATCH")
    if fr["schema"] != runner["failure_result_schema"]:
        e.append("FAILURE_SCHEMA_VERSION_MISMATCH")
    if em["runner_blob"] != runner["runner_blob"] or pm["runner_blob"] != runner["runner_blob"]:
        e.append("MANIFEST_RUNNER_MISMATCH")
    if pm["result_schema"] != sr["schema"]:
        e.append("PACKAGE_RESULT_MISMATCH")
    if ai["package_manifest_sha256"] != d["produced_package_manifest_sha256"]:
        e.append("ADJUDICATOR_PACKAGE_HASH_MISMATCH")

    ids = [m["id"] for m in em["members"]]
    if "lock" not in ids or "runner" not in ids:
        e.append("MISSING_EVIDENCE_MEMBER")
    if duplicate_values(ids):
        e.append("DUPLICATE_EVIDENCE_MEMBER")

    if d["authority"]["frozen_source"] == d["authority"]["candidate_source"]:
        e.append("SAME_SOURCE_AUTHORITY_TAUTOLOGY")
    return sorted(set(e))

def apply_fixture(canonical, fixture):
    d = copy.deepcopy(canonical)
    op = fixture["mutation"]["op"]
    if op == "set":
        set_path(d, fixture["mutation"]["path"], fixture["mutation"]["value"])
    elif op == "delete":
        delete_path(d, fixture["mutation"]["path"])
    elif op == "append_duplicate":
        arr = get_path(d, fixture["mutation"]["path"])
        arr.append(copy.deepcopy(arr[int(fixture["mutation"].get("source_index",0))]))
    else:
        raise AssertionError(f"unknown mutation op {op}")
    return d

def fixture_checks(root):
    errors = []
    idx = load_json(root / FIXTURE_INDEX)
    canonical = load_json(root / idx["canonical"])
    base_errors = validate_synthetic(canonical)
    if base_errors:
        errors.append("CANONICAL_FIXTURE_FAILED:" + ",".join(base_errors))
    seen = set()
    for case in idx["negative_cases"]:
        if case["id"] in seen:
            errors.append("DUPLICATE_NEGATIVE_FIXTURE_ID:" + case["id"])
        seen.add(case["id"])
        fixture = load_json(root / case["path"])
        mutated = apply_fixture(canonical, fixture)
        got = validate_synthetic(mutated)
        expected = fixture["expected_failure"]
        if expected not in got:
            errors.append(f"NEGATIVE_FIXTURE_NOT_REJECTED:{case['id']}:{expected}")
    required = {
        "STALE_LOCK_VERSION","WRONG_BUNDLE_DESTINATION_FILENAME","WRONG_GIT_BLOB_FIELD",
        "MISSING_EVIDENCE_MEMBER","DUPLICATE_EVIDENCE_MEMBER","RESULT_SCHEMA_VERSION_MISMATCH",
        "RUNNER_LOCK_MISMATCH","MANIFEST_RUNNER_MISMATCH","PACKAGE_RESULT_MISMATCH",
        "SAME_SOURCE_AUTHORITY_TAUTOLOGY"
    }
    if seen != required:
        errors.append("NEGATIVE_FIXTURE_SET_MISMATCH")
    return errors

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", required=True)
    ap.add_argument("--mode", choices=["p5","runtime-preflight"], default="p5")
    ap.add_argument("--emit-json")
    args = ap.parse_args()
    root = Path(args.repo_root).resolve()

    errors = []
    try:
        errors.extend(actual_repo_checks(root, args.mode))
        errors.extend(fixture_checks(root))
    except Exception as exc:
        errors.append("QA_EXCEPTION:" + repr(exc))

    result = {
        "schema":"arcllm.ttft_m2.atomic_package_qa.runtime.v0.1",
        "mode":args.mode,
        "result":"PASS" if not errors else "FAIL",
        "errors":errors,
        "zero_science":{
            "target_model_loaded":False,
            "diagnostic_executable_launched":False,
            "gpu_dispatch":False,
            "performance_measurement":False,
            "fresh_ttft_observations":0
        }
    }
    if args.emit_json:
        Path(args.emit_json).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if not errors else 2

if __name__ == "__main__":
    sys.exit(main())
