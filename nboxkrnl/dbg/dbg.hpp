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

EXPORTNUM(8) DLLEXPORT ULONG CDECL DbgPrint
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

#ifdef __cplusplus
}
#endif
