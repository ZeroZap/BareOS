# XinYi Kernel Components
# OSAL + Kernel Services

set(CMAKE_C_STANDARD 99)

# ==================== OSAL Core ====================

set(OSAL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_task.c
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_sem.c
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_queue.c
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_mem.c
)

set(OSAL_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_task.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_sem.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_mutex.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_queue.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_mem.h
)

# Backend selection
if(CONFIG_OSAL_BACKEND_FREERTOS)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/freertos/xy_os_freertos.c)
elseif(CONFIG_OSAL_BACKEND_RTTHREAD)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/rtthread/xy_os_rtthread.c)
elseif(CONFIG_OSAL_BACKEND_RTX5)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/rtx5/xy_os_rtx5.c)
elseif(CONFIG_OSAL_BACKEND_BAREMETAL)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/baremetal/xy_os_baremetal.c)
endif()

# ==================== Misc Services ====================

set(MISC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/misc/src/xy_sysmon.c
)

set(MISC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/misc/inc/xy_sysmon.h
)

# ==================== Services ====================

set(SERVICE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/service/src/xy_scheduler.c
)

set(SERVICE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/service/inc/xy_scheduler.h
)

# ==================== Aggregate ====================

set(KERNEL_SOURCES
    ${OSAL_SOURCES}
    ${MISC_SOURCES}
    ${SERVICE_SOURCES}
    PARENT_SCOPE
)

set(KERNEL_HEADERS
    ${OSAL_HEADERS}
    ${MISC_HEADERS}
    ${SERVICE_HEADERS}
    PARENT_SCOPE
)

set(KERNEL_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc
    ${CMAKE_CURRENT_LIST_DIR}/misc/inc
    ${CMAKE_CURRENT_LIST_DIR}/service/inc
    PARENT_SCOPE
)

# ==================== Tests ====================

if(BUILD_TESTING)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/osal/tests)
endif()

# ==================== Summary ====================

message(STATUS "Kernel Components:")
message(STATUS "  OSAL Backend: ${CONFIG_OSAL_BACKEND}")
message(STATUS "  Sources: ${OSAL_SOURCES}")
