#pragma once
#include "..\types.hpp"

extern "C" [[noreturn]] EXPORTNUM(352) void XBOXAPI RtlRip
(
	const CHAR * ApiName,
	const CHAR * Expression,
	const CHAR * Message
);

[[noreturn]] void CDECL RipWithMsg(const char* Func, const char* Msg, ...);

#define RIP_UNIMPLEMENTED() RtlRip(__func__, nullptr, const_cast<PCHAR>("unimplemented!"))
#define RIP_API_MSG(Msg) RtlRip(const_cast<PCHAR>(__func__), nullptr, const_cast<PCHAR>(Msg))
#define RIP_API_FMT(Msg, ...) RipWithMsg(__func__, Msg __VA_OPT__(,) __VA_ARGS__)

#if DBG
#define NT_ASSERT_MESSAGE(expr, Msg, ...) if (!(expr)) {RipWithMsg(__func__, Msg __VA_OPT__(,) __VA_ARGS__);}
#define NT_ASSERT(expr) NT_ASSERT_MESSAGE((expr), "Assertion \"%s\" failed in function %s line %d", #expr, __func__)
#else
#define NT_ASSERT_MESSAGE(expr, Msg, ...) ((void)0)
#define NT_ASSERT(expr) ((void)0)
#endif

#define ASSERT_IRQL_LESS_OR_EQUAL(x) NT_ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) NT_ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) NT_ASSERT(KeGetCurrentIrql()<(x))