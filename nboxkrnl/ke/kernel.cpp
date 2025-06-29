/*
 * ergo720                Copyright (c) 2022
 * PatrickvL              Copyright (c) 2016-2017
 */

#pragma once

#include "ki.hpp"
#include "ex.hpp"
#include "..\kernel.hpp"
#include <assert.h>
#include "hal.hpp"

#define XBOX_ACPI_FREQUENCY 3375000 // 3.375 MHz

static_assert(sizeof(IoInfoBlockOc) == 36);


static LONG IoHostFileHandle = FIRST_FREE_HANDLE;

XBOX_KEY_DATA XboxCERTKey;

EXPORTNUM(120) volatile KSYSTEM_TIME KeInterruptTime = { 0, 0, 0 };

EXPORTNUM(154) volatile KSYSTEM_TIME KeSystemTime = { 0, 0, 0 };

EXPORTNUM(156) volatile DWORD KeTickCount = 0;

EXPORTNUM(157) ULONG KeTimeIncrement = CLOCK_TIME_INCREMENT;

EXPORTNUM(321) XBOX_KEY_DATA XboxEEPROMKey;

EXPORTNUM(322) XBOX_HARDWARE_INFO XboxHardwareInfo =
{
	0,     // Flags: 1=INTERNAL_USB, 2=DEVKIT, 4=MACROVISION, 8=CHIHIRO
	0xA2,  // GpuRevision, byte read from NV2A first register, at 0xFD0000000 - see NV_PMC_BOOT_0
	0xD3,  // McpRevision, Retail 1.6
	0,     // unknown
	0      // unknown
};

EXPORTNUM(324) XBOX_KRNL_VERSION XboxKrnlVersion =
{
	1,
	0,
	5838, // kernel build 5838
#if _DEBUG
	1 | (1 << 15) // =0x8000 -> debug kernel flag
#else
	1
#endif
};

// Source: Cxbx-Reloaded
EXPORTNUM(125) ULONGLONG XBOXAPI KeQueryInterruptTime()
{
	LARGE_INTEGER InterruptTime;

	while (true) {
		InterruptTime.u.HighPart = KeInterruptTime.HighTime;
		InterruptTime.u.LowPart = KeInterruptTime.LowTime;

		if (InterruptTime.u.HighPart == KeInterruptTime.High2Time) {
			break;
		}
	}

	return (ULONGLONG)InterruptTime.QuadPart;
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

// Source: Cxbx-Reloaded
EXPORTNUM(128) VOID XBOXAPI KeQuerySystemTime
(
	PLARGE_INTEGER CurrentTime
)
{
	LARGE_INTEGER SystemTime;

	while (true) {
		SystemTime.u.HighPart = KeSystemTime.HighTime;
		SystemTime.u.LowPart = KeSystemTime.LowTime;

		if (SystemTime.u.HighPart == KeSystemTime.High2Time) {
			break;
		}
	}

	*CurrentTime = SystemTime;
}

// Source: Cxbx-Reloaded
VOID KeSetSystemTime(PLARGE_INTEGER NewTime, PLARGE_INTEGER OldTime)
{
	KIRQL OldIrql, OldIrql2;
	LARGE_INTEGER DeltaTime;
	PLIST_ENTRY ListHead, NextEntry;
	PKTIMER Timer;
	LIST_ENTRY TempList, TempList2;
	ULONG Hand, i;

	/* Sanity checks */
	assert((NewTime->u.HighPart & 0xF0000000) == 0);
	assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	/* Lock the dispatcher, and raise IRQL */
	OldIrql = KeRaiseIrqlToDpcLevel();
	OldIrql2 = KfRaiseIrql(HIGH_LEVEL);

	/* Query the system time now */
	KeQuerySystemTime(OldTime);

	KeSystemTime.High2Time = NewTime->HighPart;
	KeSystemTime.LowTime = NewTime->LowPart;
	KeSystemTime.HighTime = NewTime->HighPart;

	/* Calculate the difference between the new and the old time */
	DeltaTime.QuadPart = NewTime->QuadPart - OldTime->QuadPart;

	KfLowerIrql(OldIrql2);

	/* Setup a temporary list of absolute timers */
	InitializeListHead(&TempList);

	/* Loop current timers */
	for (i = 0; i < TIMER_TABLE_SIZE; i++) {
		/* Loop the entries in this table and lock the timers */
		ListHead = &KiTimerTableListHead[i];
		NextEntry = ListHead->Flink;
		while (NextEntry != ListHead) {
			/* Get the timer */
			Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
			NextEntry = NextEntry->Flink;

			/* Is it absolute? */
			if (Timer->Header.Absolute) {
				/* Remove it from the timer list */
				KiRemoveTimer(Timer);

				/* Insert it into our temporary list */
				InsertTailList(&TempList, &Timer->TimerListEntry);
			}
		}
	}

	/* Setup a temporary list of expired timers */
	InitializeListHead(&TempList2);

	/* Loop absolute timers */
	while (TempList.Flink != &TempList) {
		/* Get the timer */
		Timer = CONTAINING_RECORD(TempList.Flink, KTIMER, TimerListEntry);
		RemoveEntryList(&Timer->TimerListEntry);

		/* Update the due time and handle */
		Timer->DueTime.QuadPart -= DeltaTime.QuadPart;
		Hand = KiComputeTimerTableIndex(Timer->DueTime.QuadPart);

		/* Lock the timer and re-insert it */
		if (KiReinsertTimer(Timer, Timer->DueTime)) {
			/* Remove it from the timer list */
			KiRemoveTimer(Timer);

			/* Insert it into our temporary list */
			InsertTailList(&TempList2, &Timer->TimerListEntry);
		}
	}

	/* Process expired timers. This releases the dispatcher and timer locks */
	KiTimerListExpire(&TempList2, OldIrql);
}
