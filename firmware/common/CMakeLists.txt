set(target_name common)
add_library(${target_name} repl.cpp)
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

