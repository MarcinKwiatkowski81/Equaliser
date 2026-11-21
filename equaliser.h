#ifndef EQUALISER_H
#define EQUALISER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EQSTATE {
    double lf;
    double f1p0;
    double f1p1;
    double f1p2;
    double f1p3;

    double hf;
    double f2p0;
    double f2p1;
    double f2p2;
    double f2p3;

    double sdm1;
    double sdm2;
    double sdm3;

    double lg;
    double mg;
    double hg;
} EQSTATE;

typedef enum equaliser_result {
    EQUALISER_OK = 0,
    EQUALISER_INVALID_ARGUMENT = 1,
    EQUALISER_INVALID_FREQUENCY = 2
} equaliser_result;

/* Typical cutoffs are 880 Hz and 5000 Hz when both are below Nyquist. */
int equaliser_init_checked(EQSTATE* state, int low_frequency,
                           int high_frequency, int sample_rate);

/* Legacy API. For error reporting, use equaliser_init_checked instead. */
void init_equaliser(EQSTATE* state, int low_frequency,
                    int high_frequency, int sample_rate);
double equalise(EQSTATE* state, double sample);

#ifdef __cplusplus
}
#endif

#endif
