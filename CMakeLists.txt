cmake_minimum_required(VERSION 3.7)
project(abrade)
set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

# OpenSSL stuff here
find_package(OpenSSL)

if(OPENSSL_FOUND)
  message("[+] Found OpenSSL")
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# BOOST stuff here
set(BOOST_COROUTINES_NO_DEPRECATION_WARNING)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
find_package(Boost COMPONENTS system date_time regex coroutine context program_options filesystem)

if(Boost_FOUND)
  message("[+] Found Boost. Include: " ${Boost_INCLUDE_DIRS}
          " Libraries: " ${Boost_LIBRARIES})
  include_directories(${Boost_INCLUDE_DIRS})
  # Until BEAST is in BOOST
  include_directories(beast/include)
endif()

# For ASIO
add_definitions(-D_WIN32_WINNT=0x0501)

# Artifacts
add_library(abradelib Options.cpp UriGenerator.h UriGenerator.cpp Action.h Options.h Query.h Scraper.h Connection.h Exception.h Exception.cpp Writer.h Controller.h Controller.cpp)
target_compile_definitions(abradelib PUBLIC BOOST_COROUTINES_NO_DEPRECATION_WARNING=1)

add_executable(abrade main.cpp)
add_executable(abrade_test test_main.cpp UriGeneratorTest.cpp OptionsTest.cpp)

# Linkage
target_link_libraries(abrade abradelib ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(abrade_test abradelib ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Compiler Options
IF (WIN32)
  SET_TARGET_PROPERTIES(abrade PROPERTIES COMPILE_FLAGS "/bigobj")
ELSE()
  SET_TARGET_PROPERTIES(abrade PROPERTIES COMPILE_FLAGS "-flto -O3")
ENDIF()
