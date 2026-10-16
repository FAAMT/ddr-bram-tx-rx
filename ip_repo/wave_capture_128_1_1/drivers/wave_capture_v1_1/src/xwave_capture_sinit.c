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
#include "xwave_capture.h"

extern XWave_capture_Config XWave_capture_ConfigTable[];

#ifdef SDT
XWave_capture_Config *XWave_capture_LookupConfig(UINTPTR BaseAddress) {
	XWave_capture_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XWave_capture_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XWave_capture_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XWave_capture_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XWave_capture_Initialize(XWave_capture *InstancePtr, UINTPTR BaseAddress) {
	XWave_capture_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XWave_capture_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XWave_capture_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XWave_capture_Config *XWave_capture_LookupConfig(u16 DeviceId) {
	XWave_capture_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XWAVE_CAPTURE_NUM_INSTANCES; Index++) {
		if (XWave_capture_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XWave_capture_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XWave_capture_Initialize(XWave_capture *InstancePtr, u16 DeviceId) {
	XWave_capture_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XWave_capture_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XWave_capture_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

