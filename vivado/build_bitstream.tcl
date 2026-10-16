# Build the recreated project and export PYNQ/Vitis artifacts.
# Usage: vivado -mode batch -source build_bitstream.tcl -tclargs ?project.xpr? ?jobs?

set script_dir [file dirname [file normalize [info script]]]
set root_dir [file normalize "$script_dir/.."]
set project_name rfsoc_waveform_tx_rx
set bd_name rfsoc_waveform_tx_rx_bd

if {$argc > 2} {
    error "Usage: build_bitstream.tcl ?project.xpr? ?jobs?"
}

if {$argc >= 1} {
    set project_file [file normalize [lindex $argv 0]]
} else {
    set project_file [file normalize "$root_dir/build/vivado/${project_name}.xpr"]
}

if {$argc == 2} {
    set jobs [lindex $argv 1]
} else {
    set jobs 8
}

if {![file exists $project_file]} {
    error "Project not found: $project_file"
}

open_project $project_file
source "$script_dir/verify_design.tcl"

launch_runs impl_1 -to_step write_bitstream -jobs $jobs
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
    error "Implementation did not complete: [get_property STATUS [get_runs impl_1]]"
}

open_run impl_1
set output_dir [file normalize "$root_dir/output"]
file mkdir $output_dir

report_timing_summary -delay_type min_max -report_unconstrained \
    -check_timing_verbose -max_paths 10 \
    -file "$output_dir/timing_summary.rpt"
report_drc -file "$output_dir/drc.rpt"

set project_dir [get_property DIRECTORY [current_project]]
set bit_source "$project_dir/${project_name}.runs/impl_1/${bd_name}_wrapper.bit"
set hwh_source "$project_dir/${project_name}.gen/sources_1/bd/$bd_name/hw_handoff/${bd_name}.hwh"

if {![file exists $bit_source]} {
    error "Bitstream was not generated at expected path: $bit_source"
}
if {![file exists $hwh_source]} {
    error "HWH was not generated at expected path: $hwh_source"
}

file copy -force $bit_source "$output_dir/${project_name}.bit"
file copy -force $hwh_source "$output_dir/${project_name}.hwh"
write_hw_platform -fixed -include_bit -force \
    -file "$output_dir/${project_name}.xsa"

puts "Build artifacts written to: $output_dir"
close_project
