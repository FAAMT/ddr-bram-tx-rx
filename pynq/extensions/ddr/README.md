# DDR-to-TX-BRAM PYNQ Extension

These files apply only to an overlay containing `ddr_bram_bridge_tx` and a connection to PS `S_AXI_HP0_FPD`. Follow the root README to run `make ddr-project` and `make bitstream`, then run `make pynq`. The packaging script detects the DDR design from the HWH file.

`rfsoc_sounder_ddr.py` uses `pynq.allocate()` to create a contiguous DDR buffer, flushes the waveform data from the CPU cache, and lets the HLS AXI master copy it into TX BRAM. Do not use this controller with the base `iq_bram_bridge_tx` overlay.

