#include <fstream>
#include <iostream>
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

const int max_amplitude = 32760; // max amplitude for 16-bit audio
                                 // 2 ^ 16 = 655536, but we need one bit for the sign
                                 // 2 ^ 15 - 1 = 32767, used 32760 as a guess

void write_as_bytes(ofstream &file, int value, int byte_size) {
    file.write(reinterpret_cast<const char *>(&value), byte_size);
}

double generate_waveform(string type, double frequency, double& phase) {
    double sample = 0;
    double increment = frequency / sample_rate;

    phase += increment;

    if (phase >= 1) {
        phase -= floor(phase);
    }

    if (type == "sine")
    {
        sample = sin(2 * M_PI * phase) * max_amplitude;
    }

    else if (type == "sawtooth") {
        sample = (2 * phase - 1) * max_amplitude;
    }

    else if (type == "square") {
        sample = (phase < 0.5 ? 1 : -1) * max_amplitude;
    }

    return sample;
}

double generate_sweep(string type, double start_frequency, double end_frequency, 
                    double i, int N, double &phase) {
    double frequency = start_frequency * pow(end_frequency / start_frequency, i / N);
    double sample = generate_waveform(type, frequency, phase);
    return sample;
}

void render_file(string filename, string type, double start_frequency, 
    double duration, bool sweep = false, double end_frequency = 0) {
    
    if (start_frequency <= 0) {
        cerr << "start_frequency must be > 0, got " << start_frequency << "\n";
        return;
    }

    if (sweep && end_frequency <= 0) {
        cerr << "sweep end_frequency must be > 0, got " << end_frequency << "\n";
        return;
    }
    
    ofstream wav;
    wav.open(filename, ios::binary);

    if (!wav.is_open()) {
        cerr << "failed to open " << filename << "\n";
        return;
    }

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

    int start_audio = wav.tellp();

    double phase = 0;
    int N = sample_rate * duration;

    for (int i = 0; i < N; i++) {
        double sample = 0;
        if (sweep == true) {
            sample = generate_sweep(type, start_frequency, end_frequency, i, N, phase);
        }
        else {
            sample = generate_waveform(type, start_frequency, phase);
        }

        double channel1 = sample;
        double channel2 = sample;

        write_as_bytes(wav, (int) channel1, 2);
        write_as_bytes(wav, (int) channel2, 2);
    }

    int end_audio = wav.tellp();

    wav.seekp(start_audio - 4);
    write_as_bytes(wav, end_audio - start_audio, 4);

    wav.seekp(4, ios::beg);
    write_as_bytes(wav, end_audio - 8, 4); // chunk size = file size - 8 bytes

    wav.close();
}

int main() {
    render_file("sawtooth_sweep.wav", "sawtooth", 20, 5, true, 20000);
    render_file("square_sweep.wav", "square", 20, 5, true, 20000);
    render_file("sine_sweep.wav", "sine", 20, 5, true, 20000);
    render_file("sine.wav", "sine", 220, 5);
    render_file("sawtooth.wav", "sawtooth", 220, 5);
    render_file("square.wav", "square", 220, 5);
return 0;

}
