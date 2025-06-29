/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "..\types.hpp"
#include "ke.hpp"
#include "..\kernel.hpp"
#include "..\driverspecs.h"

#define EXCEPTION_CHAIN_END2 0xFFFFFFFF
#define EXCEPTION_CHAIN_END reinterpret_cast<EXCEPTION_REGISTRATION_RECORD *>(EXCEPTION_CHAIN_END2)

// cr0 flags
#define CR0_TS (1 << 3) // task switched
#define CR0_EM (1 << 2) // emulation
#define CR0_MP (1 << 1) // monitor coprocessor

#define TIMER_TABLE_SIZE 32
#define CLOCK_TIME_INCREMENT 10000 // one clock interrupt every ms -> 1ms == 10000 units of 100ns

#define NUM_OF_THREAD_PRIORITIES 32

using KGDT = uint64_t;
using KIDT = uint64_t;
using KTSS = uint32_t[26];


#define CONTEXT_i386                0x00010000
#define CONTEXT_CONTROL             (CONTEXT_i386 | 0x00000001L)
#define CONTEXT_INTEGER             (CONTEXT_i386 | 0x00000002L)
#define CONTEXT_SEGMENTS            (CONTEXT_i386 | 0x00000004L)
#define CONTEXT_FLOATING_POINT      (CONTEXT_i386 | 0x00000008L)
#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L)

struct CONTEXT {
	DWORD ContextFlags;
	FLOATING_SAVE_AREA FloatSave;
	DWORD Edi;
	DWORD Esi;
	DWORD Ebx;
	DWORD Edx;
	DWORD Ecx;
	DWORD Eax;
	DWORD Ebp;
	DWORD Eip;
	DWORD SegCs;
	DWORD EFlags;
	DWORD Esp;
	DWORD SegSs;
};
using PCONTEXT = CONTEXT *;

inline KTHREAD KiIdleThread;
inline uint8_t alignas(16) KiIdleThreadStack[KERNEL_STACK_SIZE]; // must be 16 byte aligned because it's used by fxsave and fxrstor

// List of all thread that can be scheduled, one for each priority level
inline LIST_ENTRY KiReadyThreadLists[NUM_OF_THREAD_PRIORITIES];

// Bitmask of KiReadyThreadLists -> bit position is one if there is at least one ready thread at that priority
inline DWORD KiReadyThreadMask = 0;

inline LIST_ENTRY KiWaitInListHead;

inline LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];

inline KDPC KiTimerExpireDpc;

extern KTSS KiTss;
extern const KGDT KiGdt[5];
extern KIDT KiIdt[64];
inline constexpr uint16_t KiGdtLimit = sizeof(KiGdt) - 1;
inline constexpr uint16_t KiIdtLimit = sizeof(KiIdt) - 1;
inline constexpr size_t KiFxAreaSize = sizeof(FX_SAVE_AREA);
extern KPROCESS KiUniqueProcess;
extern KPROCESS KiIdleProcess;


VOID InitializeCrt();
[[noreturn]] VOID KiInitializeKernel();
VOID KiInitSystem();
[[noreturn]] void KiIdleLoop();
DWORD KiSwapThreadContext();
VOID XBOXAPI KiExecuteApcQueue();
VOID XBOXAPI KiExecuteDpcQueue();
PKTHREAD XBOXAPI KiQuantumEnd();
VOID KiAdjustQuantumThread();
NTSTATUS XBOXAPI KiSwapThread();

VOID KiInitializeProcess(PKPROCESS Process, KPRIORITY BasePriority, LONG ThreadQuantum);

VOID XBOXAPI KiTimerExpiration(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
BOOLEAN KiInsertTimer(PKTIMER Timer, LARGE_INTEGER DueTime);
BOOLEAN KiReinsertTimer(PKTIMER Timer, ULARGE_INTEGER DueTime);
VOID KiRemoveTimer(PKTIMER Timer);
ULONG KiComputeTimerTableIndex(ULONGLONG DueTime);
PLARGE_INTEGER KiRecalculateTimerDueTime(PLARGE_INTEGER OriginalTime, PLARGE_INTEGER DueTime, PLARGE_INTEGER NewTime);
VOID KiTimerListExpire(PLIST_ENTRY ExpiredListHead, KIRQL OldIrql);
VOID KiTimerListExpire(PLIST_ENTRY ExpiredListHead, KIRQL OldIrql);

VOID KiWaitTest(PVOID Object, KPRIORITY Increment);
VOID KiUnwaitThread(PKTHREAD Thread, LONG_PTR WaitStatus, KPRIORITY Increment);

_Acquires_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiAcquireSpinLock(IN PKSPIN_LOCK SpinLock);

_Releases_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiReleaseSpinLock(IN PKSPIN_LOCK SpinLock);
