#include "ex.hpp"
#include "ke.hpp"
#include <ps.hpp>

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

VOID NTAPI ExpTimerApcKernelRoutine
(
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE* NormalRoutine,
    IN OUT PVOID* NormalContext,
    IN OUT PVOID* SystemArgument1,
    IN OUT PVOID* SystemArguemnt2
)
{
    PETIMER Timer;
    KIRQL OldIrql;
    ULONG DerefsToDo = 1;
    PETHREAD Thread = PsGetCurrentThread();

    /* We need to find out which Timer we are */
    Timer = CONTAINING_RECORD(Apc, ETIMER, TimerApc);

    /* Lock the Timer */
    KeAcquireSpinLock(&Timer->Lock, &OldIrql);

    #ifdef SMP
    /* Lock the Thread's Active Timer List*/
    KeAcquireSpinLockAtDpcLevel(&Thread->ActiveTimerListLock);
    #endif

    /* Make sure that the Timer is valid, and that it belongs to this thread */
    if ((Timer->ApcAssociated) && (&Thread->Tcb == Timer->TimerApc.Thread))
    {
        /* Check if it's not periodic */
        if (!Timer->Period)
        {
            /* Remove it from the Active Timers List */
            RemoveEntryList(&Timer->ActiveTimerListEntry);

            /* Disable it */
            Timer->ApcAssociated = FALSE;
            DerefsToDo++;
        }
    }
    else
    {
        /* Clear the normal routine */
        *NormalRoutine = NULL;
    }

    /* Release locks */
    #ifdef SMP
    KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);
    #endif
    KeReleaseSpinLock(&Timer->Lock, OldIrql);

    /* Dereference as needed */
    ObDereferenceObjectEx(Timer, DerefsToDo);
}

EXPORTNUM(229) NTSTATUS NTAPI NtSetTimerEx
(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN KPROCESSOR_MODE ApcMode,
    IN PVOID TimerContext OPTIONAL,
    IN BOOLEAN WakeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
)
{
    PETIMER Timer;
    KIRQL OldIrql;
    BOOLEAN State;
    PETHREAD Thread = PsGetCurrentThread();
   
    PETHREAD TimerThread;
    ULONG DerefsToDo = 1;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check for a valid Period */
    if (Period < 0) return STATUS_INVALID_PARAMETER_6;

    LARGE_INTEGER TimerDueTime = *DueTime;

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle, &ExTimerObjectType, (PVOID*)&Timer);

/*
 * Tell the user we don't support Wake Timers...
 * when we have the ability to use/detect the Power Management
 * functionality required to support them, make this check dependent
 * on the actual PM capabilities
 */
    if (NT_SUCCESS(Status) && WakeTimer)
    {
        Status = STATUS_TIMER_RESUME_IGNORED;
    }

    /* Check status */
    if (NT_SUCCESS(Status))
    {
        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Cancel Running Timer */
        if (Timer->ApcAssociated)
        {
            /* Get the Thread. */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread,
                ETHREAD,
                Tcb);

            #ifdef SMP
            /* Lock its active list */
            KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);
            #endif

            /* Remove it */
            RemoveEntryList(&Timer->ActiveTimerListEntry);
            Timer->ApcAssociated = FALSE;

            #ifdef SMP
            /* Unlock the list */
            KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);
            #endif

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

        /* Read the State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Handle Wake Timers */
        Timer->WakeTimer = WakeTimer;
        KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);
        if ((WakeTimer) && !(Timer->WakeTimerListEntry.Flink))
        {
            /* Insert it into the list */
            InsertTailList(&ExpWakeList, &Timer->WakeTimerListEntry);
        }
        else if (!(WakeTimer) && (Timer->WakeTimerListEntry.Flink))
        {
            /* Remove it from the list */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimerListEntry.Flink = NULL;
        }
        KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);

        /* Set up the APC Routine if specified */
        Timer->Period = Period;
        if (TimerApcRoutine)
        {
            /* Initialize the APC */
            KeInitializeApc(&Timer->TimerApc,
                &Thread->Tcb,
                ExpTimerApcKernelRoutine,
                (PKRUNDOWN_ROUTINE)NULL,
                (PKNORMAL_ROUTINE)TimerApcRoutine,
                ApcMode,
                TimerContext);

            /* Lock the Thread's Active List and Insert */
            #ifdef SMP
            KeAcquireSpinLockAtDpcLevel(&Thread->ActiveTimerListLock);
            #endif
            InsertTailList(&Thread->ActiveTimerListHead,
                &Timer->ActiveTimerListEntry);
            Timer->ApcAssociated = TRUE;
            #ifdef SMP
            KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);
            #endif

            /* One less dereference to do */
            DerefsToDo--;
        }

       /* Enable and Set the Timer */
        KeSetTimerEx(&Timer->KeTimer,
            TimerDueTime,
            Period,
            TimerApcRoutine ? &Timer->TimerDpc : NULL);

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference if it was previously enabled */
        if (DerefsToDo) ObDereferenceObjectEx(Timer, DerefsToDo);

        /* Check if we need to return the State */
        if (PreviousState)
        {
            *PreviousState = State;
        }
    }

    /* Return to Caller */
    return Status;
}

