// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xddr_bram_bridge.h"

extern XDdr_bram_bridge_Config XDdr_bram_bridge_ConfigTable[];

#ifdef SDT
XDdr_bram_bridge_Config *XDdr_bram_bridge_LookupConfig(UINTPTR BaseAddress) {
	XDdr_bram_bridge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XDdr_bram_bridge_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XDdr_bram_bridge_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XDdr_bram_bridge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDdr_bram_bridge_Initialize(XDdr_bram_bridge *InstancePtr, UINTPTR BaseAddress) {
	XDdr_bram_bridge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDdr_bram_bridge_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDdr_bram_bridge_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XDdr_bram_bridge_Config *XDdr_bram_bridge_LookupConfig(u16 DeviceId) {
	XDdr_bram_bridge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XDDR_BRAM_BRIDGE_NUM_INSTANCES; Index++) {
		if (XDdr_bram_bridge_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XDdr_bram_bridge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDdr_bram_bridge_Initialize(XDdr_bram_bridge *InstancePtr, u16 DeviceId) {
	XDdr_bram_bridge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDdr_bram_bridge_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDdr_bram_bridge_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

