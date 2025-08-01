/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "io.hpp"


#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(184) NTSTATUS XBOXAPI NtAllocateVirtualMemory
(
	PVOID *BaseAddress,
	ULONG ZeroBits,
	PULONG AllocationSize,
	DWORD AllocationType,
	DWORD Protect
);

EXPORTNUM(187) NTSTATUS XBOXAPI NtClose
(
	HANDLE Handle
);

EXPORTNUM(188) NTSTATUS XBOXAPI NtCreateDirectoryObject
(
	PHANDLE DirectoryHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
);

EXPORTNUM(190) NTSTATUS XBOXAPI NtCreateFile
(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions
);

EXPORTNUM(191) NTSTATUS NTAPI NtCreateIoCompletion
(
	OUT PHANDLE IoCompletionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG NumberOfConcurrentThreads
);

NTSTATUS NTAPI NtOpenIoCompletion
(
	OUT PHANDLE IoCompletionHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

EXPORTNUM(192) NTSTATUS XBOXAPI NtCreateMutant
(
	PHANDLE MutantHandle,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN InitialOwner
);

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
);

#define DUPLICATE_CLOSE_SOURCE        0x00000001

EXPORTNUM(197) NTSTATUS NTAPI NtDuplicateObject
(
	IN HANDLE SourceHandle,
	OUT PHANDLE TargetHandle OPTIONAL,
	IN ULONG Options
);

EXPORTNUM(199) NTSTATUS XBOXAPI NtFreeVirtualMemory
(
	PVOID *BaseAddress,
	PULONG FreeSize,
	ULONG FreeType
);

EXPORTNUM(201) NTSTATUS NTAPI NtOpenDirectoryObject
(
	OUT PHANDLE DirectoryHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

EXPORTNUM(202) NTSTATUS XBOXAPI NtOpenFile
(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions
);

EXPORTNUM(203) NTSTATUS XBOXAPI NtOpenSymbolicLinkObject
(
	PHANDLE LinkHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
);

EXPORTNUM(204) NTSTATUS XBOXAPI NtProtectVirtualMemory
(
	PVOID *BaseAddress,
	PSIZE_T RegionSize,
	ULONG NewProtect,
	PULONG OldProtect
);

/*++
 * @name NtQueueApcThreadEx
 * NT4
 *
 *    This routine is used to queue an APC from user-mode for the specified
 *    thread.
 *
 * @param ThreadHandle
 *        Handle to the Thread.
 *        This handle must have THREAD_SET_CONTEXT privileges.
 *
 * @param ApcRoutine
 *        Pointer to the APC Routine to call when the APC executes.
 *
 * @param NormalContext
 *        Pointer to the context to send to the Normal Routine.
 *
 * @param SystemArgument[1-2]
 *        Pointer to a set of two parameters that contain untyped data.
 *
 * @return STATUS_SUCCESS or failure cute from associated calls.
 *
 * @remarks The thread must enter an alertable wait before the APC will be
 *          delivered.
 *
 *--*/
EXPORTNUM(206) NTSTATUS NTAPI NtQueueApcThread
(
	IN HANDLE ThreadHandle,
	IN PKNORMAL_ROUTINE ApcRoutine,
	IN PVOID NormalContext,
	IN OPTIONAL PVOID SystemArgument1,
	IN OPTIONAL PVOID SystemArgument2
);

EXPORTNUM(211) NTSTATUS XBOXAPI NtQueryInformationFile
(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass
);


//
// Information Structures for NtQueryMutant
//
typedef struct _MUTANT_BASIC_INFORMATION
{
	LONG CurrentCount;
	BOOLEAN OwnedByCaller;
	BOOLEAN AbandonedState;
} MUTANT_BASIC_INFORMATION, * PMUTANT_BASIC_INFORMATION;

/*
 * @implemented
 */
EXPORTNUM(213) NTSTATUS NTAPI NtQueryMutant
(
	IN HANDLE MutantHandle,
	OUT PMUTANT_BASIC_INFORMATION MutantInformation
);

EXPORTNUM(218) NTSTATUS XBOXAPI NtQueryVolumeInformationFile
(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PFILE_FS_SIZE_INFORMATION FileInformation,
	ULONG Length,
	FS_INFORMATION_CLASS FileInformationClass
);

EXPORTNUM(219) NTSTATUS XBOXAPI NtReadFile
(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset
);

EXPORTNUM(221) NTSTATUS XBOXAPI NtReleaseMutant
(
	HANDLE MutantHandle,
	PLONG PreviousCount
);

EXPORTNUM(224) NTSTATUS NTAPI NtResumeThread
(
	IN  HANDLE  ThreadHandle,
	OUT PULONG  PreviousSuspendCount OPTIONAL
);

EXPORTNUM(227) NTSTATUS NTAPI NtSetIoCompletion
(
	IN HANDLE IoCompletionPortHandle,
	IN PVOID CompletionKey,
	IN PVOID CompletionContext,
	IN NTSTATUS CompletionStatus,
	IN ULONG CompletionInformation
);

/*
 * FUNCTION: Sets the system time.
 * PARAMETERS:
 *        NewTime - Points to a variable that specified the new time
 *        of day in the standard time format.
 *        OldTime - Optionally points to a variable that receives the
 *        old time of day in the standard time format.
 * RETURNS: Status
 */
EXPORTNUM(228) NTSTATUS NTAPI NtSetSystemTime
(
	IN PLARGE_INTEGER SystemTime,
	OUT PLARGE_INTEGER PreviousTime OPTIONAL
);

EXPORTNUM(231) NTSTATUS NTAPI NtSuspendThread
(
	IN  HANDLE  ThreadHandle,
	OUT PULONG  PreviousSuspendCount OPTIONAL
);

EXPORTNUM(232) VOID XBOXAPI NtUserIoApcDispatcher
(
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG Reserved
);

EXPORTNUM(233) NTSTATUS XBOXAPI NtWaitForSingleObject
(
	HANDLE Handle,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
);

EXPORTNUM(234) NTSTATUS XBOXAPI NtWaitForSingleObjectEx
(
	HANDLE Handle,
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
);

EXPORTNUM(235) NTSTATUS NTAPI NtWaitForMultipleObjectsEx
(
	IN ULONG ObjectCount,
	IN PHANDLE Handles,
	IN WAIT_TYPE WaitType,
	IN KPROCESSOR_MODE WaitMode,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER TimeOut OPTIONAL
);

EXPORTNUM(236) NTSTATUS XBOXAPI NtWriteFile
(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset
);

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
);

#ifdef __cplusplus
}
#endif
