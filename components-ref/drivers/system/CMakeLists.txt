# System Drivers

# Key
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/key)
    add_subdirectory(key)
endif()

# RTC
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rtc)
    add_subdirectory(rtc)
endif()

# Watchdog
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/watchdog)
    add_subdirectory(watchdog)
endif()

# Additional system files in this directory
file(GLOB SYSTEM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(SYSTEM_SOURCES)
    xy_add_srcs(${SYSTEM_SOURCES})
endif()
