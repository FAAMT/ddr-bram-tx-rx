# RFSoC Waveform TX/RX Rebuildable Project

This is a relocatable and version-control-friendly RFSoC4x2 project intended for continued development. Source files and generated artifacts are kept separate; do not commit Vivado or Vitis working directories.

## Project scope

- Tool version: Vivado/Vitis 2025.1
- FPGA: `xczu48dr-ffvg1517-2-e`
- Board: `realdigital.org:rfsoc4x2:part0:1.0`
- Base data path: PS AXI-Lite → TX BRAM → DAC, and ADC → RX BRAM → PS
- Optional extension: PS DDR → `S_AXI_HP0_FPD` → TX BRAM

The BD, BDA, and 12 cell XCI files in `vivado/source_bd/` are the authoritative rebuild source for the base design. `vivado/block_design.tcl` is provided only as an inspection and editing reference; it cannot replace the complete BD bundle. The BD Tcl exported by Vivado 2025.1 does not preserve the propagated BRAM `READ_LATENCY` metadata required by this design.

## Directory layout

```text
board_files/       Bundled RFSoC4x2 board part
hls/               Four Vitis HLS IPs, each with src/, tb/, and hls_config.cfg
ip_packages/       Packaged IP ZIP files
ip_repo/           IP repositories ready for registration in Vivado
vivado/            Project, BD, constraints, verification, and bitstream Tcl
pynq/              PYNQ controllers, notebooks, and optional DDR extension
scripts/           HLS, structural validation, IP staging, and PYNQ packaging
docs/              Parameters, addresses, RFDC, validation, and extension notes
reference/         Known-good base BIT/HWH/XSA files and implementation reports
build/             Generated Vivado/Vitis work directories (ignored)
output/            Newly generated BIT/HWH/XSA files (ignored)
dist/              Bundles ready to copy to a PYNQ board (ignored)
```

## Quick start

Load the 2025.1 tool environment first:

```bash
source ~/Xilinx/2025.1/Vivado/settings64.sh
source ~/Xilinx/2025.1/Vitis/settings64.sh
cd rfsoc_waveform_tx_rx_rebuildable
make check
```

Recreate and open the base Vivado project:

```bash
make vivado-project
vivado build/vivado/rfsoc_waveform_tx_rx.xpr
```

Generate the bitstream, HWH, and XSA, then package the PYNQ overlay:

```bash
make bitstream JOBS=8
make pynq
```

Generated artifacts:

- `output/rfsoc_waveform_tx_rx.bit`
- `output/rfsoc_waveform_tx_rx.hwh`
- `output/rfsoc_waveform_tx_rx.xsa`
- `dist/pynq/`

If the current machine does not have an implementation license for `xczu48dr`, package the known-good PYNQ overlay instead:

```bash
make pynq-golden
```

## Rebuild the HLS IP

```bash
make hls
make stage-ip
```

The HLS flow runs C simulation, C synthesis, and IP packaging in sequence. New IP is first expanded into `generated_ip_repo/`; it does not automatically overwrite the validated `ip_repo/`. Compare the interfaces, VLNVs, and RTL before deliberately switching the BD to a regenerated IP repository.

## Apply the DDR-to-BRAM extension

```bash
make vivado-project
make ddr-project
make bitstream
make pynq
```

By default, `make ddr-project` uses the bundled `ip_repo/ddr_bram_bridge_1_0`, replaces the TX bridge in the generated Vivado project, and connects PS HP0. After modifying the HLS source, run `make hls && make stage-ip`, then select the regenerated repository explicitly:

```bash
make ddr-project DDR_IP_REPO=generated_ip_repo/ddr_bram_bridge_1_0
```

See [docs/DDR_TO_BRAM_EXTENSION.md](docs/DDR_TO_BRAM_EXTENSION.md) for register definitions, cache-flush requirements, and address rules.

## Extension guidelines

1. Modify HLS code only under `hls/<ip>/src`, `tb`, and the corresponding configuration. Run C simulation before updating an IP package.
2. Modify the BD in a recreated Vivado project. After validation, use `vivado/stage_source_bd.tcl` to export the complete BD and XCI bundle.
3. Keep constraints in `vivado/constraints.xdc`; add any new constraints explicitly in `create_project.tcl`.
4. Do not hard-code development-machine paths in PYNQ code. BIT and HWH files must share a base name and come from the same implementation.
5. When extending addresses, BRAM depth, clocks, or RFDC settings, update both `vivado/verify_design.tcl` and the relevant documentation.

Before running a hardware loopback, confirm that the DAC-to-ADC path has suitable attenuation. Static BD validation does not replace on-board validation of clocks, RFDC behavior, and the analog signal chain.

