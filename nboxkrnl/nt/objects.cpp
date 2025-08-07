/*
 * ergo720                Copyright (c) 2023
 */

#include "nt.hpp"
#include "obp.hpp"
#include "rtl.hpp"
#include <string.h>
#include <ex.hpp>
#include <dbg.hpp>


EXPORTNUM(187) NTSTATUS XBOXAPI NtClose
(
	HANDLE Handle
)
{
	return ObfCloseHandle(Handle);
}

EXPORTNUM(197) NTSTATUS NTAPI NtDuplicateObject
(
	IN HANDLE SourceHandle,
	OUT PHANDLE TargetHandle OPTIONAL,
	IN ULONG Options
)
{
	/* Call the internal routine */
	NTSTATUS Status = ObfDuplicateObject(SourceHandle, TargetHandle);

	if (Options & DUPLICATE_CLOSE_SOURCE && NT_SUCCESS(Status))
		Status = NtClose(SourceHandle);

	return Status;
}

EXPORTNUM(188) NTSTATUS XBOXAPI NtCreateDirectoryObject
(
	PHANDLE DirectoryHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
)
{
	*DirectoryHandle = NULL_HANDLE;
	PVOID Object;
	NTSTATUS Status = ObCreateObject(&ObDirectoryObjectType, ObjectAttributes, sizeof(OBJECT_DIRECTORY), &Object);

	if (NT_SUCCESS(Status)) {
		memset(Object, 0, sizeof(OBJECT_DIRECTORY));
		Status = ObInsertObject(Object, ObjectAttributes, 0, DirectoryHandle);
	}

	return Status;
}

/* FUNCTIONS **************************************************************/

/*++
* @name NtOpenDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject routine opens a namespace directory object.
*
* @param DirectoryHandle
*        Variable which receives the directory handle.
*
* @param ObjectAttributes
*        Structure describing the directory.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
EXPORTNUM(201) NTSTATUS NTAPI NtOpenDirectoryObject
(
	OUT PHANDLE DirectoryHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	NT_ASSERT(DirectoryHandle);

	HANDLE Directory;
	NTSTATUS Status;
	PAGED_CODE();


	/* Open the directory object */
	Status = ObOpenObjectByName(ObjectAttributes,
		&ObDirectoryObjectType,
		NULL,
		&Directory);
	if (NT_SUCCESS(Status))
	{
		*DirectoryHandle = Directory;
	}

	/* Return the status to the caller */
	return Status;
}

EXPORTNUM(203) NTSTATUS XBOXAPI NtOpenSymbolicLinkObject
(
	PHANDLE LinkHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
)
{
	return ObOpenObjectByName(ObjectAttributes, &ObSymbolicLinkObjectType, nullptr, LinkHandle);
}

EXPORTNUM(233) NTSTATUS XBOXAPI NtWaitForSingleObject
(
	HANDLE Handle,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
)
{
	return NtWaitForSingleObjectEx(Handle, KernelMode, Alertable, Timeout);
}

#define TAG_WAIT 'tiaw'

EXPORTNUM(234) NTSTATUS XBOXAPI NtWaitForSingleObjectEx
(
	HANDLE Handle,
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
)
{
	PVOID Object;
	NTSTATUS Status = ObReferenceObjectByHandle(Handle, nullptr, &Object);

	if (NT_SUCCESS(Status)) {
		// The following has to extract the member DISPATCHER_HEADER::Header of the object to be waited on. If Handle refers to an object that cannot be waited on,
		// then OBJECT_TYPE::DefaultObject holds the address of ObpDefaultObject, which is a global KEVENT always signalled, and thus the wait will be satisfied
		// immediately. Notice that, because the kernel has a fixed load address of 0x80010000, then &ObpDefaultObject will be interpreted as a negative number,
		// allowing us to distinguish the can't wait/can wait kinds of objects apart.

		POBJECT_HEADER Obj = GetObjHeader(Object); // get Ob header (NOT the same as DISPATCHER_HEADER)
		PVOID ObjectToWaitOn = Obj->Type->DefaultObject;

		if ((LONG_PTR)ObjectToWaitOn >= 0) {
			ObjectToWaitOn = (PCHAR)Object + (ULONG_PTR)ObjectToWaitOn; // DefaultObject is the offset of DISPATCHER_HEADER::Header
		}

		// KeWaitForSingleObject will raise an exception if the Handle is a mutant and its limit is exceeded
		__try {
			Status = KeWaitForSingleObject(ObjectToWaitOn, UserRequest, WaitMode, Alertable, Timeout);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			Status = GetExceptionCode();
		}

		ObfDereferenceObject(Object);
	}

	return Status;
}

// todo: set these correctly
#define MAXIMUM_WAIT_OBJECTS 0x100
#define THREAD_WAIT_OBJECTS 0x20

EXPORTNUM(235) NTSTATUS NTAPI NtWaitForMultipleObjectsEx
(
    IN ULONG ObjectCount,
    IN PHANDLE Handles,
    IN WAIT_TYPE WaitType,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER TimeOut OPTIONAL
)
{
    PKWAIT_BLOCK WaitBlockArray;
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    ULONG i, ReferencedObjects, j;
    LARGE_INTEGER SafeTimeOut;
    BOOLEAN LockInUse;
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check for valid Object Count */
    if ((ObjectCount > MAXIMUM_WAIT_OBJECTS) || !(ObjectCount))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check for valid Wait Type */
    if ((WaitType != WaitAll) && (WaitType != WaitAny))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check if we can use the internal Wait Array */
    if (ObjectCount > THREAD_WAIT_OBJECTS)
    {
        /* Allocate from Pool */
        WaitBlockArray = ExNewArrayFromPool<KWAIT_BLOCK>(ObjectCount, TAG_WAIT);
        if (!WaitBlockArray)
        {
            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        /* No need for the array  */
        WaitBlockArray = NULL;
    }

    /* Enter a critical region since we'll play with handles */
    LockInUse = TRUE;
    KeEnterCriticalRegion();

    /* Start the loop */
    i = 0;
    ReferencedObjects = 0;
    do
    {
        Status = ObReferenceObjectByHandle(Handles[i], NULL, &Objects[i]);
        if (!NT_SUCCESS(Status))
            goto Exit;

        ReferencedObjects++;

        /* Get the Object Header */
        ObjectHeader = GetObjHeader(Objects[i]);

        PVOID ObjectToWaitOn = ObjectHeader->Type->DefaultObject;

        if ((LONG_PTR)ObjectToWaitOn >= 0)
        {
            ObjectToWaitOn = (PCHAR)Objects + (ULONG_PTR)ObjectToWaitOn; // DefaultObject is the offset of DISPATCHER_HEADER::Header
        }

        WaitObjects[i] = ObjectToWaitOn;

        /* Keep looping */
        i++;
    } while (i < ObjectCount);

    /* For a Waitall, we can't have the same object more then once */
    if (WaitType == WaitAll)
    {
        /* Clear the main loop variable */
        i = 0;

        /* Start the loop */
        do
        {
            /* Check the current and forward object */
            for (j = i + 1; j < ObjectCount; j++)
            {
                /* Make sure they don't match */
                if (WaitObjects[i] == WaitObjects[j])
                {
                    /* Fail */
                    Status = STATUS_INVALID_PARAMETER_MIX;
                    DBG_TRACE("Passed a duplicate object to NtWaitForMultipleObjects");
                    goto Exit;
                }
            }

            /* Keep looping */
            i++;
        } while (i < ObjectCount);
    }

    /* Now we can finally wait. Always use SEH since it can raise an exception */
    _SEH2_TRY
    {
        /* We're done playing with handles */
        LockInUse = FALSE;
        KeLeaveCriticalRegion();

        /* Do the kernel wait */
        Status = KeWaitForMultipleObjects(ObjectCount,
                                          WaitObjects,
                                          WaitType,
                                          UserRequest,
                                          WaitMode,
                                          Alertable,
                                          TimeOut,
                                          WaitBlockArray);
    }
        _SEH2_EXCEPT((_SEH2_GetExceptionCode() == STATUS_MUTANT_LIMIT_EXCEEDED) ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Exit:
    /* First derefence */
    while (ReferencedObjects)
    {
        /* Decrease the number of objects */
        ReferencedObjects--;

        /* Check if we had a valid object in this position */
        if (Objects[ReferencedObjects])
        {
            /* Dereference it */
            ObDereferenceObject(Objects[ReferencedObjects]);
        }
    }

    /* Free wait block array */
    if (WaitBlockArray) ExFreePoolWithTag(WaitBlockArray, TAG_WAIT);

    /* Re-enable APCs if needed */
    if (LockInUse) KeLeaveCriticalRegion();

    /* Return status */
    return Status;
}

/*++
* @name NtMakeTemporaryObject
* @implemented NT4
*
*     The NtMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS NTAPI NtMakeTemporaryObject
(
	IN HANDLE ObjectHandle
)
{
	PVOID ObjectBody;
	NTSTATUS Status;
	PAGED_CODE();

	/* Reference the object for DELETE access */
	Status = ObReferenceObjectByHandle(ObjectHandle,
		NULL,
		&ObjectBody);

	if (Status != STATUS_SUCCESS) return Status;

	/* Set it as temporary and dereference it */
	ObMakeTemporaryObject(ObjectBody);
	ObDereferenceObject(ObjectBody);
	return STATUS_SUCCESS;
}
