/*
 * ergo720                Copyright (c) 2022
 */

#include "ki.hpp"
#include "..\kernel.hpp"
#include "rtl.hpp"
#include <hal.hpp>


KPCR KiPcr = { 0 };
KPROCESS KiUniqueProcess = { 0 };
KPROCESS KiIdleProcess = { 0 };

// Sanity checks: make sure that the members of KiPcr are at the offsets the XBE expects them to be. This is required because they will be
// accessed at those offsets with the FS register
static_assert(offsetof(KPCR, NtTib) == 0x00);
static_assert(offsetof(KPCR, SelfPcr) == 0x1C);
static_assert(offsetof(KPCR, Prcb) == 0x20);
static_assert(offsetof(KPCR, Irql) == 0x24);
static_assert(offsetof(KPCR, PrcbData) == 0x28);
static_assert(offsetof(KPCR, NtTib.ExceptionList) == 0x00);
static_assert(offsetof(KPCR, NtTib.StackBase) == 0x04);
static_assert(offsetof(KPCR, NtTib.StackLimit) == 0x08);
static_assert(offsetof(KPCR, NtTib.SubSystemTib) == 0x0C);
static_assert(offsetof(KPCR, NtTib.FiberData) == 0x10);
static_assert(offsetof(KPCR, NtTib.Version) == 0x10);
static_assert(offsetof(KPCR, NtTib.ArbitraryUserPointer) == 0x14);
static_assert(offsetof(KPCR, NtTib.Self) == 0x18);
static_assert(offsetof(KPCR, PrcbData.CurrentThread) == 0x28);
static_assert(offsetof(KPCR, PrcbData.NextThread) == 0x2C);
static_assert(offsetof(KPCR, PrcbData.IdleThread) == 0x30);
static_assert(offsetof(KPCR, PrcbData.NpxThread) == 0x34);
static_assert(offsetof(KPCR, PrcbData.DpcListHead) == 0x50);
static_assert(offsetof(KPCR, PrcbData.DpcRoutineActive) == 0x58);


VOID KiInitSystem()
{
	InitializeListHead(&KiWaitInListHead);

	KeInitializeDpc(&KiTimerExpireDpc, KiTimerExpiration, nullptr);
	for (unsigned i = 0; i < TIMER_TABLE_SIZE; ++i) {
		InitializeListHead(&KiTimerTableListHead[i]);
	}

	for (unsigned i = 0; i < NUM_OF_THREAD_PRIORITIES; ++i) {
		InitializeListHead(&KiReadyThreadLists[i]);
	}
}

VOID KiInitializeProcess(PKPROCESS Process, KPRIORITY BasePriority, LONG ThreadQuantum)
{
	InitializeListHead(&Process->ReadyListHead);
	InitializeListHead(&Process->ThreadListHead);
	Process->BasePriority = BasePriority;
	Process->ThreadQuantum = ThreadQuantum;
}


[[noreturn]] void KiIdleLoop()
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    /* Now loop forever */
    while (TRUE)
    {
        /* Start of the idle loop: disable interrupts */
        _enable();
        YieldProcessor();
        YieldProcessor();
        _disable();

        
        Prcb->DpcListHead.Flink;
        /* Check for pending timers, pending DPCs, or pending ready threads */
 
       // if ((Prcb->DpcData[0].DpcQueueDepth) ||
        //    (Prcb->TimerRequest) ||
       //     (Prcb->DeferredReadyListHead.Next))
        // just guessing this is close enough?
        if (!IsListEmpty(&Prcb->DpcListHead))
        {
            /* Quiesce the DPC software interrupt */
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);

            /* Handle it */
            KiExecuteDpcQueue();
        }

        /* Check if a new thread is scheduled for execution */
        if (Prcb->NextThread)
        {
            /* Enable interrupts */
            _enable();

            /* Capture current thread data */
            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;

            /* Set new thread data */
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NewThread;

            /* The thread is now running */
            NewThread->State = Running;

            #ifdef CONFIG_SMP
                        /* Do the swap at SYNCH_LEVEL */
            KfRaiseIrql(SYNCH_LEVEL);
            #endif

                        /* Switch away from the idle thread */
            KiSwapContext(APC_LEVEL, OldThread);

            #ifdef CONFIG_SMP
                        /* Go back to DISPATCH_LEVEL */
            KeLowerIrql(DISPATCH_LEVEL);
            #endif
        }
        else
        {
            /* Continue staying idle. Note the HAL returns with interrupts on */
            //Prcb->PowerState.IdleFunction(&Prcb->PowerState);
        }
    }
}
