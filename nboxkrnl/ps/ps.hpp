/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "ke.hpp"
#include "ob.hpp"
#include "ex.hpp"
#include "nt.hpp"


#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(254) NTSTATUS XBOXAPI PsCreateSystemThread
(
	PHANDLE ThreadHandle,
	PHANDLE ThreadId,
	PKSTART_ROUTINE StartRoutine,
	PVOID StartContext,
	BOOLEAN DebuggerThread
);

EXPORTNUM(255) NTSTATUS XBOXAPI PsCreateSystemThreadEx
(
	PHANDLE ThreadHandle,
	SIZE_T ThreadExtensionSize,
	SIZE_T KernelStackSize,
	SIZE_T TlsDataSize,
	PHANDLE ThreadId,
	PKSTART_ROUTINE StartRoutine,
	PVOID StartContext,
	BOOLEAN CreateSuspended,
	BOOLEAN DebuggerThread,
	PKSYSTEM_ROUTINE SystemRoutine
);

typedef struct _PS_STATISTICS
{
	ULONG Length;
	ULONG ThreadCount;
	ULONG HandleCount;
}
PS_STATISTICS, * PPS_STATISTICS;

// ******************************************************************
// * 0x0100 - PsQueryStatistics() - source CXBX-reloaded
// ******************************************************************
EXPORTNUM(256) NTSTATUS NTAPI PsQueryStatistics
(
	IN OUT PPS_STATISTICS ProcessStatistics
);

EXPORTNUM(258) VOID XBOXAPI PsTerminateSystemThread
(
	NTSTATUS ExitStatus
);

EXPORTNUM(259) extern OBJECT_TYPE PsThreadObjectType;


#ifdef __cplusplus
}
#endif

BOOLEAN PsInitSystem();

_IRQL_requires_max_(DISPATCH_LEVEL)
inline PETHREAD NTAPI PsGetCurrentThread(VOID)
{
	return (PETHREAD)KeGetCurrentThread();
}
