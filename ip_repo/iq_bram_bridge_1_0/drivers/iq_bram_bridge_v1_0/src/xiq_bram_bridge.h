// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.1 (64-bit)
// Tool Version Limit: 2025.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XIQ_BRAM_BRIDGE_H
#define XIQ_BRAM_BRIDGE_H

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
#include "xiq_bram_bridge_hw.h"

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
} XIq_bram_bridge_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XIq_bram_bridge;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XIq_bram_bridge_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XIq_bram_bridge_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XIq_bram_bridge_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XIq_bram_bridge_ReadReg(BaseAddress, RegOffset) \
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
int XIq_bram_bridge_Initialize(XIq_bram_bridge *InstancePtr, UINTPTR BaseAddress);
XIq_bram_bridge_Config* XIq_bram_bridge_LookupConfig(UINTPTR BaseAddress);
#else
int XIq_bram_bridge_Initialize(XIq_bram_bridge *InstancePtr, u16 DeviceId);
XIq_bram_bridge_Config* XIq_bram_bridge_LookupConfig(u16 DeviceId);
#endif
int XIq_bram_bridge_CfgInitialize(XIq_bram_bridge *InstancePtr, XIq_bram_bridge_Config *ConfigPtr);
#else
int XIq_bram_bridge_Initialize(XIq_bram_bridge *InstancePtr, const char* InstanceName);
int XIq_bram_bridge_Release(XIq_bram_bridge *InstancePtr);
#endif

void XIq_bram_bridge_Start(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_IsDone(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_IsIdle(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_IsReady(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_EnableAutoRestart(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_DisableAutoRestart(XIq_bram_bridge *InstancePtr);

void XIq_bram_bridge_Set_sample0(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_sample0(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_Set_sample1(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_sample1(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_Set_sample2(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_sample2(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_Set_sample3(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_sample3(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_Set_word_index(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_word_index(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_Set_command(XIq_bram_bridge *InstancePtr, u32 Data);
u32 XIq_bram_bridge_Get_command(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read0(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read0_vld(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read1(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read1_vld(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read2(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read2_vld(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read3(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_Get_read3_vld(XIq_bram_bridge *InstancePtr);

void XIq_bram_bridge_InterruptGlobalEnable(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_InterruptGlobalDisable(XIq_bram_bridge *InstancePtr);
void XIq_bram_bridge_InterruptEnable(XIq_bram_bridge *InstancePtr, u32 Mask);
void XIq_bram_bridge_InterruptDisable(XIq_bram_bridge *InstancePtr, u32 Mask);
void XIq_bram_bridge_InterruptClear(XIq_bram_bridge *InstancePtr, u32 Mask);
u32 XIq_bram_bridge_InterruptGetEnabled(XIq_bram_bridge *InstancePtr);
u32 XIq_bram_bridge_InterruptGetStatus(XIq_bram_bridge *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
