#include "equaliser.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t read_u16_le(const unsigned char* bytes) {
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t read_u32_le(const unsigned char* bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void write_u16_le(unsigned char* bytes, uint16_t value) {
    bytes[0] = (unsigned char)(value & 0xff);
    bytes[1] = (unsigned char)(value >> 8);
}

static void write_u32_le(unsigned char* bytes, uint32_t value) {
    bytes[0] = (unsigned char)(value & 0xff);
    bytes[1] = (unsigned char)((value >> 8) & 0xff);
    bytes[2] = (unsigned char)((value >> 16) & 0xff);
    bytes[3] = (unsigned char)(value >> 24);
}

static int seek_to_chunk_end(FILE* file, long payload_offset,
                             uint32_t chunk_size) {
    const uint64_t padded_size =
        (uint64_t)chunk_size + (uint64_t)(chunk_size & 1u);
    if (payload_offset < 0 ||
        (uint64_t)padded_size > (uint64_t)LONG_MAX - (uint64_t)payload_offset) {
        return 0;
    }
    return fseek(file, payload_offset + (long)padded_size, SEEK_SET) == 0;
}

static int read_wav_info(FILE* input, uint32_t* sample_rate,
                         uint32_t* data_size, long* data_offset) {
    unsigned char riff_header[12];
    int have_format = 0;
    int have_data = 0;

    if (fread(riff_header, 1, sizeof(riff_header), input) !=
            sizeof(riff_header) ||
        memcmp(riff_header, "RIFF", 4) != 0 ||
        memcmp(riff_header + 8, "WAVE", 4) != 0) {
        fprintf(stderr, "Input is not a RIFF/WAVE file\n");
        return 0;
    }

    while (!have_format || !have_data) {
        unsigned char chunk_header[8];
        uint32_t chunk_size;
        long payload_offset;

        if (fread(chunk_header, 1, sizeof(chunk_header), input) !=
            sizeof(chunk_header)) {
            fprintf(stderr, "WAV file is missing a format or data chunk\n");
            return 0;
        }
        chunk_size = read_u32_le(chunk_header + 4);
        payload_offset = ftell(input);
        if (payload_offset < 0) {
            fprintf(stderr, "Unable to locate WAV chunk\n");
            return 0;
        }

        if (memcmp(chunk_header, "fmt ", 4) == 0) {
            unsigned char format[16];
            if (chunk_size < sizeof(format) ||
                fread(format, 1, sizeof(format), input) != sizeof(format)) {
                fprintf(stderr, "Invalid WAV format chunk\n");
                return 0;
            }
            if (read_u16_le(format) != 1 || read_u16_le(format + 2) != 1 ||
                read_u16_le(format + 12) != 2 ||
                read_u16_le(format + 14) != 16) {
                fprintf(stderr,
                        "WAV input must be mono, 16-bit integer PCM\n");
                return 0;
            }
            *sample_rate = read_u32_le(format + 4);
            if (*sample_rate == 0 ||
                read_u32_le(format + 8) != (uint64_t)*sample_rate * 2u) {
                fprintf(stderr, "Invalid WAV sample or byte rate\n");
                return 0;
            }
            have_format = 1;
        } else if (memcmp(chunk_header, "data", 4) == 0) {
            *data_offset = payload_offset;
            *data_size = chunk_size;
            have_data = 1;
        }

        if (!seek_to_chunk_end(input, payload_offset, chunk_size)) {
            fprintf(stderr, "Invalid or unsupported WAV chunk size\n");
            return 0;
        }
    }

    if ((*data_size & 1u) != 0) {
        fprintf(stderr, "WAV data chunk contains an incomplete 16-bit sample\n");
        return 0;
    }
    return 1;
}

static int write_wav_header(FILE* output, uint32_t sample_rate,
                            uint32_t data_size) {
    unsigned char header[44] = {0};
    const uint32_t byte_rate = sample_rate * 2u;

    memcpy(header, "RIFF", 4);
    write_u32_le(header + 4, 36u + data_size);
    memcpy(header + 8, "WAVEfmt ", 8);
    write_u32_le(header + 16, 16);
    write_u16_le(header + 20, 1);
    write_u16_le(header + 22, 1);
    write_u32_le(header + 24, sample_rate);
    write_u32_le(header + 28, byte_rate);
    write_u16_le(header + 32, 2);
    write_u16_le(header + 34, 16);
    memcpy(header + 36, "data", 4);
    write_u32_le(header + 40, data_size);

    return fwrite(header, 1, sizeof(header), output) == sizeof(header);
}

int main(int argc, char** argv) {
    const int low_frequency = 300;
    const int high_frequency = 2400;
    FILE* input;
    FILE* output;
    uint32_t sample_rate;
    uint32_t data_size;
    long data_offset;
    EQSTATE eq;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.wav output.wav\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Input and output paths must be different\n");
        return 1;
    }

    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("Unable to open input WAV");
        return 1;
    }

    if (!read_wav_info(input, &sample_rate, &data_size, &data_offset)) {
        fclose(input);
        return 1;
    }
    if (sample_rate > INT_MAX || data_size > UINT32_MAX - 36u ||
        sample_rate > UINT32_MAX / 2u) {
        fprintf(stderr, "WAV sample rate or data size is too large\n");
        fclose(input);
        return 1;
    }
    if (equaliser_init_checked(&eq, low_frequency, high_frequency,
                               (int)sample_rate) != EQUALISER_OK) {
        fprintf(stderr,
                "Sample rate %u Hz is too low for the 300/2400 Hz cutoffs\n",
                (unsigned int)sample_rate);
        fclose(input);
        return 1;
    }

    eq.lg = 0.75;
    eq.mg = 1.0;
    eq.hg = 1.1;

    output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("Unable to open output WAV");
        fclose(input);
        return 1;
    }
    if (!write_wav_header(output, sample_rate, data_size) ||
        fseek(input, data_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Unable to prepare WAV output\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    for (uint32_t index = 0; index < data_size / 2u; ++index) {
        unsigned char bytes[2];
        uint16_t encoded_sample;
        int32_t sample;
        double filtered;
        int32_t output_sample;

        if (fread(bytes, 1, sizeof(bytes), input) != sizeof(bytes)) {
            fprintf(stderr, "WAV data chunk is truncated\n");
            fclose(input);
            fclose(output);
            return 1;
        }
        encoded_sample = read_u16_le(bytes);
        sample = encoded_sample < 0x8000u
                     ? (int32_t)encoded_sample
                     : (int32_t)encoded_sample - 65536;
        filtered = equalise(&eq, (double)sample);

        if (filtered > 32767.0) {
            output_sample = 32767;
        } else if (filtered < -32768.0) {
            output_sample = -32768;
        } else {
            output_sample = filtered >= 0.0 ? (int32_t)(filtered + 0.5)
                                             : (int32_t)(filtered - 0.5);
        }
        write_u16_le(bytes, (uint16_t)(int16_t)output_sample);
        if (fwrite(bytes, 1, sizeof(bytes), output) != sizeof(bytes)) {
            fprintf(stderr, "Unable to write processed WAV data\n");
            fclose(input);
            fclose(output);
            return 1;
        }
    }

    {
        const int input_close_failed = fclose(input) != 0;
        const int output_close_failed = fclose(output) != 0;
        if (input_close_failed || output_close_failed) {
            fprintf(stderr, "Error closing WAV file\n");
            return 1;
        }
    }
    printf("Processed %u samples at %u Hz into %s\n",
           (unsigned int)(data_size / 2u), (unsigned int)sample_rate,
           argv[2]);
    return 0;
}
