# CMakeList.txt : CMake project for CompCodegen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set (CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable (CompCodegen "CompCodegen.cpp" "CompCodegen.h")
set_property(TARGET CompCodegen PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(CompCodegen PRIVATE nlohmann_json::nlohmann_json)


# TODO: Add tests and install targets if needed.
