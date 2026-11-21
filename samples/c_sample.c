#include "equaliser.h"

#include <math.h>
#include <stdio.h>

int main(void) {
    const int sample_rate = 8000;
    const int sample_count = 32000;
    EQSTATE eq;

    if (equaliser_init_checked(&eq, 300, 2400, sample_rate) != EQUALISER_OK) {
        fprintf(stderr, "Unable to initialize equaliser\n");
        return 1;
    }

    eq.lg = 0.75;
    eq.mg = 1.0;
    eq.hg = 1.1;

    for (int index = 0; index < sample_count; ++index) {
        const double input = sin(2.0 * 3.141592653589793 * 440.0 * index /
                                 sample_rate);
        const double output = equalise(&eq, input);
        if (index == 0) {
            printf("First output sample: %f\n", output);
        }
    }
    printf("Processed %d samples\n", sample_count);
    return 0;
}
