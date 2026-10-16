// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read/COR)
//        bit 7  - auto_restart (Read/Write)
//        bit 9  - interrupt (Read)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0 - enable ap_done interrupt (Read/Write)
//        bit 1 - enable ap_ready interrupt (Read/Write)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0 - ap_done (Read/TOW)
//        bit 1 - ap_ready (Read/TOW)
//        others - reserved
// 0x10 : Data signal of sample0
//        bit 31~0 - sample0[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of sample1
//        bit 31~0 - sample1[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of sample2
//        bit 31~0 - sample2[31:0] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of sample3
//        bit 31~0 - sample3[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of word_index
//        bit 7~0 - word_index[7:0] (Read/Write)
//        others  - reserved
// 0x34 : reserved
// 0x38 : Data signal of command
//        bit 1~0 - command[1:0] (Read/Write)
//        others  - reserved
// 0x3c : reserved
// 0x40 : Data signal of read0
//        bit 31~0 - read0[31:0] (Read)
// 0x44 : Control signal of read0
//        bit 0  - read0_ap_vld (Read/COR)
//        others - reserved
// 0x48 : Data signal of read1
//        bit 31~0 - read1[31:0] (Read)
// 0x4c : Control signal of read1
//        bit 0  - read1_ap_vld (Read/COR)
//        others - reserved
// 0x50 : Data signal of read2
//        bit 31~0 - read2[31:0] (Read)
// 0x54 : Control signal of read2
//        bit 0  - read2_ap_vld (Read/COR)
//        others - reserved
// 0x58 : Data signal of read3
//        bit 31~0 - read3[31:0] (Read)
// 0x5c : Control signal of read3
//        bit 0  - read3_ap_vld (Read/COR)
//        others - reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL         0x00
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_GIE             0x04
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER             0x08
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_ISR             0x0c
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE0_DATA    0x10
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_SAMPLE0_DATA    32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE1_DATA    0x18
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_SAMPLE1_DATA    32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE2_DATA    0x20
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_SAMPLE2_DATA    32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE3_DATA    0x28
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_SAMPLE3_DATA    32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_WORD_INDEX_DATA 0x30
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_WORD_INDEX_DATA 8
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_COMMAND_DATA    0x38
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_COMMAND_DATA    2
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ0_DATA      0x40
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_READ0_DATA      32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ0_CTRL      0x44
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ1_DATA      0x48
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_READ1_DATA      32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ1_CTRL      0x4c
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ2_DATA      0x50
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_READ2_DATA      32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ2_CTRL      0x54
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ3_DATA      0x58
#define XIQ_BRAM_BRIDGE_CONTROL_BITS_READ3_DATA      32
#define XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ3_CTRL      0x5c

