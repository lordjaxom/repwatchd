cmake_minimum_required(VERSION 3.9)
project(repwatchd)

set(CMAKE_CXX_STANDARD 14)
if(WIN32)
    set(CMAKE_CXX_FLAGS "-Wa,-mbig-obj")
endif()

find_package(Boost 1.66.0 COMPONENTS system coroutine REQUIRED)
set(Boost_DEFINITIONS BOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_path(json_INCLUDE_DIRS nlohmann/json.hpp HINTS "${JSON_ROOT}/include")

find_path(lib3dprnet_INCLUDE_DIRS core/config.hpp HINTS "${LIB3DPRNET_ROOT}")
find_library(lib3dprnet_LIBRARIES 3dprnet HINTS "${LIB3DPRNET_ROOT}/cmake-build-debug")

add_executable(repwatchd main.cpp fhem_service.cpp fhem_service.hpp)
target_compile_definitions(repwatchd PRIVATE ${Boost_DEFINITIONS})
target_include_directories(repwatchd PRIVATE ${Boost_INCLUDE_DIRS} ${json_INCLUDE_DIRS} ${lib3dprnet_INCLUDE_DIRS})
target_link_libraries(repwatchd ${Boost_LIBRARIES} ${lib3dprnet_LIBRARIES} stdc++fs)
if(WIN32)
    target_link_libraries(repwatchd ws2_32)
else()
    target_link_libraries(repwatchd pthread)
endif()
