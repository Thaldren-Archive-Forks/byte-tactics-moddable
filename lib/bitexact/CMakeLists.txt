cmake_minimum_required(VERSION 3.20)
project(bitexact_portable C)

set(BITEXACT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BX_TABLES "" CACHE FILEPATH "A bx_tables.c to use instead of generating one with uv")
set(BX_TABLES_OUT "${CMAKE_CURRENT_BINARY_DIR}/bx_tables.c")

if(BX_TABLES)
  set(BX_TABLES_SOURCE "${BX_TABLES}")
else()
  find_program(UV uv REQUIRED)
  add_custom_command(
    OUTPUT "${BX_TABLES_OUT}"
    COMMAND "${UV}" run --project "${BITEXACT_ROOT}" "${BITEXACT_ROOT}/tools/bitexact_tables.py" "${BX_TABLES_OUT}"
    DEPENDS "${BITEXACT_ROOT}/tools/bitexact_tables.py"
    VERBATIM)
  set(BX_TABLES_SOURCE "${BX_TABLES_OUT}")
endif()

add_library(bitexact_portable STATIC
  trig.c math.c strtof.c rand.c qsort.c
  "${BITEXACT_ROOT}/cmake/bitexact/sscanf.c"
  "${BX_TABLES_SOURCE}")
target_include_directories(bitexact_portable PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(bitexact_portable_test test/portable_test.c)
target_link_libraries(bitexact_portable_test PRIVATE bitexact_portable)

include(CheckCSourceCompiles)
check_c_source_compiles("
#if defined(__i386__) || defined(_M_IX86)
int main(void) { return 0; }
#else
#error not x86
#endif" BX_IS_X86_32)

set(BX_FLAGS "")
if(MSVC)
  list(APPEND BX_FLAGS /fp:strict)
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    list(APPEND BX_FLAGS /clang:-ffp-contract=off /clang:-fwrapv /clang:-fno-strict-aliasing)
    if(BX_IS_X86_32)
      list(APPEND BX_FLAGS -msse2)
    endif()
  elseif(BX_IS_X86_32)
    list(APPEND BX_FLAGS /arch:SSE2)
  endif()
else()
  list(APPEND BX_FLAGS -ffp-contract=off -fwrapv -fno-strict-aliasing)
  if(BX_IS_X86_32)
    list(APPEND BX_FLAGS -msse2 -mfpmath=sse)
  endif()
endif()
target_compile_options(bitexact_portable PRIVATE ${BX_FLAGS})
target_compile_options(bitexact_portable_test PRIVATE ${BX_FLAGS})
if(NOT MSVC)
  target_compile_options(bitexact_portable PRIVATE -Wno-comment)
endif()
if(MSVC)
  target_compile_definitions(bitexact_portable PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(bitexact_portable_test PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()
add_test(NAME bitexact_portable_hashes
  COMMAND "${CMAKE_COMMAND}"
    "-DEXE=$<TARGET_FILE:bitexact_portable_test>"
    "-DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/test/portable_hashes.txt"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test/compare_hashes.cmake")
