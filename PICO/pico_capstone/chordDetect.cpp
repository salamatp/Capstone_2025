//#define PICO_ON_LINUX

#include <iostream>
#include <cmath>  // For sqrt and other math functions

// #ifdef PICO_ON_LINUX
// // #include <unistd.h>  // For sleep in Linux
// #include <fstream>  // File stream library
// #else
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
//#endif

#include "kiss_fftr.h"

/////////////////////////////////////
// Hardware Configuration
/////////////////////////////////////
#define SPI_PORT spi0
#define SPI_CLK 1 * 1000 * 1000  // 1 MHz SPI clock
#define SPI_MISO 4
#define SPI_MOSI 3
#define SPI_SCK 2
#define SPI_CS 5

//#define N 1024  // Size of the FFT
//#define SAMPLE_RATE 44100  // Sampling rate, for example

// this function normalized the 10 bit ADC value between -1 to 1:
float Normalizer(uint16_t sample) {
    const float ADC_MAX = 1023.0f;  // Max ADC value
    float normalized = (2.0f * static_cast<float>(sample) / ADC_MAX) - 1.0f;
    // std::cout <<"LETS PRINT THE NORMALIZED INSIDE NORAMALIZER    " << normalized << '\n';
    return normalized;
}


// #ifdef PICO_ON_LINUX
// // Stub functions for Linux (No real SPI or GPIO)
// void spi_init_custom() {
//     std::cout << "SPI initialization (Linux Stub)\n";
// }
// ///////////////////////////////////////////////////////
// // uint16_t read_adc(uint8_t channel) {
// //     return rand() % 1024;  // Simulate ADC values
// // }
// #include <cmath>
// #include <cstdint>

// #define TONE_FREQUENCY  987.7666f  // Frequency of the sine wave in Hz
// #define TONE_FREQUENCY2  440.0f  // Frequency of the sine wave in Hz
// #define SAMPLE_RATE     4500.0f // Sampling rate in Hz
// #define ADC_MAX_VALUE   1024    // 10-bit ADC resolution




// uint16_t read_adc(uint8_t channel) {
//     static float t = 0.0f; // Time tracker in seconds

//     // Calculate the sine wave value at time t
//     float sample = sin(2 * M_PI * TONE_FREQUENCY * t);
//     sample = sample + sin(2 * M_PI * TONE_FREQUENCY2 * t);

//     // Normalize sine wave from [-1, 1] to [0, 1]
//     sample = (sample + 1.0f) / 2.0f;
//     std::cout <<"SAMPLEEEE    " << sample << "\n";

//     // Scale to ADC range [0, ADC_MAX_VALUE]
//     uint16_t adc_value = static_cast<uint16_t>(sample * ADC_MAX_VALUE);
//     std::cout <<"ADC VALUEEEEE    " << adc_value << "\n";

//     // Advance time by one sample interval
//     t += 1.0f / SAMPLE_RATE;
//     return adc_value;
// }
// ///////////////////////////////////////////


// void sleep_ms(int ms) { sleep_ms(ms); }
// void sleep_us(int us) { sleep_us(us); }
// #else
    // Actual SPI Initialization for Pico
    void spi_init_custom() {
        printf("Starting SPI initialization\n");
        spi_init(SPI_PORT, SPI_CLK);
        gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
        gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
        gpio_init(SPI_CS);
        gpio_set_dir(SPI_CS, GPIO_OUT);
        gpio_put(SPI_CS, 1);  // Deselect the chip
        printf("SPI Initialized\n");
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
// void capture_audio_test(uint16_t num_samples, uint16_t sampling_rate) {
//     spi_init_custom();
//     uint32_t start_time = 0;

//     start_time = time_us_32();

//     uint32_t interval_us = 1000000 / sampling_rate;
//     for (uint16_t i = 0; i < num_samples; i++) {
//         std::cout << read_adc(0) << "\n";  // Read from ADC (real or simulated)
//         uint32_t target_time = start_time + (i + 1) * interval_us;
//         while (time_us_32() < target_time) {}  // Busy-wait to maintain timing
//     }
// }

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
  printf("Actual Sampling Rate: %.2f Hz\n", actual_sampling_rate);
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
    printf("MIDI NUMBER: %d\n", (int)midi_number);
    // Map MIDI number to note names
    uint8_t note_index =  (midi_number) % 12;  // Modulo 12 for the note names
    printf("NOTE INDEX: %d\n", (int)note_index);
    return note_index;
}


uint16_t freq_detect(float* freqs,float* fft_magnitudes, uint16_t threshold, uint16_t size){
    // {'C' : 0, 'C#' : 0, 'D' : 0, 'D#' : 0, 'E' : 0, 'F' : 0, 'F#' : 0, 'G' : 0, 'G#' : 0, 'A' : 0, 'A#' : 0, 'B' : 0}
    uint16_t notes = 0;
    // std::cout << "SIZE OF FREQS " << size << "\n";
    uint8_t note = 0;
    for (int i = 0; i < size; i++){
        // std::cout << "Index i = " << i << " \n";
        // std::cout << "magnitude[ "<< i << " ] is " << fft_magnitudes[i] << " \n";
        // std::cout << "FFT FREQ[ " << i << " ] is " <<  freqs[i] << "\n";
        // std::cout << (fft_magnitudes[i] >= threshold) << "\n";
        if(fft_magnitudes[i] >= threshold){
            // std::cout << "FFT FREQ: : " << freqs[i] <<"\n";
            // std::cout << "FFT MAGNITUDE at : "<< i << "   is   " << fft_magnitudes[i] <<"\n";
            note = frequency_2_note(freqs[i]);
            switch(note) {
                case 0:
                  // C
                    notes |= 1 << 0;
                    printf("C");
                  break;
                case 1:
                  // C#
                    notes |= 1 << 1;
                    printf("C#");
                  break;
                case 2:
                  // 'D'
                    notes |= 1 << 2;
                    printf("D");
                  break;
                case 3:
                  // 'D#'
                    notes |= 1 << 3;
                    printf("D#");
                  break;
                case 4:
                  // 'E'
                    notes |= 1 << 4;
                    printf("E");
                  break;
                case 5:
                  // 'F'
                    notes |= 1 << 5;
                    printf("F");
                  break;
                case 6:
                  // 'F#'
                    notes |= 1 << 6;
                    printf("F#");
                  break;
                case 7:
                  // 'G'
                    notes |= 1 << 7;
                    printf("G");
                  break;
                case 8:
                  // 'G#'
                    notes |= 1 << 8;
                    printf("G#");
                  break;
                case 9:
                  //A
                    notes |= 1 << 9;
                    printf("A");
                    //std::cout << "NOTES: " << notes << "\n";
                  break;
                case 10:
                  //A#
                    notes |= 1 << 10;
                    printf("A#");
                  break;
                case 11:
                  //B
                    notes |= 1 << 11;
                    printf("B");
                  break;
              }
        }
    }
    return notes;

}

void run() {
    printf("Initializing...\n");
    spi_init_custom();
    
    uint16_t sampling_rate = 4500;
    uint32_t duration = 333000; // 0.04s second in microseconds (40 ms for 9000 Hz) same as block size
    //uint32_t duration = 33300000; // 0.04s second in microseconds (40 ms for 9000 Hz) same as block size
    uint32_t num_samples = duration / (1000000 / sampling_rate); // 360 samples -> resolution = sampling rate (in Hz)/FFT size = 25 Hz
    
    kiss_fftr_cfg cfg = kiss_fftr_alloc(num_samples, 0, nullptr, nullptr);
    kiss_fft_cpx* fft_out = new kiss_fft_cpx[num_samples / 2 + 1];
    float* magnitudes = new float[num_samples / 2 + 1];
    float* frequencies = new float[num_samples / 2 + 1];

    float* samples = new float[num_samples];
    while (true){
      printf("===================================\n");
        // std::cout << "num_samples= " << num_samples <<"sampling rate= "<< sampling_rate <<"\n";
      capture_audio_test(num_samples, sampling_rate, samples);
      fft(samples, fft_out, num_samples, sampling_rate, magnitudes, frequencies, cfg);
      uint16_t note_index = freq_detect(frequencies, magnitudes, 40, num_samples / 2);
      for (int i = 0; i < num_samples; i++){
          printf("frequency: %f, magnitude: %f \n ", frequencies[i], magnitudes[i]);
      }

      printf("Notes detected: %d\n", note_index);
    }
      
   
    // delete[] samples;
    // delete[] fft_out;
    // delete[] magnitudes;
    // delete[] frequencies;

    printf("Done.\n");
    kiss_fftr_free(cfg);
    
    // #ifdef PICO_ON_LINUX
    // file_freq.close();
    // file_mag.close();
    // #endif
}

int main() {

    stdio_init_all();
    sleep_ms(2000);
    
    run();

    return 0;
}