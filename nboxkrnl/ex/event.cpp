/*
 * ergo720                Copyright (c) 2024
 * Reactos Developers
 */

#include "ex.hpp"
#include <dbg.hpp>


EXPORTNUM(16) OBJECT_TYPE ExEventObjectType = {
	ExAllocatePoolWithTag,
	ExFreePool,
	nullptr,
	nullptr,
	nullptr,
	(PVOID)offsetof(KEVENT, Header),
	'vevE'
};


/*
 * @implemented
 */
EXPORTNUM(186) NTSTATUS NTAPI NtClearEvent
(
    IN HANDLE EventHandle
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(EventHandle, &ExEventObjectType, (PVOID*)&Event);

/* Check for Success */
    if (NT_SUCCESS(Status))
    {
        /* Clear the Event and Dereference */
        KeClearEvent(Event);
        ObDereferenceObject(Event);
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
EXPORTNUM(189) NTSTATUS NTAPI NtCreateEvent
(
    OUT PHANDLE EventHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
)
{
    PKEVENT Event;
    HANDLE hEvent;
    NTSTATUS Status;
    PAGED_CODE();

    /* Create the Object */
    Status = ObCreateObject(&ExEventObjectType,
        ObjectAttributes,
        sizeof(KEVENT),
        (PVOID*)&Event);

    /* Check for Success */
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Event */
        KeInitializeEvent(Event,
            EventType,
            InitialState);

        /* Insert it */
        Status = ObInsertObject((PVOID)Event,
            NULL,
            0,
            &hEvent);

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Enter SEH for return */
            *EventHandle = hEvent;
        }
    }

    /* Return Status */
    return Status;
}


/*
 * @implemented
 */
NTSTATUS NTAPI NtOpenEvent
(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
    HANDLE hEvent;
    NTSTATUS Status;
    PAGED_CODE();
    #if DBG
    DbgPrint("NtOpenEvent(0x%p, 0x%x, 0x%p)\n",
        EventHandle, DesiredAccess, ObjectAttributes);
    #endif

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
        &ExEventObjectType,
        NULL,
        &hEvent);

/* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Return the handle to the caller */
        *EventHandle = hEvent;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
EXPORTNUM(205) NTSTATUS NTAPI NtPulseEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();
    #if DBG
    DbgPrint("NtPulseEvent(EventHandle 0%p PreviousState 0%p)\n",
        EventHandle, PreviousState);
    #endif

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
        &ExEventObjectType,
        (PVOID*)&Event);

/* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Pulse the Event */
        LONG Prev = KePulseEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if (PreviousState)
        {
            /* Return previous state */
            *PreviousState = Prev;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
EXPORTNUM(209) NTSTATUS NTAPI NtQueryEvent
(
    IN HANDLE EventHandle,
    OUT PEVENT_BASIC_INFORMATION BasicInfo
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();

    if (!BasicInfo)
        return STATUS_INVALID_PARAMETER_2;

    /* Get the Object */
    Status = ObReferenceObjectByHandle(EventHandle, &ExEventObjectType, (PVOID*)&Event);

/* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Return Event Type and State */
        BasicInfo->EventType = (EVENT_TYPE)Event->Header.Type;
        BasicInfo->EventState = KeReadStateEvent(Event);

        /* Dereference the Object */
        ObDereferenceObject(Event);
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI NtResetEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();
    #if DBG
    DbgPrint("NtResetEvent(EventHandle 0%p PreviousState 0%p)\n",
        EventHandle, PreviousState);
    #endif

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
        &ExEventObjectType,
        (PVOID*)&Event);

/* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Reset the Event */
        LONG Prev = KeResetEvent(Event);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if (PreviousState)
        {
             /* Return previous state */
            *PreviousState = Prev;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI NtSetEvent
(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();
    #if DBG
    DbgPrint("NtSetEvent(EventHandle 0%p PreviousState 0%p)\n",
        EventHandle, PreviousState);
    #endif

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
        &ExEventObjectType,
        (PVOID*)&Event);
    if (NT_SUCCESS(Status))
    {
        /* Set the Event */
        LONG Prev = KeSetEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if (PreviousState)
        {
            /* Return previous state */
            *PreviousState = Prev;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI NtSetEventBoostPriority
(
    IN HANDLE EventHandle
)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
        &ExEventObjectType,
        (PVOID*)&Event);
    if (NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEventBoostPriority(Event, NULL);
        ObDereferenceObject(Event);
    }

    /* Return Status */
    return Status;
}
