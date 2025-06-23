/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/spinlock.c
 * PURPOSE:         Spinlock and Queued Spinlock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ki.hpp"
#include "ke.hpp"

#define LQ_WAIT     1
#define LQ_OWN      2

/* PRIVATE FUNCTIONS *********************************************************/

// copied from reactos     ntoskrnl/include/internal/spinlock.h
#pragma region implementation

//
// Spinlock Acquisition at IRQL >= DISPATCH_LEVEL
//
_Acquires_nonreentrant_lock_(SpinLock)
static inline
void
KxAcquireSpinLock(
    #if defined(CONFIG_SMP) || DBG
    _Inout_
    #else
    _Unreferenced_parameter_
    #endif
    PKSPIN_LOCK SpinLock)
{
#if DBG
        /* Make sure that we don't own the lock already */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) == *SpinLock)
    {
        /* We do, bugcheck! */
        KeBugCheckEx(SPIN_LOCK_ALREADY_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
#endif

    #ifdef CONFIG_SMP
        /* Try to acquire the lock */
    while (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
    {
        #if defined(_M_IX86) && DBG
                /* On x86 debug builds, we use a much slower but useful routine */
        Kii386SpinOnSpinLock(SpinLock, 5);
        #else
                /* It's locked... spin until it's unlocked */
        while (*(volatile KSPIN_LOCK*)SpinLock & 1)
        {
                /* Yield and keep looping */
            YieldProcessor();
        }
        #endif
    }
    #endif

        /* Add an explicit memory barrier to prevent the compiler from reordering
           memory accesses across the borders of spinlocks */
    KeMemoryBarrierWithoutFence();

    #if DBG
        /* On debug builds, we OR in the KTHREAD */
    * SpinLock = (KSPIN_LOCK)KeGetCurrentThread() | 1;
    #endif
}

//
// Spinlock Release at IRQL >= DISPATCH_LEVEL
//
_Releases_nonreentrant_lock_(SpinLock)
static inline
VOID
KxReleaseSpinLock(
    #if defined(CONFIG_SMP) || DBG
    _Inout_
    #else
    _Unreferenced_parameter_
    #endif
    PKSPIN_LOCK SpinLock)
{
    #if DBG
        /* Make sure that the threads match */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) != *SpinLock)
    {
        /* They don't, bugcheck */
        KeBugCheckEx(SPIN_LOCK_NOT_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
    #endif

    #if defined(CONFIG_SMP) || DBG
        /* Clear the lock  */
    #ifdef _WIN64
    InterlockedAnd64((PLONG64)SpinLock, 0);
    #else
    _InterlockedAnd((PLONG)SpinLock, 0);
    #endif
    #endif

        /* Add an explicit memory barrier to prevent the compiler from reordering
           memory accesses across the borders of spinlocks */
    KeMemoryBarrierWithoutFence();
}

#pragma endregion

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Clear it */
    *SpinLock = 0;
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KfAcquireSpinLock(PKSPIN_LOCK SpinLock)
{
    KIRQL OldIrql;

    /* Raise to dispatch and acquire the lock */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KxAcquireSpinLock(SpinLock);
    return OldIrql;
}

/*
 * @implemented
 */
VOID
FASTCALL
KfReleaseSpinLock(PKSPIN_LOCK SpinLock,
    KIRQL OldIrql)
{
    /* Release the lock and lower IRQL back */
    KxReleaseSpinLock(SpinLock);
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
_Acquires_nonreentrant_lock_(SpinLock)
NTAPI
KeAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
_Releases_nonreentrant_lock_(SpinLock)
NTAPI
KeReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
_Releases_nonreentrant_lock_(SpinLock)
FASTCALL
KefReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
_Acquires_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
_Releases_nonreentrant_lock_(SpinLock)
VOID
FASTCALL
KiReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(IN OUT PKSPIN_LOCK SpinLock)
{
#if DBG
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Make sure that we don't own the lock already */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) == *SpinLock)
    {
        /* We do, bugcheck! */
        KeBugCheckEx(SPIN_LOCK_ALREADY_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
#endif

#ifdef CONFIG_SMP
    /* Check if it's already acquired */
    if (!(*SpinLock))
    {
        /* Try to acquire it */
        if (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
        {
            /* Someone else acquired it */
            return FALSE;
        }
    }
    else
    {
        /* It was already acquired */
        return FALSE;
    }
#endif

#if DBG
    /* On debug builds, we OR in the KTHREAD */
    *SpinLock = (ULONG_PTR)KeGetCurrentThread() | 1;
#endif

    /* All is well, return TRUE */
    return TRUE;
}
