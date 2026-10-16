# RF Data Converter Configuration

This document summarizes the critical RFDC 2.6 parameters in the final implementation and explains the GUI enum meanings to prevent future misinterpretation of their integer values. The complete machine-readable source of truth is `vivado/source_bd/ip/rfsoc_waveform_tx_rx_bd_usp_rf_data_converter_0_0/rfsoc_waveform_tx_rx_bd_usp_rf_data_converter_0_0.xci`. Use that XCI together with `verify_design.tcl` when rebuilding.

## ADC tile 0 / slice 00

| Property | Value | Meaning |
|---|---:|---|
| `ADC0_PLL_Enable` | true | Tile 0 PLL enabled |
| `ADC0_Sampling_Rate` | 3.93216 | GSPS |
| `ADC0_Refclk_Freq` | 491.520 | MHz external RF reference clock |
| `ADC0_Outclk_Freq` | 30.720 | MHz derived output clock; currently unconnected |
| `ADC0_Fabric_Freq` | 245.760 | MHz AXIS fabric rate |
| `ADC0_Multi_Tile_Sync` | false | MTS disabled |
| `ADC_Slice00_Enable` | true | Slice 00 enabled |
| `ADC_Slice01_Enable` | true (derived) | Slice 01 is derived-enabled by I/Q pairing |
| `ADC_Data_Type00` | 1 | I/Q data |
| `ADC_Data_Width00` | 4 | Four 16-bit samples per I/Q component per fabric clock |
| `ADC_Decimation_Mode00` | 4 | 4× decimation |
| `ADC_CalOpt_Mode00` | 1 | Calibration Mode 2 |
| `ADC_Mixer_Type00` | 2 | Fine mixer |
| `ADC_Mixer_Mode00` | 0 | Real → I/Q |
| `ADC_NCO_Freq00` | -1.0 | GHz |
| `ADC_Nyquist00` | 0 | Nyquist Zone 1 |

ADC output interfaces:

- `m00_axis`: 64 bit, connected to `wave_capture_0/s_axis_i`
- `m01_axis`: 64 bit, connected to `wave_capture_0/s_axis_q`
- Both streams use the same 245.76 MHz `m0_axis_aclk` and reset.

## DAC tile 0 / slice 00

| Property | Value | Meaning |
|---|---:|---|
| `DAC0_PLL_Enable` | true | Tile 0 PLL enabled |
| `DAC0_Sampling_Rate` | 3.93216 | GSPS |
| `DAC0_Refclk_Freq` | 491.520 | MHz external RF reference clock |
| `DAC0_Outclk_Freq` | 122.880 | MHz derived output clock; currently unconnected |
| `DAC0_Fabric_Freq` | 245.760 | MHz AXIS fabric rate |
| `DAC0_Multi_Tile_Sync` | false | MTS disabled |
| `DAC_Slice00_Enable` | true | Slice 00 enabled |
| `DAC_Data_Type00` | 0 | Real output after the mixer |
| `DAC_Data_Width00` | 8 | The 128-bit input contains eight 16-bit I/Q components |
| `DAC_Interpolation_Mode00` | 4 | 4× interpolation |
| `DAC_Mixer_Type00` | 2 | Fine mixer |
| `DAC_Mixer_Mode00` | 0 | I/Q → Real |
| `DAC_NCO_Freq00` | +1.0 | GHz |
| `DAC_Nyquist00` | 0 | Nyquist Zone 1 |

The DAC `s00_axis` input is 128 bits wide, is connected to `wave_player_0/m_axis`, and uses the 245.76 MHz `s0_axis_aclk`.

## Clock architecture

```text
PL 122.88 MHz differential reference clock
  -> clk_wiz_245_76
  -> 245.76 MHz player/capture/RFDC AXIS clock

ADC dedicated 491.52 MHz reference -> ADC tile 0 PLL -> 3.93216 GSPS
DAC dedicated 491.52 MHz reference -> DAC tile 0 PLL -> 3.93216 GSPS
```

The RFDC-derived `clk_adc0` and `clk_dac0` output clocks are not connected to the data path. `sysref_in_0` is exported to the top level, but MTS is disabled for both ADC and DAC.

After 4× decimation/interpolation, the complex sample rate is `983.04 MSamples/s`. At 245.76 MHz, each fabric clock processes four complex samples, matching one 128-bit BRAM word.

## Top-level dedicated RFDC ports

- ADC analog input: `vin0_01_0`
- DAC analog output: `vout00_0`
- ADC RF clock: `adc0_clk_0`, 491.52 MHz
- DAC RF clock: `dac0_clk_0`, 491.52 MHz
- SYSREF: `sysref_in_0`

These ports use dedicated RFDC resources and must not receive regular PL pin XDC constraints.

## Required on-board confirmation

The current bit/HWH proves that these parameters were implemented and routed, but the archive does not contain a complete known-tone or analog-loopback test record. Before adopting the design as a new product baseline, read back the runtime RFDC configuration and use a digital pattern or known tone to confirm I/Q lane assignment, sign, NCO direction, Nyquist zone, amplitude, and frequency.
