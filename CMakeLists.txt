cmake_minimum_required(VERSION 3.9)
project(repwatchd)

cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)
include("CMakeLocal.${HOSTNAME}.cmake")

find_package(Boost 1.66.0 COMPONENTS system coroutine REQUIRED)
set(Boost_DEFINITIONS BOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_path(json_INCLUDE_DIRS nlohmann/json.hpp HINTS "${JSON_ROOT}/include")

find_path(lib3dprnet_INCLUDE_DIRS core/config.hpp HINTS "${LIB3DPRNET_ROOT}")

find_path(lib3dprnet_INCLUDE_DIRS core/config.hpp HINTS "${LIB3DPRNET_ROOT}")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(lib3dprnet_LIB_DIR "${LIB3DPRNET_ROOT}/cmake-build-debug${LIB3DPRNET_BUILD_SUFFIX}")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(lib3dprnet_LIB_DIR "${LIB3DPRNET_ROOT}/cmake-build-release${LIB3DPRNET_BUILD_SUFFIX}")
else()
    message(FATAL_ERROR "Unknown build type ${CMAKE_BUILD_TYPE}")
endif()
find_library(lib3dprnet_LIBRARIES 3dprnet HINTS "${lib3dprnet_LIB_DIR}")

set(CMAKE_CXX_STANDARD 14)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -m64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
endif()

add_executable(repwatchd
        main.cpp
        commandline.cpp
        commandline.hpp
        fhem_service.cpp
        fhem_service.hpp)
target_compile_definitions(repwatchd PRIVATE ${Boost_DEFINITIONS})
target_include_directories(repwatchd PRIVATE ${Boost_INCLUDE_DIRS} ${json_INCLUDE_DIRS} ${lib3dprnet_INCLUDE_DIRS})
target_link_libraries(repwatchd ${Boost_LIBRARIES} ${lib3dprnet_LIBRARIES} stdc++fs)
if(WIN32)
    target_link_libraries(repwatchd ws2_32)
else()
    target_link_libraries(repwatchd pthread)
endif()
