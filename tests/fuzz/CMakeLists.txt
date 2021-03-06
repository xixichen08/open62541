include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/server)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_SOURCE_DIR}/tests)
include_directories(${PROJECT_BINARY_DIR}/src_generated)

remove_definitions(-std=c99 -Wmissing-prototypes -Wstrict-prototypes)
set (CMAKE_CXX_STANDARD 11)

if (NOT UA_BUILD_OSS_FUZZ)

    if(NOT "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
        MESSAGE(FATAL_ERROR "To build fuzzing, you need to use Clang as the compiler")
    endif()

    # oss-fuzz builds already include these flags
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp")
    endif()
    set(LIBS Fuzzer)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(LIBS $ENV{LIB_FUZZING_ENGINE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{OUT})
endif()
list(APPEND LIBS "${open62541_LIBRARIES}")


# Use different plugins for testing
set(fuzzing_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_clock.c
        ${PROJECT_SOURCE_DIR}/tests/testing_networklayers.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.c)

add_library(open62541-fuzzplugins OBJECT ${fuzzing_plugin_sources})
add_dependencies(open62541-fuzzplugins open62541)

# the fuzzer test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

set(FUZZER_TARGETS)

macro(add_fuzzer FUZZER_NAME FUZZER_SOURCE)
    add_executable(${FUZZER_NAME} $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-fuzzplugins> ${FUZZER_SOURCE} ${ARGN})
    target_link_libraries(${FUZZER_NAME} ${LIBS})
    target_include_directories(${FUZZER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
    assign_source_group(${FUZZER_SOURCE})
    list(APPEND FUZZER_TARGETS ${FUZZER_NAME})
endmacro()

# Add new fuzzers here
add_fuzzer(fuzz_binary_message fuzz_binary_message.cc)
add_fuzzer(fuzz_binary_decode fuzz_binary_decode.cc)

add_custom_target(
        run_fuzzer
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Browse.bin
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_CLO.bin
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_CreateActivateSession.bin
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_HELOPN.bin
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Read.bin
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Write.bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${FUZZER_TARGETS}
        ${MAYBE_USES_TERMINAL})
