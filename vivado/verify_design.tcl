# Assert the parameters that are easy to lose when a BD is regenerated.
# Source this file with the project and rfsoc_waveform_tx_rx_bd open.

proc require_one {label objects} {
    if {[llength $objects] != 1} {
        error "VERIFY FAILED: expected one $label, found: $objects"
    }
    return $objects
}

proc expect_property {label object property expected} {
    set actual [get_property -quiet $property $object]
    if {$actual ne $expected} {
        error "VERIFY FAILED: $label $property expected '$expected', got '$actual'"
    }
    puts "PASS: $label $property = $actual"
}

proc expect_same_intf_net {pin_a_name pin_b_name} {
    set pin_a [require_one "interface pin $pin_a_name" [get_bd_intf_pins -quiet $pin_a_name]]
    set pin_b [require_one "interface pin $pin_b_name" [get_bd_intf_pins -quiet $pin_b_name]]
    set net_a [require_one "interface net at $pin_a_name" [get_bd_intf_nets -quiet -of_objects $pin_a]]
    set net_b [require_one "interface net at $pin_b_name" [get_bd_intf_nets -quiet -of_objects $pin_b]]
    if {$net_a ne $net_b} {
        error "VERIFY FAILED: $pin_a_name and $pin_b_name are not on the same interface net"
    }
    puts "PASS: $pin_a_name <-> $pin_b_name"
}

proc expect_intf_port_pin {port_name pin_name} {
    set port [require_one "interface port $port_name" [get_bd_intf_ports -quiet $port_name]]
    set pin [require_one "interface pin $pin_name" [get_bd_intf_pins -quiet $pin_name]]
    set port_net [require_one "interface net at port $port_name" [get_bd_intf_nets -quiet -of_objects $port]]
    set pin_net [require_one "interface net at pin $pin_name" [get_bd_intf_nets -quiet -of_objects $pin]]
    if {$port_net ne $pin_net} {
        error "VERIFY FAILED: interface port $port_name is not connected to $pin_name"
    }
    puts "PASS: $port_name <-> $pin_name"
}

proc expect_same_net {pin_a_name pin_b_name} {
    set pin_a [require_one "pin $pin_a_name" [get_bd_pins -quiet $pin_a_name]]
    set pin_b [require_one "pin $pin_b_name" [get_bd_pins -quiet $pin_b_name]]
    set net_a [require_one "net at $pin_a_name" [get_bd_nets -quiet -of_objects $pin_a]]
    set net_b [require_one "net at $pin_b_name" [get_bd_nets -quiet -of_objects $pin_b]]
    if {$net_a ne $net_b} {
        error "VERIFY FAILED: $pin_a_name and $pin_b_name are not on the same net"
    }
    puts "PASS: $pin_a_name <-> $pin_b_name"
}

proc expect_address {segment_name expected_offset expected_range} {
    set path "/zynq_ultra_ps_e_0/Data/$segment_name"
    set seg [require_one "address segment $path" [get_bd_addr_segs -quiet $path]]
    expect_property $segment_name $seg OFFSET $expected_offset
    expect_property $segment_name $seg RANGE $expected_range
}

set project [current_project]
if {$project eq ""} {
    error "VERIFY FAILED: no project is open"
}

expect_property project $project PART xczu48dr-ffvg1517-2-e
expect_property project $project BOARD_PART realdigital.org:rfsoc4x2:part0:1.0

set bd_name rfsoc_waveform_tx_rx_bd
set bd_file [require_one "$bd_name.bd" [get_files -quiet ${bd_name}.bd]]
open_bd_design $bd_file
current_bd_design $bd_name
validate_bd_design

set legacy_tx_bridge [get_bd_cells -quiet iq_bram_bridge_tx]
set ddr_tx_bridge [get_bd_cells -quiet ddr_bram_bridge_tx]

if {[llength $legacy_tx_bridge] == 1 && [llength $ddr_tx_bridge] == 0} {
    set tx_bridge $legacy_tx_bridge
    set tx_bridge_name iq_bram_bridge_tx
    set tx_bridge_vlnv xilinx.com:hls:iq_bram_bridge:1.0
    set ddr_extension_enabled 0
} elseif {[llength $legacy_tx_bridge] == 0 && [llength $ddr_tx_bridge] == 1} {
    set tx_bridge $ddr_tx_bridge
    set tx_bridge_name ddr_bram_bridge_tx
    set tx_bridge_vlnv xilinx.com:hls:ddr_bram_bridge:1.0
    set ddr_extension_enabled 1
} else {
    error "VERIFY FAILED: expected exactly one supported TX bridge"
}
expect_property $tx_bridge_name $tx_bridge VLNV $tx_bridge_vlnv

foreach {cell_name expected_vlnv} {
    iq_bram_bridge_rx xilinx.com:hls:iq_bram_bridge:1.0
    wave_player_0 nyu.edu:hls:wave_player:1.0
    wave_capture_0 xilinx.com:hls:wave_capture:1.1
    usp_rf_data_converter_0 xilinx.com:ip:usp_rf_data_converter:2.6
} {
    set cell [require_one "cell $cell_name" [get_bd_cells -quiet $cell_name]]
    expect_property $cell_name $cell VLNV $expected_vlnv
}

foreach bram_name {blk_mem_gen_tx blk_mem_gen_rx} {
    set bram [require_one "cell $bram_name" [get_bd_cells -quiet $bram_name]]
    foreach {property expected} {
        CONFIG.Memory_Type True_Dual_Port_RAM
        CONFIG.Write_Depth_A 256
        CONFIG.Write_Width_A 128
        CONFIG.Read_Width_A 128
        CONFIG.Write_Width_B 128
        CONFIG.Read_Width_B 128
        CONFIG.Use_Byte_Write_Enable true
        CONFIG.Byte_Size 8
        CONFIG.Operating_Mode_A WRITE_FIRST
        CONFIG.Operating_Mode_B WRITE_FIRST
        CONFIG.Assume_Synchronous_Clk false
        CONFIG.READ_LATENCY_A 1
        CONFIG.READ_LATENCY_B 1
        CONFIG.Register_PortA_Output_of_Memory_Primitives false
        CONFIG.Register_PortA_Output_of_Memory_Core false
        CONFIG.Register_PortB_Output_of_Memory_Primitives false
        CONFIG.Register_PortB_Output_of_Memory_Core false
    } {
        expect_property $bram_name $bram $property $expected
    }
}

# Interface latency is propagated from the connected HLS IP.  It is distinct
# from the one-cycle physical BMG read latency asserted above.
set bram_interface_checks [list \
    ${tx_bridge_name}/bram_PORTA 4096 128 0 \
    blk_mem_gen_tx/BRAM_PORTA 4096 128 0 \
    wave_player_0/bram_PORTA 4096 128 1 \
    blk_mem_gen_tx/BRAM_PORTB 4096 128 1 \
    wave_capture_0/capture_bram_PORTA 4096 128 1 \
    blk_mem_gen_rx/BRAM_PORTA 4096 128 1 \
    iq_bram_bridge_rx/bram_PORTA 4096 128 0 \
    blk_mem_gen_rx/BRAM_PORTB 4096 128 0 \
]
foreach {pin_name mem_size mem_width read_latency} $bram_interface_checks {
    set pin [require_one "interface pin $pin_name" [get_bd_intf_pins -quiet $pin_name]]
    expect_property $pin_name $pin CONFIG.MEM_SIZE $mem_size
    expect_property $pin_name $pin CONFIG.MEM_WIDTH $mem_width
    expect_property $pin_name $pin CONFIG.MEM_ADDRESS_MODE BYTE_ADDRESS
    expect_property $pin_name $pin CONFIG.READ_LATENCY $read_latency
}

set rfdc [require_one "RFDC cell" [get_bd_cells -quiet usp_rf_data_converter_0]]
foreach {property expected} {
    CONFIG.ADC0_PLL_Enable true
    CONFIG.ADC0_Sampling_Rate 3.93216
    CONFIG.ADC0_Refclk_Freq 491.520
    CONFIG.ADC0_Outclk_Freq 30.720
    CONFIG.ADC0_Fabric_Freq 245.760
    CONFIG.ADC0_Multi_Tile_Sync false
    CONFIG.ADC_Slice00_Enable true
    CONFIG.ADC_Slice01_Enable true
    CONFIG.ADC_Slice10_Enable false
    CONFIG.ADC_Data_Type00 1
    CONFIG.ADC_Data_Width00 4
    CONFIG.ADC_Decimation_Mode00 4
    CONFIG.ADC_CalOpt_Mode00 1
    CONFIG.ADC_Mixer_Type00 2
    CONFIG.ADC_Mixer_Mode00 0
    CONFIG.ADC_NCO_Freq00 -1.0
    CONFIG.ADC_Nyquist00 0
    CONFIG.DAC0_PLL_Enable true
    CONFIG.DAC0_Sampling_Rate 3.93216
    CONFIG.DAC0_Refclk_Freq 491.520
    CONFIG.DAC0_Outclk_Freq 122.880
    CONFIG.DAC0_Fabric_Freq 245.760
    CONFIG.DAC0_Multi_Tile_Sync false
    CONFIG.DAC_Slice00_Enable true
    CONFIG.DAC_Data_Type00 0
    CONFIG.DAC_Data_Width00 8
    CONFIG.DAC_Interpolation_Mode00 4
    CONFIG.DAC_Mixer_Type00 2
    CONFIG.DAC_Mixer_Mode00 0
    CONFIG.DAC_NCO_Freq00 1.0
    CONFIG.DAC_Nyquist00 0
} {
    expect_property RFDC $rfdc $property $expected
}

set clk_wiz [require_one "clock wizard" [get_bd_cells -quiet clk_wiz_245_76]]
expect_property clk_wiz_245_76 $clk_wiz CONFIG.PRIM_IN_FREQ 122.880
expect_property clk_wiz_245_76 $clk_wiz CONFIG.CLKOUT1_REQUESTED_OUT_FREQ 245.760

foreach {port_name expected_freq} {
    fpga_refclk_in 122880000
    adc0_clk_0 491520000
    dac0_clk_0 491520000
} {
    set port [require_one "interface port $port_name" [get_bd_intf_ports -quiet $port_name]]
    expect_property $port_name $port CONFIG.FREQ_HZ $expected_freq
}

if {$ddr_extension_enabled} {
    expect_address SEG_ddr_bram_bridge_tx_Reg 0x00A0000000 0x0000001000
} else {
    expect_address SEG_iq_bram_bridge_0_Reg 0x00A0000000 0x0000001000
}
expect_address SEG_wave_player_0_Reg 0x00A0010000 0x0000001000
expect_address SEG_iq_bram_bridge_rx_Reg 0x00A0020000 0x0000001000
expect_address SEG_wave_capture_0_Reg 0x00A0030000 0x0000001000
expect_address SEG_usp_rf_data_converter_0_Reg 0x00A0040000 0x0000040000

expect_same_intf_net ${tx_bridge_name}/s_axi_control smartconnect_0/M00_AXI
expect_same_intf_net ${tx_bridge_name}/bram_PORTA blk_mem_gen_tx/BRAM_PORTA
expect_same_intf_net wave_player_0/bram_PORTA blk_mem_gen_tx/BRAM_PORTB
expect_same_intf_net wave_capture_0/capture_bram_PORTA blk_mem_gen_rx/BRAM_PORTA
expect_same_intf_net iq_bram_bridge_rx/bram_PORTA blk_mem_gen_rx/BRAM_PORTB
expect_same_intf_net usp_rf_data_converter_0/m00_axis wave_capture_0/s_axis_i
expect_same_intf_net usp_rf_data_converter_0/m01_axis wave_capture_0/s_axis_q
expect_same_intf_net wave_player_0/m_axis usp_rf_data_converter_0/s00_axis

expect_intf_port_pin fpga_refclk_in clk_wiz_245_76/CLK_IN1_D
expect_intf_port_pin adc0_clk_0 usp_rf_data_converter_0/adc0_clk
expect_intf_port_pin dac0_clk_0 usp_rf_data_converter_0/dac0_clk
expect_intf_port_pin sysref_in_0 usp_rf_data_converter_0/sysref_in
expect_intf_port_pin vin0_01_0 usp_rf_data_converter_0/vin0_01
expect_intf_port_pin vout00_0 usp_rf_data_converter_0/vout00

expect_same_net clk_wiz_245_76/clk_out1 wave_player_0/ap_clk
expect_same_net clk_wiz_245_76/clk_out1 wave_capture_0/ap_clk
expect_same_net clk_wiz_245_76/clk_out1 usp_rf_data_converter_0/m0_axis_aclk
expect_same_net clk_wiz_245_76/clk_out1 usp_rf_data_converter_0/s0_axis_aclk
expect_same_net zynq_ultra_ps_e_0/pl_clk0 ${tx_bridge_name}/ap_clk
expect_same_net zynq_ultra_ps_e_0/pl_clk0 iq_bram_bridge_rx/ap_clk
expect_same_net zynq_ultra_ps_e_0/pl_clk0 usp_rf_data_converter_0/s_axi_aclk

if {$ddr_extension_enabled} {
    set ps [require_one "cell zynq_ultra_ps_e_0" [get_bd_cells -quiet zynq_ultra_ps_e_0]]
    expect_property zynq_ultra_ps_e_0 $ps CONFIG.PSU__USE__S_AXI_GP2 1
    expect_property zynq_ultra_ps_e_0 $ps CONFIG.PSU__SAXIGP2__DATA_WIDTH 128

    set ddr_smartconnect [require_one \
        "cell ddr_smartconnect" \
        [get_bd_cells -quiet ddr_smartconnect]]
    expect_property ddr_smartconnect $ddr_smartconnect CONFIG.NUM_SI 1
    expect_property ddr_smartconnect $ddr_smartconnect CONFIG.NUM_MI 1
    expect_property ddr_smartconnect $ddr_smartconnect CONFIG.NUM_CLKS 1

    expect_same_intf_net \
        ddr_bram_bridge_tx/m_axi_gmem \
        ddr_smartconnect/S00_AXI
    expect_same_intf_net \
        ddr_smartconnect/M00_AXI \
        zynq_ultra_ps_e_0/S_AXI_HP0_FPD
    expect_same_net \
        zynq_ultra_ps_e_0/pl_clk0 \
        zynq_ultra_ps_e_0/saxihp0_fpd_aclk
    expect_same_net \
        zynq_ultra_ps_e_0/pl_clk0 \
        ddr_smartconnect/aclk
    expect_same_net \
        proc_sys_reset_0/peripheral_aresetn \
        ddr_smartconnect/aresetn

    set ddr_space [require_one \
        "TX bridge DDR address space" \
        [get_bd_addr_spaces -quiet ddr_bram_bridge_tx/Data_m_axi_gmem]]
    set ddr_segment [require_one \
        "TX bridge low-DDR address segment" \
        [get_bd_addr_segs -quiet -of_objects $ddr_space \
            -filter {NAME =~ "*HP0_DDR_LOW*"}]]

    set ddr_offset [get_property OFFSET $ddr_segment]
    set ddr_range [get_property RANGE $ddr_segment]
    if {$ddr_offset eq "" || $ddr_offset != 0} {
        error "VERIFY FAILED: low DDR offset expected 0, got '$ddr_offset'"
    }
    if {$ddr_range != 0x80000000} {
        error "VERIFY FAILED: low DDR range expected 0x80000000, got '$ddr_range'"
    }
    puts "PASS: DDR master maps low DDR at $ddr_offset range $ddr_range"
}

puts "============================================================"
puts "PASS: BRAM, RFDC, address map, clocks, and key connections match."
puts "============================================================"
