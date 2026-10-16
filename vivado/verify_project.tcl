# Open an existing project and run the reproducibility assertions.
# Usage: vivado -mode batch -source verify_project.tcl -tclargs project.xpr

if {$argc != 1} {
    error "Usage: verify_project.tcl <project.xpr>"
}

set script_dir [file dirname [file normalize [info script]]]
set project_file [file normalize [lindex $argv 0]]

if {![file exists $project_file]} {
    error "Project not found: $project_file"
}

open_project $project_file
source "$script_dir/verify_design.tcl"
close_project
