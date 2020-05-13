# CMakeList.txt : CMake project for CompCodegen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json EXCLUDE_FROM_ALL)
add_subdirectory(fmtlib EXCLUDE_FROM_ALL)
# TODO: Add tests and install targets if needed.




add_library(cppcoro "${CMAKE_CURRENT_SOURCE_DIR}/cppcoro/lib/static_thread_pool.cpp")

target_include_directories(cppcoro PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cppcoro/include")


add_library(cppcoro::cppcoro ALIAS cppcoro)

set_target_properties(cppcoro PROPERTIES
            CXX_STANDARD 20
            )
