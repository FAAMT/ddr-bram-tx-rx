# PYNQ Deployment

Run this command from the project root:

```bash
make pynq
```

Copy the complete `dist/pynq/` directory to the board, for example to `/home/xilinx/jupyter_notebooks/rfsoc_waveform_tx_rx/`. Start the notebook from that directory, or use:

```python
from pathlib import Path
from rfsoc_sounder import RFSoCSounder

sounder = RFSoCSounder(Path.cwd())
report = sounder.startup()
```

The packaging script requires both the BIT and HWH files. If the HWH contains `ddr_bram_bridge_tx`, it automatically includes the DDR controller and test notebook from `pynq/extensions/ddr/`. DDR-only files are not included with the base overlay.

The PYNQ image must provide RFSoC4x2-compatible versions of `pynq`, `xrfclk`, and `xrfdc`. The Python application also uses `numpy` and `matplotlib`. Before connecting a physical DAC-to-ADC loopback, confirm suitable attenuation and safe ADC input power.

