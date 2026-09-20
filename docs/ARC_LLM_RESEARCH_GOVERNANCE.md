# ArcLLM — Quyết định quản trị nghiên cứu mới

**Trạng thái:** CÓ HIỆU LỰC  
**Phạm vi:** Chỉ áp dụng cho dự án ArcLLM  
**Mức ưu tiên:** Governance này cao hơn các roadmap, kế hoạch phase và kế hoạch nghiên cứu P8/P9 riêng lẻ đối với mọi quyết định nghiên cứu được mở sau thời điểm có hiệu lực.

## 0. Hiệu lực, thứ tự ưu tiên và chuyển tiếp

Từ thời điểm tài liệu này có hiệu lực, ArcLLM chuyển sang cơ chế nghiên cứu có điểm hội tụ bắt buộc.

Nếu có xung đột giữa tài liệu này và một kế hoạch P8/P9 chưa chạy, tài liệu này có ưu tiên cao hơn.

Một experiment đã được freeze hợp lệ trước thời điểm governance có hiệu lực không được sửa contract, metric hoặc threshold chỉ để phù hợp với governance mới. Experiment đó phải hoàn tất hoặc được adjudicate theo frozen contract ban đầu.

Cụ thể, **P8-G6 — fresh production-semantic separation confirmation** đã được implementation + static-QA lock tại commit:

`7da34af2241091459b50905bfc008c7c6ba623ef`

P8-G6 tiếp tục được chạy và adjudicate theo contract đã freeze. Governance mới không thay đổi P8-G6, không thay đổi threshold của P8-G6 và không biến P8-G6 thành bằng chứng end-to-end.

Sau khi P8-G6 được adjudicate, mọi quyết định tiếp theo phải tuân thủ đầy đủ governance này và quay về chuỗi ưu tiên Q1 → Q2 → Q3.

---

## 1. Mục đích

Từ thời điểm tài liệu này có hiệu lực, ArcLLM chuyển từ cách làm mở rộng tuần tự theo từng phát hiện kỹ thuật sang cơ chế nghiên cứu có điểm hội tụ bắt buộc.

Mục tiêu không phải tạo thêm nhiều phase, benchmark hoặc subsystem.

Mục tiêu là buộc ArcLLM trả lời một câu hỏi cuối cùng có thể kiểm chứng:

> **ArcLLM có tạo ra một execution regime hữu ích và tái lập được trên phần cứng mục tiêu, so với một baseline phù hợp dưới điều kiện matched hay không?**

Mọi công việc tiếp theo phải phục vụ trực tiếp cho câu hỏi này.

---

## 2. Scope duy nhất của ArcLLM

ArcLLM chỉ nghiên cứu và chứng minh các vấn đề trực tiếp liên quan đến:

- execution của LLM trên phần cứng mục tiêu;
- đường thực thi GPU/CPU/RAM/VRAM liên quan trực tiếp đến ArcLLM;
- memory movement;
- tensor/kernel execution;
- streaming/offload;
- scheduling bên trong runtime;
- khả năng chạy model thật;
- latency;
- throughput;
- memory/resource envelope;
- stability;
- reproducibility;
- comparison với matched baseline.

Không mở rộng ArcLLM sang các bài toán nghiên cứu khác không cần thiết để trả lời claim trên.

---

## 3. Ba câu hỏi bắt buộc phải trả lời

ArcLLM phải hội tụ về ba câu hỏi.

### Q1 — Feasibility

ArcLLM có chạy được model thật end-to-end trên phần cứng mục tiêu hay không?

Không được dùng các kết quả sau làm bằng chứng cuối cho Q1:

- simulator;
- synthetic-only workload;
- isolated kernel benchmark;
- microbenchmark;
- mocked execution;
- partial pipeline không tạo được inference hoàn chỉnh.

Kết quả hợp lệ phải đến từ model thật và inference thật.

### Q2 — Performance / Resource Envelope

Khi ArcLLM chạy được model thật, phải đo dưới cấu hình frozen:

- model;
- quantization;
- context;
- prompt/input;
- output length;
- hardware;
- runtime configuration;
- warmup;
- measurement procedure.

Tối thiểu phải thu:

- TTFT;
- decode throughput;
- end-to-end latency;
- peak RAM;
- peak GPU/shared memory;
- CPU utilization;
- GPU utilization nếu đo được;
- stability/error rate;
- resource movement quan trọng đối với architecture ArcLLM.

Không được kết luận từ một metric riêng lẻ.

### Q3 — Regime Advantage

ArcLLM phải xác định liệu nó có tạo ra ít nhất một regime advantage có ý nghĩa thực tế hay không.

Regime advantage có thể là một trong các dạng sau:

- chạy được workload mà matched baseline không chạy được;
- sử dụng ít bộ nhớ hơn;
- giảm peak resource;
- hỗ trợ model/context lớn hơn;
- TTFT tốt hơn;
- throughput tốt hơn;
- latency tốt hơn;
- streaming/offload hiệu quả hơn;
- ổn định hơn trong cùng resource constraint.

Không bắt buộc ArcLLM phải thắng mọi metric.

Nhưng advantage phải:

- được định nghĩa trước;
- đo được;
- có baseline matched;
- tái lập được;
- không phụ thuộc vào cherry-pick một benchmark thuận lợi.

---

## 4. Quy tắc hội tụ

Từ thời điểm này:

**Không được mở thêm một architecture experiment chỉ vì subsystem hiện tại chưa tối ưu.**

Một architecture experiment mới chỉ được phép mở khi:

1. model thật đã được chạy;
2. benchmark end-to-end đã xác định bottleneck;
3. bottleneck đó chặn trực tiếp một trong Q1, Q2 hoặc Q3;
4. tồn tại hypothesis cụ thể rằng thay đổi kiến trúc có thể giải quyết bottleneck;
5. hypothesis có falsification condition;
6. experiment có stop condition.

Không thỏa đủ sáu điều kiện trên thì không mở experiment mới.

---

## 5. Cấm phase proliferation

Không được tự động tạo chuỗi:

```text
Pn → Pn-A → Pn-B → Pn-C → Pn-D ...
```

chỉ vì mỗi lần thực nghiệm phát hiện thêm một vấn đề.

Phase mới chỉ được tạo khi nó đại diện cho một câu hỏi khoa học hoặc kỹ thuật khác biệt rõ ràng.

Các hoạt động sau không mặc định cần phase mới:

- bug fix;
- evidence repair;
- logging improvement;
- script correction;
- packaging;
- CI repair;
- documentation update;
- reproducibility rerun;
- implementation cleanup.

Những hoạt động này phải được xử lý trong lineage hiện tại trừ khi thay đổi claim hoặc experimental contract.

---

## 6. Real-model-first rule

Ngay khi execution path đủ khả năng chạy model thật:

**Model thật có ưu tiên cao hơn mọi microbenchmark mới.**

Không được trì hoãn end-to-end validation để tiếp tục tối ưu các subsystem riêng lẻ.

Microbenchmark chỉ được mở lại sau khi end-to-end benchmark chỉ ra rõ bottleneck cần nghiên cứu.

---

## 7. Baseline rule

Mọi claim về performance hoặc resource advantage phải có matched baseline.

Baseline phải cố gắng giữ giống nhau về:

- model;
- quant;
- context;
- prompt/input;
- output length;
- hardware;
- measurement procedure;
- warmup;
- number of repetitions;
- relevant runtime constraints.

Nếu không thể matched hoàn toàn, khác biệt phải được ghi rõ và claim phải bị giới hạn tương ứng.

Không được dùng baseline yếu có chủ ý để tạo advantage giả.

---

## 8. Freeze-before-run

Mỗi confirmatory run phải freeze trước:

- hypothesis;
- configuration;
- model;
- dataset/prompt set;
- baseline;
- primary metrics;
- secondary metrics;
- success condition;
- failure condition;
- resource limits;
- seed nếu có;
- repetition count;
- evidence schema.

Không thay đổi các điều trên sau khi xem outcome.

Nếu thay đổi, run sau phải được xem là study mới.

---

## 9. Không tune theo outcome

Không áp dụng chu trình:

```text
FAIL → chỉnh architecture → chạy lại → tiếp tục chỉnh cho đến khi PASS
```

Thay vào đó:

### Nếu FAIL do implementation defect

Được sửa và rerun nếu chứng minh defect khiến experiment không thực thi đúng frozen design.

### Nếu FAIL do measurement defect

Được sửa measurement và rerun nếu measurement không đo đúng metric đã freeze.

### Nếu FAIL là kết quả hợp lệ

Phải ghi nhận failure.

Chỉ được mở hypothesis tiếp theo nếu failure xác định được một bottleneck mới có ý nghĩa trực tiếp đối với claim trung tâm.

Không tune chỉ để làm metric vượt threshold.

---

## 10. Stop rules

ArcLLM phải dừng một research direction khi xảy ra một trong các điều kiện sau.

### STOP-A — Feasibility failure

Execution path không thể chạy workload mục tiêu sau khi:

- implementation đã được xác nhận đúng;
- limitation đã được định vị;
- một repair attempt hợp lý đã được thực hiện.

Khi đó đóng claim tương ứng.

### STOP-B — No regime advantage

ArcLLM chạy được nhưng matched benchmark không cho thấy advantage có ý nghĩa trên bất kỳ regime đã freeze nào.

Khi đó:

- ghi nhận technical feasibility;
- không tiếp tục tối ưu vô hạn để tìm benchmark thắng;
- chỉ tiếp tục nếu xuất hiện một hypothesis mới độc lập và có cơ sở trước khi xem outcome mới.

### STOP-C — Advantage reproduced

Nếu ArcLLM cho thấy regime advantage và advantage tái lập được trên fresh run:

- đóng feasibility/performance validation;
- ghi claim được support;
- chuyển sang engineering/productization hoặc generalization nếu cần.

Không tiếp tục chứng minh lại cùng một claim bằng nhiều phase tương tự.

### STOP-D — Architecture cost exceeds value

Nếu complexity, implementation burden hoặc resource overhead của ArcLLM lớn hơn rõ rệt giá trị regime advantage đạt được, phải ghi nhận điều đó như một kết quả.

Không che giấu engineering cost khỏi final verdict.

---

## 11. Evidence contract

Mỗi run quan trọng phải tạo tối thiểu:

- frozen configuration;
- environment fingerprint;
- commit SHA;
- model identity/hash nếu phù hợp;
- raw measurements;
- summarized metrics;
- logs;
- failure records;
- baseline results;
- comparison results;
- run timestamp;
- evidence manifest.

Không được chỉ giữ screenshot hoặc console output rời rạc làm evidence chính.

Evidence phải đủ để người khác xác định:

> **Code nào + config nào + model nào + machine nào → tạo ra kết quả nào.**

---

## 12. Reproducibility rule

Một advantage quan trọng không được xem là established chỉ từ một run.

Sau khi initial run tạo ra signal tích cực:

1. freeze claim;
2. không tune thêm;
3. chạy fresh reproduction;
4. so với cùng baseline;
5. adjudicate.

Nếu reproduction fail, kết quả phải được đánh dấu unstable hoặc unresolved.

---

## 13. Verdict cuối

ArcLLM phải kết thúc validation bằng một trong các verdict sau.

### FEASIBILITY_NOT_ESTABLISHED

Không chứng minh được execution end-to-end hợp lệ.

### FEASIBLE_NO_DEMONSTRATED_ADVANTAGE

ArcLLM chạy được nhưng chưa chứng minh regime advantage so với matched baseline.

### REGIME_ADVANTAGE_SUPPORTED

ArcLLM chứng minh và reproduce được advantage trong một regime được mô tả rõ.

### UNRESOLVED

Chỉ dùng khi evidence không đủ để adjudicate vì nguyên nhân hợp lệ.

**UNRESOLVED không được sử dụng để né tránh một kết quả negative.**

---

## 14. Hình thức claim cuối cùng

Claim cuối phải có dạng cụ thể như:

> Với **[model]**, **[quant]**, **[context]** trên **[hardware]**, ArcLLM sử dụng **[execution configuration]** và đạt **[measured outcome]**, so với **[matched baseline]** đạt **[baseline outcome]**.

Claim phải nêu rõ:

- workload;
- regime;
- constraint;
- baseline;
- metric;
- limitation.

Không được kết luận chung chung như:

- “ArcLLM nhanh hơn”;
- “ArcLLM tốt hơn”;
- “ArcLLM tối ưu hơn”;

nếu evidence chỉ hỗ trợ một regime cụ thể.

---

## 15. Quy tắc đối với agent

Agent làm việc trên ArcLLM phải tuân thủ:

1. Không mở rộng scope ngoài ArcLLM.
2. Không tự tạo nghiên cứu mới chỉ vì thấy một ý tưởng thú vị.
3. Không tự tạo thêm phase nếu không cần thiết.
4. Không thay đổi frozen metric sau outcome.
5. Không tune để ép PASS.
6. Không dùng microbenchmark thay cho model thật.
7. Không tuyên bố advantage nếu chưa có matched baseline.
8. Không tuyên bố established nếu chưa reproduction.
9. Mọi bước phải nói rõ nó đang phục vụ Q1, Q2 hay Q3.
10. Nếu một bước không phục vụ Q1, Q2 hoặc Q3, mặc định không thực hiện.
11. Sau mỗi bước phải xác định bước tiếp theo đúng về mặt khoa học, không phải bước dễ làm nhất.
12. Khi đủ evidence để adjudicate, phải adjudicate thay vì mở thêm experiment.

---

## 16. Trình tự ưu tiên mới

Từ thời điểm tài liệu này có hiệu lực, thứ tự công việc là:

```text
Current validated infrastructure
        ↓
Real model end-to-end execution
        ↓
Frozen matched benchmark
        ↓
Resource + performance characterization
        ↓
Identify regime advantage or lack thereof
        ↓
Fresh reproduction
        ↓
Final adjudication
```

Chỉ khi một bước trong chuỗi trên bị chặn bởi một bottleneck đã được chứng minh thì mới được mở experiment phụ để giải quyết chính bottleneck đó.

Sau khi bottleneck được giải quyết, phải quay lại chuỗi chính.

---

## 17. Nguyên tắc cao nhất

ArcLLM không được đánh giá bằng số phase đã hoàn thành.

ArcLLM được đánh giá bằng khả năng đưa ra một kết luận có thể kiểm chứng:

> **Nó chạy được gì, trên phần cứng nào, với chi phí tài nguyên nào, so với baseline nào, và advantage tồn tại chính xác ở đâu.**

Một kết quả negative rõ ràng có giá trị hơn một chuỗi experiment không có điểm kết thúc.

Một result được reproduce có giá trị hơn nhiều subsystem benchmark riêng lẻ.

Từ thời điểm này, mọi quyết định kỹ thuật của ArcLLM phải phục vụ hội tụ về verdict, không phục vụ việc kéo dài research tree.
