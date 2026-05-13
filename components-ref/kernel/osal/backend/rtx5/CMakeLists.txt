# RTX5 Backend CMakeLists.txt

# Add RTX5 backend source files
set(XY_OSAL_RTX5_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_os_rtx5.c
)

# Add include directories for RTX5 backend
# Note: CMSIS-RTOS2 headers should be provided by the RTX5 package
target_include_directories(${XY_OSAL_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add CMSIS-RTOS2 definitions
target_compile_definitions(${XY_OSAL_TARGET} PRIVATE
    XY_OS_BACKEND_RTX5=1
    CMSIS_OS_VER=2
)

# Link against RTX5 library if available
# Note: This depends on your project setup
# target_link_libraries(${XY_OSAL_TARGET} PRIVATE RTX5::RTOS)
