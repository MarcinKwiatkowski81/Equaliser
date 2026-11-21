#include "equaliser.h"

int main()
  {
        EQSTATE eq;

        // low band = 0-300Hz
        // mid band = 300-2400Hz
        // hi band = 2400-4000Hz
        // sampling rate 8000
        init_equaliser(&eq,300,2400,8000);

        // Set gains:
        eq.lg = 0.75;   // Cut low by 25%
        eq.mg = 1;      // leave as-is
        eq.hg = 1.1;    // gain high by 10%

        double waveform[32000];

        //presume we have waveform in double waveform[32000]; (32k samples -> 4 seconds)
        for(int idx=0;idx<32000;idx++)
            waveform[idx]=equalise(&eq,waveform[idx]);
  }
