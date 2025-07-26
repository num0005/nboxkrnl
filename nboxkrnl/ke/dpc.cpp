/*
 * ergo720                Copyright (c) 2022
 * PatrickvL              Copyright (c) 2016
 */

#include "ke.hpp"
#include "ki.hpp"
#include "hal.hpp"

#define ASSERT_DPC(DPC) NT_ASSERT(DPC != NULL)


// Source: Cxbx-Reloaded
EXPORTNUM(107) VOID XBOXAPI KeInitializeDpc
(
	PKDPC Dpc,
	PKDEFERRED_ROUTINE DeferredRoutine,
	PVOID DeferredContext
)
{
	Dpc->Type = DpcObject;
	Dpc->Inserted = FALSE;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
}

EXPORTNUM(119) BOOLEAN XBOXAPI KeInsertQueueDpc
(
	PKDPC Dpc,
	PVOID SystemArgument1,
	PVOID SystemArgument2
)
{
	ASSERT_DPC(Dpc);

	KIRQL OldIrql = KfRaiseIrql(HIGH_LEVEL);

	BOOLEAN Inserted = Dpc->Inserted;
	if (!Inserted) {

		const PKPRCB Prcb = KeGetCurrentPrcb();

		Dpc->Inserted = TRUE;
		Dpc->SystemArgument1 = SystemArgument1;
		Dpc->SystemArgument2 = SystemArgument2;
		InsertTailList(&Prcb->DpcListHead, &Dpc->DpcListEntry);

		if ((Prcb->DpcRoutineActive == FALSE) && (Prcb->DpcInterruptRequested == FALSE)) {
			Prcb->DpcInterruptRequested = TRUE;
			HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
		}
	}

	KfLowerIrql(OldIrql);

	return Inserted == FALSE;
}

// Source: Cxbx-Reloaded
VOID FASTCALL KiExecuteDpcQueue()
{
	// On entry, interrupts must be disabled
	const PKPRCB Prcb = KeGetCurrentPrcb();

	while (!IsListEmpty(&Prcb->DpcListHead)) {
		// Extract the head entry and retrieve the containing KDPC pointer for it:
		PKDPC Dpc = CONTAINING_RECORD(RemoveHeadList(&Prcb->DpcListHead), KDPC, DpcListEntry);
		// Mark it as no longer linked into the DpcQueue
		Dpc->Inserted = FALSE;
		// Set DpcRoutineActive to support KeIsExecutingDpc:
		Prcb->DpcRoutineActive = TRUE;
		enable();

		// Call the Deferred Procedure:
		Dpc->DeferredRoutine(Dpc, Dpc->DeferredContext, Dpc->SystemArgument1, Dpc->SystemArgument2);

		disable();
		Prcb->DpcRoutineActive = FALSE;
	}

	Prcb->DpcInterruptRequested = FALSE;
}

// source: reactos
VOID __declspec(naked) FASTCALL KiExecuteDpcQueueInDpcStack(IN PVOID DpcStack)
{
	ASM_BEGIN
		// switch stack
		ASM(mov eax, esp)
		ASM(mov esp, ecx)
		ASM(push eax)
		// call wrapped function
		ASM(call KiExecuteDpcQueue)
		// switch back stacks
		ASM(pop esp)
		ASM(ret)
	ASM_END
}

/*
 * @implemented
 */
EXPORTNUM(137) BOOLEAN NTAPI KeRemoveQueueDpc
(
	IN PKDPC Dpc
)
{
	ASSERT_DPC(Dpc);

	/* Disable interrupts */
	BOOLEAN InterruptsEnabled = KeDisableInterrupts();

	/* Check if the DPC was inserted in the first place */
	BOOLEAN Inserted = Dpc->Inserted;
	if (Inserted)
	{
		/* Acquire the DPC lock */
		//KiAcquireSpinLock(Dpc->Lock);

		/* SMP: Make sure that the data didn't change (how?)*/
		RemoveEntryList(&Dpc->DpcListEntry);

		/* Release the lock */
		//KiReleaseSpinLock(Dpc->Lock);
	}

	/* Re-enable interrupts */
	KeRestoreInterrupts(InterruptsEnabled);

	/* Return if the DPC was in the queue or not */
	return Inserted;
}

void KeFlushQueuedDpcs()
{
	// SMP: change this to notify other CPUs
	HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
}


