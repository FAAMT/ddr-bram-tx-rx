# Export a fixed XSA, including the project's current implementation bitstream.
# Usage:
#   vivado -mode batch -source export_hw_platform.tcl -tclargs \
#       /path/to/project.xpr /path/to/output.xsa

if {$argc != 2} {
    error "Usage: export_hw_platform.tcl <source.xpr> <output.xsa>"
}

set source_xpr [file normalize [lindex $argv 0]]
set output_xsa [file normalize [lindex $argv 1]]

if {![file exists $source_xpr]} {
    error "Vivado project not found: $source_xpr"
}

file mkdir [file dirname $output_xsa]
open_project $source_xpr
write_hw_platform -fixed -include_bit -force -file $output_xsa
puts "Exported hardware platform to: $output_xsa"
close_project
