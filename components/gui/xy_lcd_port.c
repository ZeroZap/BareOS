/**
 * @file xy_lcd_port.c
 * @brief BSP hooks for the GUI/GDI layer — replace with your hardware driver.
 *
 * This file contains weak stub implementations of the three BSP functions
 * required by xy_gdi.c:
 *
 *   void xy_lcd_hw_init(void)   — called once by xy_gdi_init()
 *   void xy_lcd_flush(void)     — upload g_xy_lcd_buf to the display
 *   void xy_lcd_on_off(bool on) — turn panel on/off
 *
 * Replace or override these in your BSP.  Example for UC1701 (128×64):
 *
 *   void xy_lcd_hw_init(void) {
 *       // SPI/GPIO setup, reset pulse, UC1701 init sequence
 *   }
 *
 *   void xy_lcd_flush(void) {
 *       // For _SCAN_HV_ layout (page mode):
 *       for (uint8_t page = 0; page < 8; page++) {
 *           lcd_cmd(0xB0 | page);        // set page address
 *           lcd_cmd(0x00);               // col low nibble = 0
 *           lcd_cmd(0x10);               // col high nibble = 0
 *           lcd_data(&g_xy_lcd_buf[page * 128], 128);
 *       }
 *   }
 *
 *   void xy_lcd_on_off(bool on) {
 *       lcd_cmd(on ? 0xAF : 0xAE);      // display on/off
 *   }
 *
 * For SSD1306 (I2C, 128×64):
 *
 *   void xy_lcd_flush(void) {
 *       ssd1306_cmd(0x21); ssd1306_cmd(0); ssd1306_cmd(127); // col range
 *       ssd1306_cmd(0x22); ssd1306_cmd(0); ssd1306_cmd(7);   // page range
 *       ssd1306_data(g_xy_lcd_buf, XY_LCD_BUFSIZE);
 *   }
 */

#include "xy_gdi.h"

/* These are already declared __attribute__((weak)) in xy_gdi.c.
 * This file is a placeholder — delete or replace when you have real hardware.
 * If xy_gdi.c weak stubs are sufficient (NOP), you do not need this file. */
