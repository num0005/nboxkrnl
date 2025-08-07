/*
 * Luca1991              Copyright (c) 2017
 */

#include "ke.hpp"
#include "..\kernel.hpp"
#include <dbg.hpp>

#define ASSERT_DEVICE_QUEUE(Q)

// Source: Cxbx-Reloaded
EXPORTNUM(106) VOID XBOXAPI KeInitializeDeviceQueue
(
	PKDEVICE_QUEUE DeviceQueue
)
{
	DeviceQueue->Type = DeviceQueueObject;
	DeviceQueue->Size = sizeof(KDEVICE_QUEUE);
	DeviceQueue->Busy = FALSE;
	InitializeListHead(&DeviceQueue->DeviceListHead);
}

inline void KiAcquireDeviceQueueLock
(
    PKDEVICE_QUEUE DeviceQueue,
    KLOCK_QUEUE_HANDLE* DeviceLock
)
{
    DeviceLock->OldIRQL = KfRaiseIrql(DISPATCH_LEVEL);
}

inline void KiReleaseDeviceQueueLock
(
    KLOCK_QUEUE_HANDLE* DeviceLock
)
{
    KfLowerIrql(DeviceLock->OldIRQL);
}


/*
 * @implemented
 */
EXPORTNUM(115) BOOLEAN NTAPI KeInsertDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
)
{
    KLOCK_QUEUE_HANDLE DeviceLock;
    BOOLEAN Inserted;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeInsertDeviceQueue() DevQueue %p, Entry %p\n", DeviceQueue, DeviceQueueEntry);
    #endif

    /* Lock the queue (actually just goes up to DISPATCH_LEVEL on UP) */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Check if it's not busy */
    if (!DeviceQueue->Busy)
    {
        /* Set it as busy */
        Inserted = FALSE;
        DeviceQueue->Busy = TRUE;
    }
    else
    {
        /* Insert it into the list */
        Inserted = TRUE;
        InsertTailList(&DeviceQueue->DeviceListHead,
            &DeviceQueueEntry->DeviceListEntry);
    }

    /* Set the Insert state into the entry */
    DeviceQueueEntry->Inserted = Inserted;

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the state */
    return Inserted;
}

/*
 * @implemented
 */
EXPORTNUM(114) BOOLEAN NTAPI KeInsertByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
)
{
    KLOCK_QUEUE_HANDLE DeviceLock;
    BOOLEAN Inserted;
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY LastEntry;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeInsertByKeyDeviceQueue() DevQueue %p, Entry %p, SortKey 0x%x\n", DeviceQueue, DeviceQueueEntry, SortKey);
    #endif

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Set the Sort Key */
    DeviceQueueEntry->SortKey = SortKey;

    /* Check if it's not busy */
    if (!DeviceQueue->Busy)
    {
        /* Set it as busy */
        Inserted = FALSE;
        DeviceQueue->Busy = TRUE;
    }
    else
    {
        /* Make sure the list isn't empty */
        NextEntry = &DeviceQueue->DeviceListHead;
        if (!IsListEmpty(NextEntry))
        {
            /* Get the last entry */
            LastEntry = CONTAINING_RECORD(NextEntry->Blink,
                KDEVICE_QUEUE_ENTRY,
                DeviceListEntry);

/* Check if our sort key is lower */
            if (SortKey < LastEntry->SortKey)
            {
                /* Loop each sort key */
                do
                {
                    /* Get the next entry */
                    NextEntry = NextEntry->Flink;
                    LastEntry = CONTAINING_RECORD(NextEntry,
                        KDEVICE_QUEUE_ENTRY,
                        DeviceListEntry);

/* Keep looping until we find a place to insert */
                } while (SortKey >= LastEntry->SortKey);
            }
        }

        /* Now insert us */
        InsertTailList(NextEntry, &DeviceQueueEntry->DeviceListEntry);
        Inserted = TRUE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the state */
    return Inserted;
}

/*
 * @implemented
 */
EXPORTNUM(134) PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue
)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeRemoveDeviceQueue() DevQueue %p\n", DeviceQueue);
    #endif

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    NT_ASSERT(DeviceQueue->Busy);

    /* Check if this is an empty queue */
    if (IsListEmpty(&DeviceQueue->DeviceListHead))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* Remove the Entry from the List */
        ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
        ReturnEntry = CONTAINING_RECORD(ListEntry,
            KDEVICE_QUEUE_ENTRY,
            DeviceListEntry);

/* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
EXPORTNUM(133) PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveByKeyDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
)
{
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeRemoveByKeyDeviceQueue() DevQueue %p, SortKey 0x%x\n", DeviceQueue, SortKey);
    #endif

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    NT_ASSERT(DeviceQueue->Busy);

    /* Check if this is an empty queue */
    if (IsListEmpty(&DeviceQueue->DeviceListHead))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* If SortKey is greater than the last key, then return the first entry right away */
        NextEntry = &DeviceQueue->DeviceListHead;
        ReturnEntry = CONTAINING_RECORD(NextEntry->Blink,
            KDEVICE_QUEUE_ENTRY,
            DeviceListEntry);

/* Check if we can just get the first entry */
        if (ReturnEntry->SortKey <= SortKey)
        {
            /* Get the first entry */
            ReturnEntry = CONTAINING_RECORD(NextEntry->Flink,
                KDEVICE_QUEUE_ENTRY,
                DeviceListEntry);
        }
        else
        {
            /* Loop the list */
            NextEntry = DeviceQueue->DeviceListHead.Flink;
            while (TRUE)
            {
                /* Make sure we don't go beyond the end of the queue */
                NT_ASSERT(NextEntry != &DeviceQueue->DeviceListHead);

                /* Get the next entry and check if the key is low enough */
                ReturnEntry = CONTAINING_RECORD(NextEntry,
                    KDEVICE_QUEUE_ENTRY,
                    DeviceListEntry);
                if (SortKey <= ReturnEntry->SortKey) break;

                /* Try the next one */
                NextEntry = NextEntry->Flink;
            }
        }

        /* We have an entry, remove it now */
        RemoveEntryList(&ReturnEntry->DeviceListEntry);

        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveByKeyDeviceQueueIfBusy
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
)
{
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeRemoveByKeyDeviceQueueIfBusy() DevQueue %p, SortKey 0x%x\n", DeviceQueue, SortKey);
    #endif

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Check if this is an empty or idle queue */
    if (!(DeviceQueue->Busy) || (IsListEmpty(&DeviceQueue->DeviceListHead)))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* If SortKey is greater than the last key, then return the first entry right away */
        NextEntry = &DeviceQueue->DeviceListHead;
        ReturnEntry = CONTAINING_RECORD(NextEntry->Blink,
            KDEVICE_QUEUE_ENTRY,
            DeviceListEntry);

/* Check if we can just get the first entry */
        if (ReturnEntry->SortKey <= SortKey)
        {
            /* Get the first entry */
            ReturnEntry = CONTAINING_RECORD(NextEntry->Flink,
                KDEVICE_QUEUE_ENTRY,
                DeviceListEntry);
        }
        else
        {
            /* Loop the list */
            NextEntry = DeviceQueue->DeviceListHead.Flink;
            while (TRUE)
            {
                /* Make sure we don't go beyond the end of the queue */
                NT_ASSERT(NextEntry != &DeviceQueue->DeviceListHead);

                /* Get the next entry and check if the key is low enough */
                ReturnEntry = CONTAINING_RECORD(NextEntry,
                    KDEVICE_QUEUE_ENTRY,
                    DeviceListEntry);
                if (SortKey <= ReturnEntry->SortKey) break;

                /* Try the next one */
                NextEntry = NextEntry->Flink;
            }
        }

        /* We have an entry, remove it now */
        RemoveEntryList(&ReturnEntry->DeviceListEntry);

        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
EXPORTNUM(135) BOOLEAN NTAPI KeRemoveEntryDeviceQueue
(
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
)
{
    BOOLEAN OldState;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    #if DBG
    DbgPrint("KeRemoveEntryDeviceQueue() DevQueue %p, Entry %p\n", DeviceQueue, DeviceQueueEntry);
    #endif

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    NT_ASSERT(DeviceQueue->Busy);

    /* Check the insertion state */
    OldState = DeviceQueueEntry->Inserted;
    if (OldState)
    {
        /* Remove it */
        DeviceQueueEntry->Inserted = FALSE;
        RemoveEntryList(&DeviceQueueEntry->DeviceListEntry);
    }

    /* Unlock and return old state */
    KiReleaseDeviceQueueLock(&DeviceLock);
    return OldState;
}
