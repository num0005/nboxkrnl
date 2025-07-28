/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "ke.hpp"

#define PSP_MAX_CREATE_THREAD_NOTIFY 8

VOID XBOXAPI PspSystemThreadStartup(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID XBOXAPI PspTerminationRoutine(PKDPC Dpc, PVOID DpcContext, PVOID DpcArg0, PVOID DpcArg1);

inline KDPC PspTerminationDpc;
inline LIST_ENTRY PspTerminationListHead = { &PspTerminationListHead, &PspTerminationListHead };
