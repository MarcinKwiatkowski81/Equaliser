#include "equaliser.hpp"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {

const double denormal_offset = 1.0 / 4294967295.0;
const double pi = std::acos(-1.0);

bool valid_frequencies(double low_frequency, double high_frequency,
                       double sample_rate) {
    return std::isfinite(low_frequency) && std::isfinite(high_frequency) &&
           std::isfinite(sample_rate) && sample_rate > 0.0 &&
           low_frequency > 0.0 && low_frequency < high_frequency &&
           high_frequency < sample_rate / 2.0;
}

void initialize_state(EQSTATE& state, double low_frequency,
                      double high_frequency, double sample_rate) {
    std::memset(&state, 0, sizeof(state));
    state.lg = 1.0;
    state.mg = 1.0;
    state.hg = 1.0;
    state.lf = 2.0 * std::sin(pi * low_frequency / sample_rate);
    state.hf = 2.0 * std::sin(pi * high_frequency / sample_rate);
}

bool finite_gains(equaliser::Gains gains) {
    return std::isfinite(gains.low) && std::isfinite(gains.mid) &&
           std::isfinite(gains.high);
}

}  // namespace

extern "C" int equaliser_init_checked(EQSTATE* state, int low_frequency,
                                       int high_frequency, int sample_rate) {
    if (state == 0) {
        return EQUALISER_INVALID_ARGUMENT;
    }
    if (!valid_frequencies(low_frequency, high_frequency, sample_rate)) {
        return EQUALISER_INVALID_FREQUENCY;
    }
    initialize_state(*state, low_frequency, high_frequency, sample_rate);
    return EQUALISER_OK;
}

extern "C" void init_equaliser(EQSTATE* state, int low_frequency,
                                int high_frequency, int sample_rate) {
    equaliser_init_checked(state, low_frequency, high_frequency, sample_rate);
}

extern "C" double equalise(EQSTATE* state, double sample) {
    if (state == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double low;
    double mid;
    double high;

    state->f1p0 += (state->lf * (sample - state->f1p0)) + denormal_offset;
    state->f1p1 += state->lf * (state->f1p0 - state->f1p1);
    state->f1p2 += state->lf * (state->f1p1 - state->f1p2);
    state->f1p3 += state->lf * (state->f1p2 - state->f1p3);
    low = state->f1p3;

    state->f2p0 += (state->hf * (sample - state->f2p0)) + denormal_offset;
    state->f2p1 += state->hf * (state->f2p0 - state->f2p1);
    state->f2p2 += state->hf * (state->f2p1 - state->f2p2);
    state->f2p3 += state->hf * (state->f2p2 - state->f2p3);
    high = state->sdm3 - state->f2p3;

    mid = state->sdm3 - (high + low);

    low *= state->lg;
    mid *= state->mg;
    high *= state->hg;

    state->sdm3 = state->sdm2;
    state->sdm2 = state->sdm1;
    state->sdm1 = sample;

    return low + mid + high;
}

namespace equaliser {

Equaliser::Equaliser(double low_frequency, double high_frequency,
                     double sample_rate) {
    if (!valid_frequencies(low_frequency, high_frequency, sample_rate)) {
        throw std::invalid_argument(
            "cutoffs must satisfy 0 < low < high < sample_rate / 2");
    }
    initialize_state(state_, low_frequency, high_frequency, sample_rate);
}

double Equaliser::process(double sample) {
    return equalise(&state_, sample);
}

void Equaliser::process(const double* input, double* output,
                        std::size_t sample_count) {
    if (sample_count != 0 && (input == 0 || output == 0)) {
        throw std::invalid_argument("input and output must not be null");
    }
    for (std::size_t index = 0; index < sample_count; ++index) {
        output[index] = process(input[index]);
    }
}

void Equaliser::set_gains(Gains gains) {
    if (!finite_gains(gains)) {
        throw std::invalid_argument("gains must be finite");
    }
    state_.lg = gains.low;
    state_.mg = gains.mid;
    state_.hg = gains.high;
}

Gains Equaliser::gains() const noexcept {
    Gains result = {state_.lg, state_.mg, state_.hg};
    return result;
}

void Equaliser::reset() noexcept {
    const double low_filter = state_.lf;
    const double high_filter = state_.hf;
    const Gains current_gains = gains();
    std::memset(&state_, 0, sizeof(state_));
    state_.lf = low_filter;
    state_.hf = high_filter;
    state_.lg = current_gains.low;
    state_.mg = current_gains.mid;
    state_.hg = current_gains.high;
}

}  // namespace equaliser
