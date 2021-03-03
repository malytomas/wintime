cmake_minimum_required(VERSION 3.13)
project(wintime CXX)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	message(FATAL_ERROR "In-source build is disabled. Remove the already generated files and start again from dedicated build directory.")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(FATAL_ERROR "CMAKE_BUILD_TYPE needs to be set")
endif()

foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
	string(TOUPPER ${conf} conf_upper)
	string(TOLOWER ${conf} conf_lower)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper} "${CMAKE_BINARY_DIR}/result/${conf_lower}")
endforeach(conf)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++${CMAKE_CXX_STANDARD}")
endif()

file(GLOB_RECURSE wintime-sources "sources/*")
add_executable(wintime ${wintime-sources})

set_target_properties(wintime PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:wintime>")
