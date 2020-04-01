cmake_minimum_required(VERSION 3.1..3.13)
project(abrade)
set(CMAKE_CXX_STANDARD 17)

# conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
include(GNUInstallDirs)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

# Artifacts
add_library(abradelib Options.cpp Candidate.h Generator.h Generator.cpp Action.h Options.h Query.h Scraper.h
            Connection.h Exception.h Exception.cpp Writer.h Controller.h Controller.cpp)
target_link_libraries(abradelib PUBLIC CONAN_PKG::boost CONAN_PKG::openssl)

add_executable(abrade main.cpp)
add_executable(abrade_test test_main.cpp GeneratorTest.cpp OptionsTest.cpp)

# Linkage
target_link_libraries(abrade abradelib)
target_link_libraries(abrade_test abradelib)

# Compiler Options
IF (WIN32)
  SET_TARGET_PROPERTIES(abrade PROPERTIES COMPILE_FLAGS "/bigobj")
ELSE()
  SET_TARGET_PROPERTIES(abrade PROPERTIES COMPILE_FLAGS "-flto -O3")
ENDIF()
