cmake_minimum_required(VERSION 3.24)

project(abrade VERSION 0.3.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AbradeCompilerOptions)
include(AbradeStaticAnalysis)
include(AbradeTesting)

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

set(ABRADE_CORE_HEADERS
  src/abrade/action.hpp
  src/abrade/candidate.hpp
  src/abrade/connection.hpp
  src/abrade/content_filter.hpp
  src/abrade/controller.hpp
  src/abrade/endpoint.hpp
  src/abrade/exception.hpp
  src/abrade/generator.hpp
  src/abrade/http_status.hpp
  src/abrade/network_timeout.hpp
  src/abrade/options.hpp
  src/abrade/query.hpp
  src/abrade/redirect_policy.hpp
  src/abrade/run_stats.hpp
  src/abrade/scraper.hpp
  src/abrade/scraper_runtime.hpp
  src/abrade/writer.hpp
)

set(ABRADE_CORE_SOURCES
  src/abrade/controller.cpp
  src/abrade/exception.cpp
  src/abrade/generator.cpp
  src/abrade/options.cpp
)

add_library(abrade_core STATIC)
target_sources(abrade_core
  PRIVATE
    ${ABRADE_CORE_SOURCES}
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS src
      FILES ${ABRADE_CORE_HEADERS}
)
abrade_target_defaults(abrade_core)
target_link_libraries(abrade_core
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

add_executable(abrade_cli src/cli/main.cpp)
set_target_properties(abrade_cli PROPERTIES OUTPUT_NAME abrade)
abrade_target_defaults(abrade_cli)
target_link_libraries(abrade_cli PRIVATE abrade_core)

install(TARGETS abrade_cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE README.md DESTINATION "${CMAKE_INSTALL_DOCDIR}")

set(ABRADE_TIDY_SOURCES
  ${ABRADE_CORE_SOURCES}
  src/cli/main.cpp
)

set(ABRADE_FORMAT_SOURCES
  ${ABRADE_CORE_HEADERS}
  ${ABRADE_CORE_SOURCES}
  src/cli/main.cpp
)

if(BUILD_TESTING)
  set(ABRADE_UNIT_TEST_SOURCES
    tests/unit/action_test.cpp
    tests/unit/controller_test.cpp
    tests/unit/endpoint_test.cpp
    tests/unit/generator_test.cpp
    tests/unit/options_test.cpp
    tests/unit/runtime_test.cpp
  )

  abrade_add_unit_tests(abrade_unit_tests "${ABRADE_UNIT_TEST_SOURCES}")
  abrade_add_scraper_integration_test(abrade_cli)

  list(APPEND ABRADE_TIDY_SOURCES ${ABRADE_UNIT_TEST_SOURCES})
  list(APPEND ABRADE_FORMAT_SOURCES ${ABRADE_UNIT_TEST_SOURCES})
endif()

abrade_add_static_analysis_targets("${ABRADE_TIDY_SOURCES}" "${ABRADE_FORMAT_SOURCES}")
