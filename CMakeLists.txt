cmake_minimum_required(VERSION 3.22.1)
project(TankMaster LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
)

set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SFML)

add_library(sfml-app SHARED
    app/src/main/cpp/main.cpp
)

find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
find_library(android-lib android)
find_library(log-lib log)

target_link_libraries(sfml-app PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-activity
    ${EGL-lib}
    ${GLESv2-lib}
    ${android-lib}
    ${log-lib}
)