#include <iostream>

#include "wave_capture.hpp"

static axis_component_t make_i_word(int word_index) {
    axis_component_t word = 0;

    for (int lane = 0;
         lane < IQ_SAMPLES_PER_WORD;
         ++lane) {
        const ap_uint<16> sample =
            static_cast<unsigned>(
                word_index * IQ_SAMPLES_PER_WORD + lane
            );

        word.range(
            lane * 16 + 15,
            lane * 16
        ) = sample;
    }

    return word;
}

static axis_component_t make_q_word(int word_index) {
    axis_component_t word = 0;

    for (int lane = 0;
         lane < IQ_SAMPLES_PER_WORD;
         ++lane) {
        // Different pattern from I, including the sign bit.
        const ap_uint<16> sample =
            static_cast<unsigned>(
                0x8000 +
                word_index * IQ_SAMPLES_PER_WORD +
                lane
            );

        word.range(
            lane * 16 + 15,
            lane * 16
        ) = sample;
    }

    return word;
}

static bram_word_t pack_expected(
    const axis_component_t& i_word,
    const axis_component_t& q_word
) {
    bram_word_t packed_word = 0;

    for (int lane = 0;
         lane < IQ_SAMPLES_PER_WORD;
         ++lane) {
        const ap_uint<16> i_sample =
            i_word.range(lane * 16 + 15, lane * 16);

        const ap_uint<16> q_sample =
            q_word.range(lane * 16 + 15, lane * 16);

        packed_word.range(
            lane * 32 + 15,
            lane * 32
        ) = i_sample;

        packed_word.range(
            lane * 32 + 31,
            lane * 32 + 16
        ) = q_sample;
    }

    return packed_word;
}

int main() {
    hls::stream<axis_component_t> i_stream;
    hls::stream<axis_component_t> q_stream;

    bram_word_t capture_bram[BRAM_WORD_COUNT];
    bram_word_t expected[BRAM_WORD_COUNT];

    ap_uint<32> captured_words = 0;

    for (int word_index = 0;
         word_index < BRAM_WORD_COUNT;
         ++word_index) {
        const axis_component_t i_word =
            make_i_word(word_index);

        const axis_component_t q_word =
            make_q_word(word_index);

        i_stream.write(i_word);
        q_stream.write(q_word);

        expected[word_index] =
            pack_expected(i_word, q_word);

        capture_bram[word_index] = 0;
    }

    wave_capture(
        i_stream,
        q_stream,
        capture_bram,
        captured_words
    );

    int mismatch_count = 0;

    if (captured_words != BRAM_WORD_COUNT) {
        std::cout
            << "captured_words mismatch: expected "
            << BRAM_WORD_COUNT
            << ", got "
            << captured_words
            << std::endl;

        ++mismatch_count;
    }

    if (!i_stream.empty()) {
        std::cout
            << "I stream was not fully consumed."
            << std::endl;

        ++mismatch_count;
    }

    if (!q_stream.empty()) {
        std::cout
            << "Q stream was not fully consumed."
            << std::endl;

        ++mismatch_count;
    }

    for (int word_index = 0;
         word_index < BRAM_WORD_COUNT;
         ++word_index) {
        if (capture_bram[word_index] !=
            expected[word_index]) {
            if (mismatch_count < 10) {
                std::cout
                    << "BRAM mismatch at word "
                    << word_index
                    << std::endl;

                std::cout
                    << "Expected: 0x"
                    << expected[word_index].to_string(16)
                    << std::endl;

                std::cout
                    << "Actual:   0x"
                    << capture_bram[word_index].to_string(16)
                    << std::endl;
            }

            ++mismatch_count;
        }
    }

    std::cout
        << "Mismatch count: "
        << mismatch_count
        << std::endl;

    if (mismatch_count == 0) {
        std::cout
            << "DUAL AXIS WAVE CAPTURE TEST PASSED"
            << std::endl;

        return 0;
    }

    std::cout
        << "DUAL AXIS WAVE CAPTURE TEST FAILED"
        << std::endl;

    return 1;
}