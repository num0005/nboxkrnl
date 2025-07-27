/*
 * ergo720                Copyright (c) 2023
 */

#include "ps.hpp"
#include "psp.hpp"
#include "..\kernel.hpp"
#include "rtl.hpp"
#include "mm.hpp"
#include "nt.hpp"
#include "xe.hpp"
#include <string.h>

#define TAG_PS_APC 'pasP'

EXPORTNUM(259) OBJECT_TYPE PsThreadObjectType =
{
	ExAllocatePoolWithTag,
	ExFreePool,
	nullptr,
	nullptr,
	nullptr,
	(PVOID)offsetof(KTHREAD, Header),
	'erhT'
};

BOOLEAN PsInitSystem()
{
	KeInitializeDpc(&PspTerminationDpc, PspTerminationRoutine, nullptr);

	HANDLE Handle;
	NTSTATUS Status = PsCreateSystemThread(&Handle, nullptr, XbeStartupThread, nullptr, FALSE);
	
	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	NtClose(Handle);

	return TRUE;
}

EXPORTNUM(254) NTSTATUS XBOXAPI PsCreateSystemThread
(
	PHANDLE ThreadHandle,
	PHANDLE ThreadId,
	PKSTART_ROUTINE StartRoutine,
	PVOID StartContext,
	BOOLEAN DebuggerThread
)
{
	return PsCreateSystemThreadEx(ThreadHandle, 0, KERNEL_STACK_SIZE, 0, ThreadId, StartRoutine, StartContext, FALSE, DebuggerThread, PspSystemThreadStartup);
}

EXPORTNUM(255) NTSTATUS XBOXAPI PsCreateSystemThreadEx
(
	PHANDLE ThreadHandle,
	SIZE_T ThreadExtensionSize,
	SIZE_T KernelStackSize,
	SIZE_T TlsDataSize,
	PHANDLE ThreadId,
	PKSTART_ROUTINE StartRoutine,
	PVOID StartContext,
	BOOLEAN CreateSuspended,
	BOOLEAN DebuggerThread,
	PKSYSTEM_ROUTINE SystemRoutine
)
{
	PETHREAD eThread;
	NTSTATUS Status = ObCreateObject(&PsThreadObjectType, nullptr, sizeof(ETHREAD) + ThreadExtensionSize, (PVOID *)&eThread);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	memset(eThread, 0, sizeof(ETHREAD) + ThreadExtensionSize);

	InitializeListHead(&eThread->ReaperLink);
	InitializeListHead(&eThread->IrpList);

	KernelStackSize = ROUND_UP_4K(KernelStackSize);
	if (KernelStackSize < KERNEL_STACK_SIZE) {
		KernelStackSize = KERNEL_STACK_SIZE;
	}

	PVOID KernelStack = MmCreateKernelStack(KernelStackSize, DebuggerThread);
	if (KernelStack == nullptr) {
		ObfDereferenceObject(eThread);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	eThread->StartAddress = StartRoutine;
	KeInitializeThread(&eThread->Tcb, KernelStack, KernelStackSize, TlsDataSize, SystemRoutine, StartRoutine, StartContext, &KiUniqueProcess);

	if (CreateSuspended) {
		KeSuspendThread(&eThread->Tcb);
	}

	// After this point, failures will be dealt with by PsTerminateSystemThread

	// Increment the ref count of the thread once more, so that we can safely use eThread if this thread terminates before PsCreateSystemThreadEx has finished
	ObfReferenceObject(eThread);

	// Increment the ref count of the thread once more. This is to guard against the case the XBE closes the thread handle manually
	// before this thread terminates with PsTerminateSystemThread
	ObfReferenceObject(eThread);

	Status = ObInsertObject(eThread, nullptr, 0, &eThread->UniqueThread);

	if (NT_SUCCESS(Status)) {
		PspCallThreadNotificationRoutines(eThread, TRUE);
		Status = ObOpenObjectByPointer(eThread, &PsThreadObjectType, ThreadHandle);
	}

	if (!NT_SUCCESS(Status)) {
		eThread->Tcb.HasTerminated = TRUE;
		if (CreateSuspended) {
			KeResumeThread(&eThread->Tcb);
		}
	}
	else {
		if (ThreadId) {
			*ThreadId = eThread->UniqueThread;
		}
	}

	KeQuerySystemTime(&eThread->CreateTime);
	KeScheduleThread(&eThread->Tcb);

	ObfDereferenceObject(eThread);

	return Status;
}

EXPORTNUM(258) VOID XBOXAPI PsTerminateSystemThread
(
	NTSTATUS ExitStatus
)
{
	PKTHREAD kThread = KeGetCurrentThread();
	kThread->HasTerminated = TRUE;

	KfLowerIrql(PASSIVE_LEVEL);

	PETHREAD eThread = (PETHREAD)kThread;
	if (eThread->UniqueThread) {
		PspCallThreadNotificationRoutines(eThread, FALSE);
	}

	if (kThread->Priority < LOW_REALTIME_PRIORITY) {
		KeSetPriorityThread(kThread, LOW_REALTIME_PRIORITY);
	}

	/* Rundown Timers */
	ExTimerRundown();

	// TODO: cancel I/O, timers and mutants associated to this thread

	KeQuerySystemTime(&eThread->ExitTime);
	eThread->ExitStatus = ExitStatus;

	if (eThread->UniqueThread) {
		NtClose(eThread->UniqueThread);
		eThread->UniqueThread = NULL_HANDLE;
	}

	KeEnterCriticalRegion();
	kThread->ApcState.ApcQueueable = FALSE;
	// TODO: resume thread if it was suspended
	KeLeaveCriticalRegion();

	// TODO: flush APC lists

	KeRaiseIrqlToDpcLevel();

	// TODO: process thread's queue

	eThread->Tcb.Header.SignalState = 1;
	// TODO: satisfy waiters that were waiting on this thread

	RemoveEntryList(&kThread->ThreadListEntry);

	kThread->State = Terminated;
	kThread->ApcState.Process->StackCount -= 1;

	if (KiPcr.PrcbData.NpxThread == kThread) {
		KiPcr.PrcbData.NpxThread = nullptr;
	}

	InsertTailList(&PspTerminationListHead, &eThread->ReaperLink);
	KeInsertQueueDpc(&PspTerminationDpc, nullptr, nullptr);

	KiSwapThread(); // won't return

	KeBugCheckLogEip(NORETURN_FUNCTION_RETURNED);
}


EXPORTNUM(231) NTSTATUS NTAPI NtSuspendThread
(
	IN  HANDLE  ThreadHandle,
	OUT PULONG  PreviousSuspendCount OPTIONAL
)
{
	/* get a reference to the thread object */
	PETHREAD Thread;
	NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, &PsThreadObjectType, reinterpret_cast<PVOID*>(&Thread));

	if (!NT_SUCCESS(Status))
		return Status;

	// TODO: reactos uses rundown protection here, see PsSuspendThread in reactos source code
	// essentially it seems that if we are supending another thread we need to handle the case where another thread at the same time tries to terminate the thread
	// we just raise the IRQL to a level high enough that we cannot be preempted
	KIRQL OldIrql = KfRaiseIrql(SYNCH_LEVEL);

	/* we hold a reference now */

	if (Thread != PsGetCurrentThread()) {
		if (Thread->Tcb.HasTerminated) {
			ObfDereferenceObject(Thread);
			return STATUS_THREAD_IS_TERMINATING;
		}
	}

	ULONG PrevSuspendCount = KeSuspendThread(&Thread->Tcb);

	// HACK: see above
	KfLowerIrql(OldIrql);

	ObfDereferenceObject(Thread);
	if (PrevSuspendCount == STATUS_SUSPEND_COUNT_EXCEEDED) {
		return STATUS_SUSPEND_COUNT_EXCEEDED;
	}

	if (PreviousSuspendCount) {
		*PreviousSuspendCount = PrevSuspendCount;
	}

	return STATUS_SUCCESS;
}

EXPORTNUM(224) NTSTATUS NTAPI NtResumeThread
(
	IN  HANDLE  ThreadHandle,
	OUT PULONG  PreviousSuspendCount OPTIONAL
)
{
	/*
	* No need for rundown protection here since we don't need to avoid resuming a terminating thread (only suspending)
	*/
	PETHREAD Thread;
	NTSTATUS Status = ObReferenceObjectByHandle(ThreadHandle, &PsThreadObjectType, reinterpret_cast<PVOID*>(&Thread));

	if (!NT_SUCCESS(Status))
		return Status;

	ULONG PrevSuspendCount = KeResumeThread(&Thread->Tcb);
	ObfDereferenceObject(Thread);

	if (PreviousSuspendCount) {
		*PreviousSuspendCount = PrevSuspendCount;
	}

	return STATUS_SUCCESS;
}

VOID NTAPI PspQueueApcSpecialApc
(
	IN PKAPC Apc,
	IN OUT PKNORMAL_ROUTINE* NormalRoutine,
	IN OUT PVOID* NormalContext,
	IN OUT PVOID* SystemArgument1,
	IN OUT PVOID* SystemArgument2
)
{
	/* Free the APC and do nothing else */
	ExFreePool(Apc);
}

/*++
 * @name NtQueueApcThreadEx
 * NT4
 *
 *    This routine is used to queue an APC from user-mode for the specified
 *    thread.
 *
 * @param ThreadHandle
 *        Handle to the Thread.
 *        This handle must have THREAD_SET_CONTEXT privileges.
 *
 * @param ApcRoutine
 *        Pointer to the APC Routine to call when the APC executes.
 *
 * @param NormalContext
 *        Pointer to the context to send to the Normal Routine.
 *
 * @param SystemArgument[1-2]
 *        Pointer to a set of two parameters that contain untyped data.
 *
 * @return STATUS_SUCCESS or failure cute from associated calls.
 *
 * @remarks The thread must enter an alertable wait before the APC will be
 *          delivered.
 *
 *--*/
EXPORTNUM(206) NTSTATUS NTAPI NtQueueApcThread
(
	IN HANDLE ThreadHandle,
	IN PKNORMAL_ROUTINE ApcRoutine,
	IN PVOID NormalContext,
	IN OPTIONAL PVOID SystemArgument1,
	IN OPTIONAL PVOID SystemArgument2
)
{
	PETHREAD Thread;
	NTSTATUS Status = STATUS_SUCCESS;
	PAGED_CODE();

	/* Get ETHREAD from Handle */
	Status = ObReferenceObjectByHandle(ThreadHandle, &PsThreadObjectType, (PVOID*)&Thread);
	if (!NT_SUCCESS(Status)) return Status;

	/* Allocate an APC */
	PKAPC Apc = ExNewFromPool<KAPC>(TAG_PS_APC);

	if (!Apc)
	{
		/* Fail */
		Status = STATUS_NO_MEMORY;
		goto Quit;
	}

	/* Initialize the APC */
	KeInitializeApc(Apc,
		&Thread->Tcb,
		PspQueueApcSpecialApc,
		NULL,
		ApcRoutine,
		UserMode,
		NormalContext);

	/* Queue it */
	if (!KeInsertQueueApc(Apc,
		SystemArgument1,
		SystemArgument2,
		IO_NO_INCREMENT))
	{
		/* We failed, free it */
		ExFreePool(Apc);
		STATUS_SUCCESS;
		Status = STATUS_UNSUCCESSFUL;
	}

	/* Dereference Thread and Return */
Quit:
	ObDereferenceObject(Thread);
	return Status;
}

// ******************************************************************
// * 0x0100 - PsQueryStatistics() - source CXBX-reloaded
// ******************************************************************
EXPORTNUM(256) NTSTATUS NTAPI PsQueryStatistics
(
	IN OUT PPS_STATISTICS ProcessStatistics
)
{
	NT_ASSERT(ProcessStatistics);

	NTSTATUS Status;

	if (ProcessStatistics && ProcessStatistics->Length == sizeof(PS_STATISTICS))
	{
		ProcessStatistics->ThreadCount = KiUniqueProcess.StackCount;
		ProcessStatistics->HandleCount = ObpObjectHandleTable.HandleCount;
		
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_INVALID_PARAMETER;
	}

	return Status;
}
