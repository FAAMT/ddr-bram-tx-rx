#ifndef WAVE_CAPTURE_HPP
#define WAVE_CAPTURE_HPP

#include <ap_int.h>
#include <hls_stream.h>

static const int IQ_SAMPLES_PER_WORD = 4;
static const int IQ_SAMPLES_PER_FRAME = 1024;
static const int BRAM_WORD_COUNT =
    IQ_SAMPLES_PER_FRAME / IQ_SAMPLES_PER_WORD;

// RFDC Real->I/Q mode:
// One 64-bit stream contains four 16-bit I samples.
// The other contains four 16-bit Q samples.
using axis_component_t = ap_uint<64>;

// RX BRAM keeps the same format as the TX/RX bridge:
// {Q3,I3,Q2,I2,Q1,I1,Q0,I0}
using bram_word_t = ap_uint<128>;

void wave_capture(
    hls::stream<axis_component_t>& s_axis_i,
    hls::stream<axis_component_t>& s_axis_q,
    bram_word_t capture_bram[BRAM_WORD_COUNT],
    ap_uint<32>& captured_words
);

#endif