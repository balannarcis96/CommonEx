#################################################
# 		    CommonEx by Narcis96				#
#################################################

cmake_minimum_required (VERSION 3.8)

project ("CommonEx" VERSION 1.0.0)

#Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#We remove exceptions and rtti and we add 
#	-multiprocessor compilation (/MP) 
#	-latest c++ version(/std:c++latest) 
#	-intrinsic functions(/Oi) 
#	-favor speed (/Ot)
set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /std:c++latest /MP /Oi /Ot")

# Include sub-projects.

#ThirdPary
add_subdirectory ("thirdparty")

#Libs
add_subdirectory ("CommonEx")

#set_target_properties(CommonEx PROPERTIES FOLDER "CommonEx")

#Examples
add_subdirectory("examples")

set_target_properties(AsyncIOUsage PROPERTIES FOLDER "Examples")
set_target_properties(BasicUsage PROPERTIES FOLDER "Examples")
set_target_properties(ConnectionUsage PROPERTIES FOLDER "Examples")
set_target_properties(MemoryUsage PROPERTIES FOLDER "Examples")
set_target_properties(PacketsUsage PROPERTIES FOLDER "Examples")
set_target_properties(TaskUsage PROPERTIES FOLDER "Examples")
set_target_properties(WorkerUsage PROPERTIES FOLDER "Examples")
set_target_properties(AsyncDispatcherUsage PROPERTIES FOLDER "Examples")
set_target_properties(SystemUsage PROPERTIES FOLDER "Examples")
set_target_properties(ControllerUsage PROPERTIES FOLDER "Examples")

#Tests
add_subdirectory("tests")

set_target_properties(CommonTests PROPERTIES FOLDER "Tests")
