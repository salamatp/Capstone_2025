import spidev
import time
import numpy as np
import matplotlib.pyplot as plt
from librosa import load
from scipy.io.wavfile import write


# Initialize SPI
spi = spidev.SpiDev()
spi.open(0, 0)  # Open SPI bus 0, device 0
spi.max_speed_hz = 2_500_000  # Adjusted for proper samplizng

# Read from MCP3008
def read_adc(channel):
    if channel < 0 or channel > 7:
        raise ValueError("Channel must be between 0 and 7")
    
    cmd = [1, (8 + channel) << 4, 0]
    adc_response = spi.xfer2(cmd)
    result = ((adc_response[1] & 3) << 8) + adc_response[2]
    return result


def capture_audio(duration, sampling_rate):
    num_samples = int(duration * sampling_rate)
    samples = np.zeros(num_samples)
    start_time = time.perf_counter()  # Use high precision timer

    # Target time for each sample based on the sampling rate
    interval = 1.0 / sampling_rate

    for i in range(num_samples):
        # Record the current time before taking a sample
        current_time = time.perf_counter()

        # Read from the ADC (this step will take some time, but we account for it below)
        samples[i] = read_adc(0)  # Read from channel 0 (ADC)

        # Calculate the elapsed time since the start of the capture
        elapsed_time = time.perf_counter() - start_time

        # Calculate when the next sample should be taken
        target_time = (i + 1) * interval + start_time

        # Calculate the remaining time until the target time
        sleep_time = target_time - time.perf_counter()

        # If there's still time left until the next sample, sleep for the remaining time
        if sleep_time > 0:
            time.sleep(sleep_time)

    # Calculate the actual sampling rate after capture
    actual_sampling_rate = num_samples / (time.perf_counter() - start_time)
    print(f"Expected Sampling Rate: {sampling_rate} Hz")
    print(f"Actual Sampling Rate: {actual_sampling_rate:.2f} Hz")

    return samples


#testing
def capture_audio_new(duration, sampling_rate):
    num_samples = int(duration * sampling_rate)
    samples = np.zeros(num_samples)
    start_time = time.time()

    for i in range(num_samples):
        samples[i] = read_adc(0)  # Read from ADC
        elapsed_time = time.time() - start_time
        next_sample_time = (i + 1) / sampling_rate
        sleep_time = next_sample_time - elapsed_time
        if sleep_time > 0:
            time.sleep(sleep_time)

    actual_sampling_rate = num_samples / (time.time() - start_time)
    print(f"Expected Sampling Rate: {sampling_rate} Hz")
    print(f"Actual Sampling Rate: {actual_sampling_rate:.2f} Hz")
    return samples

# Capture audio data
#Issues: time.sleep(interval) is fixed, which doesnt consider the fact that it takes time for ADC to read the data and process the sample. This causes drift in timing for sample collection
#Because of that the samplig rate might be higher that expected. 
def capture_audio_old(duration, sampling_rate):
    samples = []
    start_time = time.time()
    interval = 1.0 / sampling_rate
    num_samples = int(duration * sampling_rate)
    

    while time.time() - start_time < duration:
        sample = read_adc(0)  # Read from channel 0
        samples.append(sample)
        time.sleep(interval)

    actual_sampling_rate = num_samples / (time.time() - start_time)
    print(f"Actual Sampling Rate: {actual_sampling_rate} Hz")

    return np.array(samples)


def write_audio(audio, sampling_rate):
    #127.5 is chosen to avoide the values exceeding the range due to rounding (255)
    scaled = np.int16((audio / np.max(np.abs(audio))) * 32767)
    # Writing scaled data into wav file
    write('mic_out.wav', sampling_rate, scaled)

def write_audio_txt(audio):    
    scaled = np.int16((audio / np.max(np.abs(audio))) * 32767)
    np.savetxt('mic_out.txt', scaled)

# Perform FFT on a block 
def fft_block(audio_data, sampling_rate):
    # Normalize the data
    audio_data = (audio_data - 512) / 512.0

    # Perform FFT
    n = len(audio_data)
    fft_result = np.fft.fft(audio_data)
    fft_magnitudes = np.abs(fft_result[:n // 2])  # Magnitudes for positive frequencies
    freqs = np.fft.fftfreq(n, 1 / sampling_rate)[:n // 2]  # Positive frequencies


    return freqs,fft_magnitudes

# this function will scan through the frequency block and output all the chords present in it
def freq_detect(freqs, fft_magnitudes, threshold):

    notes = {'C' : 0, 'C#' : 0, 'D' : 0, 'D#' : 0, 'E' : 0, 'F' : 0, 'F#' : 0, 'G' : 0, 'G#' : 0, 'A' : 0, 'A#' : 0, 'B' : 0}
    for i in range(1,len(freqs)):
        print(fft_magnitudes[i] , freqs[i])
        if(fft_magnitudes[i] >= threshold):
            note = frequency_2_note(freqs[i])
            if(notes[note]) == 0:
                notes[note] = 1 
    
    return notes


#the frequency is in Hz    
# Map frequency to musical note
def frequency_2_note(frequency):
    # Reference: A4 = 440 Hz
    A4_freq = 440.0
    A4_note = 69  # MIDI number for A4

    # Calculate the closest MIDI number
    if frequency <= 0:
        return "Unknown"
    midi_number = int(round(12 * np.log2(frequency / A4_freq) + A4_note))

    # Map MIDI number to note names
    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    note_index = (midi_number) % 12  # Modulo 12 for the note names
    note = note_names[note_index]
    return note


# Process audio (example: plot waveform)
def plot_audio(audio_data, sampling_rate):
    # Normalize data (0-1023 to -1 to 1 range)
    # 0-5V -> 0-1023 -> (-1)-1 , as we discussed this should help with plotting
    # Normilization helps with FFT calculation by centering the signal on 0 it removes the DC component
    audio_data = (audio_data - 512) / 512.0

    # Plot the waveform
    time_axis = np.linspace(0, len(audio_data) / sampling_rate, len(audio_data))
    plt.plot(time_axis, audio_data)
    plt.title("Audio Waveform")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")
    plt.show()


# Process audio frequency (example: plot waveform)
def plot_audio_freq(audio_data, sampling_rate):
    
    n = len(audio_data)
    fft_result = np.fft.fft(audio_data)
    fft_magnitudes = np.abs(fft_result[:n // 2])  # Magnitudes for positive frequencies
    freqs = np.fft.fftfreq(n, 1 / sampling_rate)[:n // 2]  # Positive frequencies

    # Plot the waveform
    freq_axis = np.linspace(0, freqs[-1], len(freqs))
    plt.plot(freq_axis, fft_magnitudes)
    plt.title("Audio Frequency Waveform")
    plt.xlabel("Freq (Hz)")
    plt.ylabel("Amplitude")
    plt.show()


audio_file = "c7.wav"
def librosa(audio_file, sampling_rate):

    audio = load(audio_file, sr = sampling_rate)

    return audio[0]




# Main function for real-time note detection
if __name__ == "__main__":
    print("Starting real-time note detection...")
    sampling_rate = 8375
    sampling_rate = 18000
    sampling_rate = 44100
    #sampling_rate = 4122
    duration = 15  # Analyze in 1-second chunks
    #audio_data = capture_audio(duration, sampling_rate)

    #write_audio(audio_data,sampling_rate)
    #write_audio_txt(audio_data)

    with open("ADC_data.txt", "r") as fh:
     adc_data = fh.read().splitlines()
    data_array = np.array(adc_data, dtype= float)
    
    plot_audio_freq(data_array, 6000)
    # below is the code to detect notes from audio
    # data = librosa(audio_file, sampling_rate)
    # f1, m1  = fft_block(data, sampling_rate)

    # print(freq_detect(f1, m1, 0.6))

   
    #plot_audio(data, sampling_rate)
    #plot_audio_freq(data, sampling_rate)
    #print(frequency_2_note(4065))
    # try:
    #     while True:
    #         # Capture audio
    #         #audio_data = capture_audio(duration, sampling_rate)

            
    #         #plot_audio_freq(audio_data,sampling_rate)
    #         #plot_audio(audio_data,sampling_rate)
    #         # Detect frequency

    #         #dominant_frequency = detect_frequency(audio_data, sampling_rate)
    #         #if dominant_frequency > 0:
    #         #    note = frequency_to_note(dominant_frequency)
    #          #   print(f"Detected frequency: {dominant_frequency:.2f} Hz, Note: {note}")
    #         #else:
    #          #   print("No dominant frequency detected")

    # except KeyboardInterrupt:
    #     print("\nExiting...")
