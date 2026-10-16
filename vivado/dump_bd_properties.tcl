# Print the properties used by the reproducibility checks.
# Usage: vivado -mode batch -source dump_bd_properties.tcl -tclargs project.xpr

if {$argc != 1} {
    error "Usage: dump_bd_properties.tcl <project.xpr>"
}

open_project [file normalize [lindex $argv 0]]
open_bd_design [get_files rfsoc_waveform_tx_rx_bd.bd]
current_bd_design rfsoc_waveform_tx_rx_bd

puts "---- ADDRESS SEGMENTS ----"
foreach seg [get_bd_addr_segs -of_objects [get_bd_addr_spaces zynq_ultra_ps_e_0/Data]] {
    puts "SEG=$seg NAME=[get_property NAME $seg] OFFSET=[get_property OFFSET $seg] RANGE=[get_property RANGE $seg]"
    foreach prop [list_property $seg] {
        if {[string match *TARGET* $prop] || [string match *ADDRESS* $prop]} {
            puts "  $prop=[get_property -quiet $prop $seg]"
        }
    }
}

puts "---- BRAM CELLS ----"
foreach cell_name {blk_mem_gen_tx blk_mem_gen_rx} {
    set cell [get_bd_cells $cell_name]
    puts "CELL=$cell_name"
    foreach prop {
        CONFIG.Memory_Type
        CONFIG.Write_Depth_A
        CONFIG.Write_Width_A
        CONFIG.Read_Width_A
        CONFIG.Write_Width_B
        CONFIG.Read_Width_B
        CONFIG.Use_Byte_Write_Enable
        CONFIG.Byte_Size
        CONFIG.Operating_Mode_A
        CONFIG.Operating_Mode_B
        CONFIG.Assume_Synchronous_Clk
        CONFIG.READ_LATENCY_A
        CONFIG.READ_LATENCY_B
    } {
        puts "  $prop=[get_property -quiet $prop $cell]"
    }
}

puts "---- BRAM INTERFACES ----"
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
    puts "PIN=$pin_name MEM_SIZE=[get_property -quiet CONFIG.MEM_SIZE $pin] MEM_WIDTH=[get_property -quiet CONFIG.MEM_WIDTH $pin] READ_LATENCY=[get_property -quiet CONFIG.READ_LATENCY $pin]"
}

close_project
