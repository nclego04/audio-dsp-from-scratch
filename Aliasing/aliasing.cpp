#define _USE_MATH_DEFINES
#include <chrono>
#include <cmath>
#include <cstdint>
#include <complex>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

struct WavData {
    int sample_rate = 0;
    int num_channels = 0;
    int bits_per_sample = 0;
    std::vector<double> samples; // one channel only
};

WavData read_wav(const std::string& filename) {
    WavData wav;
    std::ifstream f(filename, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "failed to open " << filename << "\n";
        return wav;
    }

    char id[4];
    f.read(id, 4); // "RIFF"
    f.seekg(4, std::ios::cur); // skip overall chunk size, unused
    f.read(id, 4); // "WAVE"

    while (f) {
        char chunk_id[4];
        uint32_t chunk_size;
        f.read(chunk_id, 4);
        f.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (!f) break;

        if (std::string(chunk_id, 4) == "fmt ") {
            int16_t audio_format, num_channels, block_align, bits_per_sample;
            int32_t sample_rate, byte_rate;

            f.read(reinterpret_cast<char*>(&audio_format), 2);
            f.read(reinterpret_cast<char*>(&num_channels), 2);
            f.read(reinterpret_cast<char*>(&sample_rate), 4);
            f.read(reinterpret_cast<char*>(&byte_rate), 4);
            f.read(reinterpret_cast<char*>(&block_align), 2);
            f.read(reinterpret_cast<char*>(&bits_per_sample), 2);

            wav.num_channels = num_channels;
            wav.sample_rate = sample_rate;
            wav.bits_per_sample = bits_per_sample;

            if (chunk_size > 16) {
                f.seekg(chunk_size - 16, std::ios::cur); // skip any extra fmt bytes
            }
        } else if (std::string(chunk_id, 4) == "data") {
            int bytes_per_sample = wav.bits_per_sample / 8;
            int frame_size = bytes_per_sample * wav.num_channels;
            int num_frames = chunk_size / frame_size;

            wav.samples.resize(num_frames);
            for (int i = 0; i < num_frames; i++) {
                int16_t sample;
                f.read(reinterpret_cast<char*>(&sample), 2);
                wav.samples[i] = (double)sample;
                if (wav.num_channels > 1) {
                    f.seekg(bytes_per_sample * (wav.num_channels - 1), std::ios::cur);
                }
            }
            break; // data is the last chunk we care about
        } else {
            f.seekg(chunk_size, std::ios::cur); // skip unknown chunk
        }
    }

    return wav;
}

std::vector<std::complex<double>> dft(const std::vector<double>& x) {
    int N = (int)x.size();
    std::vector<std::complex<double>> X(N,0);
    for (int k = 0; k < N; k++) {
        std::complex<double> sum = 0;
        for (int n = 0; n < N; n++) {
            double angle = -2 * M_PI * k * n /  N;
            std::complex<double> W(cos(angle), sin(angle));
            sum += W * x[n];
        }
        X[k] = sum;
    }
    return X;
}

std::vector<double> magnitude(const std::vector<std::complex<double>>& X) {
    std::vector<double> mag(X.size());
    for (size_t k = 0; k < X.size(); k++) {
        mag[k] = std::abs(X[k]);
    }
    return mag;
}

double bin_to_hz(int k, double fs, int N) {
    double f = k * fs / N;
    return f;
}

int main()
{
    
    /*
    have sawtooth sweep WAV
    run through read_wav, returns WavData structure
    take double vector samples from WavData structure, represents sample data
    extract samples 214,327 - 214,527 into another vector, this represents the 
    window we want to look at
    run window vector through dft, which returns a complex double vector, this
    represents the DFT
    run DFT vector through magnitude, returns complex double vector, represents
    bins
    search through bins vector to find peak bin
    convert bin to Hz
    compare with expected results
    */
    double fs = 44100;
    int N = 200;

    WavData data = read_wav("sawtooth_sweep.wav");

    std::vector<double> samples = data.samples;

    std::vector<double> window(samples.begin() + 214327, samples.begin() + 214527);

    std::vector<std::complex<double>> dFT = dft(window);

    std::vector<double> bins = magnitude(dFT);

    double largest = 0;
    int largest_index = 0;
    double largest_frequency = 0;
    for (int i = 45; i <= 55; i++)
    {
        if (bins[i] > largest)
        {
            largest = bins[i];
            largest_index = i;
            largest_frequency = bin_to_hz(i, fs, N);
        }
    }

    std::cout << "bin\t\tfrequency\tmagnitude \n";
    for (int i = 45; i <= 55; i++)
    {
        std::cout << i << "\t\t" << bin_to_hz(i, fs, N) << "\t\t" << bins[i] << "\n";
    }

    double f_2nd_harmonic = 33075;
    double predicted_hz = fs - f_2nd_harmonic;
    double measured_hz = largest_frequency;

    std::cout << "Predicted Hz is " << predicted_hz << ". ";
    std::cout << "Measured Hz is " << measured_hz << ". \n";

    double difference = std::abs(measured_hz - predicted_hz);

    if (difference < fs/N)
    {
        std::cout << "PASS\n";
    }
    else
    {
        std::cout << "FAIL\n";
    }
}