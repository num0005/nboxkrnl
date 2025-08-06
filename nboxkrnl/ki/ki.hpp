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


#define SIZE_OF_FPU_REGISTERS        128
#define NPX_STATE_NOT_LOADED (CR0_TS | CR0_MP) // x87 fpu, XMM, and MXCSR registers not loaded on fpu
#define NPX_STATE_LOADED 0                     // x87 fpu, XMM, and MXCSR registers loaded on fpu

#define TIMER_TABLE_SIZE 32
#define CLOCK_TIME_INCREMENT 10000 // one clock interrupt every ms -> 1ms == 10000 units of 100ns

#define NUM_OF_THREAD_PRIORITIES 32

#define XBOX_ACPI_FREQUENCY 3375000 // 3.375 MHz
#define XBOX_ACPI_TICKS_PER_MS (((ULONG64)XBOX_ACPI_FREQUENCY)/1000)

using KGDT = uint64_t;
using KIDT = uint64_t;
using KTSS = uint32_t[26];


inline KTHREAD KiIdleThread;
inline uint8_t alignas(16) KiIdleThreadStack[KERNEL_STACK_SIZE]; // must be 16 byte aligned because it's used by fxsave and fxrstor

// List of all thread that can be scheduled, one for each priority level
inline LIST_ENTRY KiReadyThreadLists[NUM_OF_THREAD_PRIORITIES];

// Bitmask of KiReadyThreadLists -> bit position is one if there is at least one ready thread at that priority
inline DWORD KiReadyThreadMask = 0;

inline LIST_ENTRY KiWaitInListHead;

inline LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];

inline KDPC KiTimerExpireDpc;

#define BUILD_KIDT(function) ((uint64_t)0x8 << 16) | ((uint64_t)&function & 0x0000FFFF) | (((uint64_t)&function & 0xFFFF0000) << 32) | ((uint64_t)0x8E00 << 32)

extern KTSS KiTss;
extern const KGDT KiGdt[5];
extern KIDT KiIdt[64];
inline constexpr uint16_t KiGdtLimit = sizeof(KiGdt) - 1;
inline constexpr uint16_t KiIdtLimit = sizeof(KiIdt) - 1;
inline constexpr size_t KiFxAreaSize = sizeof(FX_SAVE_AREA);
extern KPROCESS KiUniqueProcess;
extern KPROCESS KiIdleProcess;

// reactos compat: no-op on uniprocessor system
#define KiAcquirePrcbLock(PRCB)
// reactos compat: no-op on uniprocessor system
#define KiReleasePrcbLock(PRCB)
// reactos compat: no-op on uniprocessor system
#define KiCheckDeferredReadyList(PRCB)
// reactos compat: no-op on uniprocessor system
#define KiSetThreadSwapBusy(THREAD)
// reactos compat: no-op on uniprocessor system
#define KiAcquireThreadLock(THREAD)
// reactos compat: no-op on uniprocessor system
#define KiReleaseThreadLock(THREAD)

#define KeNumberProcessors 1

// SMP: change this
#define KiAcquireDispatcherLock() KfRaiseIrql(SYNCH_LEVEL)
#define KiReleaseDispatcherLock(OldIrql) KfLowerIrql(OldIrql)


VOID InitializeCrt();
CODE_SEG("INIT") VOID KiInitializeKernel();
VOID KiInitSystem();
[[noreturn]] void KiIdleLoop();
DWORD KiSwapThreadContext();
VOID XBOXAPI KiExecuteApcQueue();
// called KiRetireDpcList on reactos
VOID FASTCALL KiExecuteDpcQueue();
// called KiRetireDpcListInDpcStack on reactos
VOID FASTCALL KiExecuteDpcQueueInDpcStack(IN PVOID DpcStack);
PKTHREAD XBOXAPI KiQuantumEnd();
VOID KiAdjustQuantumThread(PKTHREAD Thread = nullptr);
NTSTATUS XBOXAPI KiSwapThread();
inline NTSTATUS XBOXAPI KiSwapThread(PKTHREAD Thread, PKPRCB Prcb)
{
    NT_ASSERT(Prcb->NextThread);
    Prcb->NextThread = Thread;
    return KiSwapThread();
}
BOOLEAN FASTCALL KiSwapContext(
	IN KIRQL WaitIrql,
	IN PKTHREAD OldThread
);

VOID KiInitializeProcess(PKPROCESS Process, KPRIORITY BasePriority, LONG ThreadQuantum);

VOID XBOXAPI KiTimerExpiration(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
BOOLEAN KiInsertTimer(PKTIMER Timer, LARGE_INTEGER DueTime);
#if  0
inline BOOLEAN KxInsertTimer(PKTIMER Timer, ULONG Hand)
{
    NT_ASSERT(Timer);
    return KiInsertTimer(Timer, *(LARGE_INTEGER*)(&Timer->DueTime));
};
#endif
BOOLEAN KiReinsertTimer(PKTIMER Timer, ULARGE_INTEGER DueTime);
VOID KiRemoveTimer(PKTIMER Timer);
// reactos wrapper
#define KxRemoveTreeTimer(Timer) KiRemoveTimer(Timer)
ULONG KiComputeTimerTableIndex(ULONGLONG DueTime);
PLARGE_INTEGER KiRecalculateTimerDueTime(PLARGE_INTEGER OriginalTime, PLARGE_INTEGER DueTime, PLARGE_INTEGER NewTime);
VOID KiTimerListExpire(PLIST_ENTRY ExpiredListHead, KIRQL OldIrql);
VOID KiTimerListExpire(PLIST_ENTRY ExpiredListHead, KIRQL OldIrql);

VOID KiWaitTest(PVOID Object, KPRIORITY Increment);
VOID KiUnwaitThread(PKTHREAD Thread, LONG_PTR WaitStatus, KPRIORITY Increment);

VOID FASTCALL KiActivateWaiterQueue(IN PKQUEUE Queue);
LONG FASTCALL KiInsertQueue
(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    IN BOOLEAN Head
);

inline PLARGE_INTEGER KiRecalculateDueTime
(
    IN PLARGE_INTEGER OriginalDueTime,
    IN PLARGE_INTEGER DueTime,
    IN OUT PLARGE_INTEGER NewDueTime
)
{
    /* Don't do anything for absolute waits */
    if (OriginalDueTime->QuadPart >= 0) return OriginalDueTime;

    /* Otherwise, query the interrupt time and recalculate */
    NewDueTime->QuadPart = KeQueryInterruptTime();
    NewDueTime->QuadPart -= DueTime->QuadPart;
    return NewDueTime;
}

_Acquires_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiAcquireSpinLock(IN PKSPIN_LOCK SpinLock);

_Releases_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiReleaseSpinLock(IN PKSPIN_LOCK SpinLock);

template<bool AddToTail>
inline VOID FASTCALL KiAddThreadToReadyList(PKTHREAD Thread)
{
	NT_ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	NT_ASSERT((Thread->Priority >= 0) && (Thread->Priority <= HIGH_PRIORITY));

	Thread->State = Ready;
	if constexpr (AddToTail)
	{
		InsertTailList(&KiReadyThreadLists[Thread->Priority], &Thread->WaitListEntry);
	}
	else
	{
		InsertHeadList(&KiReadyThreadLists[Thread->Priority], &Thread->WaitListEntry);
	}
	KiReadyThreadMask |= (1 << Thread->Priority);
}

_Requires_lock_held_(Prcb->PrcbLock)
_Releases_lock_(Prcb->PrcbLock)
inline
VOID
KiQueueReadyThread(IN PKTHREAD Thread,
	IN PKPRCB Prcb)

{
    /* Set thread ready for execution */
    Thread->State = Ready;

    /* Save current priority and if someone had pre-empted it */
    BOOLEAN Priority = Thread->Priority;
    KPRIORITY Preempted = Thread->Preempted;

    /* We're not pre-empting now, and set the wait time */
    Thread->Preempted = FALSE;
    Thread->WaitTime = KeTickCount;

    /* Sanity check */
    NT_ASSERT((Priority >= 0) && (Priority <= HIGH_PRIORITY));

    /* Insert this thread in the appropriate order */
	Preempted ? KiAddThreadToReadyList<false>(Thread) : KiAddThreadToReadyList<true>(Thread);

    /* Sanity check */
	NT_ASSERT(Priority == Thread->Priority);

    /* Release the PRCB lock */
    KiReleasePrcbLock(Prcb);
}

inline VOID FASTCALL KiReadyThread
(
    IN PKTHREAD Thread
)
{
    /* Sanity checks */
    NT_ASSERT((Thread->Priority >= 0) && (Thread->Priority <= HIGH_PRIORITY));

    /*
    * Reactos checks here for priority adjustments but I do no believe the XBOX has similar logic
    */

    /* Clear thread preemption status and save current values */
    KPRIORITY OldPriority = Thread->Priority;

    /* Get the PRCB and lock it */
    PKPRCB Prcb = KeGetCurrentPrcb();
    KiAcquirePrcbLock(Prcb);

    /*
    * Reactos checks here if the processor is idle (no threads queued)
    * 
    * Not sure what exactly that does
    */

    /* Get the next scheduled thread */
    PKTHREAD NextThread = Prcb->NextThread;
    if (NextThread)
    {
        /* Sanity check */
        NT_ASSERT(NextThread->State == Standby);

        /* Check if priority changed */
        if (OldPriority > NextThread->Priority)
        {
            /* Preempt the thread */
            NextThread->Preempted = TRUE;

            /* Put this one as the next one */
            Thread->State = Standby;
            Prcb->NextThread = Thread;
            Thread->Preempted = FALSE; // clear preempted flag

            /* Set it in ready mode */
            NextThread->State = Ready;
            KiReleasePrcbLock(Prcb);
            KiReadyThread(NextThread);
            return;
        }
    }
    else
    {
        /* Set the next thread as the current thread */
        NextThread = Prcb->CurrentThread;
        if (OldPriority > NextThread->Priority)
        {
            /* Preempt it if it's already running */
            if (NextThread->State == Running) NextThread->Preempted = TRUE;

            /* Set the thread on standby and as the next thread */
            Thread->State = Standby;
            Prcb->NextThread = Thread;
            Thread->Preempted = FALSE; // clear preempted flag

            /* Release the lock */
            KiReleasePrcbLock(Prcb);

            return;
        }
    }

    /* Sanity check */
    NT_ASSERT((OldPriority >= 0) && (OldPriority <= HIGH_PRIORITY));

    /* Sanity check */
    NT_ASSERT(OldPriority == Thread->Priority);

    /* Set this thread as ready (and release lock) */
    KiQueueReadyThread(Thread, Prcb);
}

//
// This routine computes the new priority for a thread. It is only valid for
// threads with priorities in the dynamic priority range.
//
inline
SCHAR
KiComputeNewPriority(IN PKTHREAD Thread,
                     IN SCHAR Adjustment)
{
    SCHAR Priority;

    /* Priority sanity checks */
    NT_ASSERT((Thread->PriorityDecrement >= 0) &&
           (Thread->PriorityDecrement <= Thread->Priority));
    NT_ASSERT((Thread->Priority < LOW_REALTIME_PRIORITY) ?
            TRUE : (Thread->PriorityDecrement == 0));

    /* Get the current priority */
    Priority = Thread->Priority;
    if (Priority < LOW_REALTIME_PRIORITY)
    {
        /* Decrease priority by the priority decrement */
        Priority -= (Thread->PriorityDecrement + Adjustment);

        /* Don't go out of bounds */
        if (Priority < Thread->BasePriority) Priority = Thread->BasePriority;

        /* Reset the priority decrement */
        Thread->PriorityDecrement = 0;
    }

    /* Sanity check */
    NT_ASSERT((Thread->BasePriority == 0) || (Priority != 0));

    /* Return the new priority */
    return Priority;
}

//
// Returns a thread's FPU save area
//
inline
PFX_SAVE_AREA
KiGetThreadNpxArea(IN PKTHREAD Thread)
{
	NT_ASSERT((ULONG_PTR)Thread->StackBase % 16 == 0);
	return (PFX_SAVE_AREA)((ULONG_PTR)Thread->StackBase - sizeof(FX_SAVE_AREA));
}

// SMP: make these lock something
/* This is a no-op at SYNCH_LEVEL for UP systems */
#define KiAcquireDispatcherLockAtSynchLevel() NT_ASSERT(KeGetCurrentIrql() >= SYNCH_LEVEL)

#define KiReleaseDispatcherLockFromSynchLevel()
