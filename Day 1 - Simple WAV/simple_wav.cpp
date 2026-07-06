#include <fstream>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

// RIFF Chunk
const string chunk_id = "RIFF";
const string chunk_size = "----";
const string format = "WAVE";

// FMT Sub-chunk
const string subchunk1_id = "fmt "; // important to put space because subchunk 1 id is 4 bytes
                                    // forgetting space will shift all the data off
const int subchunk1_size = 16;
const int audio_format = 1;
const int num_channels = 2;
const int sample_rate = 44100;
const int bits_per_sample = 16;
const int byte_rate = sample_rate * num_channels * (bits_per_sample / 8);
const int block_align = num_channels * (bits_per_sample / 8);

// Data Sub-chunk
const string subchunk2_id = "data";
const string subchunk2_size = "----";

const int duration = 2;
const int max_amplitude = 32760; // max amplitude for 16-bit audio
                                 // 2 ^ 16 = 655536, but we need one bit for the sign
                                 // 2 ^ 15 - 1 = 32767, used 32760 as a guess

const double frequency = 440;

void write_as_bytes(ofstream &file, int value, int byte_size) {
    file.write(reinterpret_cast<const char *>(&value), byte_size);
}

int main() {
    ofstream wav;
    wav.open("test.wav", ios::binary);

    if (wav.is_open()) {
        wav << chunk_id;
        wav << chunk_size;
        wav << format;

        wav << subchunk1_id;
        write_as_bytes(wav, subchunk1_size, 4);
        write_as_bytes(wav, audio_format, 2);
        write_as_bytes(wav, num_channels, 2);
        write_as_bytes(wav, sample_rate, 4);
        write_as_bytes(wav, byte_rate, 4);
        write_as_bytes(wav, block_align, 2);
        write_as_bytes(wav, bits_per_sample, 2);
        
        wav << subchunk2_id;
        wav << subchunk2_size;
    }

    int start_audio = wav.tellp();
    
    for (int i = 0; i < sample_rate * duration; i++) { 
        // Respect max amplitude
        // operate as a wave.
        double amplitude = (double) i / sample_rate * max_amplitude / duration;
        double value = sin(2 * M_PI * i * frequency / sample_rate);

        double channel1 = amplitude * value;
        double channel2 = (max_amplitude - amplitude) * value;
        
        write_as_bytes(wav, (int) channel1, 2);
        write_as_bytes(wav, (int) channel2, 2);
    }
    

    int end_audio = wav.tellp();

    wav.seekp(start_audio - 4);
    write_as_bytes(wav, end_audio - start_audio, 4);

    wav.seekp(4, ios::beg);
    write_as_bytes(wav, end_audio - 8, 4); // chunk size = file size - 8 bytes

wav.close();

return 0;

}