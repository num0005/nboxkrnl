/*
 * ergo720                Copyright (c) 2022
 */

#include "ke.hpp"
#include "halp.hpp"
#include "dbg.hpp"
#include <intrin.h>

EXPORTNUM(162) ULONG_PTR KiBugCheckData[5];


EXPORTNUM(95) VOID XBOXAPI KeBugCheck
(
	ULONG BugCheckCode
)
{
	KeBugCheckEx(BugCheckCode, 0, 0, 0, 0);
}

EXPORTNUM(96) VOID XBOXAPI KeBugCheckEx
(
	ULONG BugCheckCode,
	ULONG_PTR BugCheckParameter1,
	ULONG_PTR BugCheckParameter2,
	ULONG_PTR BugCheckParameter3,
	ULONG_PTR BugCheckParameter4
)
{
	KiBugCheckData[0] = BugCheckCode;
	KiBugCheckData[1] = BugCheckParameter1;
	KiBugCheckData[2] = BugCheckParameter2;
	KiBugCheckData[3] = BugCheckParameter3;
	KiBugCheckData[4] = BugCheckParameter4;

	RIP_API_FMT("Fatal error of the kernel with code: 0x%08lx (0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)",
		BugCheckCode,
		BugCheckParameter1,
		BugCheckParameter2,
		BugCheckParameter3,
		BugCheckParameter4);

	HalpShutdownSystem();
}

VOID __declspec(noinline) CDECL KeBugCheckLogEip(ULONG BugCheckCode)
{
	// This function must never inlined because it needs to capture the return address placed on the stack by the caller

	void* caller = _ReturnAddress();
	KeBugCheckEx(BugCheckCode, reinterpret_cast<ULONG_PTR>(caller), 0, 0, 0);
}

[[noreturn]] VOID CDECL KeBugCheckNoReturnGuard
(
	ULONG_PTR BugCheckParameter1,
	ULONG_PTR BugCheckParameter2,
	ULONG_PTR BugCheckParameter3,
	ULONG_PTR BugCheckParameter4
)
{
	RIP_API_FMT("Noreturn stack guard catch illegal return (0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)",
		BugCheckParameter1,
		BugCheckParameter2,
		BugCheckParameter3,
		BugCheckParameter4);
	KeBugCheckEx(NORETURN_FUNCTION_RETURNED, BugCheckParameter1, BugCheckParameter2, BugCheckParameter3, BugCheckParameter4);
}
