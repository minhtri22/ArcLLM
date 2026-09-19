# P7-K Implementation

The optimized kernel keeps local_size 8x8 and token tile 16. Workgroup X now covers 16 output rows. Each lane owns row_a=row0+local_x and row_b=row0+local_x+8, and computes both token halves for both rows.

Shared weight tile grows from 8x32 to 16x32 (256 -> 512 floats). Shared activation tile remains 16x32 (512 floats). K tile, barriers, packed dequant math, accumulation order per output, output layout and token dispatch geometry remain unchanged.

This isolates output-row reuse without changing workgroup size or introducing fusion.
