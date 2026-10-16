// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xwave_player.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XWave_player_CfgInitialize(XWave_player *InstancePtr, XWave_player_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XWave_player_Start(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL) & 0x80;
    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XWave_player_IsDone(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XWave_player_IsIdle(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XWave_player_IsReady(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XWave_player_EnableAutoRestart(XWave_player *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XWave_player_DisableAutoRestart(XWave_player *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_AP_CTRL, 0);
}

void XWave_player_Set_repeat_count(XWave_player *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_REPEAT_COUNT_DATA, Data);
}

u32 XWave_player_Get_repeat_count(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_REPEAT_COUNT_DATA);
    return Data;
}

u32 XWave_player_Get_frames_completed(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_FRAMES_COMPLETED_DATA);
    return Data;
}

u32 XWave_player_Get_frames_completed_vld(XWave_player *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_FRAMES_COMPLETED_CTRL);
    return Data & 0x1;
}

void XWave_player_InterruptGlobalEnable(XWave_player *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_GIE, 1);
}

void XWave_player_InterruptGlobalDisable(XWave_player *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_GIE, 0);
}

void XWave_player_InterruptEnable(XWave_player *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_IER);
    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_IER, Register | Mask);
}

void XWave_player_InterruptDisable(XWave_player *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_IER);
    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_IER, Register & (~Mask));
}

void XWave_player_InterruptClear(XWave_player *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_player_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_ISR, Mask);
}

u32 XWave_player_InterruptGetEnabled(XWave_player *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_IER);
}

u32 XWave_player_InterruptGetStatus(XWave_player *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XWave_player_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_PLAYER_CONTROL_ADDR_ISR);
}

