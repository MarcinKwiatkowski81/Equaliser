# Equaliser

Three-band frequency corrector implemented in C++ with a C-compatible API.
The low and high cutoff frequencies must satisfy
`0 < low < high < sample_rate / 2` (the Nyquist frequency).

## Original source

Author: Marcin Kwiatkowski <marcin@kwiatkowski.info.pl>  
Created: September 22, 2010, 11:09 AM

## C++ API

```cpp
#include "equaliser.hpp"

equaliser::Equaliser eq(300.0, 2400.0, 8000.0);
eq.set_gains(equaliser::Gains{0.75, 1.0, 1.1});
double output = eq.process(input_sample);
```

`Equaliser` also provides block processing, in-place processing, and `reset()`.
Reset clears filter history but retains the configured gains.

## C API

Include `equaliser.h` from C or C++. Prefer `equaliser_init_checked()` for
input validation and error reporting. The original `init_equaliser()` and
`equalise()` functions are retained for source compatibility.

## Build, test, and examples

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build produces `equaliser_cpp_sample` and `equaliser_c_sample`. Their
sources are in `samples/`; C API and C++ behavior tests are in `tests/`.
