/*
 * ergo720                Copyright (c) 2023
 * LukeUsher              Copyright (c) 2018
 */

#include "ki.hpp"
#include "rtl.hpp"
#include "ex.hpp"


// Source: Cxbx-Reloaded
static VOID KiWaitSatisfyMutant(PKMUTANT Mutant, PKTHREAD Thread)
{
	Mutant->Header.SignalState -= 1;
	if (Mutant->Header.SignalState == 0) {
		Mutant->OwnerThread = Thread;
		if (Mutant->Abandoned == TRUE) {
			Mutant->Abandoned = FALSE;
			Thread->WaitStatus = STATUS_ABANDONED;
		}

		InsertHeadList(Thread->MutantListHead.Blink, &Mutant->MutantListEntry);
	}
}

// Source: Cxbx-Reloaded
static VOID KiWaitSatisfyAny(PVOID Object, PKTHREAD Thread)
{
	PKMUTANT Mutant = (PKMUTANT)Object;
	if ((Mutant->Header.Type & SYNCHRONIZATION_OBJECT_TYPE_MASK) == EventSynchronizationObject) {
		Mutant->Header.SignalState = 0;
	}
	else if (Mutant->Header.Type == SemaphoreObject) {
		Mutant->Header.SignalState -= 1;
	}
	else if (Mutant->Header.Type == MutantObject) {
		KiWaitSatisfyMutant(Mutant, Thread);
	}
}

// Source: Cxbx-Reloaded
static VOID KiWaitSatisfyAll(PKWAIT_BLOCK WaitBlock)
{
	PKWAIT_BLOCK CurrWaitBlock = WaitBlock;
	PKTHREAD Thread = CurrWaitBlock->Thread;
	do {
		if (CurrWaitBlock->WaitKey != (USHORT)STATUS_TIMEOUT) {
			KiWaitSatisfyAny(CurrWaitBlock->Object, Thread);
		}
		CurrWaitBlock = CurrWaitBlock->NextWaitBlock;
	} while (CurrWaitBlock != WaitBlock);
}

EXPORTNUM(99) NTSTATUS XBOXAPI KeDelayExecutionThread
(
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Interval
)
{
	PKTHREAD Thread = KeGetCurrentThread();
	if (Thread->WaitNext) {
		Thread->WaitNext = FALSE;
	}
	else {
		Thread->WaitIrql = KeRaiseIrqlToDpcLevel();
	}

	NTSTATUS Status;
	LARGE_INTEGER DueTime, DummyTime;
	PLARGE_INTEGER CapturedTimeout = Interval;
	BOOLEAN HasWaited = FALSE;
	while (true) {
		Thread->WaitStatus = STATUS_SUCCESS;

		if (Alertable) {
			RIP_API_MSG("Thread alerts are not supported");
		}
		else if ((WaitMode == UserMode) && Thread->ApcState.UserApcPending) {
			Status = STATUS_USER_APC;
			break;
		}

		PKTIMER Timer = &Thread->Timer;
		PKWAIT_BLOCK WaitTimer = &Thread->TimerWaitBlock;
		Timer->Header.WaitListHead.Flink = &WaitTimer->WaitListEntry;
		Timer->Header.WaitListHead.Blink = &WaitTimer->WaitListEntry;
		WaitTimer->NextWaitBlock = WaitTimer;
		Thread->WaitBlockList = WaitTimer;
		if (KiInsertTimer(Timer, *Interval) == FALSE) {
			Status = STATUS_SUCCESS;
			break;
		}

		DueTime.QuadPart = Timer->DueTime.QuadPart;

		if (Thread->Queue) {
			RIP_API_MSG("Thread queues are not supported");
		}

		Thread->Alertable = Alertable;
		Thread->WaitMode = WaitMode;
		Thread->WaitReason = DelayExecution;
		Thread->WaitTime = KeTickCount;
		Thread->State = Waiting;
		InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

		Status = KiSwapThread(); // returns either with a kernel APC or when the wait is satisfied
		HasWaited = TRUE;

		if (Status == STATUS_USER_APC) {
			RIP_API_MSG("User APCs are not supported");
		}

		if (Status != STATUS_KERNEL_APC) {
			if (Status == STATUS_TIMEOUT) {
				return STATUS_SUCCESS;
			}
			return Status;
		}

		Interval = KiRecalculateTimerDueTime(CapturedTimeout, &DueTime, &DummyTime);

		Thread->WaitIrql = KeRaiseIrqlToDpcLevel();
	}

	if (HasWaited == FALSE) {
		KiAdjustQuantumThread();
	}

	KiUnlockDispatcherDatabase(Thread->WaitIrql);
	if (Status == STATUS_USER_APC) {
		RIP_API_MSG("User APCs are not supported");
	}

	return Status;
}

// Source: partially from Cxbx-Reloaded
EXPORTNUM(159) NTSTATUS XBOXAPI KeWaitForSingleObject
(
	PVOID Object,
	KWAIT_REASON WaitReason,
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
)
{
	PKTHREAD Thread = KeGetCurrentThread();
	if (Thread->WaitNext) {
		Thread->WaitNext = FALSE;
	}
	else {
		Thread->WaitIrql = KeRaiseIrqlToDpcLevel();
	}

	NTSTATUS Status;
	KWAIT_BLOCK LocalWaitBlock;
	LARGE_INTEGER DueTime, DummyTime;
	PLARGE_INTEGER CapturedTimeout = Timeout;
	PKWAIT_BLOCK WaitBlock = &LocalWaitBlock;
	BOOLEAN HasWaited = FALSE;
	while (true) {
		PKMUTANT Mutant = (PKMUTANT)Object;
		Thread->WaitStatus = STATUS_SUCCESS;

		if (Mutant->Header.Type == MutantObject) {
			if ((Mutant->Header.SignalState > 0) || (Thread == Mutant->OwnerThread)) { // not owned or owned by current thread
				if (Mutant->Header.SignalState != MUTANT_LIMIT) {
					KiWaitSatisfyMutant(Mutant, Thread);
					Status = Thread->WaitStatus;
					break;
				}
				else {
					KiUnlockDispatcherDatabase(Thread->WaitIrql);
					ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED); // won't return
				}
			}
		}
		else if (Mutant->Header.SignalState > 0) {
			if ((Mutant->Header.Type & SYNCHRONIZATION_OBJECT_TYPE_MASK) == EventSynchronizationObject) {
				Mutant->Header.SignalState = 0;
			}
			else if (Mutant->Header.Type == SemaphoreObject) {
				Mutant->Header.SignalState -= 1;
			}
			Status = STATUS_SUCCESS;
			break;
		}

		Thread->WaitBlockList = WaitBlock;
		WaitBlock->Object = Object;
		WaitBlock->WaitKey = (USHORT)STATUS_SUCCESS;
		WaitBlock->WaitType = WaitAny;
		WaitBlock->Thread = Thread;

		if (Alertable) {
			RIP_API_MSG("Thread alerts are not supported");
		}
		else if ((WaitMode == UserMode) && Thread->ApcState.UserApcPending) {
			Status = STATUS_USER_APC;
			break;
		}

		if (Timeout) {
			if (Timeout->QuadPart == 0) {
				Status = STATUS_TIMEOUT;
				break;
			}

			PKTIMER Timer = &Thread->Timer;
			PKWAIT_BLOCK WaitTimer = &Thread->TimerWaitBlock;
			WaitBlock->NextWaitBlock = WaitTimer;
			Timer->Header.WaitListHead.Flink = &WaitTimer->WaitListEntry;
			Timer->Header.WaitListHead.Blink = &WaitTimer->WaitListEntry;
			WaitTimer->NextWaitBlock = WaitBlock;
			if (KiInsertTimer(Timer, *Timeout) == FALSE) {
				Status = STATUS_TIMEOUT;
				break;
			}

			DueTime.QuadPart = Timer->DueTime.QuadPart;
		}
		else {
			WaitBlock->NextWaitBlock = WaitBlock;
		}

		InsertTailList(&Mutant->Header.WaitListHead, &WaitBlock->WaitListEntry);

		if (Thread->Queue) {
			RIP_API_MSG("Thread queues are not supported");
		}

		Thread->Alertable = Alertable;
		Thread->WaitMode = WaitMode;
		Thread->WaitReason = (UCHAR)WaitReason;
		Thread->WaitTime = KeTickCount;
		Thread->State = Waiting;
		InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

		Status = KiSwapThread(); // returns either with a kernel APC or when the wait is satisfied
		HasWaited = TRUE;

		if (Status == STATUS_USER_APC) {
			RIP_API_MSG("User APCs are not supported");
		}

		if (Status != STATUS_KERNEL_APC) {
			return Status;
		}

		if (Timeout) {
			Timeout = KiRecalculateTimerDueTime(CapturedTimeout, &DueTime, &DummyTime);
		}

		Thread->WaitIrql = KeRaiseIrqlToDpcLevel();
	}

	if (HasWaited == FALSE) {
		KiAdjustQuantumThread();
	}

	Thread->WaitStatus;

	KiUnlockDispatcherDatabase(Thread->WaitIrql);
	if (Status == STATUS_USER_APC) {
		RIP_API_MSG("User APCs are not supported");
	}

	return Status;
}

#define STATUS_WAIT_0 0

#define KxMultiThreadWait()                                                 \
    /* Link wait block array to the thread */                               \
    Thread->WaitBlockList = WaitBlockArray;                                 \
                                                                            \
    /* Reset the index */                                                   \
    Index = 0;                                                              \
                                                                            \
    /* Loop wait blocks */                                                  \
    do                                                                      \
    {                                                                       \
        /* Fill out the wait block */                                       \
        WaitBlock = &WaitBlockArray[Index];                                 \
        WaitBlock->Object = Object[Index];                                  \
        WaitBlock->WaitKey = (USHORT)Index;                                 \
        WaitBlock->WaitType = WaitType;                                     \
        WaitBlock->Thread = Thread;                                         \
                                                                            \
        /* Link to next block */                                            \
        WaitBlock->NextWaitBlock = &WaitBlockArray[Index + 1];              \
        Index++;                                                            \
    } while (Index < Count);                                                \
                                                                            \
    /* Link the last block */                                               \
    WaitBlock->NextWaitBlock = WaitBlockArray;                              \
                                                                            \
    /* Set default wait status */                                           \
    Thread->WaitStatus = STATUS_WAIT_0;                                     \
                                                                            \
    /* Check if we have a timer */                                          \
    if (Timeout)                                                            \
    {                                                                       \
        /* Link to the block */                                             \
        TimerBlock->NextWaitBlock = WaitBlockArray;                         \
                                                                            \
        /* Setup the timer */                                               \
        KxSetTimerForThreadWait(Timer, *Timeout, &Hand);                    \
                                                                            \
        /* Save the due time for the caller */                              \
        DueTime.QuadPart = Timer->DueTime.QuadPart;                         \
                                                                            \
        /* Initialize the list */                                           \
        InitializeListHead(&Timer->Header.WaitListHead);                    \
    }                                                                       \
                                                                            \
    /* Set wait settings */                                                 \
    Thread->Alertable = Alertable;                                          \
    Thread->WaitMode = WaitMode;                                            \
    Thread->WaitReason = WaitReason;                                        \
                                                                            \
    /* Check if we can swap the thread's stack */                           \
    Thread->WaitListEntry.Flink = NULL;                                     \
    Swappable = KiCheckThreadStackSwap(Thread, WaitMode);                   \
                                                                            \
    /* Set the wait time */                                                 \
    Thread->WaitTime = KeTickCount.LowPart;
/*
 * @implemented
 */
EXPORTNUM(158) NTSTATUS NTAPI KeWaitForMultipleObjects
(
	IN ULONG Count,
	IN PVOID Object[],
	IN WAIT_TYPE WaitType,
	IN KWAIT_REASON WaitReason,
	IN KPROCESSOR_MODE WaitMode,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL,
	OUT PKWAIT_BLOCK WaitBlockArray OPTIONAL
)
{
	#if 0
	PKMUTANT CurrentObject;
	PKWAIT_BLOCK WaitBlock;
	PKTHREAD Thread = KeGetCurrentThread();
	PKWAIT_BLOCK TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
	PKTIMER Timer = &Thread->Timer;
	NTSTATUS WaitStatus = STATUS_SUCCESS;
	BOOLEAN Swappable;
	PLARGE_INTEGER OriginalDueTime = Timeout;
	LARGE_INTEGER DueTime ={ {0} }, NewDueTime, InterruptTime;
	ULONG Index, Hand = 0;

	if (Thread->WaitNext)
	{
		NT_ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);
	}
	else if (!Timeout || (Timeout->QuadPart != 0))
	{
		NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	}
	else
	{
		NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	}

	/* Make sure the Wait Count is valid */
	if (!WaitBlockArray)
	{
		/* Check in regards to the Thread Object Limit */
		if (Count > THREAD_WAIT_OBJECTS)
		{
			/* Bugcheck */
			KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
		}

		/* Use the Thread's Wait Block */
		WaitBlockArray = &Thread->WaitBlock[0];
	}
	else
	{
		/* Using our own Block Array, so check with the System Object Limit */
		if (Count > MAXIMUM_WAIT_OBJECTS)
		{
			/* Bugcheck */
			KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
		}
	}

	/* Sanity check */
	NT_ASSERT(Count != 0);

	/* Check if the lock is already held */
	if (!Thread->WaitNext) goto WaitStart;

	/*  Otherwise, we already have the lock, so initialize the wait */
	Thread->WaitNext = FALSE;
	/*  Note that KxMultiThreadWait is a macro, defined in ke_x.h, that  */
	/*  uses  (and modifies some of) the following local                 */
	/*  variables:                                                       */
	/*  Thread, Index, WaitBlock, Timer, Timeout, Hand and Swappable.    */
	/*  If it looks like this code doesn't actually wait for any objects */
	/*  at all, it's because the setup is done by that macro.            */
	KxMultiThreadWait();

	/* Start wait loop */
	for (;;)
	{
		/* Disable pre-emption */
		Thread->Preempted = FALSE;

		/* Check if a kernel APC is pending and we're below APC_LEVEL */
		if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
			(Thread->WaitIrql < APC_LEVEL))
		{
			/* Unlock the dispatcher */
			KiReleaseDispatcherLock(Thread->WaitIrql);
		}
		else
		{
			/* Check what kind of wait this is */
			Index = 0;
			if (WaitType == WaitAny)
			{
				/* Loop blocks */
				do
				{
					/* Get the Current Object */
					CurrentObject = (PKMUTANT)Object[Index];
					NT_ASSERT(CurrentObject->Header.Type != QueueObject);

					/* Check if the Object is a mutant */
					if (CurrentObject->Header.Type == MutantObject)
					{
						/* Check if it's signaled */
						if ((CurrentObject->Header.SignalState > 0) ||
							(Thread == CurrentObject->OwnerThread))
						{
							/* This is a Wait Any, so unwait this and exit */
							if (CurrentObject->Header.SignalState !=
								(LONG)MINLONG)
							{
								/* Normal signal state, unwait it and return */
								KiSatisfyMutantWait(CurrentObject, Thread);
								WaitStatus = (NTSTATUS)Thread->WaitStatus | Index;
								goto DontWait;
							}
							else
							{
								/* Raise an exception (see wasm.ru) */
								KiReleaseDispatcherLock(Thread->WaitIrql);
								ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
							}
						}
					}
					else if (CurrentObject->Header.SignalState > 0)
					{
						/* Another signaled object, unwait and return */
						KiSatisfyNonMutantWait(CurrentObject);
						WaitStatus = Index;
						goto DontWait;
					}

					/* Go to the next block */
					Index++;
				} while (Index < Count);
			}
			else
			{
				/* Loop blocks */
				do
				{
					/* Get the Current Object */
					CurrentObject = (PKMUTANT)Object[Index];
					NT_ASSERT(CurrentObject->Header.Type != QueueObject);

					/* Check if we're dealing with a mutant again */
					if (CurrentObject->Header.Type == MutantObject)
					{
						/* Check if it has an invalid count */
						if ((Thread == CurrentObject->OwnerThread) &&
							(CurrentObject->Header.SignalState == (LONG)MINLONG))
						{
							/* Raise an exception */
							KiReleaseDispatcherLock(Thread->WaitIrql);
							ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
						}
						else if ((CurrentObject->Header.SignalState <= 0) &&
							(Thread != CurrentObject->OwnerThread))
						{
							/* We don't own it, can't satisfy the wait */
							break;
						}
					}
					else if (CurrentObject->Header.SignalState <= 0)
					{
						/* Not signaled, can't satisfy */
						break;
					}

					/* Go to the next block */
					Index++;
				} while (Index < Count);

				/* Check if we've went through all the objects */
				if (Index == Count)
				{
					/* Loop wait blocks */
					WaitBlock = WaitBlockArray;
					do
					{
						/* Get the object and satisfy it */
						CurrentObject = (PKMUTANT)WaitBlock->Object;
						KiSatisfyObjectWait(CurrentObject, Thread);

						/* Go to the next block */
						WaitBlock = WaitBlock->NextWaitBlock;
					} while (WaitBlock != WaitBlockArray);

					/* Set the wait status and get out */
					WaitStatus = (NTSTATUS)Thread->WaitStatus;
					goto DontWait;
				}
			}

			/* Make sure we can satisfy the Alertable request */
			WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
			if (WaitStatus != STATUS_WAIT_0) break;

			/* Enable the Timeout Timer if there was any specified */
			if (Timeout)
			{
				/* Check if the timer expired */
				InterruptTime.QuadPart = KeQueryInterruptTime();
				if ((ULONGLONG)InterruptTime.QuadPart >=
					Timer->DueTime.QuadPart)
				{
					/* It did, so we don't need to wait */
					WaitStatus = STATUS_TIMEOUT;
					goto DontWait;
				}

				/* It didn't, so activate it */
				Timer->Header.Inserted = TRUE;

				/* Link the wait blocks */
				WaitBlock->NextWaitBlock = TimerBlock;
			}

			/* Insert into Object's Wait List*/
			WaitBlock = WaitBlockArray;
			do
			{
				/* Get the Current Object */
				CurrentObject = WaitBlock->Object;

				/* Link the Object to this Wait Block */
				InsertTailList(&CurrentObject->Header.WaitListHead,
					&WaitBlock->WaitListEntry);

	 /* Move to the next Wait Block */
				WaitBlock = WaitBlock->NextWaitBlock;
			} while (WaitBlock != WaitBlockArray);

			/* Handle Kernel Queues */
			if (Thread->Queue) KiActivateWaiterQueue(Thread->Queue);

			/* Setup the wait information */
			Thread->State = Waiting;

			/* Add the thread to the wait list */
			KiAddThreadToWaitList(Thread, Swappable);

			/* Activate thread swap */
			NT_ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
			KiSetThreadSwapBusy(Thread);

			/* Check if we have a timer */
			if (Timeout)
			{
				/* Insert it */
				KxInsertTimer(Timer, Hand);
			}
			else
			{
				/* Otherwise, unlock the dispatcher */
				KiReleaseDispatcherLockFromSynchLevel();
			}

			/* Swap the thread */
			WaitStatus = (NTSTATUS)KiSwapThread(Thread, KeGetCurrentPrcb());

			/* Check if we were executing an APC */
			if (WaitStatus != STATUS_KERNEL_APC) return WaitStatus;

			/* Check if we had a timeout */
			if (Timeout)
			{
				/* Recalculate due times */
				Timeout = KiRecalculateDueTime(OriginalDueTime,
					&DueTime,
					&NewDueTime);
			}
		}

	WaitStart:
			/* Setup a new wait */
		Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
		KxMultiThreadWait();
		KiAcquireDispatcherLockAtSynchLevel();
	} 

	/* We are done */
	KiReleaseDispatcherLock(Thread->WaitIrql);
	return WaitStatus;

DontWait:
	/* Release dispatcher lock but maintain high IRQL */
	KiReleaseDispatcherLockFromSynchLevel();

	/* Adjust the Quantum and return the wait status */
	KiAdjustQuantumThread(Thread);
	return WaitStatus;
	#endif
	return STATUS_NOT_IMPLEMENTED;
}


VOID KiWaitTest(PVOID Object, KPRIORITY Increment)
{
	PKMUTANT FirstObject = (PKMUTANT)Object;
	PLIST_ENTRY WaitList = &FirstObject->Header.WaitListHead;
	PLIST_ENTRY WaitEntry = WaitList->Flink;

	while ((FirstObject->Header.SignalState > 0) && (WaitEntry != WaitList)) {
		PKWAIT_BLOCK WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
		PKTHREAD WaitThread = WaitBlock->Thread;

		if (WaitBlock->WaitType == WaitAny) {
			KiWaitSatisfyAny(FirstObject, WaitThread);
		}
		else {
			PKWAIT_BLOCK NextBlock = WaitBlock->NextWaitBlock;
			while (NextBlock != WaitBlock) {
				if (NextBlock->WaitKey != STATUS_TIMEOUT) {
					PKMUTANT Mutant = (PKMUTANT)NextBlock->Object;
					if (Mutant->Header.Type == MutantObject) {
						RIP_API_MSG("Mutant objects are not supported");
					}
					else if (Mutant->Header.SignalState <= 0) {
						goto NextWaitEntry;
					}
				}

				NextBlock = NextBlock->NextWaitBlock;
			}

			KiWaitSatisfyAll(WaitBlock);
		}

		KiUnwaitThread(WaitThread, WaitBlock->WaitKey, Increment);
	NextWaitEntry:
		WaitEntry = WaitEntry->Flink;
	}
}

VOID KiUnwaitThread(PKTHREAD Thread, LONG_PTR WaitStatus, KPRIORITY Increment)
{
	Thread->WaitStatus |= WaitStatus;
	PKWAIT_BLOCK WaitBlock = Thread->WaitBlockList;
	do {
		RemoveEntryList(&WaitBlock->WaitListEntry);
		WaitBlock = WaitBlock->NextWaitBlock;
	} while (WaitBlock != Thread->WaitBlockList);

	RemoveEntryList(&Thread->WaitListEntry);

	if (Thread->Timer.Header.Inserted) {
		KiRemoveTimer(&Thread->Timer);
	}

	if (Thread->Queue) {
		RIP_API_MSG("Thread queues are not supported");
	}

	if (Thread->Priority < LOW_REALTIME_PRIORITY) {
		KPRIORITY NewPriority = Thread->BasePriority + Increment;
		if (NewPriority > Thread->Priority) {
			if (NewPriority >= LOW_REALTIME_PRIORITY) {
				Thread->Priority = LOW_REALTIME_PRIORITY - 1;
			}
			else {
				Thread->Priority = (SCHAR)NewPriority;
			}
		}

		if (Thread->BasePriority >= TIME_CRITICAL_BASE_PRIORITY) {
			Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
		}
		else {
			Thread->Quantum -= WAIT_QUANTUM_DECREMENT;
			if (Thread->Quantum <= 0) {
				Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
				Thread->Priority -= 1;
				if (Thread->Priority < Thread->BasePriority) {
					Thread->Priority = Thread->BasePriority;
				}
			}
		}
	}
	else {
		Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
	}

	KiScheduleThread(Thread);
}
