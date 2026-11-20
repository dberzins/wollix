# The cmake_consumer test, run by CTest in script mode (cmake -P). It installs
# the wollix build into a scratch prefix, then configures, builds and runs
# tests/cmake_consumer against that prefix with the same generator and
# compiler, and finally reads the installed version file the way a 32-bit
# consumer's find_package would.
#
# Inputs (-D): WOLLIX_BUILD_DIR, CONSUMER_SOURCE_DIR, WORK_DIR, GENERATOR,
# GENERATOR_PLATFORM, GENERATOR_TOOLSET, C_COMPILER, CONFIG (empty for a
# single-config build without CMAKE_BUILD_TYPE), EXPECT_VERSION.

function(step what)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "cmake_consumer: ${what} failed (${rc})")
  endif()
endfunction()

set(prefix ${WORK_DIR}/prefix)
set(build ${WORK_DIR}/build)
file(REMOVE_RECURSE ${WORK_DIR})

set(config_args)
set(ctest_config_args)
if(CONFIG)
  set(config_args --config ${CONFIG})
  set(ctest_config_args -C ${CONFIG})
endif()

step("install" ${CMAKE_COMMAND} --install ${WOLLIX_BUILD_DIR} --prefix ${prefix} ${config_args})

set(configure_args -S ${CONSUMER_SOURCE_DIR} -B ${build} -G ${GENERATOR}
    -DCMAKE_C_COMPILER=${C_COMPILER}
    -DWOLLIX_PREFIX=${prefix}
    -DWOLLIX_EXPECT_VERSION=${EXPECT_VERSION})
if(GENERATOR_PLATFORM)
  list(APPEND configure_args -A ${GENERATOR_PLATFORM})
endif()
if(GENERATOR_TOOLSET)
  list(APPEND configure_args -T ${GENERATOR_TOOLSET})
endif()
if(CONFIG)
  list(APPEND configure_args -DCMAKE_BUILD_TYPE=${CONFIG})
endif()
step("configure" ${CMAKE_COMMAND} ${configure_args})
step("build" ${CMAKE_COMMAND} --build ${build} ${config_args})
step("run" ${CMAKE_CTEST_COMMAND} --test-dir ${build} --output-on-failure ${ctest_config_args})

# A header-only package suits every pointer size: the version file must not
# turn a 32-bit consumer away.
file(GLOB_RECURSE version_file ${prefix}/*/wollixConfigVersion.cmake)
if(NOT version_file)
  message(FATAL_ERROR "cmake_consumer: no wollixConfigVersion.cmake under ${prefix}")
endif()
list(GET version_file 0 version_file)
set(PACKAGE_FIND_VERSION ${EXPECT_VERSION})
string(REPLACE "." ";" _parts ${EXPECT_VERSION})
list(GET _parts 0 PACKAGE_FIND_VERSION_MAJOR)
list(GET _parts 1 PACKAGE_FIND_VERSION_MINOR)
list(GET _parts 2 PACKAGE_FIND_VERSION_PATCH)
set(PACKAGE_FIND_VERSION_COUNT 3)
set(CMAKE_SIZEOF_VOID_P 4)
include(${version_file})
if(PACKAGE_VERSION_UNSUITABLE OR NOT PACKAGE_VERSION_COMPATIBLE)
  message(FATAL_ERROR "cmake_consumer: a 32-bit consumer would be refused "
                      "(compatible '${PACKAGE_VERSION_COMPATIBLE}', unsuitable "
                      "'${PACKAGE_VERSION_UNSUITABLE}', version '${PACKAGE_VERSION}')")
endif()
message(STATUS "cmake_consumer: installed, built, ran, and suits a 32-bit consumer")
