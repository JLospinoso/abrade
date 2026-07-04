include_guard(GLOBAL)

function(abrade_add_unit_tests target unit_test_sources)
  find_package(Catch2 3 REQUIRED)
  list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}")
  include(Catch)

  add_executable(${target} ${unit_test_sources})
  abrade_target_defaults(${target})
  target_link_libraries(${target} PRIVATE abrade_core Catch2::Catch2WithMain)
  catch_discover_tests(${target})
endfunction()

function(abrade_add_scraper_integration_test cli_target)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_program(ABRADE_OPENSSL_EXECUTABLE
    NAMES openssl openssl.exe
    HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openssl"
  )
  set(ABRADE_SCRAPER_INTEGRATION_COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/scraper_integration.py"
    "$<TARGET_FILE:${cli_target}>"
  )
  if(ABRADE_OPENSSL_EXECUTABLE)
    list(APPEND ABRADE_SCRAPER_INTEGRATION_COMMAND "--openssl" "${ABRADE_OPENSSL_EXECUTABLE}")
  endif()
  add_test(
    NAME ScraperIntegration
    COMMAND ${ABRADE_SCRAPER_INTEGRATION_COMMAND}
  )
  set_tests_properties(ScraperIntegration PROPERTIES TIMEOUT 90)
endfunction()
