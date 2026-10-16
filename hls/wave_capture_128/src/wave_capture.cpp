#include "wave_capture.hpp"

void wave_capture(
    hls::stream<axis_component_t>& s_axis_i,
    hls::stream<axis_component_t>& s_axis_q,
    bram_word_t capture_bram[BRAM_WORD_COUNT],
    ap_uint<32>& captured_words
) {
#pragma HLS INTERFACE mode=axis \
    port=s_axis_i \
    register_mode=both

#pragma HLS INTERFACE mode=axis \
    port=s_axis_q \
    register_mode=both

#pragma HLS INTERFACE mode=bram \
    port=capture_bram

#pragma HLS INTERFACE mode=s_axilite \
    port=captured_words \
    bundle=control \
    offset=0x10

#pragma HLS INTERFACE mode=s_axilite \
    port=return \
    bundle=control

    captured_words = 0;

capture_loop:
    for (int word_index = 0;
         word_index < BRAM_WORD_COUNT;
         ++word_index) {
#pragma HLS PIPELINE II=1

        // Each stream supplies four 16-bit samples per clock.
        const axis_component_t i_word = s_axis_i.read();
        const axis_component_t q_word = s_axis_q.read();

        bram_word_t packed_word = 0;

pack_lanes:
        for (int lane = 0;
             lane < IQ_SAMPLES_PER_WORD;
             ++lane) {
#pragma HLS UNROLL

            const ap_uint<16> i_sample =
                i_word.range(lane * 16 + 15, lane * 16);

            const ap_uint<16> q_sample =
                q_word.range(lane * 16 + 15, lane * 16);

            // Each 32-bit IQ sample:
            // bits 15:0  = I
            // bits 31:16 = Q
            packed_word.range(
                lane * 32 + 15,
                lane * 32
            ) = i_sample;

            packed_word.range(
                lane * 32 + 31,
                lane * 32 + 16
            ) = q_sample;
        }

        capture_bram[word_index] = packed_word;
    }

    captured_words = BRAM_WORD_COUNT;
}