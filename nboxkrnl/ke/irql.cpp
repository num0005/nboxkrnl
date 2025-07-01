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

EXPORTNUM(163) __declspec(naked) VOID FASTCALL KiUnlockDispatcherDatabase
(
	KIRQL OldIrql
)
{
	ASM_BEGIN
		ASM(mov eax, [KiPcr]KPCR.PrcbData.NextThread);
		ASM(test eax, eax);
		ASM(jz end_func); // check to see if a new thread was selected by the scheduler
		ASM(cmp cl, DISPATCH_LEVEL);
		ASM(jb thread_switch); // check to see if we can context switch to the new thread
		ASM(mov eax, [KiPcr]KPCR.PrcbData.DpcRoutineActive);
		ASM(jnz end_func); // we can't, so request a dispatch interrupt if we are not running a DPC already
		ASM(push ecx);
		ASM(mov cl, DISPATCH_LEVEL);
		ASM(call HalRequestSoftwareInterrupt);
		ASM(pop ecx);
		ASM(jmp end_func);
	thread_switch:
		// Save non-volatile registers
		ASM(push esi);
		ASM(push edi);
		ASM(push ebx);
		ASM(push ebp);
		ASM(mov edi, eax);
		ASM(mov esi, [KiPcr]KPCR.PrcbData.CurrentThread);
		ASM(mov byte ptr [esi]KTHREAD.WaitIrql, cl); // we still need to lower the IRQL when this thread is switched back, so save it for later use
		ASM(mov ecx, esi);
		ASM(call KeAddThreadToTailOfReadyList);
		ASM(mov [KiPcr]KPCR.PrcbData.CurrentThread, edi);
		ASM(mov dword ptr [KiPcr]KPCR.PrcbData.NextThread, 0); // dword ptr required or else MSVC will aceess NextThread as a byte
		ASM(movzx ebx, byte ptr [esi]KTHREAD.WaitIrql);
		ASM(call KiSwapThreadContext); // when this returns, it means this thread was switched back again
		ASM(test eax, eax);
		ASM(mov cl, byte ptr [edi]KTHREAD.WaitIrql);
		ASM(jnz deliver_apc);
	restore_regs:
		// Restore non-volatile registers
		ASM(pop ebp);
		ASM(pop ebx);
		ASM(pop edi);
		ASM(pop esi);
		ASM(jmp end_func);
	deliver_apc:
		ASM(mov cl, APC_LEVEL);
		ASM(call KfLowerIrql);
		ASM(call KiExecuteApcQueue);
		ASM(xor ecx, ecx); // if KiSwapThreadContext signals an APC, then WaitIrql of the previous thread must have been zero
		ASM(jmp restore_regs);
	end_func:
		ASM(jmp KfLowerIrql);
	ASM_END
}
