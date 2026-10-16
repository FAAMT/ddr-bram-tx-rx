#include <iostream>
#include "wave_player.hpp"

static ap_uint<32> pack_iq(
    ap_int<16> i_value,
    ap_int<16> q_value
) {
    ap_uint<32> packed = 0;

    packed.range(15, 0) =
        static_cast<ap_uint<16> >(i_value);

    packed.range(31, 16) =
        static_cast<ap_uint<16> >(q_value);

    return packed;
}

static bram_word_t make_bram_word(
    int word_index
) {
    bram_word_t word = 0;

    int base_sample =
        word_index * IQ_SAMPLES_PER_WORD;

    for (int lane = 0;
         lane < IQ_SAMPLES_PER_WORD;
         ++lane) {

        int sample_index =
            base_sample + lane;

        ap_int<16> i_value =
            static_cast<ap_int<16> >(
                sample_index - 512
            );

        ap_int<16> q_value =
            static_cast<ap_int<16> >(
                511 - sample_index
            );

        ap_uint<32> packed =
            pack_iq(i_value, q_value);

        word.range(
            lane * 32 + 31,
            lane * 32
        ) = packed;
    }

    return word;
}

static int test_repeat_count(
    const bram_word_t bram[BRAM_WORD_COUNT],
    ap_uint<32> repeat_count
) {
    hls::stream<axis_word_t> output_stream;

    ap_uint<32> frames_completed = 0;

    wave_player(
        bram,
        output_stream,
        repeat_count,
        frames_completed
    );

    int mismatch_count = 0;

    if (frames_completed != repeat_count) {
        std::cout
            << "frames_completed mismatch: expected "
            << repeat_count
            << ", got "
            << frames_completed
            << std::endl;

        ++mismatch_count;
    }

    int expected_beats =
        static_cast<int>(repeat_count) *
        AXIS_BEATS_PER_FRAME;

    for (int beat = 0;
         beat < expected_beats;
         ++beat) {

        if (output_stream.empty()) {
            std::cout
                << "Stream empty at output beat "
                << beat
                << std::endl;

            ++mismatch_count;
            break;
        }

        axis_word_t actual =
            output_stream.read();

        int word_index =
            beat % BRAM_WORD_COUNT;

        bram_word_t expected_data =
            bram[word_index];

        if (actual.data != expected_data) {
            if (mismatch_count < 10) {
                std::cout
                    << "TDATA mismatch at beat "
                    << beat
                    << ", BRAM word "
                    << word_index
                    << std::endl;
            }

            ++mismatch_count;
        }

        ap_uint<16> expected_keep =
            0xFFFF;

        ap_uint<16> expected_strb =
            0xFFFF;

        if (actual.keep != expected_keep) {
            std::cout
                << "TKEEP mismatch at beat "
                << beat
                << std::endl;

            ++mismatch_count;
        }

        if (actual.strb != expected_strb) {
            std::cout
                << "TSTRB mismatch at beat "
                << beat
                << std::endl;

            ++mismatch_count;
        }

        bool expected_last =
            (word_index ==
             BRAM_WORD_COUNT - 1);

        if (static_cast<bool>(actual.last) !=
            expected_last) {

            std::cout
                << "TLAST mismatch at beat "
                << beat
                << std::endl;

            ++mismatch_count;
        }
    }

    if (!output_stream.empty()) {
        std::cout
            << "Unexpected extra AXIS data"
            << std::endl;

        ++mismatch_count;
    }

    return mismatch_count;
}

int main()
{
    bram_word_t bram[BRAM_WORD_COUNT];

    for (int word_index = 0;
         word_index < BRAM_WORD_COUNT;
         ++word_index) {

        bram[word_index] =
            make_bram_word(word_index);
    }

    int total_mismatches = 0;

    // Test 1: repeat_count = 3
    //
    // Expected:
    // 3 waveforms
    // 3 x 1024 IQ samples
    // 3 x 256 AXIS beats
    total_mismatches +=
        test_repeat_count(bram, 3);

    // Test 2: repeat_count = 1
    total_mismatches +=
        test_repeat_count(bram, 1);

    // Test 3: repeat_count = 0
    //
    // No AXIS beats should be generated.
    total_mismatches +=
        test_repeat_count(bram, 0);

    std::cout
        << "Mismatch count: "
        << total_mismatches
        << std::endl;

    if (total_mismatches == 0) {
        std::cout
            << "128-BIT WAVE PLAYER TEST PASSED"
            << std::endl;

        return 0;
    }

    std::cout
        << "128-BIT WAVE PLAYER TEST FAILED"
        << std::endl;

    return 1;
}
