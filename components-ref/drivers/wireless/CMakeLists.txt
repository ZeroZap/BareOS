# Wireless Drivers

# RFID
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rfid)
    add_subdirectory(rfid)
endif()

# WiFi
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wifi)
    add_subdirectory(wifi)
endif()

# Bluetooth
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth)
    add_subdirectory(bluetooth)
endif()
