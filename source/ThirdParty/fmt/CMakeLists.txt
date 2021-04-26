cmake_minimum_required (VERSION 3.8)
project(fmt VERSION 7.1.3)

add_library(fmt STATIC 
	"include/fmt/chrono.h"
	"include/fmt/color.h"
	"include/fmt/compile.h"
	"include/fmt/core.h"
	"include/fmt/format.h"
	"include/fmt/format-inl.h"
	"include/fmt/locale.h"
	"include/fmt/ostream.h"
	"include/fmt/posix.h"
	"include/fmt/printf.h"
	"include/fmt/ranges.h"
	"include/fmt/os.h"
		
	"src/os.cc"
	"src/format.cc"
)

# Set C++17
set_property(TARGET fmt PROPERTY CXX_STANDARD 17)

target_include_directories(fmt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")


