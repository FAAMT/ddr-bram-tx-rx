# Replace only the TX IQ bridge with ddr_bram_bridge 1.0 and connect its
# 128-bit AXI4 read master to the PS S_AXI_HP0_FPD DDR port.
#
# This script intentionally requires a user-generated HLS IP repository.  It
# never invokes Vitis or HLS itself.
#
# Usage:
#   vivado -mode batch -source vivado/apply_ddr_to_bram.tcl \
#     -tclargs <project.xpr> <ddr_bram_bridge_1_0_repo>

if {$argc != 2} {
    error "Usage: apply_ddr_to_bram.tcl <project.xpr> <ddr_bram_bridge_1_0_repo>"
}

set script_dir [file dirname [file normalize [info script]]]
set project_file [file normalize [lindex $argv 0]]
set bridge_repo [file normalize [lindex $argv 1]]

if {![file exists $project_file]} {
    error "Project not found: $project_file"
}
if {![file exists "$bridge_repo/component.xml"]} {
    error "DDR BRAM bridge 1.0 repository is incomplete: $bridge_repo"
}

open_project $project_file

set existing_repos [get_property IP_REPO_PATHS [current_project]]
if {$bridge_repo ni $existing_repos} {
    set_property IP_REPO_PATHS [linsert $existing_repos 0 $bridge_repo] [current_project]
}
update_ip_catalog

set required_vlnv xilinx.com:hls:ddr_bram_bridge:1.0
if {![llength [get_ipdefs -all -quiet $required_vlnv]]} {
    error "Required generated IP is missing from the catalog: $required_vlnv"
}

set bd_name rfsoc_waveform_tx_rx_bd
set bd_file [get_files -quiet ${bd_name}.bd]
if {[llength $bd_file] != 1} {
    error "Expected one ${bd_name}.bd, found: $bd_file"
}

open_bd_design $bd_file
current_bd_design $bd_name
validate_bd_design

# RX remains iq_bram_bridge 1.0.  Replace only the TX cell; replace_bd_cell
# transfers every compatible clock, reset, AXI-Lite, and BRAM connection.
if {[llength [get_bd_cells -quiet ddr_bram_bridge_tx]]} {
    error "BD already contains ddr_bram_bridge_tx"
}

set old_tx_cell [get_bd_cells -quiet iq_bram_bridge_tx]
if {[llength $old_tx_cell] != 1} {
    error "Expected one iq_bram_bridge_tx cell, found: $old_tx_cell"
}
if {[get_property VLNV $old_tx_cell] ne "xilinx.com:hls:iq_bram_bridge:1.0"} {
    error "Unexpected original TX bridge: [get_property VLNV $old_tx_cell]"
}

# Rename the master-side mapping before replacing the cell.  Vivado preserves
# this mapping object across replace_bd_cell; without the explicit rename it
# keeps SEG_iq_bram_bridge_0_Reg even after the DDR bridge is installed.
set old_tx_mapping [get_bd_addr_segs -quiet \
    zynq_ultra_ps_e_0/Data/SEG_iq_bram_bridge_0_Reg]
if {[llength $old_tx_mapping] != 1} {
    error "Expected one original TX address mapping, found: $old_tx_mapping"
}
set_property NAME SEG_ddr_bram_bridge_tx_Reg $old_tx_mapping

set tx_cell [create_bd_cell \
    -type ip \
    -vlnv $required_vlnv \
    ddr_bram_bridge_tx]
replace_bd_cell $old_tx_cell $tx_cell
delete_bd_objs $old_tx_cell

if {[get_property VLNV $tx_cell] ne $required_vlnv} {
    error "New TX bridge did not select $required_vlnv"
}

set ddr_master [get_bd_intf_pins -quiet ddr_bram_bridge_tx/m_axi_gmem]
if {[llength $ddr_master] != 1} {
    error "Upgraded TX bridge does not expose m_axi_gmem"
}

set ps [get_bd_cells zynq_ultra_ps_e_0]
set_property -dict [list \
    CONFIG.PSU__USE__S_AXI_GP2 {1} \
    CONFIG.PSU__SAXIGP2__DATA_WIDTH {128} \
] $ps

if {[llength [get_bd_cells -quiet ddr_smartconnect]]} {
    error "BD already contains ddr_smartconnect"
}
set ddr_smartconnect [create_bd_cell \
    -type ip \
    -vlnv xilinx.com:ip:smartconnect:1.0 \
    ddr_smartconnect]
set_property -dict [list \
    CONFIG.NUM_SI {1} \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_CLKS {1} \
] $ddr_smartconnect

connect_bd_intf_net \
    $ddr_master \
    [get_bd_intf_pins ddr_smartconnect/S00_AXI]
connect_bd_intf_net \
    [get_bd_intf_pins ddr_smartconnect/M00_AXI] \
    [get_bd_intf_pins zynq_ultra_ps_e_0/S_AXI_HP0_FPD]
connect_bd_net \
    [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] \
    [get_bd_pins zynq_ultra_ps_e_0/saxihp0_fpd_aclk] \
    [get_bd_pins ddr_smartconnect/aclk]
connect_bd_net \
    [get_bd_pins proc_sys_reset_0/peripheral_aresetn] \
    [get_bd_pins ddr_smartconnect/aresetn]

# Expose the non-coherent low 2 GiB DDR aperture to the HLS AXI master.  The
# software buffer passed to the IP must have a physical address in this range.
assign_bd_address \
    -offset 0xA0000000 \
    -range 0x00001000 \
    -with_name SEG_ddr_bram_bridge_tx_Reg \
    -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e_0/Data] \
    [get_bd_addr_segs ddr_bram_bridge_tx/s_axi_control/Reg] \
    -force

assign_bd_address \
    -offset 0x00000000 \
    -range 0x80000000 \
    -target_address_space \
        [get_bd_addr_spaces ddr_bram_bridge_tx/Data_m_axi_gmem] \
    [get_bd_addr_segs zynq_ultra_ps_e_0/SAXIGP2/HP0_DDR_LOW] \
    -force

validate_bd_design
save_bd_design
generate_target all $bd_file

set wrapper_file [make_wrapper -files $bd_file -top]
add_files -norecurse $wrapper_file
set_property top ${bd_name}_wrapper [get_filesets sources_1]
update_compile_order -fileset sources_1
update_compile_order -fileset sim_1

source "$script_dir/verify_design.tcl"
close_project

puts "============================================================"
puts "ddr_bram_bridge TX path applied and verified: $project_file"
puts "============================================================"
