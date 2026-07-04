include_guard(GLOBAL)

option(ABRADE_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ABRADE_ENABLE_WARNINGS "Enable strict compiler warnings for Abrade targets" ON)
option(ABRADE_WARNINGS_AS_ERRORS "Treat Abrade compiler warnings as errors" ON)
option(ABRADE_ENABLE_COVERAGE "Enable compiler coverage instrumentation for Abrade targets" OFF)

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

  if(ABRADE_ENABLE_WARNINGS)
    target_compile_options(${target} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W4>
      $<$<CXX_COMPILER_ID:MSVC>:/permissive->
      $<$<BOOL:${ABRADE_WARNINGS_AS_ERRORS}>:$<$<CXX_COMPILER_ID:MSVC>:/WX>>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wconversion>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wsign-conversion>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wshadow>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wnon-virtual-dtor>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wold-style-cast>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wcast-align>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Woverloaded-virtual>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wdouble-promotion>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wformat=2>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wimplicit-fallthrough>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wextra-semi>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wnull-dereference>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wnewline-eof>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wzero-as-null-pointer-constant>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wdeprecated>
      $<$<BOOL:${ABRADE_WARNINGS_AS_ERRORS}>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>>
    )
  endif()

  if(ABRADE_ENABLE_COVERAGE)
    target_compile_options(${target} PRIVATE
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-fprofile-instr-generate>
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-fcoverage-mapping>
      $<$<CXX_COMPILER_ID:GNU>:--coverage>
    )
    target_link_options(${target} PRIVATE
      $<$<CXX_COMPILER_ID:AppleClang,Clang>:-fprofile-instr-generate>
      $<$<CXX_COMPILER_ID:GNU>:--coverage>
    )
  endif()
endfunction()
