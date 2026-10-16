# RFSoC4x2 PL differential reference clock
# Board LMK supplies 122.88 MHz to the PL Clocking Wizard input.

set_property PACKAGE_PIN AN11 [get_ports {fpga_refclk_in_clk_p}]
set_property PACKAGE_PIN AP11 [get_ports {fpga_refclk_in_clk_n}]

set_property IOSTANDARD LVDS [get_ports {
    fpga_refclk_in_clk_p
    fpga_refclk_in_clk_n
}]

# RFDC dedicated ports do not receive ordinary PL PACKAGE_PIN constraints:
# adc0_clk_0
# dac0_clk_0
# sysref_in_0
# vin0_01_0
# vout00_0
