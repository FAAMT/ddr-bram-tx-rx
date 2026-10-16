#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"

required_dirs=(
    board_files/rfsoc4x2/1.0
    hls/iq_bram_bridge/src
    hls/iq_bram_bridge/tb
    hls/ddr_bram_bridge/src
    hls/ddr_bram_bridge/tb
    hls/wave_player_128/src
    hls/wave_player_128/tb
    hls/wave_capture_128/src
    hls/wave_capture_128/tb
    ip_repo/iq_bram_bridge_1_0
    ip_repo/ddr_bram_bridge_1_0
    ip_repo/wave_player_128_1_0
    ip_repo/wave_capture_128_1_1
    vivado/source_bd
    pynq/src
)

required_files=(
    vivado/source_bd/rfsoc_waveform_tx_rx_bd.bd
    vivado/create_project.tcl
    vivado/build_bitstream.tcl
    vivado/verify_design.tcl
    vivado/constraints.xdc
    pynq/src/rfsoc_sounder.py
    pynq/src/zadoff_chu_bram_signal.py
)

for relative_path in "${required_dirs[@]}"; do
    if [[ ! -d "$project_root/$relative_path" ]]; then
        echo "ERROR: missing directory: $relative_path" >&2
        exit 1
    fi
done

for relative_path in "${required_files[@]}"; do
    if [[ ! -f "$project_root/$relative_path" ]]; then
        echo "ERROR: missing file: $relative_path" >&2
        exit 1
    fi
done

for component in "$project_root"/ip_repo/*/component.xml; do
    [[ -f "$component" ]] || {
        echo "ERROR: incomplete IP repository entry: $component" >&2
        exit 1
    }
done

xci_count="$(find "$project_root/vivado/source_bd/ip" -type f -name '*.xci' | wc -l)"
if [[ "$xci_count" -ne 12 ]]; then
    echo "ERROR: expected 12 BD cell XCI files, found $xci_count" >&2
    exit 1
fi

python3 -c '
import pathlib
import sys

for filename in sys.argv[1:]:
    path = pathlib.Path(filename)
    compile(path.read_text(encoding="utf-8"), str(path), "exec")
' \
    "$project_root/pynq/src/rfsoc_sounder.py" \
    "$project_root/pynq/src/zadoff_chu_bram_signal.py" \
    "$project_root/pynq/extensions/ddr/rfsoc_sounder_ddr.py" \
    "$project_root/pynq/extensions/ddr/zadoff_chu_ddr_loopback.py"

echo "Project structure OK: $project_root"
echo "Validated four HLS src/tb trees, four IP repositories, and 12 BD XCI files."
