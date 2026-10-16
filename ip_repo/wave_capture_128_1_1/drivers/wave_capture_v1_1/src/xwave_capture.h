// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XWAVE_CAPTURE_H
#define XWAVE_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xwave_capture_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Control_BaseAddress;
} XWave_capture_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XWave_capture;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XWave_capture_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XWave_capture_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XWave_capture_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XWave_capture_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XWave_capture_Initialize(XWave_capture *InstancePtr, UINTPTR BaseAddress);
XWave_capture_Config* XWave_capture_LookupConfig(UINTPTR BaseAddress);
#else
int XWave_capture_Initialize(XWave_capture *InstancePtr, u16 DeviceId);
XWave_capture_Config* XWave_capture_LookupConfig(u16 DeviceId);
#endif
int XWave_capture_CfgInitialize(XWave_capture *InstancePtr, XWave_capture_Config *ConfigPtr);
#else
int XWave_capture_Initialize(XWave_capture *InstancePtr, const char* InstanceName);
int XWave_capture_Release(XWave_capture *InstancePtr);
#endif

void XWave_capture_Start(XWave_capture *InstancePtr);
u32 XWave_capture_IsDone(XWave_capture *InstancePtr);
u32 XWave_capture_IsIdle(XWave_capture *InstancePtr);
u32 XWave_capture_IsReady(XWave_capture *InstancePtr);
void XWave_capture_EnableAutoRestart(XWave_capture *InstancePtr);
void XWave_capture_DisableAutoRestart(XWave_capture *InstancePtr);

u32 XWave_capture_Get_captured_words(XWave_capture *InstancePtr);
u32 XWave_capture_Get_captured_words_vld(XWave_capture *InstancePtr);

void XWave_capture_InterruptGlobalEnable(XWave_capture *InstancePtr);
void XWave_capture_InterruptGlobalDisable(XWave_capture *InstancePtr);
void XWave_capture_InterruptEnable(XWave_capture *InstancePtr, u32 Mask);
void XWave_capture_InterruptDisable(XWave_capture *InstancePtr, u32 Mask);
void XWave_capture_InterruptClear(XWave_capture *InstancePtr, u32 Mask);
u32 XWave_capture_InterruptGetEnabled(XWave_capture *InstancePtr);
u32 XWave_capture_InterruptGetStatus(XWave_capture *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
