cmake_minimum_required (VERSION 3.8)
project("CommonEx Project" VERSION 1.0.0)

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}")

file( GLOB_RECURSE _private_files LIST_DIRECTORIES false "${_src_root_path}/private/*.cpp" )
file( GLOB_RECURSE _public_files LIST_DIRECTORIES false "${_src_root_path}/public/*.h" )

add_library (CommonEx STATIC 
            ${_private_files}
            ${_public_files} 
            )
			
source_group("private"	FILES ${_private_files})
source_group("public" 	FILES ${_public_files})

# Set C++20
set_property(TARGET CommonEx PROPERTY CXX_STANDARD 20)

target_include_directories(CommonEx PUBLIC "${_src_root_path}/public/")

target_link_libraries(CommonEx PUBLIC fmt)
target_link_libraries(CommonEx PUBLIC mysql)
target_link_libraries(CommonEx PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(CommonEx PUBLIC rapidXML)
target_link_libraries(CommonEx PUBLIC Datastore_Lib)
