cmake_minimum_required(VERSION 3.16)
project(arcllm_q2_llama_adapter LANGUAGES C CXX)

if(NOT DEFINED LLAMA_CPP_DIR)
  message(FATAL_ERROR "LLAMA_CPP_DIR is required")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_COMMON OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_APP OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_UI OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_IS_DEV OFF CACHE BOOL "" FORCE)
set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
set(GGML_VULKAN ON CACHE BOOL "" FORCE)
set(GGML_BACKEND_DL OFF CACHE BOOL "" FORCE)

add_subdirectory("${LLAMA_CPP_DIR}" "${CMAKE_BINARY_DIR}/llama-src")

add_executable(q2_llama_adapter q2_llama_adapter.cpp)
target_link_libraries(q2_llama_adapter PRIVATE llama)
target_compile_features(q2_llama_adapter PRIVATE cxx_std_17)

add_executable(i003_llama_adapter i003_llama_adapter.cpp)
target_link_libraries(i003_llama_adapter PRIVATE llama)
target_compile_features(i003_llama_adapter PRIVATE cxx_std_17)


add_executable(token_xray_r1_tokenizer_probe r1_tokenizer_probe.cpp)
target_link_libraries(token_xray_r1_tokenizer_probe PRIVATE llama)
target_compile_features(token_xray_r1_tokenizer_probe PRIVATE cxx_std_17)
