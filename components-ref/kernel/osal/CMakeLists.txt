##
## @file CMakeLists.txt
## @brief CMake configuration for XY OSAL (OS Abstraction Layer)
## @version 2.0
## @date 2026-02-28
##
## Note: Third party RTOS sources are managed in third_party/ directory.
##       This CMakeLists.txt only builds the OSAL adaptation layer.
##

cmake_minimum_required(VERSION 3.12)

project(xy_osal C)

# ==================== Backend Selection ====================

set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend selection")
set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos rtthread cmsis_rtx)

# Validate backend selection
if(NOT OSAL_BACKEND IN_LIST OSAL_BACKEND_OPTIONS)
    set(OSAL_BACKEND_OPTIONS baremetal freertos rtthread cmsis_rtx)
endif()

# ==================== Source Files ====================

# Include directories
set(OSAL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc
)

# Common sources - backend implementations include all necessary functionality
# xy_tick and xy_timer_sw are provided by components/kernel/misc module
set(OSAL_COMMON_SOURCES
)

# Backend-specific sources
if(OSAL_BACKEND STREQUAL "baremetal")
    set(OSAL_BACKEND_SOURCES
        backend/baremetal/xy_os_baremetal.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_BAREMETAL
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
    )
    message(STATUS "XY OSAL: Using Bare-metal backend")

elseif(OSAL_BACKEND STREQUAL "freertos")
    set(OSAL_BACKEND_SOURCES
        backend/freertos/xy_os_freertos.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_FREERTOS
        USE_FREERTOS
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        # FreeRTOS headers from third_party
        ${CMAKE_SOURCE_DIR}/third_party/freertos/include
    )
    message(STATUS "XY OSAL: Using FreeRTOS backend")

elseif(OSAL_BACKEND STREQUAL "rtthread")
    set(OSAL_BACKEND_SOURCES
        backend/rtthread/xy_os_rtthread.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_RTTHREAD
        USE_RTTHREAD
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        # RT-Thread headers from third_party
        ${CMAKE_SOURCE_DIR}/third_party/rt-thread/include
    )
    message(STATUS "XY OSAL: Using RT-Thread backend")

elseif(OSAL_BACKEND STREQUAL "cmsis_rtx")
    set(OSAL_BACKEND_SOURCES
        backend/cmsis_rtx/xy_os_cmsis_rtx.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_CMSIS_RTX
        USE_CMSIS_RTX
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        # CMSIS-RTX headers from third_party
        ${CMAKE_SOURCE_DIR}/third_party/cmsis-rtx/Include
    )
    message(STATUS "XY OSAL: Using CMSIS-RTX backend")

else()
    message(FATAL_ERROR "Invalid OSAL backend: ${OSAL_BACKEND}")
endif()

# ==================== Feature Configuration ====================

option(XY_OS_FEATURE_THREAD "Enable thread management" ON)
option(XY_OS_FEATURE_THREAD_FLAGS "Enable thread flags" ON)
option(XY_OS_FEATURE_MUTEX "Enable mutex support" ON)
option(XY_OS_FEATURE_SEMAPHORE "Enable semaphore support" ON)
option(XY_OS_FEATURE_EVENT_FLAGS "Enable event flags" ON)
option(XY_OS_FEATURE_MESSAGE_QUEUE "Enable message queues" ON)
option(XY_OS_FEATURE_MEMORY_POOL "Enable memory pools" ON)
option(XY_OS_FEATURE_TIMER "Enable software timers" ON)
option(XY_OS_FEATURE_DELAY "Enable delay functions" ON)

# ==================== Debug Configuration ====================

option(XY_OS_PARAM_CHECK "Enable parameter validation" ON)
option(XY_OS_ISR_CHECK "Enable ISR context detection" ON)
option(XY_OS_NULL_CHECK "Enable NULL pointer checks" ON)
option(XY_OS_ENABLE_STATS "Enable statistics collection" OFF)

# ==================== Build Library ====================

set(OSAL_SOURCES
    src/xy_os_tick.c
    ${OSAL_COMMON_SOURCES}
    ${OSAL_BACKEND_SOURCES}
)

add_library(xy_osal STATIC ${OSAL_SOURCES})

# Link against xy_misc for xy_tick and xy_timer_sw
target_link_libraries(xy_osal PUBLIC xy_misc)

# Include directories
target_include_directories(xy_osal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OSAL_BACKEND_INCLUDES}
)

# Compile definitions
target_compile_definitions(xy_osal PUBLIC
    ${OSAL_BACKEND_DEFINITIONS}
)

# Feature definitions
if(XY_OS_FEATURE_THREAD)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_THREAD=1)
endif()
if(XY_OS_FEATURE_THREAD_FLAGS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_THREAD_FLAGS=1)
endif()
if(XY_OS_FEATURE_MUTEX)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MUTEX=1)
endif()
if(XY_OS_FEATURE_SEMAPHORE)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_SEMAPHORE=1)
endif()
if(XY_OS_FEATURE_EVENT_FLAGS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_EVENT_FLAGS=1)
endif()
if(XY_OS_FEATURE_MESSAGE_QUEUE)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MESSAGE_QUEUE=1)
endif()
if(XY_OS_FEATURE_MEMORY_POOL)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MEMORY_POOL=1)
endif()
if(XY_OS_FEATURE_TIMER)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_TIMER=1)
endif()
if(XY_OS_FEATURE_DELAY)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_DELAY=1)
endif()

# Debug definitions
if(XY_OS_PARAM_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_PARAM_CHECK=1)
endif()
if(XY_OS_ISR_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_ISR_CHECK=1)
endif()
if(XY_OS_NULL_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_NULL_CHECK=1)
endif()
if(XY_OS_ENABLE_STATS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_ENABLE_STATS=1)
endif()

# ==================== Link Libraries ====================

# Link with backend RTOS if needed
if(OSAL_BACKEND STREQUAL "freertos")
    if(TARGET freertos_kernel)
        target_link_libraries(xy_osal PUBLIC freertos_kernel)
    endif()
elseif(OSAL_BACKEND STREQUAL "rt-thread")
    if(TARGET rtthread)
        target_link_libraries(xy_osal PUBLIC rtthread)
    endif()
elseif(OSAL_BACKEND STREQUAL "cmsis-rtx")
    if(TARGET rtx5)
        target_link_libraries(xy_osal PUBLIC rtx5)
    endif()
endif()

# ==================== Installation ====================

install(TARGETS xy_osal
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    xy_os.h
    xy_os_cfg.h
    DESTINATION include/xy_osal
)

# ==================== Examples ====================

option(BUILD_OSAL_EXAMPLES "Build OSAL examples" OFF)

if(BUILD_OSAL_EXAMPLES)
    add_executable(xy_osal_example examples/osal_example.c)
    target_link_libraries(xy_osal_example xy_osal)
endif()

# ==================== Summary ====================

message(STATUS "XY OSAL Configuration:")
message(STATUS "  Backend: ${OSAL_BACKEND}")
message(STATUS "  Thread Support: ${XY_OS_FEATURE_THREAD}")
message(STATUS "  Mutex Support: ${XY_OS_FEATURE_MUTEX}")
message(STATUS "  Semaphore Support: ${XY_OS_FEATURE_SEMAPHORE}")
message(STATUS "  Timer Support: ${XY_OS_FEATURE_TIMER}")
message(STATUS "  Parameter Checks: ${XY_OS_PARAM_CHECK}")
message(STATUS "  Build Examples: ${BUILD_OSAL_EXAMPLES}")
