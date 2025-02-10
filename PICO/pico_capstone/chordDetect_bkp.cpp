#include <iostream>
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
    printf("Starting SPI initialization...\n");
    spi_init(SPI_PORT, SPI_CLK);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1); // Deselect the chip
    printf("SPI Initialized!\n");
}

uint16_t read_adc(uint8_t channel) {
    if (channel > 7) return 0; // MCP3008 supports only 8 channels (0-7)

    uint8_t tx_data[3] = {0x01, (uint8_t)(0x80 | (channel << 4)), 0x00};  
    uint8_t rx_data[3] = {0};

    gpio_put(SPI_CS, 0); // Select the chip
    spi_write_read_blocking(SPI_PORT, tx_data, rx_data, 3);
    gpio_put(SPI_CS, 1); // Deselect the chip

    uint16_t result = ((rx_data[1] & 0x03) << 8) | rx_data[2];
    printf("ADC Value: %d\n", result);
    return result;
}

void capture_audio(uint32_t num_samples, uint16_t sampling_rate, uint16_t* samples) {
    printf("Starting audio capture...\n");
    
    uint32_t start_time = time_us_32();
    uint32_t interval_us = 1000000 / sampling_rate;

    for (uint32_t i = 0; i < num_samples; i++) {
        samples[i] = read_adc(0);
        printf("Sample %d: %d\n", i, samples[i]);

        uint32_t target_time = start_time + (i + 1) * interval_us;
        int32_t sleep_time = target_time - time_us_32();
        if (sleep_time > 0) sleep_us(sleep_time);
    }
    
    printf("End of audio capture.\n");
}

void fft(uint16_t* input, kiss_fft_cpx* fft_out, uint32_t num_samples, float sampling_rate, float* magnitudes, float* frequencies, kiss_fftr_cfg cfg) {
    float* input_data = new float[num_samples];
    for (uint32_t i = 0; i < num_samples; i++) {
        input_data[i] = static_cast<float>(input[i]);
    }

    kiss_fftr(cfg, input_data, fft_out);

    for (uint32_t i = 0; i < num_samples / 2 + 1; i++) {
        magnitudes[i] = std::sqrt(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
        frequencies[i] = i * sampling_rate / num_samples;
    }

    delete[] input_data;
}

void run() {
    printf("Initializing...\n");
    spi_init_custom();

    uint16_t sampling_rate = 9000;
    uint32_t duration = 1000000; // 1 second in microseconds
    uint32_t num_samples = duration / (1000000 / sampling_rate);

    printf("Sampling Rate: %u Hz\n", sampling_rate);
    printf("Duration: %u us\n", duration);
    printf("Number of Samples: %u\n", num_samples);

    uint16_t* samples = new uint16_t[num_samples];

    capture_audio(num_samples, sampling_rate, samples);

    kiss_fftr_cfg cfg = kiss_fftr_alloc(num_samples, 0, nullptr, nullptr);
    kiss_fft_cpx* fft_out = new kiss_fft_cpx[num_samples / 2 + 1];

    float* magnitudes = new float[num_samples / 2 + 1];
    float* frequencies = new float[num_samples / 2 + 1];

    fft(samples, fft_out, num_samples, sampling_rate, magnitudes, frequencies, cfg);

    printf("FFT Results:\n");
    for (uint32_t i = 0; i < num_samples / 2 + 1; i++) {
        printf("Frequency: %.2f Hz, Magnitude: %.2f\n", frequencies[i], magnitudes[i]);
    }

    delete[] samples;
    delete[] fft_out;
    delete[] magnitudes;
    delete[] frequencies;

    printf("Done.\n");
    kiss_fftr_free(cfg);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Allow time for USB serial to initialize
    printf("Starting program...\n");

    int count = 0;
    while (true) {
        printf("Running iteration %d...\n", count);
        run();
        printf("=====================================\n");
        sleep_ms(5000);
        count++;
        if (count > 5) break;
    }

    return 0;
}
