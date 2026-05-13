#include "xy_hal_gpio.h"
#include "xy_hal_error.h"
#include <stdio.h>

int main() {
    printf("Starting minimal GPIO test...\n");
    
    // Test GPIO initialization
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result = xy_hal_gpio_init((void*)0, 0, &config);
    printf("GPIO init result: %d\n", result);
    
    if (result == XY_HAL_OK) {
        printf("GPIO initialization successful!\n");
        
        // Test GPIO write
        result = xy_hal_gpio_write((void*)0, 0, 1);
        printf("GPIO write result: %d\n", result);
        
        if (result == XY_HAL_OK) {
            printf("GPIO write successful!\n");
            
            // Test GPIO read
            int32_t value = xy_hal_gpio_read((void*)0, 0);
            printf("GPIO read value: %d\n", value);
            
            if (value >= 0) {
                printf("GPIO read successful!\n");
                printf("Minimal GPIO test PASSED!\n");
                return 0;
            }
        }
    }
    
    printf("Minimal GPIO test FAILED!\n");
    return -1;
}