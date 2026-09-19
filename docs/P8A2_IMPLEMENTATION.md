# P8-A2 Implementation

The planner parses the same frozen local Ollama GGUF and reconstructs exact packed row geometry for every tensor.

For a tensor with dims [ne0, ne1, ...], row_bytes is computed from ne0 and ggml_type; rows is product(ne1...). A tensor <=256 MiB stays one piece. An oversize tensor is divided into the maximum whole-row segments that fit the cap.

Each physical piece records logical tensor name, ggml type, segment index, global row start/count, row_bytes, relative file start/end and packed bytes. Pieces are then packed in original file order into contiguous physical arenas; arena boundaries may occur between row-aligned segments.

The output separately records logical segmented tensors and the segment-to-arena address translation required by future embedding and LM-head work.

P8-A2 reuses only GgufReader. No TensorStore mapping, Vulkan allocation, shader compilation or inference is performed.
