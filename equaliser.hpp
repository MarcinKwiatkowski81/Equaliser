#ifndef EQUALISER_HPP
#define EQUALISER_HPP

#include "equaliser.h"

#include <cstddef>

namespace equaliser {

struct Gains {
    double low;
    double mid;
    double high;
};

class Equaliser {
public:
    Equaliser(double low_frequency, double high_frequency, double sample_rate);

    double process(double sample);
    // input and output may be identical; other overlapping ranges are unsupported.
    void process(const double* input, double* output, std::size_t sample_count);

    void set_gains(Gains gains);
    Gains gains() const noexcept;
    void reset() noexcept;

private:
    EQSTATE state_;
};

}  // namespace equaliser

#endif
