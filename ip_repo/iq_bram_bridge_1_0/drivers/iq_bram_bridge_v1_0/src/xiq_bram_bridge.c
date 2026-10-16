// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xiq_bram_bridge.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XIq_bram_bridge_CfgInitialize(XIq_bram_bridge *InstancePtr, XIq_bram_bridge_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XIq_bram_bridge_Start(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XIq_bram_bridge_IsDone(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XIq_bram_bridge_IsIdle(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XIq_bram_bridge_IsReady(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XIq_bram_bridge_EnableAutoRestart(XIq_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XIq_bram_bridge_DisableAutoRestart(XIq_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_AP_CTRL, 0);
}

void XIq_bram_bridge_Set_sample0(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE0_DATA, Data);
}

u32 XIq_bram_bridge_Get_sample0(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE0_DATA);
    return Data;
}

void XIq_bram_bridge_Set_sample1(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE1_DATA, Data);
}

u32 XIq_bram_bridge_Get_sample1(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE1_DATA);
    return Data;
}

void XIq_bram_bridge_Set_sample2(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE2_DATA, Data);
}

u32 XIq_bram_bridge_Get_sample2(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE2_DATA);
    return Data;
}

void XIq_bram_bridge_Set_sample3(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE3_DATA, Data);
}

u32 XIq_bram_bridge_Get_sample3(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_SAMPLE3_DATA);
    return Data;
}

void XIq_bram_bridge_Set_word_index(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_WORD_INDEX_DATA, Data);
}

u32 XIq_bram_bridge_Get_word_index(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_WORD_INDEX_DATA);
    return Data;
}

void XIq_bram_bridge_Set_command(XIq_bram_bridge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_COMMAND_DATA, Data);
}

u32 XIq_bram_bridge_Get_command(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_COMMAND_DATA);
    return Data;
}

u32 XIq_bram_bridge_Get_read0(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ0_DATA);
    return Data;
}

u32 XIq_bram_bridge_Get_read0_vld(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ0_CTRL);
    return Data & 0x1;
}

u32 XIq_bram_bridge_Get_read1(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ1_DATA);
    return Data;
}

u32 XIq_bram_bridge_Get_read1_vld(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ1_CTRL);
    return Data & 0x1;
}

u32 XIq_bram_bridge_Get_read2(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ2_DATA);
    return Data;
}

u32 XIq_bram_bridge_Get_read2_vld(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ2_CTRL);
    return Data & 0x1;
}

u32 XIq_bram_bridge_Get_read3(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ3_DATA);
    return Data;
}

u32 XIq_bram_bridge_Get_read3_vld(XIq_bram_bridge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_READ3_CTRL);
    return Data & 0x1;
}

void XIq_bram_bridge_InterruptGlobalEnable(XIq_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_GIE, 1);
}

void XIq_bram_bridge_InterruptGlobalDisable(XIq_bram_bridge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_GIE, 0);
}

void XIq_bram_bridge_InterruptEnable(XIq_bram_bridge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER);
    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER, Register | Mask);
}

void XIq_bram_bridge_InterruptDisable(XIq_bram_bridge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER);
    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XIq_bram_bridge_InterruptClear(XIq_bram_bridge *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XIq_bram_bridge_WriteReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_ISR, Mask);
}

u32 XIq_bram_bridge_InterruptGetEnabled(XIq_bram_bridge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_IER);
}

u32 XIq_bram_bridge_InterruptGetStatus(XIq_bram_bridge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XIq_bram_bridge_ReadReg(InstancePtr->Control_BaseAddress, XIQ_BRAM_BRIDGE_CONTROL_ADDR_ISR);
}

