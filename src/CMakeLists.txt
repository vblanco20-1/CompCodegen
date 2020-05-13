# CMakeList.txt : CMake project for CompCodegen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)
set (CMAKE_CXX_STANDARD 20)

add_compile_options("/await")

# Add source to this project's executable.
add_executable (CompCodegen "CompCodegen.cpp" "CompCodegen.h" "tokenizer.h")
set_property(TARGET CompCodegen PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(CompCodegen PRIVATE nlohmann_json::nlohmann_json fmt::fmt cppcoro::cppcoro)


target_precompile_headers(CompCodegen PRIVATE <cppcoro/generator.hpp> <nlohmann/json.hpp> <fmt/format.h> <iostream> <fstream>  <charconv>)
# TODO: Add tests and install targets if needed.
