/*
* ergo720                Copyright (c) 2025
*/

#include "nt.hpp"


EXPORTNUM(196) NTSTATUS XBOXAPI NtDeviceIoControlFile
(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
)
{
	return IopControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer,
		OutputBufferLength, IRP_MJ_DEVICE_CONTROL);
}

EXPORTNUM(191) NTSTATUS NTAPI NtCreateIoCompletion
(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG NumberOfConcurrentThreads
)
{
    PKQUEUE Queue;
    HANDLE hIoCompletionHandle;
    NTSTATUS Status;
    PAGED_CODE();

    if (!IoCompletionHandle)
        return STATUS_INVALID_PARAMETER_1;

    /* Create the Object */
    Status = ObCreateObject(&IoCompletionObjectType,
        ObjectAttributes,
        &Queue);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Queue */
        KeInitializeQueue(Queue, NumberOfConcurrentThreads);

        /* Insert it */
        Status = ObInsertObject(Queue,
            NULL,
            0,
            &hIoCompletionHandle);
        if (NT_SUCCESS(Status))
        {
            *IoCompletionHandle = hIoCompletionHandle;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS NTAPI NtOpenIoCompletion
(
    OUT PHANDLE IoCompletionHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
    HANDLE hIoCompletionHandle;
    NTSTATUS Status;
    PAGED_CODE();

    if (!IoCompletionHandle)
        return STATUS_INVALID_PARAMETER_1;

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
        &IoCompletionObjectType,
        NULL,
        &hIoCompletionHandle);
    if (NT_SUCCESS(Status))
    {
        *IoCompletionHandle = hIoCompletionHandle;
    }

    /* Return Status */
    return Status;
}


EXPORTNUM(227) NTSTATUS NTAPI NtSetIoCompletion
(
    IN HANDLE IoCompletionPortHandle,
    IN PVOID CompletionKey,
    IN PVOID CompletionContext,
    IN NTSTATUS CompletionStatus,
    IN ULONG CompletionInformation
)
{
    NTSTATUS Status;
    PKQUEUE Queue;
    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionPortHandle, &IoCompletionObjectType, (PVOID*)&Queue);
    if (NT_SUCCESS(Status))
    {
        /* Set the Completion */
        Status = IoSetIoCompletion(Queue,
            CompletionKey,
            CompletionContext,
            CompletionStatus,
            CompletionInformation);

        /* Dereference the object */
        ObDereferenceObject(Queue);
    }

    /* Return status */
    return Status;
}
