cmake_minimum_required(VERSION 3.16)
project(c_utils_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

include(FetchContent)

FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG v2.6.0
)

FetchContent_MakeAvailable(unity)

if(TARGET unity)
  target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
endif()

if(NOT TARGET unity)
  add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)
  target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src)
  target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
endif()

function(add_cutils_test test_name)
  add_executable(${test_name} tests/${test_name}.c)
  target_include_directories(${test_name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${unity_SOURCE_DIR}/src
  )
  target_link_libraries(${test_name} PRIVATE unity)
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_cutils_test(test_darr)
add_cutils_test(test_dstr)
add_cutils_test(test_ht)
add_cutils_test(test_strv)
add_cutils_test(test_json)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(sync_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Syncing compile_commands.json to project root for clangd"
    VERBATIM
  )
endif()
