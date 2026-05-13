# DM Components Test CMake Configuration
cmake_minimum_required(VERSION 3.10)

# Test executable names
set(TEST_TLV test_tlv)
set(TEST_BASE64 test_base64)
set(TEST_FACTORY test_factory)
set(TEST_NVM test_nvm)

# TLV Test
add_executable(${TEST_TLV} test_tlv.c)
target_include_directories(${TEST_TLV} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../xy_tlv
    ${CMAKE_CURRENT_SOURCE_DIR}/../xy_tlv/..
)
target_compile_definitions(${TEST_TLV} PRIVATE
    XY_TLV_ENABLE_VALIDATION=1
    XY_TLV_MAX_NESTING_LEVEL=4
)

# Base64 Test
add_executable(${TEST_BASE64} test_base64.c)
target_include_directories(${TEST_BASE64} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../xy_base64
)

# Factory Test
add_executable(${TEST_FACTORY} test_factory.c)
target_include_directories(${TEST_FACTORY} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../factory
    ${CMAKE_CURRENT_SOURCE_DIR}/../xy_tlv
)

# NVM Test
add_executable(${TEST_NVM} test_nvm.c)
target_include_directories(${TEST_NVM} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../xy_nvm/inc
)

# Test reports
enable_testing()

add_test(NAME DM_TLV_TEST COMMAND ${TEST_TLV})
add_test(NAME DM_BASE64_TEST COMMAND ${TEST_BASE64})
add_test(NAME DM_FACTORY_TEST COMMAND ${TEST_FACTORY})
add_test(NAME DM_NVM_TEST COMMAND ${TEST_NVM})
