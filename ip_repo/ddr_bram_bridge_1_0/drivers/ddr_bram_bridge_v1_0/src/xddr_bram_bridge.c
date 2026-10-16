// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xddr_bram_bridge.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XDdr_bram_bridge_CfgInitialize(XDdr_bram_bridge *InstancePtr, XDdr_bram_bridge_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XDdr_bram_bridge_Start(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XDdr_bram_bridge_IsDone(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDdr_bram_bridge_IsIdle(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDdr_bram_bridge_IsReady(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDdr_bram_bridge_EnableAutoRestart(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XDdr_bram_bridge_DisableAutoRestart(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, 0);
}

void XDdr_bram_bridge_Set_sample0(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE0_DATA, Data);
}

u32 XDdr_bram_bridge_Get_sample0(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE0_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_sample1(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE1_DATA, Data);
}

u32 XDdr_bram_bridge_Get_sample1(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE1_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_sample2(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE2_DATA, Data);
}

u32 XDdr_bram_bridge_Get_sample2(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE2_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_sample3(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE3_DATA, Data);
}

u32 XDdr_bram_bridge_Get_sample3(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE3_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_word_index(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_WORD_INDEX_DATA, Data);
}

u32 XDdr_bram_bridge_Get_word_index(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_WORD_INDEX_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_command(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_COMMAND_DATA, Data);
}

u32 XDdr_bram_bridge_Get_command(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_COMMAND_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_read0(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ0_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_read0_vld(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ0_CTRL);
    return Data & 0x1;
}

u32 XDdr_bram_bridge_Get_read1(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ1_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_read1_vld(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ1_CTRL);
    return Data & 0x1;
}

u32 XDdr_bram_bridge_Get_read2(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ2_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_read2_vld(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ2_CTRL);
    return Data & 0x1;
}

u32 XDdr_bram_bridge_Get_read3(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ3_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_read3_vld(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_READ3_CTRL);
    return Data & 0x1;
}

void XDdr_bram_bridge_Set_ddr(XDdr_bram_bridge *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_DDR_DATA, (u32)(Data));
    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_DDR_DATA + 4, (u32)(Data >> 32));
}

u64 XDdr_bram_bridge_Get_ddr(XDdr_bram_bridge *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_DDR_DATA);
    Data += (u64)XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_DDR_DATA + 4) << 32;
    return Data;
}

void XDdr_bram_bridge_Set_bram_start_index(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_BRAM_START_INDEX_DATA, Data);
}

u32 XDdr_bram_bridge_Get_bram_start_index(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_BRAM_START_INDEX_DATA);
    return Data;
}

void XDdr_bram_bridge_Set_transfer_word_count(XDdr_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_TRANSFER_WORD_COUNT_DATA, Data);
}

u32 XDdr_bram_bridge_Get_transfer_word_count(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_TRANSFER_WORD_COUNT_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_words_transferred(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_WORDS_TRANSFERRED_DATA);
    return Data;
}

u32 XDdr_bram_bridge_Get_words_transferred_vld(XDdr_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_WORDS_TRANSFERRED_CTRL);
    return Data & 0x1;
}

void XDdr_bram_bridge_InterruptGlobalEnable(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_GIE, 1);
}

void XDdr_bram_bridge_InterruptGlobalDisable(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_GIE, 0);
}

void XDdr_bram_bridge_InterruptEnable(XDdr_bram_bridge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_IER);
    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_IER, Register | Mask);
}

void XDdr_bram_bridge_InterruptDisable(XDdr_bram_bridge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_IER);
    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XDdr_bram_bridge_InterruptClear(XDdr_bram_bridge *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_ISR, Mask);
}

u32 XDdr_bram_bridge_InterruptGetEnabled(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_IER);
}

u32 XDdr_bram_bridge_InterruptGetStatus(XDdr_bram_bridge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDdr_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XDDR_BRAM_BRIDGE_CONTROL_ADDR_ISR);
}

