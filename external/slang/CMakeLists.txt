add_library(slang slang.cpp)
target_link_directories(slang PUBLIC ${APH_EXTERNAL_DIR}/slang/bin/linux-x64/release/)
target_link_libraries(slang PUBLIC libslang.so)
