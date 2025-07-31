/*
 * ergo720                Copyright (c) 2023
 */

#include "nt.hpp"
#include "obp.hpp"
#include "rtl.hpp"
#include <string.h>


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
