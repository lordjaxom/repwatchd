cmake_minimum_required(VERSION 3.9)
project(repwatchd)

cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)
include("CMakeLocal.${HOSTNAME}.cmake")

find_package(Boost 1.66.0 COMPONENTS system coroutine REQUIRED)
set(Boost_DEFINITIONS BOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_path(json_INCLUDE_DIRS nlohmann/json.hpp HINTS "${JSON_ROOT}/include")

find_path(utf8_INCLUDE_DIRS utf8.h HINTS "${UTF8_ROOT}/source")

find_path(mosquitto_INCLUDE_DIRS mosquitto.h HINTS "${MOSQUITTO_ROOT}/lib")
find_path(mosquittopp_INCLUDE_DIRS mosquittopp.h HINTS "${MOSQUITTO_ROOT}/lib/cpp")
set(mosquitto_LIBRARIES mosquittopp)

find_path(wiringPi_INCLUDE_DIRS wiringPi.h HINTS "${WIRINGPI_ROOT}")
set(wiringPi_LIBRARIES wiringPi)

string(TOLOWER "${CMAKE_BUILD_TYPE}" LIB3DPRNET_BUILD_TYPE)
find_path(lib3dprnet_INCLUDE_DIRS 3dprnet/core/config.hpp HINTS "${LIB3DPRNET_ROOT}/include")
find_library(lib3dprnet_LIBRARIES 3dprnet HINTS "${LIB3DPRNET_ROOT}/cmake-build-${LIB3DPRNET_BUILD_TYPE}${LIB3DPRNET_BUILD_SUFFIX}" )

set(CMAKE_CXX_STANDARD 14)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -m64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
endif()

add_executable(repwatchd
        main.cpp
        commandline.cpp
        commandline.hpp
        mqtt/session.cpp
        mqtt/session.hpp 
        mqtt/types.cpp
        mqtt/types.hpp
        wiring/output.cpp
        wiring/output.hpp)
target_compile_definitions(repwatchd PRIVATE ${Boost_DEFINITIONS})
target_include_directories(repwatchd PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${json_INCLUDE_DIRS}
        ${utf8_INCLUDE_DIRS}
        ${mosquitto_INCLUDE_DIRS}
        ${mosquittopp_INCLUDE_DIRS}
        ${wiringPi_INCLUDE_DIRS}
        ${lib3dprnet_INCLUDE_DIRS})
target_link_libraries(repwatchd
        ${Boost_LIBRARIES}
        ${mosquitto_LIBRARIES}
        ${wiringPi_LIBRARIES}
        ${lib3dprnet_LIBRARIES}
        stdc++fs)
if(WIN32)
    target_link_libraries(repwatchd ws2_32)
else()
    target_link_libraries(repwatchd pthread)
endif()
