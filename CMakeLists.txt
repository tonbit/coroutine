cmake_minimum_required(VERSION 3.5)

project(coroutine
	LANGUAGES CXX
	VERSION 1.0
)

include(CTest)
include(GNUInstallDirs)

set(_installIncludeDir ${CMAKE_INSTALL_INCLUDEDIR}/coroutine)
set(_buildIncludeDir ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads)

add_library(coroutine INTERFACE IMPORTED)
target_link_libraries(coroutine INTERFACE 
    Threads::Threads
)
target_include_directories(coroutine INTERFACE
    "$<BUILD_INTERFACE:${_buildIncludeDir}>"                  # For build-tree
    $<INSTALL_INTERFACE:${_installIncludeDir}>              # For install-tree
)
install(FILES coroutine.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coroutine
)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo coroutine)
set_property(TARGET demo PROPERTY CXX_STANDARD 11)

add_test(NAME run_demo
    COMMAND demo
)
