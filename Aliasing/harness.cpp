#define _USE_MATH_DEFINES
#include <chrono>
#include <cmath>
#include <cstdint>
#include <complex>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

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

std::vector<std::pair<double,double>> measure(double (*system)(double), int N, double fs) {
    std::vector<double>  x(N,0.0);
    x[0] = 1;

    std::vector<double> h(N,0.0);
    for (int k = 0; k < N; k++) {
        h[k] = system(x[k]);
    }   

    std::vector<std::complex<double>> H = dft(h);
    std::vector<double> H_mag = magnitude(H);

    std::vector<double> H_mag_dB(N);

    for (int i = 0; i < N; i++) {
        H_mag_dB[i] = 20 * log10(std::max(1e-12, H_mag[i]));
    }

    std::vector<std::pair<double, double>> result;
    for (int m = 0; m <= N/2; m++) {
        double hz = bin_to_hz(m, fs, N);
        result.push_back({hz, H_mag_dB[m]});
    }
    return result;
}

double passthrough(double x) {
    return x;
}

void write_csv(const std::vector<std::pair<double,double>>& data, const std::string& filename) {
    std::ofstream f(filename);
    f << "hz,dB\n";
    for (size_t k = 0; k < data.size(); k++) {
        f << data[k].first << "," << data[k].second << "\n";
    }
}

int main() {
    std::vector<std::pair<double, double>> result = measure(passthrough, 512, 44100.0);
    
    for (int i = 0; i < 5; i++) {
        std::cout << "hz = " << result[i].first << ", dB = " << result[i].second << "\n";
    }

    write_csv(result, "passthrough_response.csv");
    
    return 0;
}