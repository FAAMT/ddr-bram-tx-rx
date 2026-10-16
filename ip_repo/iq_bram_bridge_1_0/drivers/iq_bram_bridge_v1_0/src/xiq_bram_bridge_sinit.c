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
#include "xiq_bram_bridge.h"

extern XIq_bram_bridge_Config XIq_bram_bridge_ConfigTable[];

#ifdef SDT
XIq_bram_bridge_Config *XIq_bram_bridge_LookupConfig(UINTPTR BaseAddress) {
	XIq_bram_bridge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XIq_bram_bridge_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XIq_bram_bridge_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XIq_bram_bridge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XIq_bram_bridge_Initialize(XIq_bram_bridge *InstancePtr, UINTPTR BaseAddress) {
	XIq_bram_bridge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XIq_bram_bridge_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XIq_bram_bridge_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XIq_bram_bridge_Config *XIq_bram_bridge_LookupConfig(u16 DeviceId) {
	XIq_bram_bridge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XIQ_BRAM_BRIDGE_NUM_INSTANCES; Index++) {
		if (XIq_bram_bridge_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XIq_bram_bridge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XIq_bram_bridge_Initialize(XIq_bram_bridge *InstancePtr, u16 DeviceId) {
	XIq_bram_bridge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XIq_bram_bridge_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XIq_bram_bridge_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

