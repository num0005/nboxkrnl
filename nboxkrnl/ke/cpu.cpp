#include "ke.hpp"
#include "ki.hpp"
#include "..\kernel.hpp"
#include "mm.hpp"
#include "halp.hpp"
#include "rtl.hpp"
#include "ps.hpp"
#include "hal.hpp"
#include <string.h>
#include <assert.h>

constexpr ULONG KeI386NpxPresent = TRUE;
using PFLOATING_SAVE_CONTEXT = void*;

/**
 * @brief
 * Saves the current floating point unit state
 * context of the current calling thread.
 *
 * @param[out] Save
 * The saved floating point context given to the
 * caller at the end of function's operations.
 * The structure whose data contents are opaque
 * to the calling thread.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has
 * successfully completed its operations.
 * STATUS_INSUFFICIENT_RESOURCES is returned
 * if the function couldn't allocate memory
 * for FPU state information.
 *
 * @remarks
 * The function performs a FPU state save
 * in two ways. A normal FPU save (FNSAVE)
 * is performed if the system doesn't have
 * SSE/SSE2, otherwise the function performs
 * a save of FPU, MMX and SSE states save (FXSAVE).
 */
#if defined(__clang__)
__attribute__((__target__("sse")))
#endif
EXPORTNUM(142) NTSTATUS NTAPI KeSaveFloatingPointState
(
    _Out_ PKFLOATING_SAVE Save
)
{
    PFLOATING_SAVE_CONTEXT FsContext;
    PFX_SAVE_AREA FxSaveAreaFrame;
    PKPRCB CurrentPrcb;

    /* Sanity checks */
    NT_ASSERT(Save);
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    NT_ASSERT(KeI386NpxPresent);
    #if 0

    /* Initialize the floating point context */
    FsContext = ExAllocatePoolWithTag(sizeof(FLOATING_SAVE_CONTEXT),
        TAG_FLOATING_POINT_CONTEXT);
    if (!FsContext)
    {
        /* Bail out if we failed */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Allocate some memory pool for the buffer. The size
     * of this allocated buffer is the FX area plus the
     * alignment requirement needed for FXSAVE as a 16-byte
     * aligned pointer is compulsory in order to save the
     * FPU state.
     */
    FsContext->Buffer = ExAllocatePoolWithTag(sizeof(FX_SAVE_AREA) + FXSAVE_ALIGN,
        TAG_FLOATING_POINT_FX);
    if (!FsContext->Buffer)
    {
        /* Bail out if we failed */
        ExFreePoolWithTag(FsContext, TAG_FLOATING_POINT_CONTEXT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Now cache the allocated buffer into the save area
     * and align the said area to a 16-byte boundary. Why
     * do we have to do this is because of ExAllocate function.
     * We gave the necessary alignment requirement in the pool
     * allocation size although the function will always return
     * a 8-byte aligned pointer. Aligning the given pointer directly
     * can cause issues when freeing it from memory afterwards. With
     * that said, we have to cache the buffer to the area so that we
     * do not touch or mess the allocated buffer any further.
     */
    FsContext->PfxSaveArea = ALIGN_UP_POINTER_BY(FsContext->Buffer, 16);

    /* Disable interrupts and get the current processor control region */
    _disable();
    CurrentPrcb = KeGetCurrentPrcb();

    /* Store the current thread to context */
    FsContext->CurrentThread = KeGetCurrentThread();

    /*
     * Save the previous NPX thread state registers (aka Numeric
     * Processor eXtension) into the current context so that
     * we are informing the scheduler the current FPU state
     * belongs to this thread.
     */
    if (FsContext->CurrentThread != CurrentPrcb->NpxThread)
    {
        if ((CurrentPrcb->NpxThread != NULL) &&
            (CurrentPrcb->NpxThread->NpxState == NPX_STATE_LOADED))
        {
            /* Get the FX frame */
            FxSaveAreaFrame = KiGetThreadNpxArea(CurrentPrcb->NpxThread);

            /* Save the FPU state */
            Ke386SaveFpuState(FxSaveAreaFrame);

            /* NPX thread has lost its state */
            CurrentPrcb->NpxThread->NpxState = NPX_STATE_NOT_LOADED;
            FxSaveAreaFrame->NpxSavedCpu = 0;
        }

        /* The new NPX thread is the current thread */
        CurrentPrcb->NpxThread = FsContext->CurrentThread;
    }

    /* Perform the save */
    Ke386SaveFpuState(FsContext->PfxSaveArea);

    /* Store the NPX IRQL */
    FsContext->OldNpxIrql = FsContext->CurrentThread->Header.NpxIrql;

    /* Set the current IRQL to NPX */
    FsContext->CurrentThread->Header.NpxIrql = KeGetCurrentIrql();

    /* Initialize the FPU */
    Ke386FnInit();

    /* Enable interrupts back */
    _enable();

    /* Give the saved FPU context to the caller */
    *((PVOID*)Save) = FsContext;
    #endif
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Restores the original FPU state context that has
 * been saved by a API call of KeSaveFloatingPointState.
 * Callers are expected to restore the floating point
 * state by calling this function when they've finished
 * doing FPU operations.
 *
 * @param[in] Save
 * The saved floating point context that is to be given
 * to the function to restore the FPU state.
 *
 * @return
 * Returns STATUS_SUCCESS indicating the function
 * has fully completed its operations.
 */
#if defined(__clang__)
__attribute__((__target__("sse")))
#endif
EXPORTNUM(139) NTSTATUS NTAPI KeRestoreFloatingPointState
(
    _In_ PKFLOATING_SAVE Save
)
{
    PFLOATING_SAVE_CONTEXT FsContext;

    /* Sanity checks */
    NT_ASSERT(Save);
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    NT_ASSERT(KeI386NpxPresent);

    /* Cache the saved FS context */
    FsContext = *((PVOID*)Save);
    #if 0

    /*
     * We have to restore the regular saved FPU
     * state. For this we must first do some
     * validation checks so that we are sure
     * ourselves the state context is saved
     * properly. Check if we are in the same
     * calling thread.
     */
    if (FsContext->CurrentThread != KeGetCurrentThread())
    {
        /*
         * This isn't the thread that saved the
         * FPU state context, crash the system!
         */
        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
            0x2,
            (ULONG_PTR)FsContext->CurrentThread,
            (ULONG_PTR)KeGetCurrentThread(),
            0);
    }

    /* Are we under the same NPX interrupt level? */
    if (FsContext->CurrentThread->Header.NpxIrql != KeGetCurrentIrql())
    {
        /* The interrupt level has changed, crash the system! */
        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
            0x1,
            (ULONG_PTR)FsContext->CurrentThread->Header.NpxIrql,
            (ULONG_PTR)KeGetCurrentIrql(),
            0);
    }

    /* Disable interrupts */
    _disable();

    /*
     * The saved FPU state context is valid,
     * it's time to restore the state. First,
     * clear FPU exceptions now.
     */
    Ke386ClearFpExceptions();

    /* Restore the state */
    Ke386RestoreFpuState(FsContext->PfxSaveArea);

    /* Give the saved NPX IRQL back to the NPX thread */
    FsContext->CurrentThread->Header.NpxIrql = FsContext->OldNpxIrql;

    /* Enable interrupts back */
    _enable();

    /* We're done, free the allocated area and context */
    ExFreePoolWithTag(FsContext->Buffer, TAG_FLOATING_POINT_FX);
    ExFreePoolWithTag(FsContext, TAG_FLOATING_POINT_CONTEXT);
    #endif

    return STATUS_SUCCESS;
}

EXPORTNUM(126) ULONGLONG XBOXAPI KeQueryPerformanceCounter()
{
    // forward to HAL, this depends on what the target platform is (nxbx, hardware, etc)
    return HalQueryPerformanceCounter();
}

EXPORTNUM(127) ULONGLONG XBOXAPI KeQueryPerformanceFrequency()
{
    return XBOX_ACPI_FREQUENCY;
}


EXPORTNUM(151) VOID NTAPI KeStallExecutionProcessor
(
    ULONG MicroSeconds
)
{
    ULONG64 StartTime, EndTime;

    /* Get the initial time */
    StartTime = __rdtsc();

    /* Calculate the ending time */
    EndTime = StartTime + XBOX_ACPI_TICKS_PER_MS * MicroSeconds;

    /* Loop until time is elapsed */
    while (__rdtsc() < EndTime);
}

