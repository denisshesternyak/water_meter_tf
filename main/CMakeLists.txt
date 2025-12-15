cmake_minimum_required(VERSION 3.13.1)

OPTION(DEFINE_DEBUG
    "Build the project using debugging code"
    OFF)
IF(DEFINE_DEBUG)
    MESSAGE("Adding Debug flag...")
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
    MESSAGE("Build type is " ${CMAKE_BUILD_TYPE})
ENDIF(DEFINE_DEBUG)

idf_component_register(
    SRCS 
        "main.cpp" 
        "cam/camera.cpp" 
        "sd/sd_card.cpp"
    INCLUDE_DIRS 
        "."
        "cam"
        "sd"
)