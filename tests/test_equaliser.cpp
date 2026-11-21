#include "equaliser.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double first, double second, double tolerance = 1e-10) {
    return std::fabs(first - second) <= tolerance;
}

void test_unity_gains_reproduce_delayed_input() {
    equaliser::Equaliser eq(300.0, 2400.0, 8000.0);
    const double input[] = {0.25, -0.5, 0.75, 0.125, -0.25, 0.5};

    for (std::size_t index = 0; index < 6; ++index) {
        const double output = eq.process(input[index]);
        const double expected = index < 3 ? 0.0 : input[index - 3];
        require(near(output, expected), "unity gains must preserve signal");
    }
}

void test_block_and_in_place_processing_match_scalar_processing() {
    const double input[] = {0.1, 0.2, -0.1, 0.4, -0.3, 0.0};
    double output[6] = {};
    double in_place[6];
    for (std::size_t index = 0; index < 6; ++index) {
        in_place[index] = input[index];
    }

    equaliser::Equaliser block(300.0, 2400.0, 8000.0);
    equaliser::Equaliser scalar(300.0, 2400.0, 8000.0);
    equaliser::Equaliser in_place_eq(300.0, 2400.0, 8000.0);
    block.set_gains(equaliser::Gains{0.75, 1.0, 1.1});
    scalar.set_gains(equaliser::Gains{0.75, 1.0, 1.1});
    in_place_eq.set_gains(equaliser::Gains{0.75, 1.0, 1.1});

    block.process(input, output, 6);
    in_place_eq.process(in_place, in_place, 6);
    for (std::size_t index = 0; index < 6; ++index) {
        require(near(output[index], scalar.process(input[index])),
                "block processing must match scalar processing");
        require(near(in_place[index], output[index]),
                "in-place processing must match out-of-place processing");
    }
}

void test_reset_preserves_gains_and_clears_history() {
    equaliser::Equaliser eq(300.0, 2400.0, 8000.0);
    eq.set_gains(equaliser::Gains{0.5, 1.25, 0.75});
    for (int index = 0; index < 12; ++index) {
        eq.process(index == 0 ? 1.0 : 0.0);
    }

    eq.reset();
    const equaliser::Gains gains = eq.gains();
    require(near(gains.low, 0.5) && near(gains.mid, 1.25) &&
                near(gains.high, 0.75),
            "reset must preserve gains");

    equaliser::Equaliser fresh(300.0, 2400.0, 8000.0);
    fresh.set_gains(gains);
    for (int index = 0; index < 8; ++index) {
        const double input = index == 0 ? 1.0 : 0.0;
        require(near(eq.process(input), fresh.process(input)),
                "reset must clear filter and sample history");
    }
}

void test_invalid_parameters_are_rejected() {
    bool rejected = false;
    try {
        equaliser::Equaliser invalid(300.0, 4000.0, 8000.0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "cutoff at or above Nyquist must be rejected");

    equaliser::Equaliser eq(300.0, 2400.0, 8000.0);
    rejected = false;
    try {
        eq.set_gains(equaliser::Gains{1.0, std::nan(""), 1.0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "non-finite gains must be rejected");
}

}  // namespace

int main() {
    try {
        test_unity_gains_reproduce_delayed_input();
        test_block_and_in_place_processing_match_scalar_processing();
        test_reset_preserves_gains_and_clears_history();
        test_invalid_parameters_are_rejected();
    } catch (const std::exception& error) {
        std::cerr << "equaliser test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "equaliser C++ tests passed\n";
    return 0;
}
