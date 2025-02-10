
//#include <vector>
#include <iostream>
//#include <unordered_map>
//#include <cmath>
//#include <string>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "kiss_fftr.h"
#include <cmath> // For sqrt and other math functions


/////////////////////////////////////
#define SAMPLES 128                // Must be a power of 2
#define SAMPLING_FREQUENCY 1000    // Hz
#define SPI_PORT spi0              // Use SPI0
#define SPI_CLK 1 * 1000 * 1000    // 1 MHz SPI clock
#define SPI_MISO 4                 // GPIO for SPI MISO
#define SPI_MOSI 3                 // GPIO for SPI MOSI
#define SPI_SCK 2                  // GPIO for SPI SCK
#define SPI_CS 5                   // GPIO for SPI CS
/////////////////////////////////////

#define N 1024  // Size of the FFT
#define SAMPLE_RATE 44100  // Sampling rate, for example

void spi_init_custom() {
    printf("Starting SPI initilazation\n");
    spi_init(SPI_PORT, SPI_CLK); // 1 MHz
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1); // Deselect the chip
    printf("---------------\n");
    printf("Initialized SPI\n");
}

uint16_t read_adc(uint8_t channel) {
    if (channel > 7) return 0; // MCP3008 supports only 8 channels (0-7)

    uint8_t tx_data[3];
    uint8_t rx_data[3];

    // Construct the command to send to MCP3008
    tx_data[0] = 0x01;                                // Start bit
    tx_data[1] = 0x80 | (channel << 4);               // Single-ended mode + channel (high 3 bits)
    tx_data[2] = 0x00;                                // Dummy byte for clocking out the result

    gpio_put(SPI_CS, 0); // Select the chip
    spi_write_read_blocking(SPI_PORT, tx_data, rx_data, 3);
    gpio_put(SPI_CS, 1); // Deselect the chip

    // Combine the result from rx_data (10-bit result is in bits 1-10 of the response)
    uint16_t result_int = ((rx_data[1] & 0x03) << 8) | rx_data[2];
    uint16_t result_final = result_int >> 2;
    uint8_t result = result_final;
    printf("ADC Value: %d\n", result_int); //to debug 
    // printf("result_int: %d\n",   result_int); 
    // printf("result_final: %d\n", result_final); 
    // printf("result: %d\n",       result);  
    return result_int;
}


void capture_audio(uint16_t num_samples, uint16_t sampling_rate, uint16_t* samples) {
    printf("Starting audio capture\n");
    // Record the start time for the capture
    uint32_t start_time = time_us_32();  // Start time in seconds
    
    uint32_t interval_us = 1000000/sampling_rate; //the interval for each sample
    uint32_t target_time;
    uint32_t current_time;
    uint32_t elapsed_time;
    uint32_t sleep_time;

    for(uint16_t i = 0; i < num_samples; i++ ){
      // Record the current time before taking a sample
      uint32_t current_time = time_us_32();
      uint32_t elapsed_time = current_time - start_time;

      // Read from the ADC (this step will take some time, but we account for it below)
      samples[i] = read_adc(0);  // Read from channel 0 (ADC)
      printf("Sample %d: %d\n", i, samples[i]); // for debug
      // Calculate the elapsed time since the start of the capture
      elapsed_time = time_us_32() - start_time;

      // Calculate when the next sample should be taken
      target_time = (i + 1) * interval_us + start_time;

      // Calculate the remaining time until the target time
      sleep_time = target_time - time_us_32();

      // If there's still time left until the next sample, sleep for the remaining time
      if (sleep_time > 0) sleep_us(sleep_time);
    }
    
    printf("End of audio capture\n");
}


///KISSFFT////////////////////////////////////////////////////////////////////////////

void fft(uint16_t* input, kiss_fft_cpx* fft_out, uint16_t num_samples, float sampling_rate, float* magnitudes, float* frequencies, kiss_fftr_cfg cfg ) {
    // Convert uint16_t input to float
    float* input_data = new float[num_samples];
    for (size_t i = 0; i < num_samples; i++) {
        input_data[i] = static_cast<float>(input[i]);
        printf("Input Data[%zu]: %f\n", i, input_data[i]);  // Debug print
    }

    // Perform the FFT
    kiss_fftr(cfg, input_data, fft_out);

    // Calculate magnitudes and frequencies for each FFT bin
    for (size_t i = 0; i < num_samples / 2 + 1; i++) {
        // Magnitude is the square root of the sum of squares of the real and imaginary parts
        magnitudes[i] = std::sqrt(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
        
        // Frequency corresponding to this bin
        frequencies[i] = i * sampling_rate / num_samples;
    }

    // Clean up
    delete[] input_data;
    delete[] fft_out;

}
///KISSFFT////////////////////////////////////////////////////////////////////////////


void run() {
  
  stdio_init_all();
  printf("Lets start");
  spi_init_custom();

  uint16_t sampling_rate = 9000;
  float sampling_rate_us = (float)(sampling_rate) / 1000000.0; // in us
  int duration = 1e6; // in us
  uint16_t num_samples = (uint16_t)(duration * sampling_rate_us);
  uint16_t samples[num_samples];
  
  printf("sampling rate: %u Hz\n", sampling_rate); // 9000 
  printf("sampling_rate_us rate: %f\n", sampling_rate_us); //0.009
  printf("duration: %d\n", duration); //10000000
  printf("num_samples =%u\n", num_samples); //0.00000z
  
  //capture input audio samples
  capture_audio(num_samples, sampling_rate, samples);

  kiss_fftr_cfg cfg = kiss_fftr_alloc(num_samples, 0, nullptr, nullptr);  // 0 means forward FFT
  kiss_fft_cpx* fft_out = new kiss_fft_cpx[num_samples / 2 + 1];

  ////////////////////////////FFT/////////////////////////////////////////////
  // Prepare arrays for FFT results
  float* magnitudes = new float[num_samples / 2 + 1];
  float* frequencies = new float[num_samples / 2 + 1];

  // Perform FFT
  fft(samples, fft_out, num_samples, sampling_rate, magnitudes, frequencies, cfg);

  // Printing the magnitudes and corresponding frequencies
  printf("FFT Results:\n");
  for (size_t i = 0; i < num_samples / 2 + 1; i++) {
      printf("Frequency: %.2f Hz, Magnitude: %.2f\n", frequencies[i], magnitudes[i]);
  }
  // Clean up
  delete[] magnitudes;
  delete[] frequencies;
  ////////////////////////////FFT/////////////////////////////////////////////
  printf("Done.\n");
  kiss_fftr_free(cfg);

/* 
for(uint8_t i = 0; i < 50; i++){
  printf("ADC Value: %d\n", samples[i]);
}*/
 
}



int main(){
    stdio_init_all();
    sleep_ms(2000);   // Allow time for USB to initialize
    printf("Starting program...\n");
    spi_init_custom();
    printf("SPI initialized.\n");
    int count = 0;
    while (true) {
         printf("3\n");
         run();
         printf("=====================================\n");
         sleep_ms(5000);
         count++;
         if (count > 5) break;  // Break after 5 iterations for testing
    }
    
    return 0;
}
