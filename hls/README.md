# HLS Sources

| Directory | Packaged VLNV | Target clock |
|---|---|---:|
| `iq_bram_bridge/` | `xilinx.com:hls:iq_bram_bridge:1.0` | 100 MHz |
| `ddr_bram_bridge/` | `xilinx.com:hls:ddr_bram_bridge:1.0` | 100 MHz |
| `wave_player_128/` | `nyu.edu:hls:wave_player:1.0` | 245.76 MHz |
| `wave_capture_128/` | `xilinx.com:hls:wave_capture:1.1` | 4 ns |

Every `hls_config.cfg` uses relative source and testbench paths, so it remains valid after moving the directory tree. The common entry point is `../scripts/rebuild_hls.sh`. For each of the four designs, the script runs C simulation, `v++ -c --mode hls` C synthesis, and `vitis-run --mode hls --package` IP packaging in sequence. Packaging must not be run before synthesis.

The current capture IP must remain at version 1.1. It provides two 64-bit `s_axis_i/s_axis_q` inputs. The obsolete version 1.0 has a single 128-bit input stream and is incompatible with the current RFDC/BD.

DDR BRAM bridge 1.0 adds a read-only, 128-bit `m_axi_gmem` master and the
`CMD_DDR_TO_BRAM` operation.  The original IQ BRAM bridge 1.0 remains in both
`hls/iq_bram_bridge/` and `ip_repo/` for the RX instance and for rebuilding the
unextended golden design.
After generating the new IP, stage it in a separate repository and use
`vivado/apply_ddr_to_bram.tcl` to replace only the TX instance.
