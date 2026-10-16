# Stage a complete validated BD source bundle (BD + all per-cell XCI files).
# This is the safe update path after extending the block design.
#
# Usage:
#   vivado -mode batch -source stage_source_bd.tcl -tclargs \
#       build/vivado/rfsoc_waveform_tx_rx.xpr /path/to/new_source_bd
#
# The destination must not already exist.  Compare the staged directory with
# vivado/source_bd/ before intentionally replacing the known-good source.

if {$argc != 2} {
    error "Usage: stage_source_bd.tcl <source.xpr> <new_source_bd_directory>"
}

set source_xpr [file normalize [lindex $argv 0]]
set output_dir [file normalize [lindex $argv 1]]
set bd_name rfsoc_waveform_tx_rx_bd

if {![file exists $source_xpr]} {
    error "Vivado project not found: $source_xpr"
}
if {[file exists $output_dir]} {
    error "Refusing to overwrite existing destination: $output_dir"
}

open_project $source_xpr
set bd_file [get_files -quiet ${bd_name}.bd]
if {[llength $bd_file] != 1} {
    error "Expected exactly one ${bd_name}.bd, found: $bd_file"
}

open_bd_design $bd_file
current_bd_design $bd_name
validate_bd_design
save_bd_design

set source_dir [file dirname [file normalize $bd_file]]
set xci_files [glob -nocomplain -types f "$source_dir/ip/*/*.xci"]
if {[llength $xci_files] == 0} {
    error "No per-cell XCI files found under: $source_dir/ip"
}

file mkdir [file dirname $output_dir]
file copy $source_dir $output_dir

puts "Staged validated BD source at: $output_dir"
puts "Copied [llength $xci_files] per-cell XCI files."
puts "Compare this directory with vivado/source_bd/ before replacing it."
close_project
