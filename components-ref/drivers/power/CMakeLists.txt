# Power Drivers

# Charger
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/charger)
    add_subdirectory(charger)
endif()

# Fuel Gauge
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fuel_gauge)
    add_subdirectory(fuel_gauge)
endif()
