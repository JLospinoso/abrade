include_guard(GLOBAL)

function(abrade_add_static_analysis_targets analysis_sources format_sources)
  find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format
    HINTS
      /opt/homebrew/opt/llvm/bin
      /usr/local/opt/llvm/bin
      /opt/homebrew/Cellar/llvm/22.1.8/bin
      "$ENV{HOME}/Applications/CLion.app/Contents/bin/clang/mac/aarch64/bin"
      "$ENV{HOME}/Applications/CLion 2026.2 Beta.app/Contents/bin/clang/mac/aarch64/bin"
  )
  if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(abrade_format
      COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${format_sources}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Formatting C++ source with clang-format"
      VERBATIM
    )
    add_custom_target(abrade_format_check
      COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${format_sources}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Checking C++ source formatting with clang-format"
      VERBATIM
    )
  endif()

  find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
  if(CPPCHECK_EXECUTABLE)
    add_custom_target(abrade_cppcheck
      COMMAND "${CPPCHECK_EXECUTABLE}"
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --enable=warning,performance,portability,style
        --std=c++23
        --inline-suppr
        --suppress=missingIncludeSystem
        --error-exitcode=2
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Running cppcheck static analysis"
      VERBATIM
    )
  endif()

  find_program(CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
    HINTS
      /opt/homebrew/opt/llvm/bin
      /opt/homebrew/Cellar/llvm/22.1.8/bin
      "$ENV{HOME}/Applications/CLion.app/Contents/bin/clang/mac/aarch64/bin"
      "$ENV{HOME}/Applications/CLion 2026.2 Beta.app/Contents/bin/clang/mac/aarch64/bin"
  )
  if(CLANG_TIDY_EXECUTABLE)
    set(ABRADE_CLANG_TIDY_EXTRA_ARGS)
    if(APPLE)
      execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE ABRADE_MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
      if(ABRADE_MACOS_SDK_PATH)
        list(APPEND ABRADE_CLANG_TIDY_EXTRA_ARGS
          --extra-arg=-isysroot
          "--extra-arg=${ABRADE_MACOS_SDK_PATH}"
        )
      endif()
    endif()
    add_custom_target(abrade_clang_tidy
      COMMAND "${CLANG_TIDY_EXECUTABLE}"
        -p "${CMAKE_BINARY_DIR}"
        ${ABRADE_CLANG_TIDY_EXTRA_ARGS}
        ${analysis_sources}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Running clang-tidy static analysis"
      VERBATIM
    )
  endif()
endfunction()
