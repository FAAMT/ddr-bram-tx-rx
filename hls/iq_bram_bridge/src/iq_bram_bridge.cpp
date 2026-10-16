#include "iq_bram_bridge.hpp"

void iq_bram_bridge(
    iq_sample_t sample0,
    iq_sample_t sample1,
    iq_sample_t sample2,
    iq_sample_t sample3,
    ap_uint<8>  word_index,
    ap_uint<2>  command,
    iq_sample_t &read0,
    iq_sample_t &read1,
    iq_sample_t &read2,
    iq_sample_t &read3,
    bram_word_t bram[BRAM_WORD_COUNT]
) {
#pragma HLS INTERFACE mode=s_axilite port=sample0    bundle=control offset=0x10
#pragma HLS INTERFACE mode=s_axilite port=sample1    bundle=control offset=0x18
#pragma HLS INTERFACE mode=s_axilite port=sample2    bundle=control offset=0x20
#pragma HLS INTERFACE mode=s_axilite port=sample3    bundle=control offset=0x28
#pragma HLS INTERFACE mode=s_axilite port=word_index bundle=control offset=0x30
#pragma HLS INTERFACE mode=s_axilite port=command    bundle=control offset=0x38

#pragma HLS INTERFACE mode=s_axilite port=read0      bundle=control offset=0x40
#pragma HLS INTERFACE mode=s_axilite port=read1      bundle=control offset=0x48
#pragma HLS INTERFACE mode=s_axilite port=read2      bundle=control offset=0x50
#pragma HLS INTERFACE mode=s_axilite port=read3      bundle=control offset=0x58

#pragma HLS INTERFACE mode=bram port=bram
#pragma HLS INTERFACE mode=s_axilite port=return bundle=control

    // Give output registers deterministic values for write/clear commands.
    read0 = 0;
    read1 = 0;
    read2 = 0;
    read3 = 0;

    if (command == CMD_WRITE) {
        bram_word_t packed_word = 0;

        packed_word.range(31, 0)   = sample0;
        packed_word.range(63, 32)  = sample1;
        packed_word.range(95, 64)  = sample2;
        packed_word.range(127, 96) = sample3;

        bram[word_index] = packed_word;
    }
    else if (command == CMD_READ) {
        bram_word_t packed_word = bram[word_index];

        read0 = packed_word.range(31, 0);
        read1 = packed_word.range(63, 32);
        read2 = packed_word.range(95, 64);
        read3 = packed_word.range(127, 96);
    }
    else if (command == CMD_CLEAR) {
clear_bram:
        for (int i = 0; i < BRAM_WORD_COUNT; ++i) {
#pragma HLS PIPELINE II=1
            bram[i] = 0;
        }
    }
}
