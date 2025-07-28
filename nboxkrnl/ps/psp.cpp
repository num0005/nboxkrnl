/*
 * ergo720                Copyright (c) 2023
 */

#include "ps.hpp"
#include "psp.hpp"
#include "seh.hpp"
#include "dbg.hpp"
#include "mm.hpp"
#include <assert.h>


static ULONG PspUnhandledExceptionFilter(PEXCEPTION_POINTERS ExpPtr)
{
	DbgPrint("Unhandled exception at address 0x%X with status 0x%08X\nInfo0: 0x%08X, Info1: 0x%08X, Info2: 0x%08X, Info3: 0x%08X",
		(ULONG_PTR)ExpPtr->ExceptionRecord->ExceptionAddress,
		ExpPtr->ExceptionRecord->ExceptionCode,
		ExpPtr->ExceptionRecord->ExceptionInformation[0],
		ExpPtr->ExceptionRecord->ExceptionInformation[1],
		ExpPtr->ExceptionRecord->ExceptionInformation[2],
		ExpPtr->ExceptionRecord->ExceptionInformation[3]);

	// Return EXCEPTION_CONTINUE_SEARCH, so that RtlDispatchException will fail since this handler is at the top of the stack for this thread, which in turn
	// will cause KiDispatchException to bug check and terminate the system

	return EXCEPTION_CONTINUE_SEARCH;
}

VOID XBOXAPI PspSystemThreadStartup(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
	// Guard the thread start routine, so that we can always catch exceptions that it doesn't handle

	__try {
		(StartRoutine)(StartContext);
	}
	__except (PspUnhandledExceptionFilter(GetExceptionInformation())) {
		// We'll never execute this handler
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID XBOXAPI PspTerminationRoutine(PKDPC Dpc, PVOID DpcContext, PVOID DpcArg0, PVOID DpcArg1)
{
	assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

	PLIST_ENTRY Entry = PspTerminationListHead.Flink;
	while (Entry != &PspTerminationListHead) {
		RemoveEntryList(Entry);
		PETHREAD Thread = CONTAINING_RECORD(Entry, ETHREAD, ReaperLink);
		MmDeleteKernelStack(Thread->Tcb.StackBase, Thread->Tcb.StackLimit);
		Thread->Tcb.StackBase = nullptr;
		ObfDereferenceObject(Thread);
		Entry = PspTerminationListHead.Flink;
	}
}
