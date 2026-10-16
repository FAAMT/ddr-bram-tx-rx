# Recreate the editable RFSoC TX/RX Vivado project from portable sources.
# Vivado 2025.1 is required.
#
# The validated BD source bundle (BD + per-cell XCI files) is authoritative.
# Vivado write_bd_tcl does not serialize the propagated, read-only BRAM
# interface READ_LATENCY metadata, so rebuilding this design only from the
# exported block_design.tcl would lose the bridge-side latency of zero.
#
# Optional Tcl argument: build directory.  The default is ../build/vivado.

set script_dir [file dirname [file normalize [info script]]]
set root_dir [file normalize "$script_dir/.."]
set project_name rfsoc_waveform_tx_rx
set bd_name rfsoc_waveform_tx_rx_bd
set required_vivado_version 2025.1

set current_vivado_version [version -short]
if {$current_vivado_version ne $required_vivado_version} {
    error "Vivado $required_vivado_version is required; current version is $current_vivado_version"
}

if {$argc > 1} {
    error "Usage: create_project.tcl ?build_directory?"
}

if {$argc == 1} {
    set project_dir [file normalize [lindex $argv 0]]
} else {
    set project_dir [file normalize "$root_dir/build/vivado"]
}

set project_file "$project_dir/${project_name}.xpr"
if {[file exists $project_file]} {
    error "Refusing to overwrite existing project: $project_file\nChoose a new build directory or remove the old build directory intentionally."
}

# Keep the board definition self-contained in this folder.
set_param board.repoPaths [list "$root_dir/board_files"]

set part_name xczu48dr-ffvg1517-2-e
set board_part_name realdigital.org:rfsoc4x2:part0:1.0

if {![llength [get_parts -quiet $part_name]]} {
    error "Required FPGA part is unavailable: $part_name"
}
if {![llength [get_board_parts -quiet $board_part_name]]} {
    error "Bundled RFSoC4x2 board part was not found: $board_part_name"
}

file mkdir $project_dir
create_project $project_name $project_dir -part $part_name
set_property BOARD_PART $board_part_name [current_project]
set_property target_language Verilog [current_project]

set ip_repos [list \
    "$root_dir/ip_repo/iq_bram_bridge_1_0" \
    "$root_dir/ip_repo/wave_player_128_1_0" \
    "$root_dir/ip_repo/wave_capture_128_1_1" \
]

foreach repo $ip_repos {
    if {![file exists "$repo/component.xml"]} {
        error "Incomplete IP repository: $repo"
    }
}

set_property IP_REPO_PATHS $ip_repos [current_project]
update_ip_catalog

foreach required_vlnv {
    xilinx.com:hls:iq_bram_bridge:1.0
    nyu.edu:hls:wave_player:1.0
    xilinx.com:hls:wave_capture:1.1
} {
    if {![llength [get_ipdefs -all -quiet $required_vlnv]]} {
        error "Required custom IP is missing from the catalog: $required_vlnv"
    }
}

set source_bd_dir "$script_dir/source_bd"
set source_bd_file "$source_bd_dir/${bd_name}.bd"
if {![file exists $source_bd_file]} {
    error "Validated block-design source is missing: $source_bd_file"
}

# Preserve the BD-relative XCI layout.  The XCI files carry the propagated
# BRAM interface metadata that Vivado omits from write_bd_tcl output.
set project_bd_dir "$project_dir/${project_name}.srcs/sources_1/bd/${bd_name}"
if {[file exists $project_bd_dir]} {
    error "Refusing to overwrite existing block-design directory: $project_bd_dir"
}
file mkdir [file dirname $project_bd_dir]
file copy -force $source_bd_dir $project_bd_dir
add_files -fileset sources_1 -norecurse "$project_bd_dir/${bd_name}.bd"

set bd_file [get_files -quiet ${bd_name}.bd]
if {[llength $bd_file] != 1} {
    error "Block design was not created: ${bd_name}.bd"
}

open_bd_design $bd_file
current_bd_design $bd_name
validate_bd_design
save_bd_design

add_files -fileset constrs_1 -norecurse "$script_dir/constraints.xdc"
set_property PROCESSING_ORDER EARLY [get_files "$script_dir/constraints.xdc"]

generate_target all $bd_file
set wrapper_file [make_wrapper -files $bd_file -top]
add_files -norecurse $wrapper_file
set_property top ${bd_name}_wrapper [get_filesets sources_1]
update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

source "$script_dir/verify_design.tcl"

puts ""
puts "============================================================"
puts "Project recreated and verified: $project_file"
puts "Block design: $bd_name"
puts "No synthesis or implementation was launched."
puts "============================================================"
