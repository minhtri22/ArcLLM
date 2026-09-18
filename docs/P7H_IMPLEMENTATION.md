# P7-H Implementation

P7-H reuses the P7-F timestamp profiler but changes the frozen prefill graph to the P7-G winner: attention projections remain tile8 while FFN gate/up/down use the tile16 kernels. Decode remains byte-path unchanged and untiled. No optimization is introduced in this phase.
