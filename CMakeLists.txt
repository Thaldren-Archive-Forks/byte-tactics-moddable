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
add_library(data OBJECT link/data.cpp)
set(GAME_COMPILE_OPTIONS
  -fms-compatibility -fms-extensions -fms-compatibility-version=11.00
  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -Xclang -fno-assume-sane-operator-new -mno-sse
  /Gz /GR- /GS- /Gw -ftrivial-auto-var-init=zero -Wno-nonportable-include-path -Wno-c++11-narrowing -Wno-writable-strings -Wno-microsoft -Wno-c++20-extensions
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

set(MSVC5_LIB "${CMAKE_SOURCE_DIR}/toolchain/msvc5-sp3/LIB")
find_program(DLLTOOL llvm-dlltool REQUIRED)
find_program(LLD_LINK lld-link REQUIRED)
find_program(LLVM_RC llvm-rc REQUIRED)
find_program(UV uv REQUIRED)

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

set(SYSTEM_LIBS "")
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
  DEPENDS game data extra $<TARGET_OBJECTS:game> $<TARGET_OBJECTS:data> $<TARGET_OBJECTS:extra> ${IMPORT_LIBS}
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
