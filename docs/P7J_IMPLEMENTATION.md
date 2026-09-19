# P7-J Implementation

The optimized Q4_K loader maps local x to output row and local y to one aligned group of four K elements. For each K32 tile it loads d/dmin/scale/min once per four weights and extracts four quant nibbles from one 32-bit packed word.

The optimized Q6_K loader uses the same 4-weight mapping. It loads d and the relevant scale once per four weights, then extracts four ql bytes and four qh 2-bit fields from aligned/unaligned 32-bit word loads. `load_u32_unaligned` composes at most two storage-buffer uint loads when a Q6_K 210-byte block causes non-4-byte alignment.

Shared-memory shape, workgroup size, token tile16, K32 tile, barriers, output layout and dispatch graph stay unchanged.
