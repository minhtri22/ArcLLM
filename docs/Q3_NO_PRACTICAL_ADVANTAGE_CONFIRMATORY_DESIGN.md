# ArcLLM Q3 — Thiết kế xác nhận No-Practical-Advantage

**Trạng thái:** DESIGN FROZEN / EXECUTION CLOSED  
**Ngày:** 2026-09-20  
**Cha:** Q2 `Q2_MATCHED_CHARACTERIZATION_COMPLETE` tại commit `b231631aed37b1ddd60a22bb75ab0d1ed51e38fc`

## 1. Câu hỏi khoa học

Q2 đã chứng minh ArcLLM chạy end-to-end và tạo được matched characterization hợp lệ, nhưng không chứng minh kiến trúc hiện tại tối ưu.

Q3 chỉ trả lời câu hỏi:

> **Với chính kiến trúc ArcLLM hiện tại, trên cùng model/hardware và hai regime W-S/W-C đã khóa, có tồn tại ít nhất một practical advantage đủ lớn và tái lập được so với matched llama.cpp hay không?**

Q3 **không phải** phase tối ưu. Không được sửa kernel, scheduling, memory path, workload, threshold hoặc baseline để cố tạo PASS.

## 2. Hypothesis

### H-NPA — bounded no-practical-advantage

Trong cả hai workload W-S và W-C, kiến trúc ArcLLM hiện tại **không chứng minh được một advantage có ý nghĩa thực tế** và tái lập qua hai fresh sessions.

### Điều kiện falsify H-NPA

H-NPA chỉ bị falsify khi tồn tại **cùng một workload + cùng một primary benefit dimension** thỏa practical-advantage gate độc lập trong **cả Session A và Session B**.

Một signal chỉ xuất hiện trong một session, một metric phụ, hoặc một benchmark mới không đủ.

## 3. Scope bị khóa

Giữ nguyên Q2:

- exact model SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`;
- size: 4,683,074,048 bytes;
- baseline: llama.cpp `v0.4.1`, commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- hardware: Core Ultra 7 258V + Arc 140V;
- context 4096;
- output 32 tokens;
- exact raw-token workloads W-S và W-C từ `config/q2_workloads.json`;
- F32 K/V baseline;
- full GPU offload baseline.

Bị cấm trước adjudication Q3:

- ArcLLM architecture change;
- kernel tuning;
- scheduler tuning;
- workload search;
- đổi baseline;
- đổi threshold sau khi thấy outcome;
- đưa NEXUS vào runtime hoặc design Q3.

## 4. Fresh reproduction

Q3 gồm **2 fresh sessions**, mỗi session là process/run độc lập.

Mỗi cell:

- 1 warmup;
- 5 measured attempts;
- không replace failed attempt.

Tổng:

```text
2 sessions
× 2 systems
× 2 workloads
× 5 measured attempts
= 40 fresh measured attempts
```

Order được counterbalance:

**Session A**

```text
ArcLLM W-S
llama.cpp W-S
llama.cpp W-C
ArcLLM W-C
```

**Session B**

```text
llama.cpp W-S
ArcLLM W-S
ArcLLM W-C
llama.cpp W-C
```

AC power, Windows power scheme, driver và hardware phải giữ matched.

## 5. Primary metrics

Chỉ bốn metric có quyền tạo practical advantage:

1. TTFT;
2. decode throughput;
3. end-to-end latency;
4. peak working set.

Private bytes và CPU utilization vẫn được ghi, nhưng chỉ là supporting metrics. Chúng **không được tự mình tạo verdict advantage**.

GPU counters tiếp tục conditional vì Q2 đã cho thấy Windows GPU Engine peak có thể >100%.

## 6. Practical-effect thresholds

Threshold được khóa trước Q3 execution và không được derive lại từ Q3 outcome.

### Primary benefit

Một workload phải đạt ít nhất một trong:

- TTFT: `Arc / baseline <= 0.90` — nhanh hơn ít nhất 10%;
- decode throughput: `Arc / baseline >= 1.10` — cao hơn ít nhất 10%;
- E2E: `Arc / baseline <= 0.90` — thấp hơn ít nhất 10%;
- working set: `Arc / baseline <= 0.85` — thấp hơn ít nhất 15%.

### Blocking-harm guard

Đồng thời **tất cả** phải đúng:

- TTFT `Arc / baseline <= 1.10`;
- decode throughput `Arc / baseline >= 0.90`;
- E2E `Arc / baseline <= 1.10`;
- working set `Arc / baseline <= 1.10`.

Ý nghĩa: ArcLLM không được gọi là có practical advantage chỉ vì thắng một metric nhỏ trong khi trả giá rất lớn ở metric chính khác.

### Stability

Candidate workload phải có:

- ArcLLM 5/5 measured attempts thành công;
- llama.cpp 5/5 measured attempts thành công;

ở **cả hai sessions**.

## 7. Quy tắc adjudication

### REGIME_ADVANTAGE_SUPPORTED

Chỉ khi cùng workload + cùng primary benefit dimension vượt threshold và qua blocking-harm guard ở **cả Session A và Session B**.

Claim cuối chỉ được giới hạn vào regime đã chứng minh.

### FEASIBLE_NO_DEMONSTRATED_ADVANTAGE

Nếu không có candidate nào thỏa điều kiện trên sau 40 fresh attempts.

Đây là **negative verdict hợp lệ**, không phải unresolved.

Khi verdict này xảy ra:

> **Đóng kiến trúc ArcLLM hiện tại. Không tiếp tục iterative tuning, benchmark search hoặc phase proliferation.**

### UNRESOLVED

Chỉ dùng khi measurement/evidence bị invalid tới mức không thể adjudicate. Không được dùng để né kết quả negative.

## 8. Ý nghĩa của Q2 đối với Q3

Q2 hiện mô tả khoảng cách rất lớn:

- TTFT ArcLLM khoảng 9–10× latency của baseline;
- decode throughput chỉ khoảng 2.5–3.1% baseline;
- E2E latency khoảng 24–39× baseline;
- working set khoảng 1.83× baseline.

Private bytes thấp hơn khoảng 3% và CPU utilization thấp hơn không đủ để tạo practical advantage vì Q3 đã định nghĩa chúng là supporting metrics.

Q3 không được “tìm cách đảo” dữ liệu này. Q3 chỉ kiểm tra xem conclusion no-practical-advantage có reproduce hay không.

## 9. Boundary với NEXUS

NEXUS **không tham gia Q3**.

Nếu Q3 kết thúc bằng `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`:

1. đóng current ArcLLM architecture line;
2. hoàn thành final ArcLLM verdict;
3. sau đó mới được mở một **Architecture Intervention Review** riêng;
4. review có thể đọc các finding đã chứng minh từ NEXUS để sinh hypothesis;
5. NEXUS chỉ là **hypothesis source**, không phải ArcLLM evidence;
6. không được chuyển PASS từ NEXUS sang ArcLLM;
7. chỉ được reopen khi một finding NEXUS có causal/mechanistic mapping cụ thể tới bottleneck Q2/Q3;
8. nếu reopen, đó là **successor architecture study**, không phải sửa Q3 sau outcome.

Như vậy nếu Q3 negative, ta dừng việc “vá ArcLLM hiện tại”. Một kiến trúc kế tiếp chỉ tồn tại nếu có lý do cơ chế đủ mạnh để biện minh chi phí nghiên cứu mới.

## 10. Stop condition

Q3 là điểm hội tụ.

```text
Q3 fresh reproduction
        ↓
same regime advantage reproduces?
        ├─ YES → REGIME_ADVANTAGE_SUPPORTED
        └─ NO  → FEASIBLE_NO_DEMONSTRATED_ADVANTAGE
                    ↓
              close current ArcLLM architecture
                    ↓
              optional post-verdict
              Architecture Intervention Review
              (NEXUS may seed hypotheses only)
```

Không có Q3-A/Q3-B để tìm cách PASS sau outcome.

## 11. Trạng thái execution

Thiết kế này chỉ freeze scientific contract.

**Q3 execution vẫn CLOSED.**

Bước tiếp theo chỉ được phép là implementation/zero-measurement qualification của runner theo đúng contract này. Không được chạy 40 fresh attempts cho đến khi implementation lock và independent preflight PASS.
