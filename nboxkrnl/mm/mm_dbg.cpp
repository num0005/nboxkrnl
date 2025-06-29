#include "..\kernel.hpp"
#include "ke.hpp"
#include "hal.hpp"
#include "rtl.hpp"
#include "fsc.hpp"
#include "mm.hpp"
#include "mi.hpp"
#include "vad_tree.hpp"
#include <assert.h>

EXPORTNUM(374) PVOID NTAPI MmDbgAllocateMemory
(
	IN ULONG NumberOfBytes,
	IN ULONG Protect
)
{
	return MiAllocateSystemMemory(NumberOfBytes, Protect, Debugger, FALSE, TRUE);
}

EXPORTNUM(375) ULONG NTAPI MmDbgFreeMemory
(
	IN PVOID BaseAddress,
	IN ULONG NumberOfBytes
)
{
	return MiFreeSystemMemory(BaseAddress, NumberOfBytes);
}

EXPORTNUM(376) ULONG NTAPI MmDbgQueryAvailablePages()
{
	return MiDevkitRegion.PagesAvailable;
}

EXPORTNUM(377) void NTAPI MmDbgReleaseAddress
(
	IN PVOID VirtualAddress,
	IN PULONG Opaque
)
{
	RIP_UNIMPLEMENTED();
}

EXPORTNUM(378) PVOID NTAPI MmDbgWriteCheck
(
	IN PVOID VirtualAddress,
	IN PULONG Opaque
)
{
	RIP_UNIMPLEMENTED();
}
