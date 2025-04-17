 
//For PICO:
#include <iostream>
#include <cmath>  // For sqrt and other math functions
#include <string>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
// Mutex for thread safety
#include "pico/mutex.h"
#include "kiss_fftr.h"
 
//For LCD:
#include <string.h>
#include <vector>
#include "/home/arnur/Capstone_2025/PICO/pico-sdk/src/rp2_common/hardware_gpio/include/hardware/gpio.h"
#include <stdio.h>
#include <unistd.h>  // For sleep in Linux
 
//////////////////////////////////////////////////////LCD CODE//////////////////////////////////////////////////////////////
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
 
const uint8_t font[55][5] = {
  // A-Z
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x41, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
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
  {0x38, 0x44, 0x44, 0x48, 0x7F},// d
  {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
  {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
  {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x48, 0x44, 0x42, 0x40}, // R (33)
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O (34)
  {0x78, 0x08, 0x08, 0x08, 0x78}, // T (35)
  {0x38, 0x54, 0x54, 0x54, 0x18}, // e (36)
  {0x7C, 0x14, 0x14, 0x14, 0x08}, // r (37)
  {0x38, 0x44, 0x44, 0x44, 0x38}, // o (38)
  {0x1C, 0x22, 0x7F, 0x22, 0x1C}, // v (39)
  {0x7E, 0x09, 0x09, 0x09, 0x06}, // S (40)
  {0x3E, 0x49, 0x49, 0x49, 0x31}, // c (41)
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // ! (42)
  {0x00, 0x36, 0x36, 0x00, 0x00}, // : (43)
  {0x44, 0x28, 0x10, 0x28, 0x44}, // x (44)
  {0x00, 0x3C, 0x14, 0x14, 0x14}, // h (45)
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L (11)
 
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
  uint8_t MADCTL = 0x60;
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
  else if (c == 'd') {
      index = 16;
  }
  else if (c == 'n') {
      index = 17;
  }
  else if (c == 'P') {
    index = 18;
  }
  else if (c == 'l') {
    index = 19;
  }
  else if (c == 'y') {
    index = 20;
  }
  else if (c == 't') {
    index = 21;
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
  else if (c == '1') {
    index = 22;  
  }
  else if (c == '2') {
    index = 23;  
  }
  else if (c == '3') {
    index = 24;  
  }
  else if (c == '4') {
    index = 25;  
  }
  else if (c == '5') {
    index = 26;  
  }
  else if (c == '6') {
    index = 27;  
  }
  else if (c == '7') {
    index = 28;  
  }
  else if (c == '8') {
    index = 29;  
  }
  else if (c == '9') {
    index = 30;  
  }
  else if (c == '0') {
      index = 31;  
  }
  else if (c == 'A') {
      index = 0;  // For 'A' to 'Z'
  }
  else if (c == 'B') {
      index = 1;  // For 'A' to 'Z'
  }
  else if (c == 'C') {
      index = 2;  // For 'A' to 'Z'
  }
  else if (c == 'D') {
      index = 3;  // For 'A' to 'Z'
  }
  else if (c == 'E') {
      index = 4;  // For 'A' to 'Z'
  }
  else if (c == 'F') {
      index = 5;  // For 'A' to 'Z'
  }
  else if (c == 'G') {
      index = 6;  // For 'A' to 'Z'
  }
  else if (c == 'M') {
      index = 32;  // For 'A' to 'Z'
  }
  else if (c == 'S') {
    index = 40;
  }
  else if (c == 'R') {
    index = 33;
  }
  else if (c == 'O') {
    index = 34;
  }
  else if (c == 'T') {
    index = 35;
  }
  else if (c == 'e') {
    index = 36;
  }
  else if (c == 'r') {
    index = 37;
  }
  else if (c == 'o') {
    index = 38;
  }
  else if (c == 'v') {
    index = 39;
  }
  else if (c == 'c') {
    index = 41;
  }
  else if (c == '!') {
    index = 42;
  }
  else if (c == ':') {
    index = 43;
  }
  else if (c == 'x') {
    index = 44;
  }
  else if (c == 'h') {
    index = 45;
  }
  else if (c == 'L'){
    index = 46;
  }
  else {
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
  // Clear only the area where the string will be drawn
  st7789_set_cursor(x, y);
  for (int i = 0; i < str.size() * 5 * scale; i++) {
      st7789_put(0x0000); // Clear background
  }
  //st7789_fill(0xFFFF);
  // Draw the new string
  for (size_t i = 0; i < str.size(); i++) {
      st7789_draw_char(x, y, str[i], color, scale);
      x += 5 * scale; // Move to the next character position
  }
}
 
//#define SPI_CLK 1 * 1000 * 1000  // 1 MHz SPI clock
#define PIN_SCK   18  // SPI0 SCK
#define PIN_DIN   19  // SPI0 MOSI (TX)
#define PIN_CS    17  // Chip Select
#define PIN_DC    16  // Data/Command
#define PIN_RST 15  // Reset
#define PIN_BLK    14  // Backlight Control (Optional)
 
#define PUSH_BUTTON_PIN 20
// You can choose your own colors. Here, we use black for the background and white for text.
#define BACKGROUND_COLOR 0x0000   // Black
#define TEXT_COLOR       0xFFFF   // White
 
 
// Function to reset (clear) the screen and write a string to the LCD.
void write_to_lcd(bool mode, std::vector<std::string> &str, bool flag) {
  // 
  
  if(mode == 0){
    st7789_fill(0x0000);
    st7789_set_cursor(0, 0);
    // Draw the string starting at position (0, 0) with TEXT_COLOR
    //for (int i = 0; i < sizeof(str) -1 ; i++){
    st7789_draw_string(0, 0, str[0], 0xFFFF, 12);
    st7789_draw_string(0, 125, str[1], 0xFFFF, 10);
    st7789_draw_string(175, 0, str[2], 0xFFFF, 3);
    st7789_draw_string(200, 125 , str[3], 0xF800, 10);
    // printf("score: %s\n", str[3].c_str());
    sleep_ms(400);
  }
  else if (mode == 1){
 
    if (flag){    
      st7789_fill(0x0000);
      st7789_set_cursor(0, 0);
      if(mode == 1){
        st7789_draw_string(0, 0, str[0], 0xFFFF, 15);
        st7789_draw_string(0, 125, str[1], 0xFFFF, 10);
        sleep_ms(400);
      }
    }
  }
  else if (mode == 2){
    st7789_fill(0x0000);
    st7789_set_cursor(0, 0);
    st7789_draw_string(0, 0, str[0], 0xFFFF, 3);
  }
     
}
 
 
volatile bool button_pressed = false;
volatile int mode = 1;  // Start in regular mode (1)
 
//////////////////////////////////////////////////////
// Button Interrupt Handler
//////////////////////////////////////////////////////
void button_callback(uint gpio, uint32_t events) {
    static uint32_t last_press_time = 0;
    uint32_t current_time = time_us_32();
   
    // Debounce - ignore presses within 200ms
    if (current_time - last_press_time > 200000) {
        button_pressed = true;
        last_press_time = current_time;
    }
}
 
 
void push_button_setup() {
    gpio_init(PUSH_BUTTON_PIN);
    gpio_set_dir(PUSH_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PUSH_BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(PUSH_BUTTON_PIN,
                                     GPIO_IRQ_EDGE_FALL,
                                     true,
                                     &button_callback);
}
 
//////////////////////////////////////////////////////PICO CODE/////////////////////////////////////////////////////////////
/////////////////////////////////////
// Hardware Configuration
/////////////////////////////////////
#define SPI_PORT spi1
#define SPI_CLK 1 * 1000 * 1000  // 1 MHz SPI clock
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 10
#define SPI_CS 13
 
#include <cstdlib>    // for rand() and srand()
#include <ctime>      // for time()
#include <vector>
#include <string>
 
 
 
// Array of possible chord roots and qualities for triads
const std::vector<std::string> chord_roots_gamify = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
 
const std::vector<std::string> chord_qualities_gamify = {
    "Maj", "Min", "Dim", "Aug"
};
 
 
// this function normalized the 10 bit ADC value between -1 to 1:
float Normalizer(uint16_t sample) {
    const float ADC_MAX = 1023.0f;  // Max ADC value
    float normalized = (2.0f * static_cast<float>(sample) / ADC_MAX) - 1.0f;
    // std::cout <<"LETS PRINT THE NORMALIZED INSIDE NORAMALIZER    " << normalized << '\n';
    return normalized;
}
    // Actual SPI Initialization for Pico
    void spi_init_custom() {
        // printf("Starting SPI initialization\n");
        spi_init(SPI_PORT, SPI_CLK);
        gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
        gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
        gpio_init(SPI_CS);
        gpio_set_dir(SPI_CS, GPIO_OUT);
        gpio_put(SPI_CS, 1);  // Deselect the chip
        // printf("SPI Initialized\n");
    }
    uint16_t read_adc(uint8_t channel) {
        if (channel > 7) return 0;
        uint8_t tx_data[3] = {0x01, (uint8_t)(0x80 | (channel << 4)), 0x00};
        uint8_t rx_data[3] = {0};
 
        gpio_put(SPI_CS, 0);
        spi_write_read_blocking(SPI_PORT, tx_data, rx_data, 3);
        gpio_put(SPI_CS, 1);
 
        uint16_t result = ((rx_data[1] & 0x03) << 8) | rx_data[2];
        // printf("ADC Value: %d\n", result);
        return result;
    }
//#endif
 
// Capture audio samples (Pico & Linux compatible)
void capture_audio(uint16_t num_samples, uint16_t sampling_rate, float* samples) {
    uint32_t start_time = 0;
 
    start_time = time_us_32();
 
    uint32_t interval_us = 1000000 / sampling_rate;
    for (uint16_t i = 0; i < num_samples; i++) {
        samples[i] = Normalizer(read_adc(0));  // Read from ADC (real or simulated)
        //std::cout << samples[i] << "\n";
        uint32_t target_time = start_time + (i + 1) * interval_us;
        while (time_us_32() < target_time) {}  // Busy-wait to maintain timing
    }
 
}
 
 
void capture_audio_test(uint16_t num_samples, uint16_t sampling_rate,  float* samples) {
 
  // Record the start time (in microseconds)
  uint32_t start_time = time_us_32();
  // Calculate the desired interval between samples in microseconds
  uint32_t interval_us = 1000000 / sampling_rate;
 
  // Capture and print samples
  for (uint16_t i = 0; i < num_samples; i++) {
      // Read from ADC and print the sample
      samples[i] = Normalizer(read_adc(0));  // Read from ADC (real or simulated)
     
      // Compute target time for the next sample
      uint32_t target_time = start_time + (i + 1) * interval_us;
      int32_t remaining_time = target_time - time_us_32();
     
      // Sleep for the remaining time if needed
      if (remaining_time > 0) {
          sleep_us(remaining_time);
      }
  }
 
  // Compute the actual elapsed time and sampling rate
  uint32_t end_time = time_us_32();
  float duration_sec = (end_time - start_time) / 1000000.0f;
  float actual_sampling_rate = num_samples / duration_sec;
 
  // Print both the expected and the actual sampling rate
  // printf("Actual Sampling Rate: %.2f Hz\n", actual_sampling_rate);
}
 
// FFT Function (Works on both Pico & Linux)
void fft(float* input, kiss_fft_cpx* fft_out, uint16_t num_samples, float sampling_rate, float* magnitudes, float* frequencies, kiss_fftr_cfg cfg) {
    float* input_data = new float[num_samples];
    for (size_t i = 0; i < num_samples; i++) {
        input_data[i] = static_cast<float>(input[i]);
    }
 
    kiss_fftr(cfg, input_data, fft_out);
 
    for (size_t i = 0; i < num_samples / 2 + 1; i++) {
        magnitudes[i] = sqrt(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
        frequencies[i] = i * sampling_rate / num_samples;
    }
 
    delete[] input_data;
}
// the frequency is in Hz    
// Map frequency to musical note
uint8_t frequency_2_note(float frequency){
    // Reference: A4 = 440 Hz
    float A4_freq = 440.0;
    uint8_t A4_note = 69;  // MIDI number for A4
 
    // Calculate the closest MIDI number
    if( frequency <= 0){
        return 0;
    }
    uint8_t midi_number = (std::round(12 * std::log2(frequency / A4_freq) + A4_note));
    // printf("MIDI NUMBER: %d\n", (int)midi_number);
    // Map MIDI number to note names
    uint8_t note_index =  (midi_number) % 12;  // Modulo 12 for the note names
    // printf("NOTE INDEX: %d\n", (int)note_index);
    return note_index;
}
 
// const int piano_frequencies[287] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,183,184,185,186,187,194,195,196,197,198,205,206,207,208,209,218,219,220,221,222,231,232,233,234,235,245,246,247,248,249,259,260,261,262,263,275,276,277,278,279,291,292,293,294,295,309,310,311,312,313,327,328,329,330,331,347,348,349,350,351,367,368,369,370,371,389,390,391,392,393,413,414,415,416,417,437,438,439,440,441,463,464,465,466,467,491,492,493,494,495,520,521,522,523,524,552,553,554,555,556,584,585,586,587,588,589,620,621,622,623,624,656,657,658,659,660,696,697,698,699,700,737,738,739,740,741};
 
// Returns an adaptive threshold based on the FFT magnitudes.
// Window Size (window parameter):
// Increase the window size if the signal is noisy or if peaks are spread over several indices. A larger window will average over a wider range.
 
// Delta (delta parameter):
// Use a delta value if you want to require a minimum difference between the candidate peak and its neighbors. If peaks are not sharp, you might set delta to a small negative value (or zero) so that even slight peaks are captured.
 
// Factor (factor parameter):
// Lower the factor if you find that the computed threshold is too high relative to the overall magnitudes.
uint16_t compute_adaptive_threshold(const float* fft_magnitudes, uint16_t size, float factor = 0.7f, int window = 7, float delta = 15.0f) {
  std::vector<float> peaks;
  bool is_peak;
   
  // Start checking after the first 6 points
  for (uint16_t i = 6; i < size - window; i++) {
      is_peak = true;
     
      // Compare with surrounding values in the window
      for (uint16_t j = 1; j <= window; j++) {
          if (fft_magnitudes[i] <= fft_magnitudes[i - j] || fft_magnitudes[i] <= fft_magnitudes[i + j]) {
              is_peak = false;
              break;
          }
      }
 
      // If it's a peak, add to the list
      if (is_peak && fft_magnitudes[i] > delta) {
          peaks.push_back(fft_magnitudes[i]);
      }
  }
 
  // If no peaks were found, return a safe default threshold
  if (peaks.empty()) {
      return delta;
  }
 
  // Manually compute the average peak magnitude
  float sum = 0.0f;
  for (float peak : peaks) {
      sum += peak;
  }
  float avg_peak = sum / peaks.size();
 
  // Return threshold as a fraction of peak magnitude
  return static_cast<uint16_t>(avg_peak * factor);
}
 
 
uint16_t freq_detect(float* freqs,float* fft_magnitudes, uint16_t size){
    // {'C' : 0, 'C#' : 0, 'D' : 0, 'D#' : 0, 'E' : 0, 'F' : 0, 'F#' : 0, 'G' : 0, 'G#' : 0, 'A' : 0, 'A#' : 0, 'B' : 0}
    uint16_t notes = 0;
    // std::cout << "SIZE OF FREQS " << size << "\n";
    uint8_t note = 13;
    uint16_t threshold = compute_adaptive_threshold(fft_magnitudes, size);
    //uint16_t threshold = 80;
    // printf("THRESHOLD: %d\n", threshold);
    for (int i = 6; i < size ; i++){
        // std::cout << "Index i = " << i << " \n";
        // std::cout << "magnitude[ "<< i << " ] is " << fft_magnitudes[i] << "   " << "FFT FREQ[ " << i << " ] is " <<  freqs[i] << "\n";
        // std::cout << (fft_magnitudes[i] >= threshold) << "\n";
        //std::cout << "FFT FREQ: : " << piano_frequencies[i] << "," << freqs[piano_frequencies[i]] <<"\n";
        // std::cout << "FFT MAGNITUDE at : "<< i << "   is   " << fft_magnitudes[i] <<"\n";
        if(fft_magnitudes[i] >= threshold){
            note = frequency_2_note(freqs[i]);
            switch(note) {
                case 0:
                  // C
                    notes |= 1 << 0;
                    // printf("C\n");
                  break;
                case 1:
                  // C#
                    notes |= 1 << 1;
                    // printf("C#\n");
                  break;
                case 2:
                  // 'D'
                    notes |= 1 << 2;
                    // printf("D\n");
                  break;
                case 3:
                  // 'D#'
                    notes |= 1 << 3;
                    // printf("D#\n");
                  break;
                case 4:
                  // 'E'
                    notes |= 1 << 4;
                    // printf("E\n");
                  break;
                case 5:
                  // 'F'
                    notes |= 1 << 5;
                    // printf("F\n");
                  break;
                case 6:
                  // 'F#'
                    notes |= 1 << 6;
                    // printf("F#\n");
                  break;
                case 7:
                  // 'G'
                    notes |= 1 << 7;
                    // printf("G\n");
                  break;
                case 8:
                  // 'G#'
                    notes |= 1 << 8;
                    // printf("G#\n");
                  break;
                case 9:
                  //A
                    notes |= 1 << 9;
                    // printf("A\n");
                  break;
                case 10:
                  //A#
                    notes |= 1 << 10;
                    // printf("A#\n");
                  break;
                case 11:
                  //B
                    notes |= 1 << 11;
                    // printf("B\n");
                  break;
                default:
                    break;
              }
        }
    }
    return notes;
 
}
 
// Note names indexed from 0 (C) to 11 (B)
const char* note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
 
// Chord templates: relative intervals from root
const uint8_t major[3] = {0, 4, 7};      // Root, major third, perfect fifth
const uint8_t minor[3] = {0, 3, 7};      // Root, minor third, perfect fifth
const uint8_t diminished[3] = {0, 3, 6}; // Root, minor third, diminished fifth
const uint8_t augmented[3] = {0, 4, 8};  // Root, major third, augmented fifth
 
// Function to extract active notes from the bitmask
void extract_notes(uint16_t notes, uint8_t* active_notes, uint8_t& count) {
    count = 0;
    for (uint8_t i = 0; i < 12; i++) {
        if (notes & (1 << i)) {
            active_notes[count++] = i;
        }
    }
}
 
// Function to check if a set of notes matches a chord pattern
bool matches_chord(const uint8_t* notes, uint8_t note_count, const uint8_t* pattern, uint8_t pattern_size, uint8_t root) {
    if (note_count != pattern_size) return false; // Must have exactly 3 notes
    for (uint8_t i = 0; i < pattern_size; i++) {
        bool found = false;
        for (uint8_t j = 0; j < note_count; j++) {
            if (notes[j] == (root + pattern[i]) % 12) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}
 
// Function to determine the best matching chord (only triads)
bool get_chord_name(uint16_t notes, std::string* chord_name, std::string* chord_quality) {
    uint8_t active_notes[12];
    uint8_t note_count = 0;
   
    // Extract active notes
    extract_notes(notes, active_notes, note_count);
 
    // If only one note is detected, return it
    if (note_count == 1) {
      *chord_name = note_names[active_notes[0]];
      *chord_quality = "";
      return true;
 
    }
 
    // Ignore if more than 3 notes are present (not a triad)
    if (note_count != 3) {
      *chord_name = "";
      return false;
    }
 
    // Try every note as a potential root
    for (uint8_t root = 0; root < 12; root++) {
        if (matches_chord(active_notes, note_count, major, 3, root)){
          *chord_name = (std::string(note_names[root]));
          *chord_quality = "Maj";
          return true;
        }
        if (matches_chord(active_notes, note_count, minor, 3, root)){
          *chord_name =  (std::string(note_names[root]));
          *chord_quality =  "Min";
          return true;
        }
        if (matches_chord(active_notes, note_count, diminished, 3, root)){
          *chord_name = std::string(note_names[root]);
          *chord_quality = "Dim";
          return true;
        }  
        if (matches_chord(active_notes, note_count, augmented, 3, root)){
          *chord_name =  std::string(note_names[root]);
          *chord_quality =  "Aug";
          return true;
        }
    }
 
    return false;  // No matching chord
}
 
 
void lcd(){
  // Initialize the LCD
  push_button_setup();
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
  st7789_init(&config, 320, 240);
}
 
// Shared chord variable
std::string detected_chord = "------";
std::string detected_quality = "------";
bool is_detected = false;
mutex_t chord_mutex;
 
// Chord detection task
void run() {
    //printf("Initializing...\n");
    spi_init_custom();
    //gpio_put(25, 0);  // Turn on the backlight
 
    uint16_t sampling_rate = 4500;
    uint32_t duration = 333000; // 0.04s second in microseconds (40 ms for 9000 Hz) same as block size
    //uint32_t duration = 33300000; // 0.04s second in microseconds (40 ms for 9000 Hz) same as block size
    uint32_t num_samples = duration / (1000000 / sampling_rate); // 360 samples -> resolution = sampling rate (in Hz)/FFT size = 25 Hz
   
    kiss_fftr_cfg cfg = kiss_fftr_alloc(num_samples, 0, nullptr, nullptr);
    kiss_fft_cpx* fft_out = new kiss_fft_cpx[num_samples / 2 + 1];
    float* magnitudes = new float[num_samples / 2 + 1];
    float* frequencies = new float[num_samples / 2 + 1];
 
    float* samples = new float[num_samples];
    //std::string* detected_chord = new std::string("------");
   
    // std::vector<std::string> chord_list = {"#", "#", "#", "#"};
 
    // bool is_detected_prev = false;
   
    // int prev_mode = 1;
    // write_to_lcd(4, chord_list, true);
    std::string new_chord;
    std::string chord_quality;
    while (true){
      // printf("===================================\n");
        // std::cout << "num_samples= " << num_samples <<"sampling rate= "<< sampling_rate <<"\n";
      capture_audio_test(num_samples, sampling_rate, samples);
      fft(samples, fft_out, num_samples, sampling_rate, magnitudes, frequencies, cfg);
      uint16_t note_index = freq_detect(frequencies, magnitudes, num_samples / 2);
      // for (int i = 0; i < num_samples; i++){
      //     printf("frequency: %f, magnitude: %f \n ", frequencies[i], magnitudes[i]);
      // }
 
      mutex_enter_blocking(&chord_mutex);  // Lock before modifying
      is_detected = get_chord_name(note_index,  &new_chord, &chord_quality);
      if(is_detected){
        detected_chord = new_chord; // Update detected chord
        detected_quality = chord_quality; // Update detected chord
      }
      mutex_exit(&chord_mutex);  // Unlock
     
 
    // printf("Chord detected: %s\n", detected_chord.c_str());
    // sleep_ms(50);  // Small delay to prevent flickering
 
    }
 
    kiss_fftr_free(cfg);
}
 
void display_LCD(){
  //ON BOARD LED
  gpio_init(25);        
  gpio_set_dir(25, GPIO_OUT);  // Set it as output
 
  //REGULAR MODE
  std::string chord_name;
  std::string chord_quality;
  bool is_detected_lcd;
  std::vector<std::string> chord_list = {"#", "#"};
 
  // GAMIFY MODE
  std::string root_gamify;
  std::string quality_gamify;
  std::vector<std::string> chord_list_gamify = {"#", "#", "#", "#"};
  bool matched_gamify;
  std::string current_chord_gamify;
  std::string current_quality_gamify;
  int score_gamify = 0;
  srand(time(NULL));
 
  while(true){
    // REGULAR MODE
    if(mode == 1){
     
      while(true){
        if (button_pressed) {
          if (gpio_get(PUSH_BUTTON_PIN) == 0) {  // Button is pressed (low)
              gpio_put(25, 0);  // Turn on the LED
          } else {  // Button is not pressed (high)
              gpio_put(25, 1);  // Turn off the LED
          }
          button_pressed = false;  // Reset flag
          mode = (mode == 1) ? 0 : 1;  // Toggle between regular mode (1) and gamify mode (0)
          break;  // Exit the while loop to switch modes
        }
        mutex_enter_blocking(&chord_mutex);  // Lock before accessing
        chord_name = detected_chord;
        chord_quality = detected_quality;
        is_detected_lcd = is_detected;
        mutex_exit(&chord_mutex);  // Unlock
       
        chord_list[0] = chord_name;
        chord_list[1] = chord_quality;
        write_to_lcd(1, chord_list, is_detected_lcd);
      }
    }
      // GAMIFY MODE
    else if (mode == 0) {

     
      while (true) {
        

        // Generate a random chord
        root_gamify    = chord_roots_gamify[rand() % chord_roots_gamify.size()];
        quality_gamify = chord_qualities_gamify[rand() % chord_qualities_gamify.size()];
 
        chord_list_gamify[0] = root_gamify;
        chord_list_gamify[1] = quality_gamify;
        if(score_gamify == 5){
          chord_list_gamify[2] = "Good job!";
        }
        else{
          chord_list_gamify[2] = "Play it";
        }
        chord_list_gamify[3] = std::to_string(score_gamify);
 
        // Display the target chord and score on LCD
        write_to_lcd(0, chord_list_gamify, true);
       
        // Wait until the detected chord (updated by chord detection task) matches the target chord.
        matched_gamify = false;
 
        while (!matched_gamify) {
          mutex_enter_blocking(&chord_mutex);
          current_chord_gamify = detected_chord;
          current_quality_gamify = detected_quality;
          mutex_exit(&chord_mutex);
         
          if (current_chord_gamify == root_gamify && current_quality_gamify == quality_gamify) {matched_gamify = true;}
         
          sleep_ms(50);
          if (button_pressed) {
            mode = (mode == 1) ? 0 : 1;  // Toggle between regular mode (1) and gamify mode (0)
            write_to_lcd(1, chord_list, true);
            break;  // Exit the while loop to switch modes
          }
        }
        // When the user plays the chord correctly, increase score
        score_gamify++;
        // Wait 1 sec before starting the next round
        if (button_pressed) {button_pressed = false;score_gamify = 0;break;}  // Exit the while loop to switch modes
        sleep_ms(1000);
         
        }
      }
  }
}
 
 
int main() {
    stdio_init_all();
    lcd();
    mutex_init(&chord_mutex);  // Initialize mutex
 
    // Launch LCD display task on Core 1
    multicore_launch_core1(display_LCD);
   
    // Run chord detection on Core 0
    run();  
    return 0;
}
