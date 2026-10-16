cmake_minimum_required(VERSION 3.20)
project(TotalA C CXX)

set(MSVC5_INCLUDE "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/INCLUDE")
if(NOT EXISTS "${MSVC5_INCLUDE}/WINDOWS.H")
  message(FATAL_ERROR "MSVC 5's headers are missing: ${MSVC5_INCLUDE} (see CONTRIBUTING.md)")
endif()
get_filename_component(MSVC5_INCLUDE "${MSVC5_INCLUDE}" REALPATH)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS_RELEASE "/O2")
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
option(BITEXACT_ROUTE "Route the game logic's CRT calls to lib/bitexact" ON)
if(BITEXACT_ROUTE)
  list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/util/math_4b70a0.cpp")
endif()

find_program(UV uv REQUIRED)
set(BITEXACT_DIR "${CMAKE_SOURCE_DIR}/lib/bitexact")
set(BITEXACT_SHIM_DIR "${CMAKE_SOURCE_DIR}/cmake/bitexact")
set(BITEXACT_TABLES "${CMAKE_BINARY_DIR}/bitexact/bx_tables.c")
add_custom_command(
  OUTPUT "${BITEXACT_TABLES}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/bitexact"
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/bitexact_tables.py" "${BITEXACT_TABLES}"
  DEPENDS "${CMAKE_SOURCE_DIR}/tools/bitexact_tables.py"
  VERBATIM)
add_library(bitexact STATIC
  "${BITEXACT_DIR}/trig.c" "${BITEXACT_DIR}/math.c" "${BITEXACT_DIR}/strtof.c" "${BITEXACT_DIR}/rand.c" "${BITEXACT_DIR}/qsort.c"
  "${BITEXACT_TABLES}" "${BITEXACT_SHIM_DIR}/game_rand.c")
if(BITEXACT_ROUTE)
  target_sources(bitexact PRIVATE "${BITEXACT_SHIM_DIR}/trig.c" "${BITEXACT_SHIM_DIR}/sscanf.c")
endif()
target_include_directories(bitexact PRIVATE "${BITEXACT_DIR}")
target_include_directories(bitexact SYSTEM PRIVATE "${MSVC5_INCLUDE}")
target_compile_options(bitexact PRIVATE
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00 -msse2 -mfpmath=sse /clang:-ffp-contract=off /GS-
  -fno-strict-aliasing -fwrapv -Wno-nonportable-include-path -Wno-microsoft -Wno-comment
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")

add_library(game OBJECT ${SOURCES})
add_library(data OBJECT link/data.cpp)
set(GAME_COMPILE_OPTIONS
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00
  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -Xclang -fno-assume-sane-operator-new -mno-sse
  /Gz /GR- /GS- /Gw -Wno-nonportable-include-path -Wno-c++11-narrowing -Wno-writable-strings -Wno-microsoft -Wno-c++20-extensions
  -vfsoverlay "${CMAKE_BINARY_DIR}/overlay.json")
foreach(target game data)
  target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_include_directories(${target} SYSTEM PRIVATE "${MSVC5_INCLUDE}")
  target_compile_options(${target} PRIVATE ${GAME_COMPILE_OPTIONS})
endforeach()

foreach(source IN LISTS SOURCES)
  file(STRINGS "${source}" flags_line REGEX "^//[ \t]*FLAGS:" LIMIT_COUNT 1)
  file(STRINGS "${source}" try_line REGEX "^[ \t]*try[ \t]*[{]" LIMIT_COUNT 1)
  if(flags_line MATCHES "/GX")
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHsc")
  elseif(try_line)
    set_source_files_properties("${source}" PROPERTIES COMPILE_OPTIONS "/EHa")
  endif()
endforeach()

set(CRT_CALLS_CSV "${CMAKE_SOURCE_DIR}/data/crt_calls.csv")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CRT_CALLS_CSV}")
file(STRINGS "${CRT_CALLS_CSV}" CRT_CALL_ROWS)
set(ROUTED_SOURCES "")
foreach(row IN LISTS CRT_CALL_ROWS)
  if(row MATCHES "^0x[0-9a-f]+,[^,]+,(fsqrt|rand|srand|_hypot|_CIacos|sscanf),[a-z]+,([^,]+),game,")
    list(APPEND ROUTED_SOURCES "${CMAKE_SOURCE_DIR}/${CMAKE_MATCH_2}")
  endif()
endforeach()
list(REMOVE_DUPLICATES ROUTED_SOURCES)
if(NOT BITEXACT_ROUTE)
  set(ROUTED_SOURCES "")
endif()
foreach(source IN LISTS ROUTED_SOURCES)
  if(NOT source IN_LIST SOURCES)
    message(FATAL_ERROR "data/crt_calls.csv names ${source}, which is not a source of the build")
  endif()
  set_property(SOURCE "${source}" APPEND PROPERTY COMPILE_OPTIONS "/FI${BITEXACT_SHIM_DIR}/route.h")
endforeach()
if(BITEXACT_ROUTE)
  set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/harness/harness.cpp" APPEND PROPERTY COMPILE_DEFINITIONS BX_ROUTE)
endif()

set(MSVC5_LIB "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/LIB")
find_program(DLLTOOL llvm-dlltool REQUIRED)
find_program(LLD_LINK lld-link REQUIRED)
find_program(LLVM_RC llvm-rc REQUIRED)

set(IMPORT_LIBS "")
foreach(dll smackw32 dplayx ddraw dsound)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/${dll}.lib"
    COMMAND "${DLLTOOL}" -m i386 -k -d "${CMAKE_SOURCE_DIR}/link/${dll}.def" -l "${CMAKE_BINARY_DIR}/${dll}.lib"
    DEPENDS "${CMAKE_SOURCE_DIR}/link/${dll}.def")
  list(APPEND IMPORT_LIBS "${CMAKE_BINARY_DIR}/${dll}.lib")
endforeach()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
file(GLOB MSVC5_LIBS "${MSVC5_LIB}/*.LIB")
foreach(lib IN LISTS MSVC5_LIBS)
  get_filename_component(stem "${lib}" NAME_WE)
  string(TOLOWER "${stem}" lower)
  foreach(name "${stem}.lib" "${lower}.lib")
    file(CREATE_LINK "${lib}" "${CMAKE_BINARY_DIR}/lib/${name}" SYMBOLIC)
  endforeach()
endforeach()

set(SYSTEM_LIBS "$<TARGET_FILE:bitexact>")
foreach(lib LIBCMT LIBCPMT OLDNAMES DPLAY)
  list(APPEND SYSTEM_LIBS "${MSVC5_LIB}/${lib}.LIB")
endforeach()
foreach(lib KERNEL32 USER32 GDI32 ADVAPI32 SHELL32 WINMM)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/${lib}.def" "${CMAKE_BINARY_DIR}/${lib}.lib"
    COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
            --import-def "${MSVC5_LIB}/${lib}.LIB" --out "${CMAKE_BINARY_DIR}/${lib}.def"
    COMMAND "${DLLTOOL}" -m i386 -k -d "${CMAKE_BINARY_DIR}/${lib}.def" -l "${CMAKE_BINARY_DIR}/${lib}.lib"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/aliases.py")
  list(APPEND IMPORT_LIBS "${CMAKE_BINARY_DIR}/${lib}.lib")
endforeach()

file(GLOB ZLIB_OBJECTS "${CMAKE_SOURCE_DIR}/toolchain/thirdparty/zlib-1.0.4/*.obj")

execute_process(COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/resources.py" --print-art
  OUTPUT_VARIABLE ART_DIR OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE ART_RESULT)
if(NOT ART_RESULT EQUAL 0)
  message(FATAL_ERROR "tools/resources.py --print-art failed")
endif()
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/TotalA.res"
  COMMAND "${LLVM_RC}" /C 1252 /I "${ART_DIR}" /FO "${CMAKE_BINARY_DIR}/TotalA.res" "${CMAKE_SOURCE_DIR}/src/res/TotalA.rc"
  DEPENDS "${CMAKE_SOURCE_DIR}/src/res/TotalA.rc")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/objects.rsp" CONTENT "$<JOIN:$<TARGET_OBJECTS:game>;$<TARGET_OBJECTS:data>,\n>\n")
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/extra_data.cpp"
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          --extra-data --objects "@${CMAKE_BINARY_DIR}/objects.rsp" --out "${CMAKE_BINARY_DIR}/extra_data.cpp"
  DEPENDS game data $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          "${CMAKE_SOURCE_DIR}/data/globals.csv")
add_library(extra OBJECT "${CMAKE_BINARY_DIR}/extra_data.cpp")
target_compile_options(extra PRIVATE ${GAME_COMPILE_OPTIONS})
target_include_directories(extra SYSTEM PRIVATE "${MSVC5_INCLUDE}")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/alias_objects.rsp"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:game>;$<TARGET_OBJECTS:data>;$<TARGET_OBJECTS:extra>,\n>\n")
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/objs/objects.rsp"
  COMMAND "${UV}" run --project "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tools/aliases.py"
          --objects "@${CMAKE_BINARY_DIR}/alias_objects.rsp" --libs ${SYSTEM_LIBS} ${IMPORT_LIBS} ${ZLIB_OBJECTS}
          --winmain 0x49e830 --patch-dir "${CMAKE_BINARY_DIR}/objs" --out "${CMAKE_BINARY_DIR}/aliases.obj"
  DEPENDS game data extra bitexact $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> $<TARGET_OBJECTS:extra> ${IMPORT_LIBS}
          "${CMAKE_SOURCE_DIR}/tools/aliases.py")

add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/TotalA.exe"
  COMMAND "${LLD_LINK}" /nologo /machine:x86 /subsystem:windows,4.0 /base:0x400000 /fixed /dynamicbase:no /largeaddressaware:no /errorlimit:0 /entry:WinMainCRTStartup /include:_WinMainCRTStartup /out:${CMAKE_BINARY_DIR}/TotalA.exe
          /libpath:${CMAKE_BINARY_DIR}/lib /opt:noref,noicf /incremental:no /safeseh:no /force:multiple /map:${CMAKE_BINARY_DIR}/TotalA.map
          "@${CMAKE_BINARY_DIR}/objs/objects.rsp" "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/TotalA.res"
          ${ZLIB_OBJECTS} ${IMPORT_LIBS} ${SYSTEM_LIBS}
  DEPENDS game data extra $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> $<TARGET_OBJECTS:extra> "${CMAKE_BINARY_DIR}/aliases.obj" "${CMAKE_BINARY_DIR}/TotalA.res" ${IMPORT_LIBS}
  VERBATIM)
add_custom_target(TotalA ALL DEPENDS "${CMAKE_BINARY_DIR}/TotalA.exe")
