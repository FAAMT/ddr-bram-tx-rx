# Design Parameter Summary

## Tools and target

| Item | Fixed value |
|---|---|
| Vivado / Vitis HLS | 2025.1 |
| FPGA | `xczu48dr-ffvg1517-2-e` |
| Board part | `realdigital.org:rfsoc4x2:part0:1.0` |
| Block Design | `rfsoc_waveform_tx_rx_bd` |
| Top | `rfsoc_waveform_tx_rx_bd_wrapper` |
| RFDC IP | `xilinx.com:ip:usp_rf_data_converter:2.6`, revision 11 |

## Data path

```text
PS @ 100 MHz
  -> iq_bram_bridge_tx -> TX BRAM Port A
                            TX BRAM Port B -> wave_player @ 245.76 MHz
                                                   -> RFDC s00_axis -> DAC00

ADC00 -> RFDC m00_axis (I, 64 bit) + m01_axis (Q, 64 bit)
      -> wave_capture @ 245.76 MHz -> RX BRAM Port A
                                           RX BRAM Port B -> iq_bram_bridge_rx
                                                               -> PS @ 100 MHz
```

`wave_capture:1.1` takes four 16-bit I components and four 16-bit Q components from two 64-bit streams and combines them into one 128-bit BRAM word. `wave_player:1.0` performs the reverse operation by sending each 128-bit word to the DAC AXIS interface.

## Clocks and resets

| Domain | Frequency | Source | Primary loads |
|---|---:|---|---|
| PS control | Approximately 100 MHz | Zynq PS `pl_clk0` | SmartConnect, both bridges, RFDC AXI-Lite |
| TX/RX data | 245.76 MHz | External differential 122.88 MHz PL clock through `clk_wiz_245_76` | Player, capture, RFDC AXIS, corresponding BRAM ports |
| ADC RF reference | 491.52 MHz | RFDC dedicated `adc0_clk_0` | ADC tile 0 PLL |
| DAC RF reference | 491.52 MHz | RFDC dedicated `dac0_clk_0` | DAC tile 0 PLL |

The 245.76 MHz data clock is not driven by the RFDC `clk_adc0` or `clk_dac0` outputs; both derived output clocks are currently unconnected. Clock Wizard is configured for a differential 122.880 MHz input and a 245.760 MHz output, with `locked` enabled and no external reset.

Reset topology:

- `proc_sys_reset_0` synchronizes the PS control domain.
- `proc_sys_reset_245` synchronizes the 245.76 MHz domain, with Clock Wizard `locked` driving `dcm_locked`.
- RFDC AXIS, player, and capture use the 245.76 MHz domain reset.
- The bridges, SmartConnect, and RFDC AXI-Lite use the PS-domain reset.

## Custom IP versions

| IP | VLNV | HLS target | Function |
|---|---|---:|---|
| RX IQ BRAM Bridge | `xilinx.com:hls:iq_bram_bridge:1.0` | 100 MHz | AXI-Lite read, write, and clear access to 128-bit BRAM words |
| TX DDR BRAM Bridge extension | `xilinx.com:hls:ddr_bram_bridge:1.0` | 100 MHz | Retains AXI-Lite access and adds a 128-bit AXI4 DDR-to-BRAM reader |
| Wave Player | `nyu.edu:hls:wave_player:1.0` | 245.76 MHz | Repeatedly streams a 256-word BRAM frame to a 128-bit AXIS interface |
| Wave Capture | `xilinx.com:hls:wave_capture:1.1` | 4 ns | Captures two 64-bit I/Q AXIS streams and packs them into BRAM |

DDR BRAM bridge 1.0 is an extension source until it is packaged by the user and
applied to the BD.  The restored golden project continues to use IQ BRAM bridge
1.0 for both instances; `vivado/apply_ddr_to_bram.tcl` replaces TX only.

## SmartConnect

`NUM_SI=1`, `NUM_MI=5`, and `NUM_CLKS=2`:

| Master port | Peripheral |
|---|---|
| M00 | TX IQ bridge |
| M01 | Wave player |
| M02 | RFDC control |
| M03 | RX IQ bridge |
| M04 | Wave capture |

## Board-level ports and XDC

The regular PL XDC constrains only the differential 122.88 MHz reference:

| Port | Pin | I/O standard |
|---|---|---|
| `fpga_refclk_in_clk_p` | AN11 | LVDS |
| `fpga_refclk_in_clk_n` | AP11 | LVDS |

`adc0_clk_0`, `dac0_clk_0`, `sysref_in_0`, `vin0_01_0`, and `vout00_0` are dedicated RFDC resources. Do not add regular PL `PACKAGE_PIN` or `IOSTANDARD` constraints for them.

## Current implementation reference

- Final routed timing: WNS `0.894 ns`, WHS `0.010 ns`; all constraints are met.
- Final DRC contains only the `RTSTAT-10 No routable loads` warning.
- bit SHA-256: `6e351e81fe2637429bd5b9f9350c680cc2dc45b583b9ace4fb724e62ccf28461`
- HWH SHA-256: `bc1d8a7d2eeba953cf72e08df630bb708d429acee04244992719dd1f72b16aff`

These values are for rebuild regression and do not replace on-board RF testing.
