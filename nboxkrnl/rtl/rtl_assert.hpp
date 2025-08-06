#pragma once
#include "..\types.hpp"
#include "rtl_context.hpp"

extern "C" [[noreturn]] EXPORTNUM(352) void XBOXAPI RtlRip
(
	const CHAR * ApiName,
	const CHAR * Expression,
	const CHAR * Message
);

[[noreturn]] VOID XBOXAPI RtlRipWithContext
(
	PCONTEXT Context,
	const CHAR* ApiName,
	const CHAR* Expression,
	const CHAR* Message
);

[[noreturn]] void CDECL RtlRipPrintf(PCONTEXT Context, const char* Func, const char* Msg, ...);

#define RIP_API_MSG(Msg) { CONTEXT Context; RtlCaptureContext(&Context); RtlRipWithContext(&Context, const_cast<PCHAR>(__func__), nullptr, const_cast<PCHAR>(Msg)); }
#define RIP_UNIMPLEMENTED() RIP_API_MSG("unimplemented!")
#define RIP_API_FMT(Msg, ...) {CONTEXT Context; RtlCaptureContext(&Context); RtlRipPrintf(&Context, __func__, Msg __VA_OPT__(,) __VA_ARGS__); }

#if DBG
#define NT_ASSERT_MESSAGE(expr, Msg, ...) if (!(expr)) {CONTEXT Context; RtlCaptureContext(&Context); RtlRipPrintf(&Context, __func__, Msg __VA_OPT__(,) __VA_ARGS__);}
#define NT_ASSERT(expr) NT_ASSERT_MESSAGE((expr), "Assertion \"%s\" failed in function %s line %d", #expr, __func__, __LINE__)
#else
#define NT_ASSERT_MESSAGE(expr, Msg, ...) ((void)0)
#define NT_ASSERT(expr) ((void)0)
#endif

#define ASSERT_IRQL_LESS_OR_EQUAL(x) NT_ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) NT_ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) NT_ASSERT(KeGetCurrentIrql()<(x))