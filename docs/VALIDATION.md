# Template Validation Record

Latest validation date: 2026-08-11. Tool version: Vivado/Vitis HLS 2025.1. Target device: `xczu48dr-ffvg1517-2-e`.

The results below describe the original bridge 1.0 golden design.

## DDR extension status (2026-08-11)

- Added `ddr_bram_bridge:1.0` HLS source and testbench coverage for a bounded 128-bit
  DDR-to-BRAM copy command.
- Added a Vivado integration script for the TX DDR BRAM bridge, a dedicated DDR
  SmartConnect, and PS `S_AXI_HP0_FPD`.
- Ran the `ddr_bram_bridge:1.0` C testbench with Vitis HLS 2025.1; it completed
  with zero mismatches.
- Applied the packaged DDR bridge to a freshly recreated project.  Vivado
  validated the BD, regenerated output products, and passed all DDR-path
  assertions for HP0 width, SmartConnect, clocks, reset, AXI-Lite mapping, and
  the low 2 GiB DDR aperture.
- Fixed the integration script to rename the preserved TX AXI-Lite master
  segment when replacing the cell; this keeps the generated DDR project and
  its verification assertions consistent.

## Passed

- Ran `vivado/create_project.tcl` from a clean temporary path: project creation, complete BD-source copy and open, BD validation, generation of all output products, and HDL-wrapper generation all succeeded.
- Ran `vivado/verify_design.tcl` on the rebuilt project: all assertions passed for BRAM geometry/mode/physical latency, HLS BRAM interface latency, the five AXI-Lite address segments, critical RFDC parameters, clock frequencies, and critical connections.
- Ran `vivado/stage_source_bd.tcl`: successfully staged one complete BD plus 12 cell XCI files from the project, confirming that future design extensions can update the authoritative source with the same procedure.
- All four HLS designs completed C simulation in clean temporary working directories, with zero testbench mismatches.
- All three HLS designs completed `v++ -c --mode hls` C synthesis followed by `vitis-run --mode hls --package`, producing IP ZIP files with the expected VLNVs.
- SHA-256 hashes of the bit/HWH embedded in the golden XSA exactly match the standalone files in `reference/known_good/`.
- The routed timing report from the original final implementation shows that setup and hold timing are met: WNS `0.894 ns`, WHS `0.010 ns`. Routed DRC contains only the `RTSTAT-10` routing-status warning.

## Not completed in the current environment

Full synthesis/implementation was not repeated on 2026-08-11.  The earlier attempt reported `Common 17-345`: this machine does not have a valid synthesis license covering `xczu48dr`. Therefore, a second bitstream was not regenerated from the new project. This is a license limitation of the current machine, not a BD-validation or parameter-assertion failure.

The template contains the bit/HWH from the original final implementation, a matching XSA, and its routed timing, DRC, and route-status reports. After obtaining the required license, run:

```bash
vivado -mode batch -source vivado/build_bitstream.tcl
```

Compare the new outputs with the golden artifacts, then complete the README's board-level checks using RFDC digital test mode, a known tone, and a safe-power loopback. Static configuration reproducibility does not replace measurement of the analog front end and on-board clocks.
