#ifndef WAVE_PLAYER_HPP
#define WAVE_PLAYER_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

// ------------------------------------------------------------
// Waveform and interface parameters
// ------------------------------------------------------------
//
// One complex IQ sample:
//   I = signed 16 bits
//   Q = signed 16 bits
//   Total = 32 bits
//
// One 128-bit BRAM/AXIS word:
//   4 complex IQ samples
//
// One waveform:
//   1024 complex IQ samples
//   256 x 128-bit words
//
// Data clock:
//   245.76 MHz
//
// Complex sample rate:
//   245.76 MHz x 4 samples/clock
//   = 983.04 MSamples/s
// ------------------------------------------------------------

static const int IQ_SAMPLES_PER_WORD     = 4;
static const int IQ_SAMPLES_PER_FRAME    = 1024;
static const int BRAM_WORD_COUNT         = 256;
static const int AXIS_BEATS_PER_FRAME    = 256;

typedef ap_uint<128> bram_word_t;
typedef ap_axiu<128, 0, 0, 0> axis_word_t;

void wave_player(
    const bram_word_t bram[BRAM_WORD_COUNT],
    hls::stream<axis_word_t>& m_axis,
    ap_uint<32> repeat_count,
    ap_uint<32>& frames_completed
);

#endif
