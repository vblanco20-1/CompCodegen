# CMakeList.txt : CMake project for CompCodegen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set (CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable (CompCodegen "CompCodegen.cpp" "CompCodegen.h")

# TODO: Add tests and install targets if needed.
