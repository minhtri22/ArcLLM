# P7-N Implementation

The optimized shader keeps the P7-L local_size 8x8, row tile 8, token tile 16 and K tile 32. It loads the same two Q4_K weight tiles plus one activation tile, accumulates gate and up exactly as P7-L, then applies SiLU(gate)*up after the K reduction and writes only s.

Bindings shrink from five to four: gate weights, up weights, activation input, final s output. The separate p7_swiglu dispatch disappears only in prefill. FFN down consumes the same b_s buffer as before.

Expected dispatch counts:
- P7-L baseline prefill: 441.
- P7-N optimized prefill: 413.
- unchanged decode: 469.
