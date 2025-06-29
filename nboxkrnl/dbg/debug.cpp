/*
 * ergo720                Copyright (c) 2022
 */

#include "..\kernel.hpp"
#include "dbg.hpp"
#include <nanoprintf.h>
#include <string.h>
#include <stdarg.h>
#include <hal.hpp>


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
		npf_vsnprintf(buff, sizeof(buff), Format, vlist);
		va_end(vlist);

		HalpWriteToDebugOutput(buff, sizeof(buff));
	}

	return STATUS_SUCCESS;
}
