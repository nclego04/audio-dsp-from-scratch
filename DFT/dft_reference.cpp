// Reference implementation — for diffing against your own dft.cpp once it
// compiles and passes, not for starting from. Same pattern as sms-tools in
// the plan's Week 14 note: write yours first, then compare.

#define _USE_MATH_DEFINES
#include <cmath>
#include <complex>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>

using namespace std;

// ---- the transform itself ----
vector<complex<double>> dft(const vector<double>& x) {
    int N = (int)x.size();
    vector<complex<double>> X(N, 0.0);

    for (int k = 0; k < N; k++) {
        complex<double> sum = 0;
        for (int n = 0; n < N; n++) {
            double angle = -2 * M_PI * k * n / N;
            complex<double> W(cos(angle), sin(angle));
            sum += W * x[n];
        }
        X[k] = sum;
    }
    return X;
}

// ---- 1/2. test-signal generator, N and fs concrete ----
vector<double> generate_sine(int N, double fs, double freq, double amplitude = 1.0) {
    vector<double> x(N);
    for (int n = 0; n < N; n++) {
        x[n] = amplitude * sin(2 * M_PI * freq * n / fs);
    }
    return x;
}

// ---- 3. complex -> magnitude ----
vector<double> magnitude(const vector<complex<double>>& X) {
    vector<double> mag(X.size());
    for (size_t k = 0; k < X.size(); k++) {
        mag[k] = abs(X[k]);
    }
    return mag;
}

// ---- 6. bin -> Hz ----
double bin_to_hz(int k, double fs, int N) {
    return k * fs / N;
}

// spectrum as hz,magnitude -- plot with plot_spectrum.py
// full=false writes the single-sided range [0, N/2] (Audacity-style, positive
// frequencies only); full=true writes [0, N-1] so the mirror spike from
// conjugate symmetry (X[N-k] = conj(X[k])) is visible too.
void write_csv(const vector<double>& mag, double fs, int N, const string& filename, bool full = false) {
    ofstream f(filename);
    f << "hz,magnitude\n";
    int k_max = full ? N - 1 : N / 2;
    for (int k = 0; k <= k_max; k++) {
        f << bin_to_hz(k, fs, N) << "," << mag[k] << "\n";
    }
}

// ---- 4. inspection: strongest bin in [1, N/2], DC excluded ----
int argmax_single_sided(const vector<double>& mag, int N) {
    int best_k = 1;
    for (int k = 2; k <= N / 2; k++) {
        if (mag[k] > mag[best_k]) best_k = k;
    }
    return best_k;
}

// top-K peaks with non-max suppression so one leaky peak doesn't fill the
// whole list with its own neighbors
vector<int> top_peaks(const vector<double>& mag, int N, int count, int guard = 3) {
    vector<double> work(mag.begin(), mag.begin() + N / 2 + 1);
    work[0] = 0; // exclude DC
    vector<int> peaks;

    for (int i = 0; i < count; i++) {
        int k = (int)(max_element(work.begin(), work.end()) - work.begin());
        if (work[k] <= 0) break;
        peaks.push_back(k);
        for (int j = max(0, k - guard); j <= min((int)work.size() - 1, k + guard); j++) {
            work[j] = 0;
        }
    }
    return peaks;
}

// ---- 5. WAV reader, mirrors the RIFF/fmt/data layout wav_generator.cpp writes ----
struct WavData {
    int sample_rate = 0;
    int num_channels = 0;
    int bits_per_sample = 0;
    vector<double> samples; // channel 0 only, deinterleaved
};

string read_id(ifstream& f) {
    char id[4];
    f.read(id, 4);
    return string(id, 4);
}

uint32_t read_u32(ifstream& f) {
    uint32_t v;
    f.read(reinterpret_cast<char*>(&v), 4);
    return v;
}

uint16_t read_u16(ifstream& f) {
    uint16_t v;
    f.read(reinterpret_cast<char*>(&v), 2);
    return v;
}

WavData read_wav(const string& filename) {
    WavData wav;
    ifstream f(filename, ios::binary);
    if (!f.is_open()) {
        cerr << "failed to open " << filename << "\n";
        return wav;
    }

    if (read_id(f) != "RIFF") { cerr << filename << ": not a RIFF file\n"; return wav; }
    read_u32(f); // overall chunk size, unused
    if (read_id(f) != "WAVE") { cerr << filename << ": not a WAVE file\n"; return wav; }

    while (f) {
        string chunk_id = read_id(f);
        uint32_t chunk_size = read_u32(f);
        if (!f) break;

        if (chunk_id == "fmt ") {
            read_u16(f); // audio format
            wav.num_channels = read_u16(f);
            wav.sample_rate = read_u32(f);
            read_u32(f); // byte rate
            read_u16(f); // block align
            wav.bits_per_sample = read_u16(f);
            if (chunk_size > 16) f.seekg(chunk_size - 16, ios::cur); // skip extra fmt bytes
        } else if (chunk_id == "data") {
            int bytes_per_sample = wav.bits_per_sample / 8;
            int frame_size = bytes_per_sample * wav.num_channels;
            int num_frames = chunk_size / frame_size;

            wav.samples.resize(num_frames);
            for (int i = 0; i < num_frames; i++) {
                int16_t left;
                f.read(reinterpret_cast<char*>(&left), 2);
                wav.samples[i] = (double)left;
                if (wav.num_channels > 1) {
                    f.seekg(bytes_per_sample * (wav.num_channels - 1), ios::cur);
                }
            }
            break; // data is the last chunk we care about
        } else {
            f.seekg(chunk_size, ios::cur); // skip unknown chunk
        }
    }

    return wav;
}

// ---- driver ----
void run_sine_test() {
    const int N = 44100;
    const double fs = 44100.0;
    const double freq = 440.0;

    cout << "=== Sine test: " << freq << " Hz, N=" << N << ", fs=" << fs << " ===\n";

    vector<double> x = generate_sine(N, fs, freq);

    auto t0 = chrono::steady_clock::now();
    vector<complex<double>> X = dft(x);
    auto t1 = chrono::steady_clock::now();
    cout << "dft() took " << chrono::duration<double>(t1 - t0).count() << " s\n";

    vector<double> mag = magnitude(X);
    int peak_k = argmax_single_sided(mag, N);
    int mirror_k = N - peak_k;

    cout << "peak bin: " << peak_k << " (" << bin_to_hz(peak_k, fs, N) << " Hz), magnitude "
         << mag[peak_k] << "\n";
    cout << "mirror bin " << mirror_k << " magnitude " << mag[mirror_k]
         << " (expect ~= peak, conjugate symmetry)\n";
    cout << "DC bin (k=0) magnitude: " << mag[0] << " (expect ~0, sine has no DC)\n";

    bool pass = (peak_k == 440) && (abs(mag[peak_k] - mag[mirror_k]) < 1e-6 * mag[peak_k]);
    cout << (pass ? "PASS" : "FAIL") << ": single-bin spike at expected 440 Hz\n";

    write_csv(mag, fs, N, "sine_spectrum.csv", true);
    cout << "wrote sine_spectrum.csv\n\n";
}

void run_wav_harmonics_test(const string& filename, double fundamental_hz) {
    cout << "=== WAV harmonics test: " << filename << " ===\n";
    WavData wav = read_wav(filename);
    if (wav.samples.empty()) {
        cout << "FAIL: could not read samples\n\n";
        return;
    }

    const int ANALYSIS_N = 8192; // keep O(N^2) fast; ~186 ms of audio at 44.1kHz
    int N = min(ANALYSIS_N, (int)wav.samples.size());
    vector<double> x(wav.samples.begin(), wav.samples.begin() + N);
    double fs = wav.sample_rate;

    cout << "analyzing first " << N << " samples (" << N / fs << " s), fs=" << fs << "\n";

    auto t0 = chrono::steady_clock::now();
    vector<complex<double>> X = dft(x);
    auto t1 = chrono::steady_clock::now();
    cout << "dft() took " << chrono::duration<double>(t1 - t0).count() << " s\n";

    vector<double> mag = magnitude(X);
    vector<int> peaks = top_peaks(mag, N, 8);

    cout << "top peaks (bin, Hz, magnitude) -- compare against Audacity's spectrogram:\n";
    for (int k : peaks) {
        double hz = bin_to_hz(k, fs, N);
        cout << "  k=" << k << "  " << hz << " Hz  mag=" << mag[k]
             << "  (harmonic #" << round(hz / fundamental_hz) << ")\n";
    }

    string csv_name = filename.substr(0, filename.find_last_of('.')) + "_spectrum.csv";
    write_csv(mag, fs, N, csv_name);
    cout << "wrote " << csv_name << "\n\n";
}

int main() {
    run_sine_test();
    // run_wav_harmonics_test("sawtooth.wav", 440.0);
    // run_wav_harmonics_test("square.wav", 440.0);
    return 0;
}
