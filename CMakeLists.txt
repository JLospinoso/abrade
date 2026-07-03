cmake_minimum_required(VERSION 3.24)

project(abrade VERSION 0.2.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

option(ABRADE_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "static")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Boost REQUIRED COMPONENTS
  asio
  beast
  circular_buffer
  context
  coroutine
  filesystem
  lexical_cast
  program_options
  regex
  system
  tokenizer
)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

function(abrade_target_defaults target)
  target_compile_features(${target} PUBLIC cxx_std_23)
  set_target_properties(${target} PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  target_compile_definitions(${target} PUBLIC
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>
  )
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<AND:$<BOOL:${ABRADE_ENABLE_SANITIZERS}>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address,undefined>
    $<$<AND:$<BOOL:${ABRADE_ENABLE_SANITIZERS}>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fno-omit-frame-pointer>
  )
  target_link_options(${target} PRIVATE
    $<$<AND:$<BOOL:${ABRADE_ENABLE_SANITIZERS}>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fsanitize=address,undefined>
  )
endfunction()

add_library(abradelib STATIC
  Action.h
  Candidate.h
  Connection.h
  Controller.cpp
  Controller.h
  Exception.cpp
  Exception.h
  Generator.cpp
  Generator.h
  Options.cpp
  Options.h
  Query.h
  Scraper.h
  Writer.h
)
abrade_target_defaults(abradelib)
target_link_libraries(abradelib
  PUBLIC
    Boost::asio
    Boost::beast
    Boost::circular_buffer
    Boost::context
    Boost::coroutine
    Boost::filesystem
    Boost::lexical_cast
    Boost::program_options
    Boost::regex
    Boost::system
    Boost::tokenizer
    OpenSSL::Crypto
    OpenSSL::SSL
    Threads::Threads
)

add_executable(abrade main.cpp)
abrade_target_defaults(abrade)
target_link_libraries(abrade PRIVATE abradelib)

install(TARGETS abrade RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(BUILD_TESTING)
  find_package(Catch2 3 REQUIRED)
  list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}")
  include(Catch)

  add_executable(abrade_test
    GeneratorTest.cpp
    OptionsTest.cpp
  )
  abrade_target_defaults(abrade_test)
  target_link_libraries(abrade_test PRIVATE abradelib Catch2::Catch2WithMain)
  catch_discover_tests(abrade_test)
endif()
