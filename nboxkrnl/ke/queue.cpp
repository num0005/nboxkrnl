/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "ke.hpp"
#include <ki.hpp>

/* PRIVATE FUNCTIONS *********************************************************/

// TODO: check if the queue is valid
#define ASSERT_QUEUE(expr) (void)(0)
//#define KiInsertDeferredReadyList(thread) KiDeferredReadyThread(thread)
//#define KiReadyThread(thread) KiInsertDeferredReadyList(thread)
//#define KxQueueThreadWait()
inline void KiAddThreadToWaitList
(
    IN PKTHREAD Thread,
    BOOLEAN& Swappable
)
{
    /* Make sure it's swappable */
    if (Swappable)
    {
        /* Insert it into the PRCB's List */
       // InsertTailList(&KeGetCurrentPrcb()->WaitListHead,
        //    &Thread->WaitListEntry);
        InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);
    }
}
//
// Determines whether a thread should be added to the wait list
//
inline BOOLEAN KiCheckThreadStackSwap
(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE WaitMode
)
{
    /* Check the required conditions */
    if ((WaitMode != KernelMode) &&
        (Thread->Priority >= (LOW_REALTIME_PRIORITY + 9)))
    {
        /* We are go for swap */
        return TRUE;
    }
    else
    {
        /* Don't swap the thread */
        return FALSE;
    }
};

inline BOOLEAN KiQueueThreadWait
(
    PKTHREAD Thread, 
    PKWAIT_BLOCK WaitBlock,
    KPROCESSOR_MODE WaitMode,
    PVOID Queue,
    const PLARGE_INTEGER &Timeout,
    LARGE_INTEGER &DueTime
)
{
    PKTIMER Timer = &Thread->Timer;
    const PKWAIT_BLOCK TimerBlock = &Thread->TimerWaitBlock;

    /* Setup the Wait Block */
    Thread->WaitBlockList = WaitBlock;
    WaitBlock->WaitKey = STATUS_SUCCESS;
    WaitBlock->Object = Queue;
    WaitBlock->WaitType = WaitAny;
    WaitBlock->Thread = Thread;

    /* Clear wait status */
    Thread->WaitStatus = STATUS_SUCCESS;

    /* Check if we have a timer */
    if (Timeout)
    {
        /* Setup the timer */
      //  KxSetTimerForThreadWait(Timer, *Timeout, &Hand);

        /* Save the due time for the caller */
        DueTime.QuadPart = Timer->DueTime.QuadPart;

        /* Pointer to timer block */
        WaitBlock->NextWaitBlock = TimerBlock;
        TimerBlock->NextWaitBlock = WaitBlock;

        /* Link the timer to this Wait Block */
        Timer->Header.WaitListHead.Flink = &TimerBlock->WaitListEntry;
        Timer->Header.WaitListHead.Blink = &TimerBlock->WaitListEntry;
    }
    else
    {
        /* No timer block, just ourselves */
        WaitBlock->NextWaitBlock = WaitBlock;
    }

    /* Set wait settings */
    Thread->Alertable = FALSE;
    Thread->WaitMode = WaitMode;
    Thread->WaitReason = WrQueue;

    /* Check if we can swap the thread's stack */
    Thread->WaitListEntry.Flink = NULL;
    BOOLEAN Swappable = KiCheckThreadStackSwap(Thread, WaitMode);

    /* Set the wait time */
    Thread->WaitTime = KeTickCount;

    return Swappable;
}


/*
 * Called when a thread which has a queue entry is entering a wait state
 */
VOID FASTCALL KiActivateWaiterQueue(IN PKQUEUE Queue)
{
    PLIST_ENTRY QueueEntry;
    PLIST_ENTRY WaitEntry;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD Thread;
    ASSERT_QUEUE(Queue);

    /* Decrement the number of active threads */
    Queue->CurrentCount--;

    /* Make sure the counts are OK */
    if (Queue->CurrentCount < Queue->MaximumCount)
    {
        /* Get the Queue Entry */
        QueueEntry = Queue->EntryListHead.Flink;

        /* Get the Wait Entry */
        WaitEntry = Queue->Header.WaitListHead.Blink;

        /* Make sure that the Queue entries are not part of empty lists */
        if ((WaitEntry != &Queue->Header.WaitListHead) &&
            (QueueEntry != &Queue->EntryListHead))
        {
            /* Remove this entry */
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;

            /* Decrease the Signal State */
            Queue->Header.SignalState--;

            /* Unwait the Thread */
            WaitBlock = CONTAINING_RECORD(WaitEntry,
                                          KWAIT_BLOCK,
                                          WaitListEntry);
            Thread = WaitBlock->Thread;
            KiUnwaitThread(Thread, (LONG_PTR)QueueEntry, IO_NO_INCREMENT);
        }
    }
}

/*
 * Returns the previous number of entries in the queue
 */
LONG FASTCALL KiInsertQueue
(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    IN BOOLEAN Head
)
{
    ULONG InitialState;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    PKTIMER Timer;
    ASSERT_QUEUE(Queue);

    /* Save the old state */
    InitialState = Queue->Header.SignalState;

    /* Get the Entry */
    WaitEntry = Queue->Header.WaitListHead.Blink;

    /*
     * Why the KeGetCurrentThread()->Queue != Queue?
     * KiInsertQueue might be called from an APC for the current thread.
     * -Gunnar
     */
    if ((Queue->CurrentCount < Queue->MaximumCount) &&
        (WaitEntry != &Queue->Header.WaitListHead) &&
        ((Thread->Queue != Queue) ||
         (Thread->WaitReason != WrQueue)))
    {
        /* Remove the wait entry */
        RemoveEntryList(WaitEntry);

        /* Get the Wait Block and Thread */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;

        /* Remove the queue from the thread's wait list */
        Thread->WaitStatus = (LONG_PTR)Entry;
        if (Thread->WaitListEntry.Flink) RemoveEntryList(&Thread->WaitListEntry);

        /* Increase the active threads and remove any wait reason */
        Queue->CurrentCount++;
        Thread->WaitReason = 0;

        /* Check if there's a Thread Timer */
        Timer = &Thread->Timer;
        if (Timer->Header.Inserted) KxRemoveTreeTimer(Timer);

        /* Reschedule the Thread */
        KiReadyThread(Thread);
    }
    else
    {
        /* Increase the Entries */
        Queue->Header.SignalState++;

        /* Check which mode we're using */
        if (Head)
        {
            /* Insert in the head */
            InsertHeadList(&Queue->EntryListHead, Entry);
        }
        else
        {
            /* Insert at the end */
            InsertTailList(&Queue->EntryListHead, Entry);
        }
    }

    /* Return the previous state */
    return InitialState;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
EXPORTNUM(111) VOID NTAPI KeInitializeQueue
(
    IN PKQUEUE Queue,
    IN ULONG Count OPTIONAL
)
{
    /* Initialize the Header */
    Queue->Header.Type = QueueObject;
    Queue->Header.Size = sizeof(KQUEUE) / sizeof(ULONG);
    Queue->Header.SignalState = 0;
    Queue->Header.Absolute = 0;
    InitializeListHead(&(Queue->Header.WaitListHead));

    /* Initialize the Lists */
    InitializeListHead(&Queue->EntryListHead);
    InitializeListHead(&Queue->ThreadListHead);

    /* Set the Current and Maximum Count */
    Queue->CurrentCount = 0;
    Queue->MaximumCount = (Count == 0) ? (ULONG) KeNumberProcessors : Count;
}

/*
 * @implemented
 */
EXPORTNUM(116) LONG NTAPI KeInsertHeadQueue
(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry
)
{
    LONG PreviousState;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, TRUE);

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLock(OldIrql);

    /* Return previous State */
    return PreviousState;
}

/*
 * @implemented
 */
EXPORTNUM(117) LONG NTAPI KeInsertQueue
(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry
)
{
    LONG PreviousState;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, FALSE);

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLock(OldIrql);

    /* Return previous State */
    return PreviousState;
}

/*
 * @implemented
 */
EXPORTNUM(136) PLIST_ENTRY NTAPI KeRemoveQueue
(
    IN PKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
)
{
    PLIST_ENTRY QueueEntry;
    LONG_PTR Status;
    PKTHREAD Thread = KeGetCurrentThread();
    PKQUEUE PreviousQueue;
    //PKWAIT_BLOCK WaitBlock = &Thread->WaitBlock[0];
    //PKWAIT_BLOCK TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    KWAIT_BLOCK LocalWaitBlock = {};
   // PKWAIT_BLOCK WaitBlock = Thread->WaitBlockList;
    PKWAIT_BLOCK WaitBlock = &LocalWaitBlock;
    PKTIMER Timer = &Thread->Timer;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    LARGE_INTEGER DueTime = {{0}}, NewDueTime, InterruptTime;
    ULONG Hand = 0;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if the Lock is already held */
    if (Thread->WaitNext)
    {
        /* It is, so next time don't do expect this */
        Thread->WaitNext = FALSE;
        Swappable = KiQueueThreadWait(
            Thread,
            WaitBlock,
            WaitMode,
            Queue,
            Timeout,
            DueTime);
    }
    else
    {
        /* Raise IRQL to synch, prepare the wait, then lock the database */
        Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
        Swappable = KiQueueThreadWait(
            Thread,
            WaitBlock,
            WaitMode,
            Queue,
            Timeout,
            DueTime);
        KiAcquireDispatcherLockAtSynchLevel();
    }

    /*
     * This is needed so that we can set the new queue right here,
     * before additional processing
     */
    PreviousQueue = Thread->Queue;
    Thread->Queue = Queue;

    /* Check if this is a different queue */
    if (Queue != PreviousQueue)
    {
        /* Get the current entry */
        QueueEntry = &Thread->QueueListEntry;
        if (PreviousQueue)
        {
            /* Remove from this list */
            RemoveEntryList(QueueEntry);

            /* Wake the queue */
            KiActivateWaiterQueue(PreviousQueue);
        }

        /* Insert in this new Queue */
        InsertTailList(&Queue->ThreadListHead, QueueEntry);
    }
    else
    {
        /* Same queue, decrement waiting threads */
        Queue->CurrentCount--;
    }

    /* Loop until the queue is processed */
    while (TRUE)
    {
        /* Check if the counts are valid and if there is still a queued entry */
        QueueEntry = Queue->EntryListHead.Flink;
        if ((Queue->CurrentCount < Queue->MaximumCount) &&
            (QueueEntry != &Queue->EntryListHead))
        {
            /* Decrease the number of entries */
            Queue->Header.SignalState--;

            /* Increase numbef of running threads */
            Queue->CurrentCount++;

            /* Check if the entry is valid. If not, bugcheck */
            if (!(QueueEntry->Flink) || !(QueueEntry->Blink))
            {
                /* Invalid item */
                KeBugCheckEx(INVALID_WORK_QUEUE_ITEM,
                             (ULONG_PTR)QueueEntry,
                             (ULONG_PTR)Queue,
                             (ULONG_PTR)NULL,
                             (ULONG_PTR)NULL);
                           //  (ULONG_PTR)((PWORK_QUEUE_ITEM)QueueEntry)->
                            //             WorkerRoutine);
            }

            /* Remove the Entry */
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;

            /* Nothing to wait on */
            break;
        }
        else
        {
            /* Check if a kernel APC is pending and we're below APC_LEVEL */
            if ((Thread->ApcState.KernelApcPending) &&
                (Thread->WaitIrql < APC_LEVEL))
            {
                /* Increment the count and unlock the dispatcher */
                Queue->CurrentCount++;
                KiReleaseDispatcherLockFromSynchLevel();
                KiExitDispatcher(Thread->WaitIrql);
            }
            else
            {
                /* Fail if there's a User APC Pending */
                if ((WaitMode != KernelMode) &&
                    (Thread->ApcState.UserApcPending))
                {
                    /* Return the status and increase the pending threads */
                    QueueEntry = (PLIST_ENTRY)STATUS_USER_APC;
                    Queue->CurrentCount++;
                    break;
                }

                /* Enable the Timeout Timer if there was any specified */
                if (Timeout)
                {
                    /* Check if the timer expired */
                    InterruptTime.QuadPart = KeQueryInterruptTime();
                    if ((ULONGLONG)InterruptTime.QuadPart >= Timer->DueTime.QuadPart)
                    {
                        /* It did, so we don't need to wait */
                        QueueEntry = (PLIST_ENTRY)STATUS_TIMEOUT;
                        Queue->CurrentCount++;
                        break;
                    }

                    /* It didn't, so activate it */
                    Timer->Header.Inserted = TRUE;
                }

                /* Insert the wait block in the list */
                InsertTailList(&Queue->Header.WaitListHead,
                               &WaitBlock->WaitListEntry);

                /* Setup the wait information */
                Thread->State = Waiting;

                /* Add the thread to the wait list */
                if (Swappable)
                {
                    /* Insert it into the PRCB's List */
                  //  InsertTailList(&KeGetCurrentPrcb()->WaitListHead,
                  //      &Thread->WaitListEntry);
                }

                /* Activate thread swap */
                NT_ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
                KiSetThreadSwapBusy(Thread);

                /* Check if we have a timer */
                if (Timeout)
                {
                    /* Insert it */
                    KiInsertTimer(Timer, DueTime);
                }
                else
                {
                    /* Otherwise, unlock the dispatcher */
                    KiReleaseDispatcherLockFromSynchLevel();
                }

                /* Do the actual swap */
                Status = KiSwapThread(Thread, KeGetCurrentPrcb());

                /* Reset the wait reason */
                Thread->WaitReason = 0;

                /* Check if we were executing an APC */
                if (Status != STATUS_KERNEL_APC) return (PLIST_ENTRY)Status;

                /* Check if we had a timeout */
                if (Timeout)
                {
                    /* Recalculate due times */
                    Timeout = KiRecalculateDueTime(OriginalDueTime,
                                                   &DueTime,
                                                   &NewDueTime);
                }
            }

            /* Start another wait */
            Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
            Swappable = KiQueueThreadWait(
                Thread,
                WaitBlock,
                WaitMode,
                Queue,
                Timeout,
                DueTime);
            KiAcquireDispatcherLockAtSynchLevel();
            Queue->CurrentCount--;
        }
    }

    /* Unlock Database and return */
    KiReleaseDispatcherLockFromSynchLevel();
    KiExitDispatcher(Thread->WaitIrql);
    return QueueEntry;
}

/*
 * @implemented
 */
EXPORTNUM(141) PLIST_ENTRY NTAPI KeRundownQueue
(
    IN PKQUEUE Queue
)
{
    PLIST_ENTRY FirstEntry, NextEntry;
    PKTHREAD Thread;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    NT_ASSERT(IsListEmpty(&Queue->Header.WaitListHead));

    /* Get the Dispatcher Lock */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if the list is empty */
    FirstEntry = Queue->EntryListHead.Flink;
    if (FirstEntry == &Queue->EntryListHead)
    {
        /* We won't return anything */
        FirstEntry = NULL;
    }
    else
    {
        /* Remove this entry */
        RemoveEntryList(&Queue->EntryListHead);
    }

    /* Loop the list */
    while (!IsListEmpty(&Queue->ThreadListHead))
    {
        /* Get the next entry */
        NextEntry = Queue->ThreadListHead.Flink;

        /* Get the associated thread */
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, QueueListEntry);

        /* Clear its queue */
        Thread->Queue = NULL;

        /* Remove this entry */
        RemoveEntryList(NextEntry);
    }

    /* Release the dispatcher lock */
    KiReleaseDispatcherLockFromSynchLevel();

    /* Exit the dispatcher and return the first entry (if any) */
    KiExitDispatcher(OldIrql);
    return FirstEntry;
}

/* EOF */
