#define _USE_MATH_DEFINES
#include <chrono>
#include <cmath>
#include <cstdint>
#include <complex>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

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

std::vector<double> generate_sine(int N, double fs, double freq, double amplitude = 1.0) {
    std::vector<double> x(N);
    for (int n = 0; n < N; n++) {
        x[n] = sin(2 * M_PI * freq * n / fs);
    }
    return x;
}

std::vector<double> generate_square(int N, double fs, double freq, double amplitude = 1.0) {
    std::vector<double> x(N);
    for (int n = 0; n < N; n++) {
        double phase = std::fmod(freq * n / fs, 1.0);
        x[n] = (phase < 0.5) ? amplitude : -amplitude;
    }
    return x;
}

std::vector<double> generate_sawtooth(int N, double fs, double freq, double amplitude = 1.0) {
    std::vector<double> x(N);
    for (int n = 0; n < N; n++) {
        double phase = std::fmod(freq * n / fs, 1.0);
        x[n] = amplitude * (2 * phase - 1);
    }
    return x;
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

void write_csv(const std::vector<double>& mag, double fs, int N, const std::string& filename) {
    std::ofstream f(filename);
    f << "hz,magnitude\n";
    for (size_t k = 0; k < mag.size(); k++) {
        f << bin_to_hz(k, fs, N) << "," << mag[k] << "\n";
    }
}

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

int main() {
    int N = 2205;
    double fs = 44100, freq = 220;
    
    std::vector<double> x = generate_sine(N, fs, freq);
    std::vector<std::complex<double>> X = dft(x);
    std::vector<double> mag = magnitude(X);
    write_csv(mag, fs, N, "sine_spectrum.csv");

    x = generate_square(N, fs, freq);
    X = dft(x);
    mag = magnitude(X);
    write_csv(mag, fs, N, "square_spectrum.csv");

    x = generate_sawtooth(N, fs, freq);
    X = dft(x);
    mag = magnitude(X);
    write_csv(mag, fs, N, "sawtooth_spectrum.csv");

    WavData wav = read_wav("sawtooth.wav");
    x = std::vector<double>(wav.samples.begin(), wav.samples.begin() + N); // N = 2205, from the exact-bin math
    X = dft(x);
    mag = magnitude(X);
    write_csv(mag, fs, N, "sawtooth_wav_spectrum.csv");

    wav = read_wav("square.wav");
    x = std::vector<double>(wav.samples.begin(), wav.samples.begin() + N); // N = 2205, from the exact-bin math
    X = dft(x);
    mag = magnitude(X);
    write_csv(mag, fs, N, "square_wav_spectrum.csv");

    return 0;
}