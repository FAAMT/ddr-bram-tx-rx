#include "wave_player.hpp"

void wave_player(
    const bram_word_t bram[BRAM_WORD_COUNT],
    hls::stream<axis_word_t>& m_axis,
    ap_uint<32> repeat_count,
    ap_uint<32>& frames_completed
) {
#pragma HLS INTERFACE mode=bram port=bram

#pragma HLS INTERFACE mode=axis \
    port=m_axis \
    register_mode=both

#pragma HLS INTERFACE mode=s_axilite \
    port=repeat_count \
    bundle=control \
    offset=0x10

#pragma HLS INTERFACE mode=s_axilite \
    port=frames_completed \
    bundle=control \
    offset=0x18

#pragma HLS INTERFACE mode=s_axilite \
    port=return \
    bundle=control

    frames_completed = 0;

    // A repeat count of zero produces no AXI stream data.
    if (repeat_count == 0) {
        return;
    }

    // 256 beats per waveform.
    //
    // repeat_count is 32 bits, therefore total_beats requires
    // up to 40 bits:
    //
    // total_beats = repeat_count * 256
    //             = repeat_count << 8
    ap_uint<40> total_beats =
        static_cast<ap_uint<40> >(repeat_count) << 8;

stream_loop:
    for (ap_uint<40> beat_index = 0;
         beat_index < total_beats;
         ++beat_index) {
#pragma HLS PIPELINE II=1

        // Since one frame contains exactly 256 words,
        // the lower eight bits form the cyclic BRAM address.
        ap_uint<8> word_index =
            beat_index.range(7, 0);

        bram_word_t bram_data =
            bram[word_index];

        axis_word_t output_word;

        output_word.data = bram_data;

        // 128 bits = 16 bytes; all bytes are valid.
        output_word.keep = 0xFFFF;
        output_word.strb = 0xFFFF;

        // Assert TLAST on BRAM word 255 of every waveform.
        output_word.last =
            (word_index == BRAM_WORD_COUNT - 1);

        // Blocking stream write.
        //
        // If downstream TREADY is low, the pipeline stalls
        // without advancing to the next BRAM address.
        m_axis.write(output_word);

        if (word_index == BRAM_WORD_COUNT - 1) {
            frames_completed =
                frames_completed + 1;
        }
    }
}
