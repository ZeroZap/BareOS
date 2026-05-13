# Storage Drivers

# EEPROM
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/eeprom)
    add_subdirectory(eeprom)
endif()

# Flash
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/flash)
    add_subdirectory(flash)
endif()

# SD Card
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdcard)
    add_subdirectory(sdcard)
endif()

# Additional storage files in this directory
file(GLOB STORAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(STORAGE_SOURCES)
    xy_add_srcs(${STORAGE_SOURCES})
endif()
