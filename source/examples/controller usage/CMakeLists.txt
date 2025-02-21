cmake_minimum_required (VERSION 3.8)
project("Controller Usage Project" VERSION 1.0.0)

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}")

file( GLOB_RECURSE _private_files LIST_DIRECTORIES false "${_src_root_path}/private/*.cpp" )
file( GLOB_RECURSE _public_files LIST_DIRECTORIES false "${_src_root_path}/public/*.h" )

add_executable (ControllerUsage  
            ${_public_files} 
            ${_private_files}
            )
			
source_group("private"	FILES ${_private_files})
source_group("public" 	FILES ${_public_files})

# Set C++20
set_property(TARGET ControllerUsage PROPERTY CXX_STANDARD 20)

target_link_libraries(ControllerUsage PUBLIC fmt)
target_link_libraries(ControllerUsage PUBLIC delegate)
target_link_libraries(ControllerUsage PUBLIC CommonEx)
