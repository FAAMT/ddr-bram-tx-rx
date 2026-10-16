#include <iostream>
#include "ddr_bram_bridge.hpp"

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
    bram_word_t ddr[BRAM_WORD_COUNT] = {};

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
    ap_uint<9> words_transferred;

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

        ddr_bram_bridge(
            sample0,
            sample1,
            sample2,
            sample3,
            static_cast<ap_uint<8> >(word),
            CMD_WRITE,
            ddr,
            0,
            0,
            read0,
            read1,
            read2,
            read3,
            words_transferred,
            bram
        );
    }

    int mismatch_count = 0;

    // Read back through the HLS IP.
    for (int word = 0; word < BRAM_WORD_COUNT; ++word) {
        ddr_bram_bridge(
            0,
            0,
            0,
            0,
            static_cast<ap_uint<8> >(word),
            CMD_READ,
            ddr,
            0,
            0,
            read0,
            read1,
            read2,
            read3,
            words_transferred,
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

    // Verify the new DDR-to-BRAM path, including a non-zero destination.
    for (int word = 0; word < BRAM_WORD_COUNT; ++word) {
        ddr[word] =
            (static_cast<bram_word_t>(0xD00D0000U + word) << 96) |
            (static_cast<bram_word_t>(0xC0DE0000U + word) << 64) |
            (static_cast<bram_word_t>(0xA5A50000U + word) << 32) |
            static_cast<bram_word_t>(0x5A5A0000U + word);

        bram[word] = 0;
    }

    const int destination_start = 32;
    const int requested_words = 192;

    ddr_bram_bridge(
        0,
        0,
        0,
        0,
        0,
        CMD_DDR_TO_BRAM,
        ddr,
        destination_start,
        requested_words,
        read0,
        read1,
        read2,
        read3,
        words_transferred,
        bram
    );

    if (words_transferred != requested_words) {
        std::cout
            << "DDR copy count mismatch: expected "
            << requested_words
            << ", got "
            << words_transferred
            << std::endl;
        ++mismatch_count;
    }

    for (int word = 0; word < BRAM_WORD_COUNT; ++word) {
        bram_word_t expected = 0;

        if (word >= destination_start &&
            word < destination_start + requested_words) {
            expected = ddr[word - destination_start];
        }

        if (bram[word] != expected) {
            if (mismatch_count < 10) {
                std::cout
                    << "DDR copy mismatch at BRAM word "
                    << word
                    << std::endl;
            }

            ++mismatch_count;
        }
    }

    // A request that runs past the BRAM end must be truncated safely.
    const int bounded_start = 250;
    const int bounded_request = 20;
    const int bounded_expected = BRAM_WORD_COUNT - bounded_start;

    ddr_bram_bridge(
        0,
        0,
        0,
        0,
        0,
        CMD_DDR_TO_BRAM,
        ddr,
        bounded_start,
        bounded_request,
        read0,
        read1,
        read2,
        read3,
        words_transferred,
        bram
    );

    if (words_transferred != bounded_expected) {
        std::cout
            << "Bounded DDR copy count mismatch: expected "
            << bounded_expected
            << ", got "
            << words_transferred
            << std::endl;
        ++mismatch_count;
    }

    for (int word = 0; word < bounded_expected; ++word) {
        if (bram[bounded_start + word] != ddr[word]) {
            if (mismatch_count < 10) {
                std::cout
                    << "Bounded DDR copy mismatch at BRAM word "
                    << bounded_start + word
                    << std::endl;
            }

            ++mismatch_count;
        }
    }

    std::cout << "Mismatch count: "
              << mismatch_count
              << std::endl;

    if (mismatch_count == 0) {
        std::cout
            << "DDR BRAM BRIDGE TEST PASSED"
            << std::endl;

        return 0;
    }

    std::cout
        << "DDR BRAM BRIDGE TEST FAILED"
        << std::endl;

    return 1;
}
