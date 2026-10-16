#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
stage_root="$project_root/generated_ip_repo"

if [[ -e "$stage_root" ]]; then
    echo "ERROR: refusing to overwrite existing staging directory: $stage_root" >&2
    exit 1
fi

mkdir -p \
    "$stage_root/iq_bram_bridge_1_0" \
    "$stage_root/ddr_bram_bridge_1_0" \
    "$stage_root/wave_player_128_1_0" \
    "$stage_root/wave_capture_128_1_1"

unzip -q "$project_root/ip_packages/iq_bram_bridge_1_0.zip" \
    -d "$stage_root/iq_bram_bridge_1_0"
unzip -q "$project_root/ip_packages/ddr_bram_bridge_1_0.zip" \
    -d "$stage_root/ddr_bram_bridge_1_0"
unzip -q "$project_root/ip_packages/wave_player_128_1_0.zip" \
    -d "$stage_root/wave_player_128_1_0"
unzip -q "$project_root/ip_packages/wave_capture_128_1_1.zip" \
    -d "$stage_root/wave_capture_128_1_1"

echo "Regenerated IP repositories staged at: $stage_root"
echo "Compare them with ip_repo/ before intentionally replacing the known-good IP."
