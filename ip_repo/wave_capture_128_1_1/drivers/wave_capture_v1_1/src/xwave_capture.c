// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xwave_capture.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XWave_capture_CfgInitialize(XWave_capture *InstancePtr, XWave_capture_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XWave_capture_Start(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XWave_capture_IsDone(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XWave_capture_IsIdle(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XWave_capture_IsReady(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XWave_capture_EnableAutoRestart(XWave_capture *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XWave_capture_DisableAutoRestart(XWave_capture *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_AP_CTRL, 0);
}

u32 XWave_capture_Get_captured_words(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_CAPTURED_WORDS_DATA);
    return Data;
}

u32 XWave_capture_Get_captured_words_vld(XWave_capture *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_CAPTURED_WORDS_CTRL);
    return Data & 0x1;
}

void XWave_capture_InterruptGlobalEnable(XWave_capture *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_GIE, 1);
}

void XWave_capture_InterruptGlobalDisable(XWave_capture *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_GIE, 0);
}

void XWave_capture_InterruptEnable(XWave_capture *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_IER);
    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_IER, Register | Mask);
}

void XWave_capture_InterruptDisable(XWave_capture *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_IER);
    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XWave_capture_InterruptClear(XWave_capture *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XWave_capture_WriteReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_ISR, Mask);
}

u32 XWave_capture_InterruptGetEnabled(XWave_capture *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_IER);
}

u32 XWave_capture_InterruptGetStatus(XWave_capture *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XWave_capture_ReadReg(InstancePtr->Control_BaseAddress, XWAVE_CAPTURE_CONTROL_ADDR_ISR);
}

