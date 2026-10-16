#include <iostream>
#include "iq_bram_bridge.hpp"

static iq_sample_t pack_iq(ap_int<16> i_value, ap_int<16> q_value)
{
    iq_sample_t packed = 0;

    ap_uint<16> i_bits = static_cast<ap_uint<16> >(i_value);
    ap_uint<16> q_bits = static_cast<ap_uint<16> >(q_value);

    packed.range(15, 0)  = i_bits;
    packed.range(31, 16) = q_bits;

    return packed;
}

static ap_int<16> unpack_i(iq_sample_t packed)
{
    return static_cast<ap_int<16> >(packed.range(15, 0));
}

static ap_int<16> unpack_q(iq_sample_t packed)
{
    return static_cast<ap_int<16> >(packed.range(31, 16));
}

int main()
{
    bram_word_t bram[BRAM_WORD_COUNT] = {};

    ap_int<16> reference_i[IQ_SAMPLE_COUNT];
    ap_int<16> reference_q[IQ_SAMPLE_COUNT];

    // Deterministic signed int16 test waveform.
    for (int n = 0; n < IQ_SAMPLE_COUNT; ++n) {
        reference_i[n] = n - 512;
        reference_q[n] = 511 - n;
    }

    iq_sample_t read0;
    iq_sample_t read1;
    iq_sample_t read2;
    iq_sample_t read3;

    // Write 1024 IQ samples as 256 groups of four.
    for (int word = 0; word < BRAM_WORD_COUNT; ++word) {
        const int base = word * SAMPLES_PER_WORD;

        iq_sample_t sample0 =
            pack_iq(reference_i[base + 0], reference_q[base + 0]);

        iq_sample_t sample1 =
            pack_iq(reference_i[base + 1], reference_q[base + 1]);

        iq_sample_t sample2 =
            pack_iq(reference_i[base + 2], reference_q[base + 2]);

        iq_sample_t sample3 =
            pack_iq(reference_i[base + 3], reference_q[base + 3]);

        iq_bram_bridge(
            sample0,
            sample1,
            sample2,
            sample3,
            static_cast<ap_uint<8> >(word),
            CMD_WRITE,
            read0,
            read1,
            read2,
            read3,
            bram
        );
    }

    int mismatch_count = 0;

    // Read back through the HLS IP.
    for (int word = 0; word < BRAM_WORD_COUNT; ++word) {
        iq_bram_bridge(
            0,
            0,
            0,
            0,
            static_cast<ap_uint<8> >(word),
            CMD_READ,
            read0,
            read1,
            read2,
            read3,
            bram
        );

        iq_sample_t returned[4] = {
            read0, read1, read2, read3
        };

        for (int lane = 0; lane < 4; ++lane) {
            const int sample_index = word * 4 + lane;

            const ap_int<16> got_i = unpack_i(returned[lane]);
            const ap_int<16> got_q = unpack_q(returned[lane]);

            if (got_i != reference_i[sample_index] ||
                got_q != reference_q[sample_index]) {

                if (mismatch_count < 10) {
                    std::cout
                        << "Mismatch at sample "
                        << sample_index
                        << ": expected I="
                        << reference_i[sample_index]
                        << ", Q="
                        << reference_q[sample_index]
                        << "; got I="
                        << got_i
                        << ", Q="
                        << got_q
                        << std::endl;
                }

                ++mismatch_count;
            }
        }
    }

    std::cout << "Mismatch count: "
              << mismatch_count
              << std::endl;

    if (mismatch_count == 0) {
        std::cout
            << "IQ BRAM BRIDGE TEST PASSED"
            << std::endl;

        return 0;
    }

    std::cout
        << "IQ BRAM BRIDGE TEST FAILED"
        << std::endl;

    return 1;
}
