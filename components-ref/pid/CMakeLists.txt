# XinYi PID Controller

set(PID_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_pid.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_pid_positional.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_pid_incremental.c
)

set(PID_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_pid.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_pid_types.h
)

set(PID_SOURCES ${PID_SOURCES} PARENT_SCOPE)
set(PID_HEADERS ${PID_HEADERS} PARENT_SCOPE)
set(PID_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/inc PARENT_SCOPE)
