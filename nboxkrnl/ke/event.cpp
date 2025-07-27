/*
 * ergo720                Copyright (c) 2023
 * LukeUsher              Copyright (c) 2018
 */

#include "ke.hpp"
#include "rtl.hpp"

#define ASSERT_EVENT(event)

//
// Unwaits a Thread waiting on an event
//
inline void KxUnwaitThreadForEvent
(
	IN PKEVENT Event,
	IN KPRIORITY Increment
)
{
	PLIST_ENTRY WaitEntry, WaitList;
	PKWAIT_BLOCK WaitBlock;
	PKTHREAD WaitThread;

	/* Loop the Wait Entries */
	WaitList = &Event->Header.WaitListHead;
	NT_ASSERT(IsListEmpty(&Event->Header.WaitListHead) == FALSE);
	WaitEntry = WaitList->Flink;
	do
	{
		/* Get the current wait block */
		WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);

		/* Get the waiting thread */
		WaitThread = WaitBlock->Thread;

		/* Check the current Wait Mode */
		if (WaitBlock->WaitType == WaitAny)
		{
			/* Un-signal it */
			Event->Header.SignalState = 0;

			/* Un-signal the event and unwait the thread */
			KiUnwaitThread(WaitThread, WaitBlock->WaitKey, Increment);
			break;
		}

		/* Unwait the thread with STATUS_KERNEL_APC */
		KiUnwaitThread(WaitThread, STATUS_KERNEL_APC, Increment);

		/* Next entry */
		WaitEntry = WaitList->Flink;
	} while (WaitEntry != WaitList);
}

EXPORTNUM(108) VOID XBOXAPI KeInitializeEvent
(
	PKEVENT Event,
	EVENT_TYPE Type,
	BOOLEAN SignalState
)
{
	Event->Header.Type = Type;
	Event->Header.Size = sizeof(KEVENT) / sizeof(LONG);
	Event->Header.SignalState = SignalState;
	InitializeListHead(&(Event->Header.WaitListHead));
}

/*
 * @implemented
 */
EXPORTNUM(123) LONG NTAPI KePulseEvent
(
	IN PKEVENT Event,
	IN KPRIORITY Increment,
	IN BOOLEAN Wait
)
{
	KIRQL OldIrql;
	LONG PreviousState;
	PKTHREAD Thread;
	ASSERT_EVENT(Event);
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	/* Lock the Dispatcher Database */
	OldIrql = KiAcquireDispatcherLock();

	/* Save the Old State */
	PreviousState = Event->Header.SignalState;

	/* Check if we are non-signaled and we have stuff in the Wait Queue */
	if (!PreviousState && !IsListEmpty(&Event->Header.WaitListHead))
	{
		/* Set the Event to Signaled */
		Event->Header.SignalState = 1;

		/* Wake the Event */
		KiWaitTest(&Event->Header, Increment);
	}

	/* Unsignal it */
	Event->Header.SignalState = 0;

	/* Check what wait state was requested */
	if (Wait == FALSE)
	{
		/* Wait not requested, release Dispatcher Database and return */
		KiReleaseDispatcherLock(OldIrql);
	}
	else
	{
		/* Return Locked and with a Wait */
		Thread = KeGetCurrentThread();
		Thread->WaitNext = TRUE;
		Thread->WaitIrql = OldIrql;
	}

	/* Return the previous State */
	return PreviousState;
}

/*
 * @implemented
 */
EXPORTNUM(138) LONG NTAPI KeResetEvent
(
	IN PKEVENT Event
)
{
	KIRQL OldIrql;
	LONG PreviousState;
	ASSERT_EVENT(Event);
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	/* Lock the Dispatcher Database */
	OldIrql = KiAcquireDispatcherLock();

	/* Save the Previous State */
	PreviousState = Event->Header.SignalState;

	/* Set it to zero */
	Event->Header.SignalState = 0;

	/* Release Dispatcher Database and return previous state */
	KiReleaseDispatcherLock(OldIrql);
	return PreviousState;
}

// Source: Cxbx-Reloaded
EXPORTNUM(145) LONG XBOXAPI KeSetEvent
(
	PKEVENT Event,
	KPRIORITY Increment,
	BOOLEAN	Wait
)
{
	KIRQL OldIrql = KeRaiseIrqlToDpcLevel();
	LONG OldState = Event->Header.SignalState;

	if (IsListEmpty(&Event->Header.WaitListHead))
	{
		Event->Header.SignalState = 1;
	}
	else
	{
		PKWAIT_BLOCK WaitBlock = CONTAINING_RECORD(&Event->Header.WaitListHead.Flink, KWAIT_BLOCK, WaitListEntry);
		if ((Event->Header.Type == NotificationEvent) || (WaitBlock->WaitType == WaitAll))
		{
			if (OldState == 0)
			{
				Event->Header.SignalState = 1;
				KiWaitTest(Event, Increment);
			}
		}
		else
		{
			KiUnwaitThread(WaitBlock->Thread, WaitBlock->WaitKey, Increment);
		}
	}

	if (Wait)
	{
		PKTHREAD Thread = KeGetCurrentThread();
		Thread->WaitNext = Wait;
		Thread->WaitIrql = OldIrql;
	}
	else
	{
		KiUnlockDispatcherDatabase(OldIrql);
	}

	return OldState;
}

VOID NTAPI KeClearEvent
(
	IN PKEVENT Event
)
{
	ASSERT_EVENT(Event);

	/* Reset Signal State */
	Event->Header.SignalState = FALSE;
};

/*
 * @implemented
 */
LONG NTAPI KeReadStateEvent
(
	IN PKEVENT Event
)
{
	ASSERT_EVENT(Event);

	/* Return the Signal State */
	return Event->Header.SignalState;
};

EXPORTNUM(146) VOID NTAPI KeSetEventBoostPriority
(
	IN PKEVENT Event,
	IN PKTHREAD* WaitingThread OPTIONAL
)
{
	KIRQL OldIrql;
	PKWAIT_BLOCK WaitBlock;
	PKTHREAD Thread = KeGetCurrentThread(), WaitThread;
	NT_ASSERT(Event->Header.Type == EventSynchronizationObject);
	ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	/* Acquire Dispatcher Database Lock */
	OldIrql = KiAcquireDispatcherLock();

	/* Check if the list is empty */
	if (IsListEmpty(&Event->Header.WaitListHead))
	{
		/* Set the Event to Signaled */
		Event->Header.SignalState = 1;

		/* Return */
		KiReleaseDispatcherLock(OldIrql);
		return;
	}

	/* Get the Wait Block */
	WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
		KWAIT_BLOCK,
		WaitListEntry);

/* Check if this is a WaitAll */
	if (WaitBlock->WaitType == WaitAll)
	{
		/* Set the Event to Signaled */
		Event->Header.SignalState = 1;
		/* Unwait the thread and unsignal the event */
		KxUnwaitThreadForEvent(Event, EVENT_INCREMENT);
	}
	else
	{
		/* Return waiting thread to caller */
		WaitThread = WaitBlock->Thread;
		if (WaitingThread) *WaitingThread = WaitThread;

		/* Calculate new priority */
		Thread->Priority = KiComputeNewPriority(Thread, 0);

		/* Unlink the waiting thread */
		//KiUnlinkThread(WaitThread, STATUS_SUCCESS);

		/* Request priority boosting */
		//WaitThread->AdjustIncrement = Thread->Priority;
		//WaitThread->AdjustReason = AdjustBoost;

		/* Ready the thread */
		KiReadyThread(WaitThread);
	}

	/* Release the Dispatcher Database Lock */
	KiReleaseDispatcherLock(OldIrql);
}
