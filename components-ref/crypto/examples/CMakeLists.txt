# CMakeLists.txt for crypto examples
#

# Enable C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add individual example executables
set(EXAMPLES
    example_aes
    example_hash
    example_hmac
    example_base64
    example_hex
    example_sm2
    example_sm3
    example_sm4
    example_random
)

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.c)

    # Include the crypto headers
    target_include_directories(${example} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm2
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm3
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm4
    )

    # Link with crypto library if available
    target_link_libraries(${example} crypto)
endforeach()

# Optional: Group examples in IDE
foreach(example ${EXAMPLES})
    set_target_properties(${example} PROPERTIES FOLDER "crypto/examples")
endforeach()
