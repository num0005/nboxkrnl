#include "rtl.hpp"
#include "..\hal\halp.hpp"
#include "..\dbg\dbg.hpp"
#include "..\ex\ex.hpp"
#include "..\ke\ke.hpp"
#include "..\mm\mm.hpp"
#include <string.h>
#include <assert.h>
#include <nanoprintf.h>

NTSTATUS NTAPI RtlpWaitForCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
	NTSTATUS result;
	result = KeWaitForSingleObject(
		(PVOID)CriticalSection,
		WrExecutive,
		KernelMode,
		FALSE,
		(PLARGE_INTEGER)0
	);
	NT_ASSERT_MESSAGE(NT_SUCCESS(result), "Waiting for event of a critical section returned %lx.", result);
	if (!NT_SUCCESS(result))
	{
		KeBugCheckLogEip(CRITICAL_SECTION_INTERNAL_ERRROR);
	};

	return STATUS_SUCCESS;
}

EXPORTNUM(291) VOID XBOXAPI RtlInitializeCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
)
{
	KeInitializeEvent(&CriticalSection->Event, SynchronizationEvent, FALSE);
	CriticalSection->LockCount = -1;
	CriticalSection->RecursionCount = 0;
	CriticalSection->OwningThread = 0;
}

EXPORTNUM(294) VOID XBOXAPI RtlLeaveCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
)
{

	#if DBG
	KTHREAD* Thread = KeGetCurrentThread();

	/*
	 * In win this case isn't checked. However it's a valid check so it should
	 * only be performed in debug builds!
	 */
	if (Thread != CriticalSection->OwningThread)
	{
		DbgPrint("Releasing critical section not owned! (thread %p, owned by %p)\n", Thread, CriticalSection->OwningThread);
	}
	#endif

	CriticalSection->RecursionCount--;
	LONG count = InterlockedDecrement(&CriticalSection->LockCount);
	if (CriticalSection->RecursionCount == 0)
	{
		CriticalSection->OwningThread = 0;
		if (count >= 0)
		{
			KeSetEvent(&CriticalSection->Event, PRIORITY_BOOST_EVENT, FALSE);
		}
	}
}

EXPORTNUM(277) VOID XBOXAPI RtlEnterCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
)
{
	PKTHREAD thread = KeGetCurrentThread();

	if (InterlockedIncrement(&CriticalSection->LockCount) == 0)
	{
		CriticalSection->OwningThread = thread;
		CriticalSection->RecursionCount = 1;
	}
	else
	{
		if (CriticalSection->OwningThread != thread)
		{
			RtlpWaitForCriticalSection(CriticalSection);
			CriticalSection->OwningThread = thread;
			KeMemoryBarrier(); // no reordering
			CriticalSection->RecursionCount = 1;
		}
		else
		{
			CriticalSection->RecursionCount++;
		}
	}
}

EXPORTNUM(278) VOID XBOXAPI RtlEnterCriticalSectionAndRegion
(
	PRTL_CRITICAL_SECTION CriticalSection
)
{
	KeEnterCriticalRegion();
	RtlEnterCriticalSection(CriticalSection);
}

EXPORTNUM(295) VOID XBOXAPI RtlLeaveCriticalSectionAndRegion
(
	PRTL_CRITICAL_SECTION CriticalSection
)
{
	// NOTE: this must check RecursionCount only once, so that it can unconditionally call KeLeaveCriticalRegion if the counter is zero regardless of
	// its current value. This, to guard against the case where a thread switch happens after the counter is updated but before KeLeaveCriticalRegion is called

	ASM_BEGIN
		ASM(mov ecx, CriticalSection);
	ASM(dec[ecx]RTL_CRITICAL_SECTION.RecursionCount);
	ASM(jnz dec_count);
	ASM(dec[ecx]RTL_CRITICAL_SECTION.LockCount);
	ASM(mov[ecx]RTL_CRITICAL_SECTION.OwningThread, 0);
	ASM(jl not_signalled);
	ASM(push FALSE);
	ASM(push PRIORITY_BOOST_EVENT);
	ASM(push ecx);
	ASM(call KeSetEvent);
not_signalled:
	ASM(call KeLeaveCriticalRegion);
	ASM(jmp end_func);
dec_count:
	ASM(dec[ecx]RTL_CRITICAL_SECTION.LockCount);
end_func:
	ASM_END
}

