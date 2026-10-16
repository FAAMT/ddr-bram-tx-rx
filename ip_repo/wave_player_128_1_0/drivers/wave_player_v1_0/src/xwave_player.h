// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XWAVE_PLAYER_H
#define XWAVE_PLAYER_H

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
#include "xwave_player_hw.h"

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
} XWave_player_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XWave_player;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XWave_player_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XWave_player_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XWave_player_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XWave_player_ReadReg(BaseAddress, RegOffset) \
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
int XWave_player_Initialize(XWave_player *InstancePtr, UINTPTR BaseAddress);
XWave_player_Config* XWave_player_LookupConfig(UINTPTR BaseAddress);
#else
int XWave_player_Initialize(XWave_player *InstancePtr, u16 DeviceId);
XWave_player_Config* XWave_player_LookupConfig(u16 DeviceId);
#endif
int XWave_player_CfgInitialize(XWave_player *InstancePtr, XWave_player_Config *ConfigPtr);
#else
int XWave_player_Initialize(XWave_player *InstancePtr, const char* InstanceName);
int XWave_player_Release(XWave_player *InstancePtr);
#endif

void XWave_player_Start(XWave_player *InstancePtr);
u32 XWave_player_IsDone(XWave_player *InstancePtr);
u32 XWave_player_IsIdle(XWave_player *InstancePtr);
u32 XWave_player_IsReady(XWave_player *InstancePtr);
void XWave_player_EnableAutoRestart(XWave_player *InstancePtr);
void XWave_player_DisableAutoRestart(XWave_player *InstancePtr);

void XWave_player_Set_repeat_count(XWave_player *InstancePtr, u32 Data);
u32 XWave_player_Get_repeat_count(XWave_player *InstancePtr);
u32 XWave_player_Get_frames_completed(XWave_player *InstancePtr);
u32 XWave_player_Get_frames_completed_vld(XWave_player *InstancePtr);

void XWave_player_InterruptGlobalEnable(XWave_player *InstancePtr);
void XWave_player_InterruptGlobalDisable(XWave_player *InstancePtr);
void XWave_player_InterruptEnable(XWave_player *InstancePtr, u32 Mask);
void XWave_player_InterruptDisable(XWave_player *InstancePtr, u32 Mask);
void XWave_player_InterruptClear(XWave_player *InstancePtr, u32 Mask);
u32 XWave_player_InterruptGetEnabled(XWave_player *InstancePtr);
u32 XWave_player_InterruptGetStatus(XWave_player *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
