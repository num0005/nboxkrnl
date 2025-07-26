/*
 * ergo720                Copyright (c) 2022
 * RadWolfie              Copyright (c) 2022
 */

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

#define ASSERT_THREAD(THREAD) NT_ASSERT(THREAD != NULL)


VOID XBOXAPI KiSuspendNop(PKAPC Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2) {}

VOID XBOXAPI KiSuspendThread(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument)
{
	/* Non-alertable kernel-mode suspended wait */
	KeWaitForSingleObject(&KeGetCurrentThread()->SuspendSemaphore,
		Suspended,
		KernelMode,
		FALSE,
		NULL);
}

static VOID CDECL KiThreadStartup
(
	PKSYSTEM_ROUTINE SystemRoutine, 
	PKSTART_ROUTINE StartRoutine, 
	PVOID StartContext
)
{
	// This function is called from KiSwapThreadContext when a new thread is run for the first time
	// On entry, the esp points to a PKSTART_FRAME
	
	PKTHREAD Thread = KeGetCurrentThread();
	// drop all the way down to passive
	KfLowerIrql(PASSIVE_LEVEL);

	// detect PsCreateSystemThreadEx failure
	if (Thread->HasTerminated)
	{
		PsTerminateSystemThread(STATUS_NO_MEMORY);
	}

	SystemRoutine(StartRoutine, StartContext);
	UNREACHABLE;
}

// Source: Cxbx-Reloaded
VOID KiInitializeContextThread(PKTHREAD Thread, ULONG TlsDataSize, PKSYSTEM_ROUTINE SystemRoutine, PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
	ULONG_PTR StackAddress = reinterpret_cast<ULONG_PTR>(Thread->StackBase);

	StackAddress -= sizeof(FX_SAVE_AREA);
	PFX_SAVE_AREA FxSaveArea = reinterpret_cast<PFX_SAVE_AREA>(StackAddress);
	memset(FxSaveArea, 0, sizeof(FX_SAVE_AREA));

	FxSaveArea->FloatSave.ControlWord = 0x27F;
	FxSaveArea->FloatSave.MXCsr = 0x1F80;
	FxSaveArea->FloatSave.Cr0NpxState = NPX_STATE_NOT_LOADED;
	Thread->NpxState = NPX_STATE_NOT_LOADED;

	TlsDataSize = ROUND_UP(TlsDataSize, 4);
	StackAddress -= TlsDataSize;
	if (TlsDataSize) {
		Thread->TlsData = reinterpret_cast<PVOID>(StackAddress);
		memset(Thread->TlsData, 0, TlsDataSize);
	}
	else {
		Thread->TlsData = nullptr;
	}

	StackAddress -= sizeof(KSTART_FRAME);
	PKSTART_FRAME StartFrame = reinterpret_cast<PKSTART_FRAME>(StackAddress);
	StackAddress -= sizeof(KSWITCHFRAME);
	PKSWITCHFRAME CtxSwitchFrame = reinterpret_cast<PKSWITCHFRAME>(StackAddress);

	StartFrame->StartContext = StartContext;
	StartFrame->StartRoutine = StartRoutine;
	StartFrame->SystemRoutine = SystemRoutine;
	StartFrame->RetAddrFatal = &KeBugCheckNoReturnGuard; // bug check if KiThreadStartup ever returns

	CtxSwitchFrame->RetAddr = KiThreadStartup;
	CtxSwitchFrame->Eflags = 0x200; // interrupt enable flag
	CtxSwitchFrame->ExceptionList = EXCEPTION_CHAIN_END;

	Thread->KernelStack = CtxSwitchFrame;
}

// Source: Cxbx-Reloaded
VOID KeInitializeThread(PKTHREAD Thread, PVOID KernelStack, ULONG KernelStackSize, ULONG TlsDataSize, PKSYSTEM_ROUTINE SystemRoutine, PKSTART_ROUTINE StartRoutine,
	PVOID StartContext, PKPROCESS Process)
{
	Thread->Header.Type = ThreadObject;
	Thread->Header.Size = sizeof(KTHREAD) / sizeof(LONG);
	Thread->Header.SignalState = 0;
	InitializeListHead(&Thread->Header.WaitListHead);

	InitializeListHead(&Thread->MutantListHead);

	InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
	InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
	Thread->KernelApcDisable = FALSE;
	Thread->ApcState.Process = Process;
	Thread->ApcState.ApcQueueable = TRUE;

	KeInitializeApc(
		&Thread->SuspendApc,
		Thread,
		KiSuspendNop,
		nullptr,
		KiSuspendThread,
		KernelMode,
		nullptr);

	KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 2);

	KeInitializeTimer(&Thread->Timer);
	PKWAIT_BLOCK TimerWaitBlock = &Thread->TimerWaitBlock;
	TimerWaitBlock->Object = &Thread->Timer;
	TimerWaitBlock->WaitKey = static_cast<CSHORT>(STATUS_TIMEOUT);
	TimerWaitBlock->WaitType = WaitAny;
	TimerWaitBlock->Thread = Thread;
	TimerWaitBlock->NextWaitBlock = nullptr;
	TimerWaitBlock->WaitListEntry.Flink = &Thread->Timer.Header.WaitListHead;
	TimerWaitBlock->WaitListEntry.Blink = &Thread->Timer.Header.WaitListHead;

	Thread->StackBase = KernelStack;
	Thread->StackLimit = reinterpret_cast<PVOID>(reinterpret_cast<LONG_PTR>(KernelStack) - KernelStackSize);

	KiInitializeContextThread(Thread, TlsDataSize, SystemRoutine, StartRoutine, StartContext);

	Thread->DisableBoost = Process->DisableBoost;
	Thread->Quantum = Process->ThreadQuantum;
	Thread->Priority = Process->BasePriority;
	Thread->BasePriority = Process->BasePriority;
	Thread->State = Initialized;

	KIRQL OldIrql = KeRaiseIrqlToDpcLevel();

	InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
	Process->StackCount++;

	KfLowerIrql(OldIrql);
}

VOID FASTCALL KeAddThreadToTailOfReadyList(PKTHREAD Thread)
{
	KiAddThreadToReadyList<true>(Thread);
}

VOID KiScheduleThread(PKTHREAD Thread)
{
	if (KiPcr.PrcbData.NextThread) {
		// If another thread was selected and this one has higher priority, preempt it
		if (Thread->Priority > KiPcr.PrcbData.NextThread->Priority) {
			KiAddThreadToReadyList<false>(KiPcr.PrcbData.NextThread);
			KiPcr.PrcbData.NextThread = Thread;
			Thread->State = Standby;
			return;
		}
	}
	else {
		// If the current thread has lower priority than this one, preempt it
		if (Thread->Priority > KiPcr.PrcbData.CurrentThread->Priority) {
			KiPcr.PrcbData.NextThread = Thread;
			Thread->State = Standby;
			return;
		}
	}

	// Add thread to the tail of the ready list (round robin scheduling)
	KiAddThreadToReadyList<true>(Thread);
}

VOID KeScheduleThread(PKTHREAD Thread)
{
	KIRQL OldIrql = KeRaiseIrqlToDpcLevel();
	KiScheduleThread(Thread);
	KiUnlockDispatcherDatabase(OldIrql);
}

DWORD __declspec(naked) KiSwapThreadContext()
{
	// This function uses a custom calling convention, so never call it from C++ code
	// esi -> KPCR.PrcbData.CurrentThread before the switch
	// edi -> KPCR.PrcbData.NextThread before the switch
	// ebx -> IRQL of the previous thread. If zero, check for kernel APCs, otherwise don't

	// When the final ret instruction is executed, it will either point to the caller of this function of the next thread when it was last switched out,
	// or it will be the address of KiThreadStartup when the thread is executed for the first time ever

	ASM_BEGIN
		ASM(mov [edi]KTHREAD.State, Running);
		// Construct a KSWITCHFRAME on the stack (note that RetAddr was pushed by the call instruction to this function)
		ASM(or bl, bl); // set zf in the saved eflags so that it will reflect the IRQL of the previous thread
		ASM(pushfd); // save eflags
		ASM(push [KiPcr]KPCR.NtTib.ExceptionList); // save per-thread exception list
		ASM(cli);
		ASM(mov [esi]KTHREAD.KernelStack, esp); // save esp
		// The floating state is saved in a lazy manner, because it's expensive to save it at every context switch. Instead of actually saving it here, we
		// only set flags in cr0 so that an attempt to use any fpu/mmx/sse instructions will cause a "no math coprocessor" exception, which is then handled
		// by the kernel in KiTrap7 and it's there where the float context is restored
		ASM(mov ecx, [edi]KTHREAD.StackBase);
		ASM(sub ecx, SIZE FX_SAVE_AREA);
		ASM(mov [KiPcr]KPCR.NtTib.StackBase, ecx);
		ASM(mov eax, [edi]KTHREAD.StackLimit);
		ASM(mov [KiPcr]KPCR.NtTib.StackLimit, eax);
		ASM(mov eax, cr0);
		ASM(and eax, ~(CR0_MP | CR0_EM | CR0_TS));
		ASM(or eax, [ecx]FLOATING_SAVE_AREA.Cr0NpxState);
		ASM(mov cr0, eax);
		ASM(mov esp, [edi]KTHREAD.KernelStack); // switch to the stack of the new thread -> it points to a KSWITCHFRAME
		ASM(sti);
		ASM(inc [edi]KTHREAD.ContextSwitches); // per-thread number of context switches
		ASM(inc [KiPcr]KPCR.PrcbData.KeContextSwitches); // total number of context switches
		ASM(pop dword ptr [KiPcr]KPCR.NtTib.ExceptionList); // restore exception list; NOTE: dword ptr required or else MSVC will aceess ExceptionList as a byte
		ASM(cmp [edi]KTHREAD.ApcState.KernelApcPending, 0);
		ASM(jnz kernel_apc);
		ASM(popfd); // restore eflags
		ASM(xor eax, eax);
		ASM(ret); // load eip for the new thread
	kernel_apc:
		ASM(popfd); // restore eflags and read saved previous IRQL from zf
		ASM(jz deliver_apc); // if zero, then previous IRQL was PASSIVE so deliver an APC
		ASM(mov cl, APC_LEVEL);
		ASM(call HalRequestSoftwareInterrupt);
		ASM(xor eax, eax);
		ASM(ret); // load eip for the new thread
	deliver_apc:
		ASM(mov eax, 1);
		ASM(ret); // load eip for the new thread
	ASM_END
}

BOOLEAN
__declspec(naked)
FASTCALL
KiSwapContext(
	IN KIRQL WaitIrql,
	IN PKTHREAD OldThread
)
{
	ASM_BEGIN
		/* get new thread */
		ASM(mov eax, [KiPcr]KPCR.PrcbData.CurrentThread);
		/* zero extend IRQL */
		ASM(movzx ecx, ecx)
		/* Save all the non-volatile registers */
		ASM(push ebx)
		ASM(push esi)
		ASM(push edi)
		ASM(push ebp)

		///* build frame */
		//ASM(sub esp, 2 * 4) // ExceptionList, Eflags
		//ASM(mov eax, [esp + 4])
		//ASM(push eax) // NewThread
		//ASM(push ecx) // CurrentThread
		//ASM(push edx) // WaitIrql
		//ASM(mov ecx, esp)

		/* move arguments where KiSwapThreadContext expects them */
		ASM(mov esi, edx) // OldThread
		ASM(mov edi, eax) // NewThread
		ASM(mov ebx, ecx) // WaitIrql
		ASM(call KiSwapThreadContext)

		/* Restore the registers */
		ASM(pop ebp)
		ASM(pop edi)
		ASM(pop esi)
		ASM(pop ebx)

		/* return */
		ASM(ret)
	ASM_END
}

/*
* Find a thread to run at at least LowPriority or higher
*/
static PKTHREAD XBOXAPI KiFindAndRemoveHighestPriorityThread(KPRIORITY LowPriority)
{
	assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

	if (KiReadyThreadMask == 0) {
		return nullptr;
	}

	/* Get the highest priority possible */
	KPRIORITY HighestPriority;
	BitScanReverse((PULONG)&HighestPriority, KiReadyThreadMask);

	if (HighestPriority < LowPriority) {
		return nullptr;
	}

	PLIST_ENTRY ListHead = &KiReadyThreadLists[HighestPriority];
	/* Make sure the list isn't empty at the highest priority */
	NT_ASSERT(IsListEmpty(ListHead) == FALSE);

	/* Get the first thread on the list */
	PKTHREAD Thread = CONTAINING_RECORD(ListHead->Flink, KTHREAD, WaitListEntry);
	
	if (RemoveEntryList(&Thread->WaitListEntry)) {
		KiReadyThreadMask &= ~(1 << HighestPriority);
	}

	return Thread;
}

/*
* Find a thread to run at at least LowPriority or higher
*/
#define KiSelectReadyThread(LowPriority) KiFindAndRemoveHighestPriorityThread(LowPriority)

static VOID KiSetPriorityThread(PKTHREAD Thread, KPRIORITY NewPriority)
{
	assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

	KPRIORITY OldPriority = Thread->Priority;

	if (OldPriority != NewPriority) {
		Thread->Priority = NewPriority;

		switch (Thread->State)
		{
		case Ready:
			RemoveEntryList(&Thread->WaitListEntry);
			if (IsListEmpty(&KiReadyThreadLists[OldPriority])) {
				KiReadyThreadMask &= ~(1 << OldPriority);
			}

			if (NewPriority < OldPriority) {
				// Add the thread to the tail of the ready list (round robin scheduling)
				KiAddThreadToReadyList<true>(Thread);
			}
			else {
				// If NewPriority is higher than OldPriority, check to see if the thread can preempt other threads
				KiScheduleThread(Thread);
			}
			break;

		case Standby:
			if (NewPriority < OldPriority) {
				// Attempt to find another thread to run instead of the one we were about to execute
				PKTHREAD NewThread = KiFindAndRemoveHighestPriorityThread(NewPriority);
				if (NewThread) {
					NewThread->State = Standby;
					KiPcr.PrcbData.NextThread = NewThread;
					KiScheduleThread(Thread);
				}
			}
			break;

		case Running:
			if (KiPcr.PrcbData.NextThread == nullptr) {
				if (NewPriority < OldPriority) {
					// Attempt to preempt the currently running thread (NOTE: the dispatch request should be done by the caller)
					PKTHREAD NewThread = KiFindAndRemoveHighestPriorityThread(NewPriority);
					if (NewThread) {
						NewThread->State = Standby;
						KiPcr.PrcbData.NextThread = NewThread;
					}
				}
			}
			break;

		case Initialized:
			// The thread is being created by PsCreateSystemThreadEx. The function will schedule it at the end of the initialization, so nothing to do here
			break;

		case Terminated:
			// The thread is being killed by PsTerminateSystemThread, so there's no point to re-schedule it here
			break;

		case Waiting:
			// The thread is blocked in a wait (for example, in KeWaitForSingleObject). When the wait ends, a re-schedule will happen, so nothing to do here
			break;
		}
	}
}

PKTHREAD XBOXAPI KiQuantumEnd()
{
	assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

	PKTHREAD Thread = KeGetCurrentThread();

	if (Thread->Quantum <= 0) {
		Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
		KPRIORITY NewPriority, Priority = Thread->Priority;

		if (Priority < LOW_REALTIME_PRIORITY) {
			NewPriority = Priority - 1;
			if (NewPriority < Thread->BasePriority) {
				NewPriority = Priority;
			}
		}
		else {
			NewPriority = Priority;
		}

		if (Priority != NewPriority) {
			KiSetPriorityThread(Thread, NewPriority);
		}
		else {
			if (KiPcr.PrcbData.NextThread == nullptr) {
				// Attempt to find another thread to run since the current one has just ended its quantum
				PKTHREAD NewThread = KiFindAndRemoveHighestPriorityThread(Priority);
				if (NewThread) {
					NewThread->State = Standby;
					KiPcr.PrcbData.NextThread = NewThread;
				}
			}
		}
	}

	return KiPcr.PrcbData.NextThread;
}

VOID KiAdjustQuantumThread()
{
	assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

	PKTHREAD Thread = KeGetCurrentThread();

	if ((Thread->Priority < LOW_REALTIME_PRIORITY) && (Thread->BasePriority < TIME_CRITICAL_BASE_PRIORITY)) {
		Thread->Quantum -= WAIT_QUANTUM_DECREMENT;
		KiQuantumEnd();
	}
}

NTSTATUS __declspec(naked) XBOXAPI KiSwapThread()
{
	// On entry, IRQL must be at DISPATCH_LEVEL

	ASM_BEGIN
		ASM(mov eax, [KiPcr]KPCR.PrcbData.NextThread);
		ASM(test eax, eax);
		ASM(jnz thread_switch); // check to see if a new thread was selected by the scheduler
		ASM(push LOW_PRIORITY);
		ASM(call KiFindAndRemoveHighestPriorityThread);
		ASM(jnz thread_switch);
		ASM(lea eax, KiIdleThread);
	thread_switch:
		// Save non-volatile registers
		ASM(push esi);
		ASM(push edi);
		ASM(push ebx);
		ASM(push ebp);
		ASM(mov edi, eax);
		ASM(mov esi, [KiPcr]KPCR.PrcbData.CurrentThread);
		ASM(mov [KiPcr]KPCR.PrcbData.CurrentThread, edi);
		ASM(mov dword ptr [KiPcr]KPCR.PrcbData.NextThread, 0); // dword ptr required or else MSVC will access NextThread as a byte
		ASM(movzx ebx, byte ptr [esi]KTHREAD.WaitIrql);
		ASM(call KiSwapThreadContext); // when this returns, it means this thread was switched back again
		ASM(test eax, eax);
		ASM(mov cl, byte ptr [edi]KTHREAD.WaitIrql);
		ASM(mov ebx, [edi]KTHREAD.WaitStatus);
		ASM(jnz deliver_apc);
	restore_regs:
		ASM(call KfLowerIrql);
		ASM(mov eax, ebx);
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
		ASM(ret);
	ASM_END
}

EXPORTNUM(140) ULONG XBOXAPI KeResumeThread
(
	PKTHREAD Thread
)
{
	//KLOCK_QUEUE_HANDLE ApcLock;
	ULONG PreviousCount;
	//ASSERT_THREAD(Thread);
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	/* Lock the APC Queue */
	//KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);

	/* Save the Old Count */
	PreviousCount = Thread->SuspendCount;

	/* Check if it existed */
	if (PreviousCount)
	{
		/* Decrease the suspend count */
		Thread->SuspendCount--;

		/* Check if the thrad is still suspended or not */
		if ((!Thread->SuspendCount))
		{
			/* Acquire the dispatcher lock */
			KiAcquireDispatcherLockAtSynchLevel();

			/* Signal the Suspend Semaphore */
			Thread->SuspendSemaphore.Header.SignalState++;
			KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);

			/* Release the dispatcher lock */
			KiReleaseDispatcherLockFromSynchLevel();
		}
	}

	/* Release APC Queue lock and return the Old State */
	//KiReleaseApcLockFromSynchLevel(&ApcLock);
	KiUnlockDispatcherDatabase(OldIrql);
	return PreviousCount;
}

EXPORTNUM(148) KPRIORITY XBOXAPI KeSetPriorityThread
(
	PKTHREAD Thread,
	LONG Priority
)
{
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);
	KPRIORITY OldPriority = Thread->Priority;
	Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
	KiSetPriorityThread(Thread, Priority);
	KiUnlockDispatcherDatabase(OldIrql);

	return OldPriority;
}

EXPORTNUM(152) ULONG XBOXAPI KeSuspendThread
(
	PKTHREAD Thread
)
{
	ASSERT_IRQL_LESS_OR_EQUAL(SYNCH_LEVEL);
	// SMP: add per-thread APC lock
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);

	/* Save the Old Count */
	ULONG PreviousCount = Thread->SuspendCount;

	/* Handle the maximum */
	if (PreviousCount == MAXIMUM_SUSPEND_COUNT)
	{
		/* Raise an exception */
		KfLowerIrql(OldIrql);
		RtlRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
	}

	/* Should we bother to queue at all? */
	if (Thread->ApcState.ApcQueueable)
	{
		/* Increment the suspend count */
		Thread->SuspendCount++;

		/* Check if we should suspend it */
		if (!(PreviousCount))
		{
			/* Is the APC already inserted? */
			if (!Thread->SuspendApc.Inserted)
			{
				/* Not inserted, insert it */
				Thread->SuspendApc.Inserted = TRUE;
				KiInsertQueueApc(&Thread->SuspendApc, IO_NO_INCREMENT);
			}
			else
			{
				/* Lock the dispatcher */
				KiAcquireDispatcherLockAtSynchLevel();

				/* Unsignal the semaphore, the APC was already inserted */
				Thread->SuspendSemaphore.Header.SignalState--;

				/* Release the dispatcher */
				KiReleaseDispatcherLockFromSynchLevel();
			}
		}
	}

	// SMP: add per thread APC lock
	KiUnlockDispatcherDatabase(OldIrql);

	return PreviousCount;
}

EXPORTNUM(155) BOOLEAN XBOXAPI KeTestAlertThread
(
	KPROCESSOR_MODE AlertMode
)
{
	PKTHREAD Thread = KeGetCurrentThread();
	BOOLEAN OldState;
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	/* Lock the Dispatcher Database and the APC Queue */
	// SMP: KiAcquireApcLockRaiseToSynch here + per-thread lock
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);

	/* Save the old State */
	OldState = Thread->Alerted[AlertMode];

	/* Check the Thread is alerted */
	if (OldState)
	{
		/* Disable alert for this mode */
		Thread->Alerted[AlertMode] = FALSE;
	}
	else if ((AlertMode != KernelMode) &&
		(!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
	{
		/* If the mode is User and the Queue isn't empty, set Pending */
		Thread->ApcState.UserApcPending = TRUE;
	}

	/* Release Locks and return the Old State */
	KfLowerIrql(OldIrql);
	return OldState;
}


EXPORTNUM(92) BOOLEAN NTAPI KeAlertResumeThread
(
	IN PKTHREAD Thread
)
{
	ULONG PreviousCount;
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	// SMP: add APC lock here
	/* Lock the Dispatcher Database and the APC Queue */
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);
	KiAcquireDispatcherLockAtSynchLevel();

	/* Return if Thread is already alerted. */
	if (!Thread->Alerted[KernelMode])
	{
		/* If it's Blocked, unblock if it we should */
		if ((Thread->State == Waiting) && (Thread->Alertable))
		{
			/* Abort the wait */
			KiUnwaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
		}
		else
		{
			/* If not, simply Alert it */
			Thread->Alerted[KernelMode] = TRUE;
		}
	}

	/* Save the old Suspend Count */
	PreviousCount = Thread->SuspendCount;

	/* If the thread is suspended, decrease one of the suspend counts */
	if (PreviousCount)
	{
		/* Decrease count. If we are now zero, unwait it completely */
		Thread->SuspendCount--;
		if (!(Thread->SuspendCount))
		{
			/* Signal and satisfy */
			Thread->SuspendSemaphore.Header.SignalState++;
			KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
		}
	}

	/* Release Locks and return the Old State */
	KiReleaseDispatcherLockFromSynchLevel();
	//KiReleaseApcLockFromSynchLevel(&ApcLock);
	KiUnlockDispatcherDatabase(OldIrql);
	return PreviousCount;
}

EXPORTNUM(93) BOOLEAN NTAPI KeAlertThread
(
	IN PKTHREAD Thread,
	IN KPROCESSOR_MODE AlertMode
)
{
	BOOLEAN PreviousState;
	//KLOCK_QUEUE_HANDLE ApcLock;
	ASSERT_THREAD(Thread);
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	// SMP: add APC lock here
	/* Lock the Dispatcher Database and the APC Queue */
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);
	//KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);
	KiAcquireDispatcherLockAtSynchLevel();

	/* Save the Previous State */
	PreviousState = Thread->Alerted[AlertMode];

	/* Check if it's already alerted */
	if (!PreviousState)
	{
		/* Check if the thread is alertable, and blocked in the given mode */
		if ((Thread->State == Waiting) &&
			(Thread->Alertable) &&
			(AlertMode <= Thread->WaitMode))
		{
			/* Abort the wait to alert the thread */
			KiUnwaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
		}
		else
		{
			/* Otherwise, merely set the alerted state */
			Thread->Alerted[AlertMode] = TRUE;
		}
	}

	/* Release the Dispatcher Lock */
	KiReleaseDispatcherLockFromSynchLevel();
	//KiReleaseApcLockFromSynchLevel(&ApcLock);
	KiUnlockDispatcherDatabase(OldIrql);

	/* Return the old state */
	return PreviousState;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(94) VOID NTAPI KeBoostPriorityThread
(
	IN PKTHREAD Thread,
	IN KPRIORITY Increment
)
{
	KIRQL OldIrql;
	KPRIORITY Priority;
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
	ASSERT_THREAD(Thread);

	/* Lock the Dispatcher Database */
	OldIrql = KiAcquireDispatcherLock();

	/* Only threads in the dynamic range get boosts */
	if (Thread->Priority < LOW_REALTIME_PRIORITY)
	{
		/* Lock the thread */
		KiAcquireThreadLock(Thread);

		/* Check again, and make sure there's not already a boost */
		if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
			!(Thread->PriorityDecrement))
		{
			/* Compute the new priority and see if it's higher */
			Priority = Thread->BasePriority + Increment;
			if (Priority > Thread->Priority)
			{
				if (Priority >= LOW_REALTIME_PRIORITY)
				{
					Priority = LOW_REALTIME_PRIORITY - 1;
				}

				/* Reset the quantum */
				Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;

				/* Set the new Priority */
				KiSetPriorityThread(Thread, Priority);
			}
		}

		/* Release thread lock */
		KiReleaseThreadLock(Thread);
	}

	/* Release the dispatcher lock */
	KiReleaseDispatcherLock(OldIrql);
}

#define KiGetCurrentReadySummary() KiReadyThreadMask

/*
 * @implemented
 */

EXPORTNUM(238) NTSTATUS NTAPI NtYieldExecution(VOID)
{
	NTSTATUS Status;
	KIRQL OldIrql;
	PKPRCB Prcb;
	PKTHREAD Thread, NextThread;

	/* NB: No instructions (other than entry code) should preceed this line */

	/* Fail if there's no ready summary, exit fast to avoid wasting time getting a lock */
	if (!KiGetCurrentReadySummary()) return STATUS_NO_YIELD_PERFORMED;

	/* Now get the current thread, set the status... */
	Status = STATUS_NO_YIELD_PERFORMED;
	Thread = KeGetCurrentThread();

	/* Raise IRQL to synch and get the KPRCB now */
	OldIrql = KeRaiseIrqlToSynchLevel();
	Prcb = KeGetCurrentPrcb();

	/* Now check if there's still a ready summary */
	if (KiGetCurrentReadySummary())
	{
		/* Acquire thread and PRCB lock */
		KiAcquireThreadLock(Thread);
		KiAcquirePrcbLock(Prcb);

		/* Find a new thread to run if none was selected */
		if (!Prcb->NextThread) Prcb->NextThread = KiSelectReadyThread(1);

		/* Make sure we still have a next thread to schedule */
		NextThread = Prcb->NextThread;
		if (NextThread)
		{
			/* Reset quantum and recalculate priority */
			Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
			Thread->Priority = KiComputeNewPriority(Thread, 1);

			/* Release the thread lock */
			KiReleaseThreadLock(Thread);

			/* Set context swap busy */
			KiSetThreadSwapBusy(Thread);

			/* Set the new thread as running */
			Prcb->NextThread = NULL;
			Prcb->CurrentThread = NextThread;
			NextThread->State = Running;

			/* Setup a yield wait and queue the thread */
			Thread->WaitReason = WrUserRequest;
			KiQueueReadyThread(Thread, Prcb);

			/* Make it wait at APC_LEVEL */
			Thread->WaitIrql = APC_LEVEL;

			/* Sanity check */
			NT_ASSERT(OldIrql <= DISPATCH_LEVEL);

			/* Swap to new thread */
			KiSwapContext(APC_LEVEL, Thread);
			Status = STATUS_SUCCESS;
		}
		else
		{
			/* Release the PRCB and thread lock */
			KiReleasePrcbLock(Prcb);
			KiReleaseThreadLock(Thread);
		}
	}

	/* Lower IRQL and return */
	KeLowerIrql(OldIrql);
	return Status;
}
