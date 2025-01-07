# include <iostream>
# include <vector>
# include <unordered_map>
# include <cstdint>
# include <cmath>

// this function will scan through the frequency block and output all the chords present in it
std::unordered_map<std::string, int> freq_detect(std::vector<float> freqs, std::vector<float> fft_magnitudes, float threshold)
{    
    std::unordered_map<std::string, int> notes = {{"C" , 0} , {"C#", 0}, {"D", 0}, {"D#", 0} , {"E", 0}, {"F", 0}, {"F#", 0}, {"G", 0}, {"G#", 0 }, {"A", 0}, {"A#", 0}, {"B", 0}};
    std::string note;
    for( int i = 0; i < freqs.size(); i++){
        std::cout<< fft_magnitudes[i] , freqs[i];
        if(fft_magnitudes[i] >= threshold){
            note = frequency_2_note(freqs[i]);
            if(notes[note] == 0){
                notes[note] = 1;
            }
        }
    }
    return notes;
}


// the frequency is in Hz    
// Map frequency to musical note
std::string frequency_2_note(float frequency)
    {
    // Reference: A4 = 440 Hz
    float A4_freq = 440.0;
    int A4_note = 69;  // MIDI number for A4

    // Calculate the closest MIDI number
    if (frequency <= 0.0)
        return "Unknown";
    int midi_number = (int)(std::round(12 * std::log2(frequency / A4_freq) + A4_note));

    // Map MIDI number to note names
    std::string note_names[12] = {"C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B"};
    uint8_t note_index = (midi_number) % 12; // Modulo 12 for the note names
    std::string note = note_names[note_index];
    return note;
}