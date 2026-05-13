# XinYi System Components

set(SYS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine/src/xy_state_machine.c
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys/src/xy_sys.c
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer/src/xy_timer.c
)

set(SYS_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine/inc/xy_state_machine.h
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys/inc/xy_sys.h
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer/inc/xy_timer.h
)

set(SYS_SOURCES ${SYS_SOURCES} PARENT_SCOPE)
set(SYS_HEADERS ${SYS_HEADERS} PARENT_SCOPE)
set(SYS_INCLUDE_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine/inc
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys/inc
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer/inc
    PARENT_SCOPE
)
