#include "equaliser.h"

#include <math.h>
#include <stdio.h>

int main(void) {
    EQSTATE state = {0};
    if (equaliser_init_checked(&state, 300, 2400, 8000) != EQUALISER_OK) {
        fprintf(stderr, "checked C initialization failed\n");
        return 1;
    }

    if (state.lg != 1.0 || state.mg != 1.0 || state.hg != 1.0) {
        fprintf(stderr, "C initialization did not set unity gains\n");
        return 1;
    }

    if (fabs(equalise(&state, 1.0)) > 1e-10) {
        fprintf(stderr, "C API processing failed\n");
        return 1;
    }

    {
        const double prior_sample = state.sdm1;
        if (equaliser_init_checked(&state, 300, 4000, 8000) !=
                EQUALISER_INVALID_FREQUENCY ||
            state.sdm1 != prior_sample) {
            fprintf(stderr, "C API validation or processing failed\n");
            return 1;
        }
    }

    init_equaliser(&state, 300, 2400, 8000);
    state.lg = 0.75;
    if (!isfinite(equalise(&state, 0.5))) {
        fprintf(stderr, "legacy C API returned a non-finite result\n");
        return 1;
    }

    puts("equaliser C API test passed");
    return 0;
}
