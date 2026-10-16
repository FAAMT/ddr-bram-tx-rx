#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
artifact_dir="${1:-output}"
destination="${2:-dist/pynq}"

if [[ "$artifact_dir" != /* ]]; then
    artifact_dir="$project_root/$artifact_dir"
fi
if [[ "$destination" != /* ]]; then
    destination="$project_root/$destination"
fi

overlay_name=rfsoc_waveform_tx_rx
bit_file="$artifact_dir/$overlay_name.bit"
hwh_file="$artifact_dir/$overlay_name.hwh"

for source_file in "$bit_file" "$hwh_file"; do
    if [[ ! -f "$source_file" ]]; then
        echo "ERROR: missing overlay artifact: $source_file" >&2
        exit 1
    fi
done

mkdir -p "$destination"
cp -f "$bit_file" "$hwh_file" "$destination/"
cp -f \
    "$project_root/pynq/src/rfsoc_sounder.py" \
    "$project_root/pynq/src/zadoff_chu_bram_signal.py" \
    "$project_root/pynq/notebooks/rfsoc_sounder_class_demo.ipynb" \
    "$project_root/pynq/README.md" \
    "$destination/"

if grep -q 'ddr_bram_bridge_tx' "$hwh_file"; then
    cp -f \
        "$project_root/pynq/extensions/ddr/rfsoc_sounder_ddr.py" \
        "$project_root/pynq/extensions/ddr/zadoff_chu_ddr_loopback.py" \
        "$project_root/pynq/extensions/ddr/zadoff_chu_ddr_loopback_test.ipynb" \
        "$destination/"
    echo "Detected DDR bridge in HWH; included DDR PYNQ extension."
else
    echo "Base overlay detected; DDR-only PYNQ files were not included."
fi

echo "PYNQ bundle ready: $destination"

