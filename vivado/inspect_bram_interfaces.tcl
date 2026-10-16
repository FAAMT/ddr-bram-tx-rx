# Diagnostic helper for inspecting BRAM interface metadata.
# Usage: vivado -mode batch -source inspect_bram_interfaces.tcl -tclargs project.xpr

if {$argc != 1} {
    error "Usage: inspect_bram_interfaces.tcl <project.xpr>"
}

open_project [file normalize [lindex $argv 0]]
open_bd_design [get_files rfsoc_waveform_tx_rx_bd.bd]
current_bd_design rfsoc_waveform_tx_rx_bd

foreach pin_name {
    iq_bram_bridge_tx/bram_PORTA
    blk_mem_gen_tx/BRAM_PORTA
    wave_player_0/bram_PORTA
    blk_mem_gen_tx/BRAM_PORTB
    wave_capture_0/capture_bram_PORTA
    blk_mem_gen_rx/BRAM_PORTA
    iq_bram_bridge_rx/bram_PORTA
    blk_mem_gen_rx/BRAM_PORTB
} {
    set pin [get_bd_intf_pins $pin_name]
    puts "==== $pin_name ===="
    report_property -all $pin
}

close_project
