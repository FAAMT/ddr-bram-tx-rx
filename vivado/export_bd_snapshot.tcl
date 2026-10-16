# Export a human-readable Tcl snapshot from an existing Vivado project.
# This file is useful for review, but it is not a complete reconstruction
# source because Vivado omits propagated BRAM READ_LATENCY metadata.  Use
# stage_source_bd.tcl to preserve the authoritative BD + XCI bundle.
# Usage:
#   vivado -mode batch -source export_bd_snapshot.tcl -tclargs \
#       /path/to/project.xpr /path/to/output_bd.tcl

if {$argc != 2} {
    error "Usage: export_bd_snapshot.tcl <source.xpr> <output_bd.tcl>"
}

set source_xpr [file normalize [lindex $argv 0]]
set output_tcl [file normalize [lindex $argv 1]]
set bd_name rfsoc_waveform_tx_rx_bd

if {![file exists $source_xpr]} {
    error "Vivado project not found: $source_xpr"
}

file mkdir [file dirname $output_tcl]
open_project $source_xpr

set bd_file [get_files -quiet ${bd_name}.bd]
if {[llength $bd_file] != 1} {
    error "Expected exactly one ${bd_name}.bd, found: $bd_file"
}

open_bd_design $bd_file
current_bd_design $bd_name
validate_bd_design
write_bd_tcl -force $output_tcl

puts "Exported validated block design to: $output_tcl"
close_project
