# RFID Drivers

# RC522
file(GLOB RFID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(RFID_SOURCES)
    xy_add_srcs(${RFID_SOURCES})
endif()
