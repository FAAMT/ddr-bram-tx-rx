#include "ddr_bram_bridge.hpp"

void ddr_bram_bridge(
    iq_sample_t sample0,
    iq_sample_t sample1,
    iq_sample_t sample2,
    iq_sample_t sample3,
    ap_uint<8>  word_index,
    ap_uint<2>  command,
    const bram_word_t ddr[BRAM_WORD_COUNT],
    ap_uint<8>  bram_start_index,
    ap_uint<9>  transfer_word_count,
    iq_sample_t &read0,
    iq_sample_t &read1,
    iq_sample_t &read2,
    iq_sample_t &read3,
    ap_uint<9>  &words_transferred,
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

#pragma HLS INTERFACE mode=m_axi     port=ddr bundle=gmem offset=slave depth=256 max_read_burst_length=256 num_read_outstanding=2 latency=64
#pragma HLS INTERFACE mode=s_axilite port=ddr                 bundle=control offset=0x60
#pragma HLS INTERFACE mode=s_axilite port=bram_start_index    bundle=control offset=0x70
#pragma HLS INTERFACE mode=s_axilite port=transfer_word_count bundle=control offset=0x78
#pragma HLS INTERFACE mode=s_axilite port=words_transferred   bundle=control offset=0x80

#pragma HLS INTERFACE mode=bram port=bram
#pragma HLS INTERFACE mode=s_axilite port=return bundle=control

    // Give output registers deterministic values for write/clear commands.
    read0 = 0;
    read1 = 0;
    read2 = 0;
    read3 = 0;
    words_transferred = 0;

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
    else if (command == CMD_DDR_TO_BRAM) {
        // The BRAM destination is bounded even if software requests too many
        // words.  The DDR pointer itself is supplied through the AXI-Lite
        // control register and must point to a 16-byte-aligned physical buffer.
        const ap_uint<9> available_words =
            BRAM_WORD_COUNT - static_cast<ap_uint<9> >(bram_start_index);

        ap_uint<9> words_to_copy = transfer_word_count;

        if (words_to_copy > available_words) {
            words_to_copy = available_words;
        }

ddr_to_bram:
        for (int offset = 0; offset < words_to_copy; ++offset) {
#pragma HLS PIPELINE II=1
            bram[bram_start_index + offset] = ddr[offset];
        }

        words_transferred = words_to_copy;
    }
}
