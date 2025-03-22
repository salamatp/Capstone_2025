/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#include <string.h>
#include <string>
#include <vector>
#include "/home/hoangann107/pico/Capstone_2025/PICO/pico-sdk/src/rp2_common/hardware_gpio/include/hardware/gpio.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <unistd.h>  // For sleep in Linux
//#include "hardware/timer.h"
//void sleep_ms(int ms) { usleep(ms * 1000); }
//void sleep_us(int us) { usleep(us); }
struct st7789_config {
    spi_inst_t* spi;
    uint gpio_din;
    uint gpio_clk;
    int gpio_cs;
    uint gpio_dc;
    uint gpio_rst;
    uint gpio_bl;
};
 static struct st7789_config st7789_cfg;
 static uint16_t st7789_width;
 static uint16_t st7789_height;
 static bool st7789_data_mode = false;
 
const uint8_t font[36][5] = {
    // A-Z
   {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x41, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01},// F
    {0x3E, 0x41, 0x49, 0x49, 0x3A}, // G
    {0x20, 0x54, 0x54, 0x54, 0x78},// a
    {0x7F, 0x48, 0x44, 0x44, 0x38},// b, need to find out the smaller b bitmap
    {0x08, 0x14, 0x54, 0x54, 0x3C},// g
    {0x00, 0x44, 0x7D, 0x40, 0x00},// i
    {0x20, 0x40, 0x44, 0x3D, 0x00},// j
    {0x7C, 0x04, 0x18, 0x04, 0x78},// m
    {0x3C, 0x40, 0x40, 0x20, 0x7C},// u
    {0x14, 0x7F, 0x14, 0x7F, 0x14},// #
    {0x08, 0x08, 0x08, 0x08, 0x08},// -
};

static void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len)
{
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
    st7789_data_mode = false;

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);
    
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));
    
    if (len) {
        sleep_us(1);
        gpio_put(st7789_cfg.gpio_dc, 1);
        sleep_us(1);
        
        spi_write_blocking(st7789_cfg.spi, data, len);
    }

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_caset(uint16_t xs, uint16_t xe)
{
    uint8_t data[] = {
        xs >> 8,
        xs & 0xff,
        xe >> 8,
        xe & 0xff,
    };

    // CASET (2Ah): Column Address Set
    st7789_cmd(0x2a, data, sizeof(data));
}

void st7789_raset(uint16_t ys, uint16_t ye)
{
    uint8_t data[] = {
        ys >> 8,
        ys & 0xff,
        ye >> 8,
        ye & 0xff,
    };

    // RASET (2Bh): Row Address Set
    st7789_cmd(0x2b, data, sizeof(data));
}
void st7789_set_cursor(uint16_t x, uint16_t y)
{
    st7789_caset(x, st7789_width);
    st7789_raset(y, st7789_height);
}
void st7789_ramwr()
{
    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    // RAMWR (2Ch): Memory Write
    uint8_t cmd = 0x2c;
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}
void st7789_write(const void* data, size_t len)
{
    if (!st7789_data_mode) {
        st7789_ramwr();

        if (st7789_cfg.gpio_cs > -1) {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        } else {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }

        st7789_data_mode = true;
    }

    spi_write16_blocking(st7789_cfg.spi, (const uint16_t*)data, len / 2);
}
void st7789_put(uint16_t pixel)
{
    st7789_write(&pixel, sizeof(pixel));
}
void st7789_fill(uint16_t pixel)
{
    int num_pixels = st7789_width * st7789_height;

    st7789_set_cursor(0, 0);

    for (int i = 0; i < num_pixels; i++) {
        st7789_put(pixel);
    }
}
void st7789_init(const struct st7789_config* config, uint16_t width, uint16_t height)
{
    memcpy(&st7789_cfg, config, sizeof(st7789_cfg));
    st7789_width = width;
    st7789_height = height;

    spi_init(st7789_cfg.spi, 125 * 1000 * 1000);
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }

    gpio_set_function(st7789_cfg.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(st7789_cfg.gpio_clk, GPIO_FUNC_SPI);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_init(st7789_cfg.gpio_cs);
    }
    gpio_init(st7789_cfg.gpio_dc);
    gpio_init(st7789_cfg.gpio_rst);
    gpio_init(st7789_cfg.gpio_bl);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_set_dir(st7789_cfg.gpio_cs, GPIO_OUT);
    }
    gpio_set_dir(st7789_cfg.gpio_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_rst, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_bl, GPIO_OUT);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    gpio_put(st7789_cfg.gpio_rst, 1);
    sleep_ms(100);
    
    // SWRESET (01h): Software Reset
    st7789_cmd(0x01, NULL, 0);
    sleep_ms(150);

    // SLPOUT (11h): Sleep Out
    st7789_cmd(0x11, NULL, 0);
    sleep_ms(50);

    // COLMOD (3Ah): Interface Pixel Format
    // - RGB interface color format     = 65K of RGB interface
    // - Control interface color format = 16bit/pixel
    uint8_t COLMOD = 0x55;
    st7789_cmd(0x3A, &COLMOD, 1);
    //st7789_cmd(0x3a, (uint8_t[]){ 0x55 }, 1);
    sleep_ms(10);

    // MADCTL (36h): Memory Data Access Control
    // - Page Address Order            = Top to Bottom
    // - Column Address Order          = Left to Right
    // - Page/Column Order             = Normal Mode
    // - Line Address Order            = LCD Refresh Top to Bottom
    // - RGB/BGR Order                 = RGB
    // - Display Data Latch Data Order = LCD Refresh Left to Right
    uint8_t MADCTL = 0x00;
    st7789_cmd(0x36, &MADCTL, 1);
   
    st7789_caset(0, width);
    st7789_raset(0, height);

    // INVON (21h): Display Inversion On
    st7789_cmd(0x21, NULL, 0);
    sleep_ms(10);

    // NORON (13h): Normal Display Mode On
    st7789_cmd(0x13, NULL, 0);
    sleep_ms(10);

    // DISPON (29h): Display On
    st7789_cmd(0x29, NULL, 0);
    sleep_ms(10);

    gpio_put(st7789_cfg.gpio_bl, 1);
}
void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale) {
    //st7789_fill(0xFFFF);
   
    uint8_t index = 0;  // Default index

    // Handle special characters # and b
    if (c == '#') {
        index = 14;  // Font array index for '#'
    }
    else if (c == '-') {
        index = 15; 
    }
    else if (c == 'a') {
        index = 7;  
    }
    else if (c == 'b') {
        index = 8;  
    }
    else if (c == 'g') {
        index = 9;  
    }
    else if (c == 'i') {
        index = 10;  
    }
    else if (c == 'j') {
        index = 11;  
    }
    else if (c == 'm') {
        index = 12;  
    }
    else if (c == 'u') {
        index = 13;  
    }
    else if (c >= 'A' && c <= 'Z') {
        // Get the index of the character in the  font array for A-Z
        index = c - 'A';  // For 'A' to 'Z'
    } else {
        return;  // Unsupported character
    }

    // Draw the character
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t column = font[index][i];
        for (uint8_t j = 0; j < 7; j++) { // Loop through rows
            if (column & (1 << j)) { // If pixel is set
                // Scale each pixel to a `scale x scale` block
                for (uint8_t dx = 0; dx < scale; dx++) {
                    for (uint8_t dy = 0; dy < scale; dy++) {
                        st7789_set_cursor(x + i * scale + dx, y + j * scale + dy);
                        st7789_put(color);
                    }
                }
            }
        }
        
    }
}

void st7789_draw_string(uint16_t x, uint16_t y, const std::string &str, uint16_t color, uint8_t scale) {
    //Clear the entire row of previous string
    int num_pixels = st7789_width * 8*scale; //8 is height of the letter for now

    st7789_set_cursor(x, y); 
    for (int i = 0; i < num_pixels; i++) {
        st7789_put(0x0000);
    }

    //Now, draw the current string to the screen
    for (size_t i = 0; i < str.size(); i++) {
        st7789_draw_char(x, y, str[i], color, scale);
        x += 5*scale; // Move to the next character position
        //str++;
    }
}

/**
    * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
    *
    * SPDX-License-Identifier: BSD-3-Clause
    */
    /*
    #include "pico/stdlib.h"

    // Pico W devices use a GPIO on the WIFI chip for the LED,
    // so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
    #ifdef CYW43_WL_GPIO_LED_PIN
    #include "pico/cyw43_arch.h"
    #endif

    #ifndef LED_DELAY_MS
    #define LED_DELAY_MS 250
    #endif

    // Perform initialisation
    int pico_led_init(void) {
    #if defined(PICO_DEFAULT_LED_PIN)
        // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
        // so we can use normal GPIO functionality to turn the led on and off
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
        return PICO_OK;
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        // For Pico W devices we need to initialise the driver etc
        return cyw43_arch_init();
    #endif
    }

    // Turn the led on or off
    void pico_set_led(bool led_on) {
    #if defined(PICO_DEFAULT_LED_PIN)
        // Just set the GPIO on or off
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        // Ask the wifi "driver" to set the GPIO on or off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    #endif
    }

    int main() {
        int rc = pico_led_init();
        hard_assert(rc == PICO_OK);
        while (true) {
            pico_set_led(true);
            sleep_ms(LED_DELAY_MS);
            pico_set_led(false);
            sleep_ms(LED_DELAY_MS);
        }
    }
*/

//#include "st7789.h" 
// #include "st7789.h"

//#define SCREEN_WIDTH  240
//#define SCREEN_HEIGHT 135

// Define SPI pins
#define SPI_PORT spi1
//#define SPI_CLK 1 * 1000 * 1000  // 1 MHz SPI clock
#define PIN_SCK   2  // SPI0 SCK
#define PIN_DIN   3  // SPI0 MOSI (TX)
#define PIN_CS    5  // Chip Select
#define PIN_DC    6  // Data/Command
#define PIN_RST 7  // Reset
#define PIN_BLK    8  // Backlight Control (Optional)

#define PUSH_BUTTON_PIN 15
// You can choose your own colors. Here, we use black for the background and white for text.
#define BACKGROUND_COLOR 0x0000   // Black
#define TEXT_COLOR       0xFFFF   // White


// Function to reset (clear) the screen and write a string to the LCD.
void write_to_lcd(int mode, std::vector<std::string> &str, bool flag) {
    // Clear the screen by filling it with BACKGROUND_COLOR
    if (flag){
        st7789_fill(0x0000);
        // Set the cursor to the top-left of the screen (or wherever you'd like to start)
        st7789_set_cursor(0, 0);
        // Draw the string starting at position (0, 0) with TEXT_COLOR
        //for (int i = 0; i < sizeof(str) -1 ; i++){
        if (mode == 1){
            st7789_draw_string(0, 0, str[0], 0xFFFF, 5);
            sleep_ms(1500);
        }
        if (mode == 2){
            st7789_draw_string(0, 0, str[0], 0xFFFF, 5);
            st7789_draw_string(0, 40, str[1], 0xFFFF, 5);
            sleep_ms(1500);
        }
        
        if (mode == 3){
            st7789_draw_string(0, 0, str[0], 0xFFFF, 5);
            st7789_draw_string(0, 40, str[1], 0xFFFF, 5);
            st7789_draw_string(0, 80, str[2], 0xFFFF, 5);
            sleep_ms(1500);
        }
        if (mode == 4){
            st7789_draw_string(0, 0, str[0], 0xFFFF, 5);
            st7789_draw_string(0, 40, str[1], 0xFFFF, 5);
            st7789_draw_string(0, 80, str[2], 0xFFFF, 5);
            st7789_draw_string(0, 120, str[3], 0xFFFF, 5);
            sleep_ms(1500);
        } 
    }
        
    //}
}
int mode = 1;  // Default mode
void button_callback(uint gpio, uint32_t events) {
    if (gpio == PUSH_BUTTON_PIN) {
        mode++; // Increment mode
        if (mode > 4) {
            mode = 1; // Loop back to mode 1
        }
    }
}
void push_button_setup(){
    stdio_init_all(); // Initialize USB serial output (for debugging)

    gpio_init(PUSH_BUTTON_PIN);
    gpio_set_dir(PUSH_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PUSH_BUTTON_PIN);  // Enable internal pull-up resistor
}
// Example usage in your main function:
int main() {
    push_button_setup();
    gpio_init(25);        
    gpio_set_dir(25, GPIO_OUT);  // Set it as output
    // Set up interrupt on falling edge (button press)
    gpio_set_irq_enabled_with_callback(PUSH_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback); //check if the gpio is low which is push button is pushed
    // Assume your st7789_config is already defined and initialized
    struct st7789_config config = {
        .spi = spi0,
        .gpio_din = PIN_DIN,    // Example GPIO pins; adjust for your hardware
        .gpio_clk = PIN_SCK,
        .gpio_cs = PIN_CS,
        .gpio_dc = PIN_DC,
        .gpio_rst = PIN_RST,
        .gpio_bl = PIN_BLK,
    };

    st7789_init(&config, 240, 320);
    std::vector<std::string> chord_list = {"A B# C", "E F Gb", "F A B C", "E E E", "A Ab A"};
    // Now call the write_to_lcd() function with a string.
    //write_to_lcd("A B C b D");
    
    // Your application loop here (if needed)
    int prev_mode = 1;
    /*
    while (1) {
        if (gpio_get(PUSH_BUTTON_PIN) == 0) {  // Button is pressed (low)
            gpio_put(25, 0);  // Turn on the LED
        } else {  // Button is not pressed (high)
            gpio_put(25, 1);  // Turn off the LED
        }
    }

    return 0;
    */
    bool flag = true;
    while (1) {
        tight_loop_contents();
        if (mode != prev_mode) {  // Only update LCD when mode changes
            gpio_put(25, 0);
        }
        write_to_lcd(mode, chord_list, flag);
        
    }
    
    return 0;
    
    
}
