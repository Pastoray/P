add_executable(P_bench tokenizer_bm.cpp)
target_link_libraries(P_bench PRIVATE benchmark::benchmark_main)
