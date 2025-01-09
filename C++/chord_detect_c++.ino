#include <UnorderedMap.h>
#include <Vector.h>
#include "arduinoFFT.h"
#include <SPI.h>
#include <Arduino.h>
#include <math.h>
#define onboard 5
#define slaveSelect 10 //none-sec pin which select ADC
#define mic_pin A5
#define SAMPLES 128              //Must be a power of 2
#define SAMPLING_FREQUENCY 1000 //Hz, must be less than 10000 due to ADC

ArduinoFFT<double> FFT = ArduinoFFT<double>();

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

float log2(float x){ 
  return log(x)/log(2);
}
// put function declarations here:
int readADC(int channel) {
  if (channel < 0 || channel > 7) {
    return -1; // Invalid channel
  }

  digitalWrite(slaveSelect, LOW);  // Start communication

  // Send the command to the MCP3008
  byte command = 0b00000001;                      // Start bit
  byte highByte = 0b10000000 | (channel << 4);    // get the channel number, 1 is for serial

  SPI.transfer(command);          // Send start bit
  byte resultHigh = SPI.transfer(highByte);       // Send channel and get MSB (2bits)
  byte resultLow = SPI.transfer(0x00);           // Get LSB

  //Serial.print(resultHigh, BIN);
  //Serial.println(resultLow, BIN);

  digitalWrite(slaveSelect, HIGH);

  // Combine the MSB and LSB
  int result = ((resultHigh & 0x03) << 8) | resultLow;

  return result;
}

void doFFT(Vector<float> &freqs, Vector<float> &fft_mags) {
  //Test the board
  //digitalWrite(onboard, LOW);
  //delay(1000);
  //digitalWrite(onboard, HIGH);
  //delay(1000);
  //Serial.println("done");

  //test the SPI

  int i = 0;
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();    //Overflows after around 70 minutes!
    int channel = 0; // Read from channel 0
    int analogValue = readADC(channel);
    //Serial.print("Analog Value: ");
    //Serial.println(analogValue);
    int mic_out = analogRead(mic_pin);
    //Serial.print("Direct Mic Value: ");
    //Serial.println(mic_out);
    //stor value for fft
    vReal[i] = analogValue;
    vImag[i] = 0.0;
    //wait for next sample
    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
  // Perform FFT on the collected chunk
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);
  double peak = FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  /*PRINT RESULTS*/
  //Serial.print("Freq: ");
  //Serial.println(peak);     //Print out what frequency is the most dominant.
  // Print FFT results for this chunk
  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/
    float frequency = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print("-");
    //Serial.println(vReal[i]);    //View only this line in serial plotter to visualize the bins
    freqs.push_back(frequency);
    fft_mags.push_back(vReal[i]);
  }


  Serial.print("Done");
  //while(1); //stop record for 5sec

  //float voltage = mic_out * (4.9 / 1024.0); // Convert to voltage (assuming 5V reference)
  //Serial.println(voltage);

  //Serial.println(analogValue, BIN);
}
String frequency_2_note(float frequency)
    {
    // Reference: A4 = 440 Hz
    float A4_freq = 440.0;
    int A4_note = 69;  // MIDI number for A4

    // Calculate the closest MIDI number
    if (frequency <= 0.0)
        return "Unknown";
    int midi_number = (int)(round(12 * log2(frequency / A4_freq) + A4_note));

    // Map MIDI number to note names
    String note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint8_t note_index = (midi_number) % 12; // Modulo 12 for the note names
    String note = note_names[note_index];
    return note;
}

UnorderedMap<String, int> freq_detect(Vector<float> freqs, Vector<float> fft_magnitudes, float threshold)
{
  UnorderedMap<String, int> notes;
  notes["C"] = 0;
  notes["C#"] = 0;
  notes["D"] = 0;
  notes["D#"] = 0;
  notes["E"] = 0;
  notes["F"] = 0;
  notes["F#"] = 0;
  notes["G"] = 0;
  notes["G#"] = 0;
  notes["A"] = 0;
  notes["A#"] = 0;
  notes["B"] = 0;
  String note;
  for ( int i = 0; i < freqs.size(); i++) {
    Serial.print(fft_magnitudes[i], freqs[i]);
    if (fft_magnitudes[i] >= threshold) {
      note = frequency_2_note(freqs[i]);
      if (notes[note] == 0) {
        notes[note] = 1;
      }
    }
  }
  return notes;
}
int main() {
  init();
  // put your setup code here, to run once:
  //Test the board
  //pinMode(onboard, OUTPUT);
  //Serial.begin(9600);

  //test the SPI
  Serial.begin(115200);
  pinMode(slaveSelect, OUTPUT);
  digitalWrite(slaveSelect, HIGH);  // Set CS high to deselect the MCP3008

  SPI.begin();                 // Initialize SPI
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Set SPI clock (1 MHz)
  SPI.setDataMode(SPI_MODE0);   // MCP3008 uses SPI Mode 0
  SPI.setBitOrder(MSBFIRST);    // Send data Most Significant Bit first

  Serial.println("Start Record");
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  while (true) {
    Vector<float> freqs;
    Vector<float> fft_mags;
    doFFT(freqs, fft_mags);
    
    UnorderedMap<String, int> chord_detect = freq_detect(freqs, fft_mags, 1000);
    /*
    Serial.print("HIIIII");
    for (auto it = chord_detect.begin(); it != chord_detect.end(); ++it) {
        Serial.print("Key: ");
        Serial.print((*it).key); // Access the key
        Serial.print(", Value: ");
        Serial.println((*it).value); // Access the value
    }
    */
    
    break;
  }
  return 0;
}
