cmake_minimum_required (VERSION 3.8)
project(delegate VERSION 1.0.0)

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}")

file(
	GLOB_RECURSE _public_files
    LIST_DIRECTORIES false
    "${_src_root_path}/public/*.hpp"
)

add_library (delegate INTERFACE)
			
source_group("public"                           FILES ${_public_files})

# Set C++20
set_property(TARGET delegate PROPERTY CXX_STANDARD 20)

target_include_directories(delegate INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/public/")
