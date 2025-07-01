#include "rtl.hpp"
#include "..\hal\halp.hpp"
#include "..\dbg\dbg.hpp"
#include "..\ex\ex.hpp"
#include "..\mm\mm.hpp"
#include <string.h>
#include <assert.h>
#define NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW 1
#include <nanoprintf.h>

EXPORTNUM(361) int CDECL RtlSnprintf
(
	IN PCHAR       string,
	IN SIZE_T      count,
	IN const char *format,
	...
)
{


	// UNTESTED. Possible test-case : debugchannel.xbe

	va_list ap;
	va_start(ap, format);
	INT Result = npf_snprintf(string, count, format, ap);
	va_end(ap);

	return Result;
}


EXPORTNUM(362) int CDECL RtlSprintf
(
	IN PCHAR string,
	IN const char* format,
	...
)
{
	// UNTESTED. Possible test-case : debugchannel.xbe

	va_list ap;
	va_start(ap, format);
	INT Result = npf_vsnprintf(string, 0x1000, format, ap);
	va_end(ap);

	return Result;
}


EXPORTNUM(363) int CDECL RtlVsnprintf
(
	IN PCHAR string,
	IN SIZE_T count,
	IN const char* format,
	va_list ap
)
{
	// UNTESTED. Possible test-case : debugchannel.xbe

	INT Result = npf_vsnprintf(string, count, format, ap);

	return Result;
}


EXPORTNUM(364) int CDECL RtlVsprintf
(
	IN PCHAR string,
	IN const char* format,
	va_list ap
)
{
	// UNTESTED. Possible test-case : debugchannel.xbe

	INT Result = npf_vsnprintf(string, 0x1000, format, ap);

	return Result;
}