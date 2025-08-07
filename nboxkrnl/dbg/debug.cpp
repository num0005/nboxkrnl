/*
 * ergo720                Copyright (c) 2022
 */

#include "..\kernel.hpp"
#include "dbg.hpp"
#include <string.h>
#include <stdarg.h>
#include <hal.hpp>
#include "rtl.hpp"

EXPORTNUM(5) void NTAPI DbgBreakPointWithStatus
(
	IN ULONG Status
)
{
	DBG_TRACE_NOT_IMPLEMENTED();
}

EXPORTNUM(6) void NTAPI DbgBreakPoint(void)
{
	DbgBreakPointWithStatus(STATUS_BREAKPOINT);
}

EXPORTNUM(7) void NTAPI DbgLoadImageSymbols
(
	PSTRING FileName,
	PVOID ImageBase,
	ULONG_PTR ProcessId
)
{
	DBG_TRACE_NOT_IMPLEMENTED();
}

EXPORTNUM(8) ULONG CDECL DbgPrint
(
	const CHAR *Format,
	...
)
{
	if (Format != nullptr) {
		// We print at most 512 characters long strings
		char buff[512];
		va_list vlist;
		va_start(vlist, Format);
		RtlVsnprintf(buff, sizeof(buff), Format, vlist);
		va_end(vlist);

		HalpWriteToDebugOutput(buff, sizeof(buff));
	}

	return STATUS_SUCCESS;
}

EXPORTNUM(10) ULONG NTAPI DbgPrompt
(
	PCHAR Prompt,
	PCHAR Response,
	ULONG MaximumResponseLength
)
{
	if (Prompt)
	{
		HalpWriteToDebugOutput(Prompt);
	}

	if (Response && MaximumResponseLength)
	{
		*Response = '\0';
	}

	return 0;
}

EXPORTNUM(11) VOID NTAPI DbgUnLoadImageSymbols
(
	PSTRING FileName,
	PVOID ImageBase,
	ULONG_PTR ProcessId
)
{
	DBG_TRACE_NOT_IMPLEMENTED();
}
