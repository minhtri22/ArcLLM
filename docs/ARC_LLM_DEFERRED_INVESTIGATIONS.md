# ArcLLM — Deferred Investigation Registry

**Trạng thái:** APPEND-ONLY  
**Mục đích:** Ghi nhận các phát hiện cần làm rõ nhưng không cho phép chúng tự động mở nhánh nghiên cứu mới.  
**Governance:** `docs/ARC_LLM_RESEARCH_GOVERNANCE.md`, mục 17.

## Quy tắc sử dụng

1. Ghi item ngay khi phát hiện nếu nó đáng lưu lại nhưng chưa trực tiếp chặn Q1/Q2/Q3.
2. Không investigate sâu chỉ vì item xuất hiện trong registry.
3. Không dùng registry như danh sách bắt buộc phải hoàn thành trước final verdict.
4. Chỉ promote sớm nếu item trở thành blocker trực tiếp, evidence-integrity issue hoặc implementation/measurement defect của frozen study.
5. Sau final adjudication mới thực hiện post-verdict impact review toàn registry.
6. Registry là append-only đối với lịch sử item: không xóa observation hoặc outcome cũ. Khi trạng thái thay đổi, append một update có ngày và lý do.

## Trạng thái hợp lệ

Trong quá trình validation:

- `DEFERRED`
- `PROMOTED_BLOCKER`
- `RESOLVED_BY_EXISTING_EVIDENCE`

Sau post-verdict impact review:

- `ARCHIVE_NO_ACTION`
- `ENGINEERING_FOLLOWUP`
- `DEFER_FURTHER`
- `RESEARCH_REOPEN_CANDIDATE`

## Template item

```markdown
### DI-XXXX — <tên ngắn>

- Date:
- Status: DEFERRED
- Source / commit / evidence:
- Observation:
- Open question:
- Why deferred:
- Possible impact on Q1/Q2/Q3/final claim:
- Preliminary impact note:
- Promotion trigger:
- Notes:
```

Nếu item được promote:

```markdown
#### Update YYYY-MM-DD — PROMOTED_BLOCKER

- New evidence:
- Blocked question: Q1 / Q2 / Q3
- Why the main path cannot continue without resolving this:
- Hypothesis:
- Falsification condition:
- Stop condition:
```

Nếu item được review sau final adjudication:

```markdown
#### Post-verdict impact review YYYY-MM-DD

- Potential to change final verdict:
- Potential to change claim limitation/scope:
- Engineering/productization value:
- Independent regime-advantage potential:
- Expected investigation cost:
- Evidence strength:
- Disposition: ARCHIVE_NO_ACTION / ENGINEERING_FOLLOWUP / DEFER_FURTHER / RESEARCH_REOPEN_CANDIDATE
- Rationale:
```

## Items

Chưa có item nào được ghi nhận tại thời điểm tạo registry.
