#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
work_root="$project_root/build/hls"

if ! command -v vitis-run >/dev/null 2>&1; then
    echo "ERROR: vitis-run is not on PATH. Source Vitis 2025.1 settings first." >&2
    exit 1
fi

if ! command -v v++ >/dev/null 2>&1; then
    echo "ERROR: v++ is not on PATH. Source Vitis 2025.1 settings first." >&2
    exit 1
fi

vitis_version="$(
    v++ --version 2>&1 |
        awk '$1 == "******" && $2 == "v++" { version = $3; sub(/^v/, "", version); print version }'
)"
if [[ "$vitis_version" != "2025.1" ]]; then
    echo "ERROR: this archive is pinned to Vitis 2025.1; detected '${vitis_version:-unknown}'." >&2
    exit 1
fi

mkdir -p "$work_root" "$project_root/ip_packages"

for design_name in iq_bram_bridge ddr_bram_bridge wave_player_128 wave_capture_128; do
    design_dir="$project_root/hls/$design_name"
    work_dir="$work_root/$design_name"

    echo "[HLS] C simulation: $design_name"
    (
        cd "$design_dir"
        vitis-run --mode hls --csim \
            --config hls_config.cfg \
            --work_dir "$work_dir"
    )

    echo "[HLS] C synthesis: $design_name"
    (
        cd "$design_dir"
        v++ -c --mode hls \
            --config hls_config.cfg \
            --work_dir "$work_dir"
    )

    echo "[HLS] IP package: $design_name"
    (
        cd "$design_dir"
        vitis-run --mode hls --package \
            --config hls_config.cfg \
            --work_dir "$work_dir"
    )
done

echo "HLS packages regenerated under: $project_root/ip_packages"
echo "The checked-in ip_repo remains unchanged; stage regenerated packages before switching the BD to modified IP."
