set(target_name text_exec)
add_executable(${target_name} main.cpp)
target_link_libraries(${target_name} PRIVATE common)