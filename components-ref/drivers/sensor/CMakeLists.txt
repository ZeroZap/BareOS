# Sensor Drivers

# Temperature Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/temperature)
    add_subdirectory(temperature)
endif()

# Pressure Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pressure)
    add_subdirectory(pressure)
endif()

# Motion Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/motion)
    add_subdirectory(motion)
endif()

# ADC Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/adc)
    add_subdirectory(adc)
endif()

# Additional sensor files in this directory
file(GLOB SENSOR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(SENSOR_SOURCES)
    xy_add_srcs(${SENSOR_SOURCES})
endif()
