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
// 0x10 : Data signal of captured_words
//        bit 31~0 - captured_words[31:0] (Read)
// 0x14 : Control signal of captured_words
//        bit 0  - captured_words_ap_vld (Read/COR)
//        others - reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL             0x00
#define XWAVE_CAPTURE_CONTROL_ADDR_GIE                 0x04
#define XWAVE_CAPTURE_CONTROL_ADDR_IER                 0x08
#define XWAVE_CAPTURE_CONTROL_ADDR_ISR                 0x0c
#define XWAVE_CAPTURE_CONTROL_ADDR_CAPTURED_WORDS_DATA 0x10
#define XWAVE_CAPTURE_CONTROL_BITS_CAPTURED_WORDS_DATA 32
#define XWAVE_CAPTURE_CONTROL_ADDR_CAPTURED_WORDS_CTRL 0x14

