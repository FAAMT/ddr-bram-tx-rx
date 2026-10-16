#ifndef DDR_BRAM_BRIDGE_HPP
#define DDR_BRAM_BRIDGE_HPP

#include <ap_int.h>

static const int IQ_SAMPLE_COUNT  = 1024;
static const int SAMPLES_PER_WORD = 4;
static const int BRAM_WORD_COUNT  = 256;

typedef ap_uint<32>  iq_sample_t;
typedef ap_uint<128> bram_word_t;

enum bridge_command_t {
    CMD_WRITE       = 0,
    CMD_READ        = 1,
    CMD_CLEAR       = 2,
    CMD_DDR_TO_BRAM = 3
};

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
);

#endif
