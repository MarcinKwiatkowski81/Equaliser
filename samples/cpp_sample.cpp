#include "equaliser.hpp"

#include <cmath>
#include <iostream>
#include <vector>

int main() {
    const double sample_rate = 8000.0;
    const double pi = std::acos(-1.0);
    std::vector<double> waveform(32000);
    for (std::size_t index = 0; index < waveform.size(); ++index) {
        waveform[index] = std::sin(2.0 * pi * 440.0 * index / sample_rate);
    }

    equaliser::Equaliser eq(300.0, 2400.0, sample_rate);
    eq.set_gains(equaliser::Gains{0.75, 1.0, 1.1});
    eq.process(waveform.data(), waveform.data(), waveform.size());

    std::cout << "Processed " << waveform.size()
              << " samples; first output: " << waveform.front() << '\n';
    return 0;
}
