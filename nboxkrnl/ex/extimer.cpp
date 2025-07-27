#include "ex.hpp"
#include "ke.hpp"

/* GLOBALS *******************************************************************/

// ******************************************************************
// * 0x001F - ExTimerObjectType
// * source: CXBX-reloaded
// ******************************************************************
EXPORTNUM(31) OBJECT_TYPE ExTimerObjectType =
{
    ExAllocatePoolWithTag,
    ExFreePool,
    NULL,
    NULL, // TODO : xbox::ExpDeleteTimer,
    NULL,
    (PVOID)offsetof(ETIMER, KeTimer.Header),
    'emiT' // = first four characters of "Timer" in reverse
};

/*
 * @implemented
 */
inline BOOLEAN NTAPI KeReadStateTimer(IN PKTIMER Timer)
{
    /* Return the Signal State */
    //ASSERT_TIMER(Timer);
    return (BOOLEAN)Timer->Header.SignalState;
};


KSPIN_LOCK ExpWakeListLock;
LIST_ENTRY ExpWakeList;

VOID NTAPI ExpDeleteTimer(IN PVOID ObjectBody)
{
    KIRQL OldIrql;
    PETIMER Timer = (PETIMER)ObjectBody;

    /* Check if it has a Wait List */
    if (Timer->WakeTimerListEntry.Flink)
    {
        /* Lock the Wake List */
        KeAcquireSpinLock(&ExpWakeListLock, &OldIrql);

        /* Check again, since it might've changed before we locked */
        if (Timer->WakeTimerListEntry.Flink)
        {
            /* Remove it from the Wait List */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimerListEntry.Flink = NULL;
        }

        /* Release the Wake List */
        KeReleaseSpinLock(&ExpWakeListLock, OldIrql);
    }

    /* Tell the Kernel to cancel the Timer and flush all queued DPCs */
    KeCancelTimer(&Timer->KeTimer);
    KeFlushQueuedDpcs();
}


_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
ExpTimerDpcRoutine(IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    PETIMER Timer = (PETIMER)DeferredContext;
    BOOLEAN Inserted = FALSE;

    /* Reference the timer */
    if (!ObReferenceObjectSafe(Timer)) return;

    /* Lock the Timer */
    KeAcquireSpinLockAtDpcLevel(&Timer->Lock);

    /* Check if the timer is associated */
    if (Timer->ApcAssociated)
    {
        /* Queue the APC */
        Inserted = KeInsertQueueApc(&Timer->TimerApc,
            SystemArgument1,
            SystemArgument2,
            0);
    }

    /* Release the Timer */
    KeReleaseSpinLockFromDpcLevel(&Timer->Lock);

    /* Dereference it if we couldn't queue the APC */
    if (!Inserted) ObDereferenceObject(Timer);
}

EXPORTNUM(194) NTSTATUS NTAPI NtCreateTimer
(
    OUT PHANDLE TimerHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
)
{
    PETIMER Timer;
    HANDLE hTimer;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check for correct timer type */
    if ((TimerType != NotificationTimer) &&
        (TimerType != SynchronizationTimer))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }


    /* Create the Object */
    Status = ObCreateObject(&ExTimerObjectType,
        ObjectAttributes,
        sizeof(ETIMER),
        (PVOID*)&Timer);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the DPC */
        KeInitializeDpc(&Timer->TimerDpc, ExpTimerDpcRoutine, Timer);

        /* Initialize the Kernel Timer */
        KeInitializeTimerEx(&Timer->KeTimer, TimerType);

        /* Initialize the timer fields */
        KeInitializeSpinLock(&Timer->Lock);
        Timer->ApcAssociated = FALSE;
        Timer->WakeTimer = FALSE;
        Timer->WakeTimerListEntry.Flink = NULL;

        /* Insert the Timer */
        Status = ObInsertObject((PVOID)Timer,
            NULL,
            0,
            &hTimer);

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            
            *TimerHandle = hTimer;
        }
    }

    /* Return to Caller */
    return Status;
}


EXPORTNUM(185) NTSTATUS NTAPI NtCancelTimer
(
    IN HANDLE TimerHandle,
    OUT BOOLEAN *CurrentState OPTIONAL
)
{
    PETIMER Timer;
    BOOLEAN State;
    KIRQL OldIrql;
    PETHREAD TimerThread;
    ULONG DerefsToDo = 1;
    PAGED_CODE();


    /* Get the Timer Object */
    NTSTATUS Status = ObReferenceObjectByHandle(TimerHandle, &ExTimerObjectType, reinterpret_cast<PVOID*>(&Timer));
    if (NT_SUCCESS(Status))
    {
        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Check if it's enabled */
        if (Timer->ApcAssociated)
        {
            /* Get the Thread. */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread,
                ETHREAD,
                Tcb);
            // SMP: add lock
            /* Lock its active list */
            //KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Remove it */
            RemoveEntryList(&Timer->ActiveTimerListEntry);
            Timer->ApcAssociated = FALSE;

            // SMP: release lock
            /* Unlock the list */
            //KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Cancel the Timer */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            if (KeRemoveQueueApc(&Timer->TimerApc)) DerefsToDo++;
            DerefsToDo++;
        }
        else
        {
            /* If timer was disabled, we still need to cancel it */
            KeCancelTimer(&Timer->KeTimer);
        }

        /* Handle a Wake Timer */
        if (Timer->WakeTimerListEntry.Flink)
        {
            /* Lock the Wake List */
            KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);

            /* Check again, since it might've changed before we locked */
            if (Timer->WakeTimerListEntry.Flink)
            {
                /* Remove it from the Wait List */
                RemoveEntryList(&Timer->WakeTimerListEntry);
                Timer->WakeTimerListEntry.Flink = NULL;
            }

            /* Release the Wake List */
            KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);
        }

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Read the old State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Dereference the Object */
        ObDereferenceObjectEx(Timer, DerefsToDo);

        /* Check if caller wants the state */
        if (CurrentState)
        {
            *CurrentState = State;
        }
    }

    /* Return to Caller */
    return Status;
}

EXPORTNUM(216) NTSTATUS NTAPI NtQueryTimer
(
    IN HANDLE TimerHandle,
    OUT PTIMER_BASIC_INFORMATION  TimerInformation
)
{
    PAGED_CODE();

    if (!TimerInformation)
        return STATUS_INVALID_PARAMETER_2;

    PETIMER Timer;

    /* Get the Timer Object */
    NTSTATUS Status = ObReferenceObjectByHandle(TimerHandle, &ExTimerObjectType, (PVOID*)&Timer);
    if (NT_SUCCESS(Status))
    {
        /* Return the remaining time, corrected */
        TimerInformation->TimeRemaining.QuadPart = Timer->KeTimer.DueTime.QuadPart - KeQueryInterruptTime();

        /* Return the current state */
        TimerInformation->SignalState = KeReadStateTimer(&Timer->KeTimer);

        /* Dereference Object */
        ObDereferenceObject(Timer);
    }

    /* Return Status */
    return Status;
}

