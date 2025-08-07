/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "..\types.hpp"

#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(5) void NTAPI DbgBreakPointWithStatus
(
	IN ULONG Status
);

EXPORTNUM(6) void NTAPI DbgBreakPoint(void);

EXPORTNUM(7) void NTAPI DbgLoadImageSymbols
(
	PSTRING FileName,
	PVOID ImageBase,
	ULONG_PTR ProcessId
);

EXPORTNUM(8) ULONG CDECL DbgPrint
(
	const CHAR *Format,
	...
);

EXPORTNUM(10) ULONG NTAPI DbgPrompt
(
	PCHAR Prompt,
	PCHAR Response,
	ULONG MaximumResponseLength
);

EXPORTNUM(11) VOID NTAPI DbgUnLoadImageSymbols
(
	PSTRING FileName,
	PVOID ImageBase,
	ULONG_PTR ProcessId
);

#if DBG
#define DBG_TRACE(Format, ...) DbgPrint(__FUNCTION__ "() " Format __VA_OPT__(,) __VA_ARGS__)
#else
#define DBG_TRACE(Format, ...) (void)(Format)
#endif
#define DBG_TRACE_NOT_IMPLEMENTED() DBG_TRACE("Function not implemented! Caller=%p", _ReturnAddress())

#ifdef __cplusplus
}
#endif
