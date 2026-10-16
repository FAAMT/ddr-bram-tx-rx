// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XDDR_BRAM_BRIDGE_H
#define XDDR_BRAM_BRIDGE_H

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
#include "xddr_bram_bridge_hw.h"

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
} XDdr_bram_bridge_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XDdr_bram_bridge;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XDdr_bram_bridge_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XDdr_bram_bridge_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XDdr_bram_bridge_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XDdr_bram_bridge_ReadReg(BaseAddress, RegOffset) \
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
int XDdr_bram_bridge_Initialize(XDdr_bram_bridge *InstancePtr, UINTPTR BaseAddress);
XDdr_bram_bridge_Config* XDdr_bram_bridge_LookupConfig(UINTPTR BaseAddress);
#else
int XDdr_bram_bridge_Initialize(XDdr_bram_bridge *InstancePtr, u16 DeviceId);
XDdr_bram_bridge_Config* XDdr_bram_bridge_LookupConfig(u16 DeviceId);
#endif
int XDdr_bram_bridge_CfgInitialize(XDdr_bram_bridge *InstancePtr, XDdr_bram_bridge_Config *ConfigPtr);
#else
int XDdr_bram_bridge_Initialize(XDdr_bram_bridge *InstancePtr, const char* InstanceName);
int XDdr_bram_bridge_Release(XDdr_bram_bridge *InstancePtr);
#endif

void XDdr_bram_bridge_Start(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_IsDone(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_IsIdle(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_IsReady(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_EnableAutoRestart(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_DisableAutoRestart(XDdr_bram_bridge *InstancePtr);

void XDdr_bram_bridge_Set_sample0(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_sample0(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_sample1(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_sample1(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_sample2(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_sample2(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_sample3(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_sample3(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_word_index(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_word_index(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_command(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_command(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read0(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read0_vld(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read1(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read1_vld(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read2(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read2_vld(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read3(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_read3_vld(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_ddr(XDdr_bram_bridge *InstancePtr, u64 Data);
u64 XDdr_bram_bridge_Get_ddr(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_bram_start_index(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_bram_start_index(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_Set_transfer_word_count(XDdr_bram_bridge *InstancePtr, u32 Data);
u32 XDdr_bram_bridge_Get_transfer_word_count(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_words_transferred(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_Get_words_transferred_vld(XDdr_bram_bridge *InstancePtr);

void XDdr_bram_bridge_InterruptGlobalEnable(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_InterruptGlobalDisable(XDdr_bram_bridge *InstancePtr);
void XDdr_bram_bridge_InterruptEnable(XDdr_bram_bridge *InstancePtr, u32 Mask);
void XDdr_bram_bridge_InterruptDisable(XDdr_bram_bridge *InstancePtr, u32 Mask);
void XDdr_bram_bridge_InterruptClear(XDdr_bram_bridge *InstancePtr, u32 Mask);
u32 XDdr_bram_bridge_InterruptGetEnabled(XDdr_bram_bridge *InstancePtr);
u32 XDdr_bram_bridge_InterruptGetStatus(XDdr_bram_bridge *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
