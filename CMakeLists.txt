cmake_minimum_required(VERSION 3.20)
project(TotalA CXX)

set(MSVC5_INCLUDE "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/INCLUDE")
if(NOT EXISTS "${MSVC5_INCLUDE}/WINDOWS.H")
  message(FATAL_ERROR "MSVC 5's headers are missing: ${MSVC5_INCLUDE} (see CONTRIBUTING.md)")
endif()
get_filename_component(MSVC5_INCLUDE "${MSVC5_INCLUDE}" REALPATH)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2")
set(CMAKE_BUILD_TYPE Release)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/modern_headers.py" "${MSVC5_INCLUDE}" "${CMAKE_BINARY_DIR}"
  RESULT_VARIABLE HEADERS_RESULT)
if(NOT HEADERS_RESULT EQUAL 0)
  message(FATAL_ERROR "tools/modern_headers.py failed")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library(game OBJECT ${SOURCES})
target_include_directories(game PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(game SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(game PRIVATE
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00
  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -mno-sse
  /Gz /GR- -Wno-nonportable-include-path -Wno-c++11-narrowing -Wno-writable-strings -Wno-microsoft -Wno-c++20-extensions
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")

foreach(source IN LISTS SOURCES)
  file(STRINGS "${source}" flags_line REGEX "^//[ \t]*FLAGS:" LIMIT_COUNT 1)
  file(STRINGS "${source}" try_line REGEX "^[ \t]*try[ \t]*[{]" LIMIT_COUNT 1)
  if(flags_line MATCHES "/GX")
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHsc")
  elseif(try_line)
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHa")
  endif()
endforeach()
