/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Semaphore Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include "ke.hpp"
#include "ex.hpp"

/* GLOBALS ******************************************************************/

OBJECT_TYPE ExSemaphoreObjectType ={
    ExAllocatePoolWithTag,
    ExFreePool,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    'ameS'
};

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
                  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                  IN LONG InitialCount,
                  IN LONG MaximumCount)
{
    PKSEMAPHORE Semaphore;
    HANDLE hSemaphore;
    NTSTATUS Status;
    PAGED_CODE();

    /* Make sure the counts make sense */
    if ((MaximumCount <= 0) ||
        (InitialCount < 0) ||
        (InitialCount > MaximumCount))
    {
        DPRINT("Invalid Count Data!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the Semaphore Object */
    Status = ObCreateObject(&ExSemaphoreObjectType,
                            ObjectAttributes,
                            sizeof(KSEMAPHORE),
                            (PVOID*)&Semaphore);

    /* Check for Success */
    if (NT_SUCCESS(Status))
    {
        /* Initialize it */
        KeInitializeSemaphore(Semaphore,
                              InitialCount,
                              MaximumCount);

        /* Insert it into the Object Tree */
        Status = ObInsertObject((PVOID)Semaphore,
                                ObjectAttributes,
                                0,
                                &hSemaphore);

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            *SemaphoreHandle = hSemaphore;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hSemaphore;
    NTSTATUS Status;
    PAGED_CODE();

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                &ExSemaphoreObjectType,
                                NULL,
                                &hSemaphore);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        *SemaphoreHandle = hSemaphore;
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
                 OUT PSEMAPHORE_BASIC_INFORMATION BasicInfo)
{
    PKSEMAPHORE Semaphore;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       &ExSemaphoreObjectType,
                                       (PVOID*)&Semaphore);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Return the basic information */
        BasicInfo->CurrentCount = KeReadStateSemaphore(Semaphore);
        BasicInfo->MaximumCount = Semaphore->Limit;

        /* Dereference the Object */
        ObDereferenceObject(Semaphore);
   }

   /* Return status */
   return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
                   IN LONG ReleaseCount,
                   OUT PLONG PreviousCount OPTIONAL)
{
    PKSEMAPHORE Semaphore;
    NTSTATUS Status;
    PAGED_CODE();


    /* Make sure count makes sense */
    if (ReleaseCount <= 0)
    {
        DPRINT("Invalid Release Count\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       &ExSemaphoreObjectType,
                                       (PVOID*)&Semaphore);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Release the semaphore */
         LONG PrevCount = KeReleaseSemaphore(Semaphore,
                                                0,
                                                ReleaseCount,
                                                FALSE);

            /* Return the old count if requested */
            if (PreviousCount) *PreviousCount = PrevCount;

        /* Dereference the Semaphore */
        ObDereferenceObject(Semaphore);
    }

    /* Return Status */
    return Status;
}

/* EOF */
