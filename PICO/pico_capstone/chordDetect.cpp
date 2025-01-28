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
  printf("Lets start");
  stdio_init_all();
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
    spi_init_custom();
    stdio_init_all();
    
    while (true) {
         run();
         printf("=====================================\n");
         sleep_ms(5000);
    }
    
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ArduinoFFT<double> FFT = ArduinoFFT<double>();

// unsigned int sampling_period_us;
// unsigned long microseconds;

// double vReal[SAMPLES];
// double vImag[SAMPLES];

// float log2(float x){ 
//   return log(x)/log(2);
// }
// put function declarations here:
// int readADC(int channel) {
//   if (channel < 0 || channel > 7) {
//     return -1; // Invalid channel
//   }

//   digitalWrite(slaveSelect, LOW);  // Start communication

//   // Send the command to the MCP3008
//   byte command = 0b00000001;                      // Start bit
//   byte highByte = 0b10000000 | (channel << 4);    // get the channel number, 1 is for serial

//   SPI.transfer(command);          // Send start bit
//   byte resultHigh = SPI.transfer(highByte);       // Send channel and get MSB (2bits)
//   byte resultLow = SPI.transfer(0x00);           // Get LSB

//   //Serial.print(resultHigh, BIN);
//   //Serial.println(resultLow, BIN);

//   digitalWrite(slaveSelect, HIGH);

//   // Combine the MSB and LSB
//   int result = ((resultHigh & 0x03) << 8) | resultLow;

//   return result;
// }

// void doFFT(Vector<float> &freqs, Vector<float> &fft_mags) {
//   //Test the board
//   //digitalWrite(onboard, LOW);
//   //delay(1000);
//   //digitalWrite(onboard, HIGH);
//   //delay(1000);
//   //Serial.println("done");

//   //test the SPI

//   int i = 0;
//   for (int i = 0; i < SAMPLES; i++) {
//     microseconds = micros();    //Overflows after around 70 minutes!
//     int channel = 0; // Read from channel 0
//     int analogValue = readADC(channel);
//     //Serial.print("Analog Value: ");
//     //Serial.println(analogValue);
//     int mic_out = analogRead(mic_pin);
//     //Serial.print("Direct Mic Value: ");
//     //Serial.println(mic_out);
//     //stor value for fft
//     vReal[i] = analogValue;
//     vImag[i] = 0.0;
//     //wait for next sample
//     while (micros() < (microseconds + sampling_period_us)) {
//     }
//   }
//   // Perform FFT on the collected chunk
//   FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
//   FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
//   FFT.complexToMagnitude(vReal, vImag, SAMPLES);
//   double peak = FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

//   /*PRINT RESULTS*/
//   //Serial.print("Freq: ");
//   //Serial.println(peak);     //Print out what frequency is the most dominant.
//   // Print FFT results for this chunk
//   for (int i = 0; i < (SAMPLES / 2); i++)
//   {
//     /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/
//     float frequency = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
//     //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
//     //Serial.print("-");
//     //Serial.println(vReal[i]);    //View only this line in serial plotter to visualize the bins
//     freqs.push_back(frequency);
//     fft_mags.push_back(vReal[i]);
//   }


//   Serial.print("Done");
//   //while(1); //stop record for 5sec

//   //float voltage = mic_out * (4.9 / 1024.0); // Convert to voltage (assuming 5V reference)
//   //Serial.println(voltage);

//   //Serial.println(analogValue, BIN);
// }
// String frequency_2_note(float frequency)
//     {
//     // Reference: A4 = 440 Hz
//     float A4_freq = 440.0;
//     int A4_note = 69;  // MIDI number for A4

//     // Calculate the closest MIDI number
//     if (frequency <= 0.0)
//         return "Unknown";
//     int midi_number = (int)(round(12 * log2(frequency / A4_freq) + A4_note));

//     // Map MIDI number to note names
//     String note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
//     uint8_t note_index = (midi_number) % 12; // Modulo 12 for the note names
//     String note = note_names[note_index];
//     return note;
// }

// UnorderedMap<String, int> freq_detect(Vector<float> freqs, Vector<float> fft_magnitudes, float threshold)
// {
//   UnorderedMap<String, int> notes;
//   notes["C"] = 0;
//   notes["C#"] = 0;
//   notes["D"] = 0;
//   notes["D#"] = 0;
//   notes["E"] = 0;
//   notes["F"] = 0;
//   notes["F#"] = 0;
//   notes["G"] = 0;
//   notes["G#"] = 0;
//   notes["A"] = 0;
//   notes["A#"] = 0;
//   notes["B"] = 0;
//   String note;
//   for ( int i = 0; i < freqs.size(); i++) {
//     Serial.print(fft_magnitudes[i], freqs[i]);
//     if (fft_magnitudes[i] >= threshold) {
//       note = frequency_2_note(freqs[i]);
//       if (notes[note] == 0) {
//         notes[note] = 1;
//       }
//     }
//   }
//   return notes;
// }
// int main() {
//   init();
//   // put your setup code here, to run once:
//   //Test the board
//   //pinMode(onboard, OUTPUT);
//   //Serial.begin(9600);

//   //test the SPI
//   Serial.begin(115200);
//   pinMode(slaveSelect, OUTPUT);
//   digitalWrite(slaveSelect, HIGH);  // Set CS high to deselect the MCP3008

//   SPI.begin();                 // Initialize SPI
//   SPI.setClockDivider(SPI_CLOCK_DIV16); // Set SPI clock (1 MHz)
//   SPI.setDataMode(SPI_MODE0);   // MCP3008 uses SPI Mode 0
//   SPI.setBitOrder(MSBFIRST);    // Send data Most Significant Bit first

//   Serial.println("Start Record");
//   sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
//   while (true) {
//     Vector<float> freqs;
//     Vector<float> fft_mags;
//     doFFT(freqs, fft_mags);
    
//     UnorderedMap<String, int> chord_detect = freq_detect(freqs, fft_mags, 1000);
//     /*
//     Serial.print("HIIIII");
//     for (auto it = chord_detect.begin(); it != chord_detect.end(); ++it) {
//         Serial.print("Key: ");
//         Serial.print((*it).key); // Access the key
//         Serial.print(", Value: ");
//         Serial.println((*it).value); // Access the value
//     }
//     */
    
//     break;
//   }
//   return 0;
// }


// #define onboard 5
// #define slaveSelect 10 //none-sec pin which select ADC
// #define mic_pin A5
// #define SAMPLES 128              //Must be a power of 2
// #define SAMPLING_FREQUENCY 1000 //Hz, must be less than 10000 due to ADC



/////  GLOBAL VARIABLES /////////
