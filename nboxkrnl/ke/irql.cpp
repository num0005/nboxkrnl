/*
 * ergo720                Copyright (c) 2023
 */

#include "ke.hpp"
#include "ki.hpp"
#include "hal.hpp"
#include "halp.hpp"


EXPORTNUM(129) KIRQL XBOXAPI KeRaiseIrqlToDpcLevel()
{
	return KfRaiseIrql(DISPATCH_LEVEL);
}

EXPORTNUM(130) KIRQL XBOXAPI KeRaiseIrqlToSynchLevel()
{
	return KfRaiseIrql(SYNC_LEVEL);
}

// source: reactos
EXPORTNUM(160) KIRQL FASTCALL KfRaiseIrql
(
	KIRQL NewIrql
)
{
	PKPCR Pcr = KeGetPcr();
	KIRQL CurrentIrql;

	/* Read current IRQL */
	CurrentIrql = Pcr->Irql;

	#if DBG
	/* Validate correct raise */
	if (CurrentIrql > NewIrql)
	{
		/* Crash system */
		Pcr->Irql = PASSIVE_LEVEL;
		KeBugCheck(IRQL_NOT_GREATER_OR_EQUAL);
	}
	#endif

	// don't reorder operations past IRQL raise
	KeMemoryBarrierWithoutFence();

	/* Set new IRQL */
	Pcr->Irql = NewIrql;

	// don't reorder operations past IRQL raise
	KeMemoryBarrierWithoutFence();

	/* Return old IRQL */
	return CurrentIrql;
}

EXPORTNUM(161) VOID FASTCALL KfLowerIrql
(
	KIRQL OldIrql
)
{
	PKPCR Pcr = KeGetPcr();

	#if DBG
		/* Validate correct lower */
	if (OldIrql > Pcr->Irql)
	{
		/* Crash system */
		Pcr->Irql = HIGH_LEVEL;
		KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
	}
	#endif

	/* Save EFlags and disable interrupts */
	ULONG EFlags = __readeflags();
	_disable();

	// don't reorder operations past IRQL lower
	KeMemoryBarrierWithoutFence();

	/* Set old IRQL */
	Pcr->Irql = OldIrql;

	// don't reorder operations past IRQL lower
	KeMemoryBarrierWithoutFence();

	/* Check for pending software interrupts */
	HalpCheckUnmaskedInt();

	#if 0
	DWORD PendingIrqlMask = HalpPendingInt & HalpIrqlMasks[KeGetCurrentIrql()];
	if (PendingIrqlMask && !(HalpIntInProgress & ACTIVE_IRQ_MASK))
	{
		ULONG PendingIrql;
		/* Check if pending IRQL affects hardware state */
		RtlpBitScanReverse(&PendingIrql, PendingIrqlMask);
		if (PendingIrql > DISPATCH_LEVEL)
		{
			/* Set new PIC mask */
			PIC_MASK Mask;
			Mask.Both = HalpPendingInt & 0xFFFF;
			__outbyte(PIC_MASTER_DATA, Mask.Master);
			__outbyte(PIC_SLAVE_DATA, Mask.Slave);

			/* Clear IRR bit and set in progress bit*/
			HalpPendingInt ^= (1 << PendingIrql);
		}

		 /* Now handle pending interrupt */
		SwIntHandlers[PendingIrql]();
	}
	#endif

	/* Restore interrupt state */
	__writeeflags(EFlags);
}

EXPORTNUM(163) VOID FASTCALL KiUnlockDispatcherDatabase
(
	KIRQL OldIrql
)
{
	PKPRCB Prcb = KeGetCurrentPrcb();
	PKTHREAD Thread, NextThread;
	BOOLEAN PendingApc;

	/* Make sure we're at synchronization level */
	NT_ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

	/* Check if we have deferred threads */
	KiCheckDeferredReadyList(Prcb);

	/* Check if we were called at dispatcher level or higher */
	if (OldIrql >= DISPATCH_LEVEL)
	{
		/* Check if we have a thread to schedule, and that no DPC is active */
		if ((Prcb->NextThread) && !(Prcb->DpcRoutineActive))
		{
			/* Request DPC interrupt */
			HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
		}

		/* Lower IRQL and exit */
		goto Quickie;
	}

	/* Make sure there's a new thread scheduled */
	if (!Prcb->NextThread) goto Quickie;

	/* Lock the PRCB */
	KiAcquirePrcbLock(Prcb);

	/* Get the next and current threads now */
	NextThread = Prcb->NextThread;
	Thread = Prcb->CurrentThread;

	/* Set current thread's swap busy to true */
	KiSetThreadSwapBusy(Thread);

	/* Switch threads in PRCB */
	Prcb->NextThread = NULL;
	Prcb->CurrentThread = NextThread;

	/* Set thread to running */
	NextThread->State = Running;

	/* Queue it on the ready lists */
	KiQueueReadyThread(Thread, Prcb);

	/* Set wait IRQL */
	Thread->WaitIrql = OldIrql;

	/* Swap threads and check if APCs were pending */
	PendingApc = KiSwapContext(OldIrql, Thread);
	if (PendingApc)
	{
		/* Lower only to APC */
		KeLowerIrql(APC_LEVEL);

		/* Deliver APCs */
		KiExecuteApcQueue();
		NT_ASSERT(OldIrql == PASSIVE_LEVEL);
	}

	/* Lower IRQl back */
Quickie:
	KeLowerIrql(OldIrql);
}
