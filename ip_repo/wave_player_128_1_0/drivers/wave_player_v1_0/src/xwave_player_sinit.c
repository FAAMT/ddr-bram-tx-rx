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
#include "xwave_player.h"

extern XWave_player_Config XWave_player_ConfigTable[];

#ifdef SDT
XWave_player_Config *XWave_player_LookupConfig(UINTPTR BaseAddress) {
	XWave_player_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XWave_player_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XWave_player_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XWave_player_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XWave_player_Initialize(XWave_player *InstancePtr, UINTPTR BaseAddress) {
	XWave_player_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XWave_player_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XWave_player_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XWave_player_Config *XWave_player_LookupConfig(u16 DeviceId) {
	XWave_player_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XWAVE_PLAYER_NUM_INSTANCES; Index++) {
		if (XWave_player_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XWave_player_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XWave_player_Initialize(XWave_player *InstancePtr, u16 DeviceId) {
	XWave_player_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XWave_player_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XWave_player_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

