# CMakeList.txt : CMake project for CompCodegen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(CompCodegen)

# Add source to this project's executable
add_subdirectory(third_party)
add_subdirectory(src)

# TODO: Add tests and install targets if needed.
