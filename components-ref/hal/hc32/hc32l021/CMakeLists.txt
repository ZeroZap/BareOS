# HC32L021 HAL Driver CMake Configuration

# Source files
set(HAL_HC32L021_SOURCES
    xy_hal_gpio_device.c
    xy_hal_gpio_device_minimal.c
    xy_hal_uart_device.c
    xy_hal_adc_device.c
    xy_hal_i2c_device.c
    xy_hal_spi_device.c
    xy_hal_timer_device.c
    xy_hal_pwm_device.c
    system_hc32l021.c
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

# Create static library
add_library(xy_hal_hc32l021 STATIC ${HAL_HC32L021_SOURCES})

# Minimal test executable
add_executable(minimal_gpio_test minimal_gpio_test.c)
target_link_libraries(minimal_gpio_test xy_hal_hc32l021)

# Set platform-specific definitions
target_compile_definitions(xy_hal_hc32l021 PRIVATE XY_HAL_HC32L021)
target_compile_definitions(minimal_gpio_test PRIVATE XY_HAL_HC32L021)