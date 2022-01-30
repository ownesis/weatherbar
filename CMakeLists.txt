cmake_minimum_required(VERSION 3.0)

project(weatherbar VERSION 0.1.0 DESCRIPTION "C program for show weather on waybar")

set(SRCS
    weatherbar.c
    utils.c
    config_parser.c
    meteo.c
   )

set(HDRS
    uitils.h
    config_parser.h
    meteo.h
   )

add_executable(weatherbar ${SRCS})
find_package(CURL REQUIRED)
find_library(WAYBAR_JSON REQUIRED NAMES waybar_json PATHS "/usr/local/lib")
find_library(SSFMT REQUIRED NAMES ssfmt PATHS "/usr/local/lib")
find_library(IPINFO REQUIRED NAMES ipinfo PATHS "/usr/local/lib")
find_package(JSON-C CONFIG REQUIRED)

if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
    target_link_libraries(weatherbar PRIVATE ${CURL_LIBRARY})
else()
    message(FATAL_ERROR "libcurl not found")
endif()

if(JSON-C_FOUND)
    target_link_libraries(weatherbar PRIVATE json-c::json-c)
else()
    message(FATAL_ERROR "json-c not found")
endif()

if(IPINFO)
    target_link_libraries(weatherbar PRIVATE ${IPINFO})
else()
    message(FATAL_ERROR "ipinfo not found")
endif()

if(WAYBAR_JSON)
    target_link_libraries(weatherbar PRIVATE ${WAYBAR_JSON})
else()
    message(FATAL_ERROR "waybar_json not found")
endif()

if(SSFMT)
    target_link_libraries(weatherbar PRIVATE ${SSFMT})
else()
    message(FATAL_ERROR "ssfmt not found")
endif()
