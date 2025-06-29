/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "..\kernel.hpp"
#include "ke.hpp"


using PHAL_SHUTDOWN_NOTIFICATION = VOID(XBOXAPI *)(
	struct HAL_SHUTDOWN_REGISTRATION *ShutdownRegistration
	);

struct HAL_SHUTDOWN_REGISTRATION {
	PHAL_SHUTDOWN_NOTIFICATION NotificationRoutine;
	LONG Priority;
	LIST_ENTRY ListEntry;
};
using PHAL_SHUTDOWN_REGISTRATION = HAL_SHUTDOWN_REGISTRATION *;

void HalpWriteToDebugOutput(char* buffer, size_t max_length = 0);

#include <intrin.h>

static inline VOID CDECL outl(USHORT Port, ULONG Value)
{
	__outdword(Port, Value);
}

static inline VOID CDECL outw(USHORT Port, USHORT Value)
{
	__outword(Port, Value);
}

static inline VOID CDECL outb(USHORT Port, BYTE Value)
{
	__outbyte(Port, Value);
}

static inline ULONG CDECL inl(USHORT Port)
{
	return __indword(Port);
}

static inline USHORT CDECL inw(USHORT Port)
{
	return __inword(Port);
}

static inline BYTE CDECL inb(USHORT Port)
{
	return __inbyte(Port);
}

static inline VOID CDECL enable()
{
	_enable();
}

static inline VOID CDECL disable()
{
	_disable();
}

// detect console type, on emulator builds this may be a call to host
ConsoleType HalQueryConsoleType();

// read the performance counter value
ULONGLONG HalQueryPerformanceCounter();

// query the XBE launch path when running under a supported emulator
NTSTATUS HalEmuQueryXBEPath(PSTRING XBEPath);

#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(40) DLLEXPORT extern ULONG HalDiskCachePartitionCount;

EXPORTNUM(43) DLLEXPORT VOID XBOXAPI HalEnableSystemInterrupt
(
	ULONG BusInterruptLevel,
	KINTERRUPT_MODE InterruptMode
);

EXPORTNUM(44) DLLEXPORT ULONG XBOXAPI HalGetInterruptVector
(
	ULONG BusInterruptLevel,
	PKIRQL Irql
);

EXPORTNUM(45) DLLEXPORT NTSTATUS XBOXAPI HalReadSMBusValue
(
	UCHAR SlaveAddress,
	UCHAR CommandCode,
	BOOLEAN ReadWordValue,
	ULONG *DataValue
);

EXPORTNUM(46) DLLEXPORT VOID XBOXAPI HalReadWritePCISpace
(
	ULONG BusNumber,
	ULONG SlotNumber,
	ULONG RegisterNumber,
	PVOID Buffer,
	ULONG Length,
	BOOLEAN WritePCISpace
);

EXPORTNUM(47) DLLEXPORT VOID XBOXAPI HalRegisterShutdownNotification
(
	PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
	BOOLEAN Register
);

EXPORTNUM(48) DLLEXPORT VOID FASTCALL HalRequestSoftwareInterrupt
(
	KIRQL Request
);

EXPORTNUM(50) DLLEXPORT NTSTATUS XBOXAPI HalWriteSMBusValue
(
	UCHAR SlaveAddress,
	UCHAR CommandCode,
	BOOLEAN WriteWordValue,
	ULONG DataValue
);

EXPORTNUM(356) DLLEXPORT extern ULONG HalBootSMCVideoMode;

extern "C"
{
	// helper functions that map to intrinics that are exported by the xbox kernel
	// marked as inline in case we want to use them anywhere interally to the kernel
	inline EXPORTNUM(329) DLLEXPORT void XBOXAPI READ_PORT_BUFFER_UCHAR(IN DWORD Port, IN PUCHAR Buffer, IN ULONG Count)
	{
		__inbytestring(Port, Buffer, Count);
	}

	inline EXPORTNUM(331) DLLEXPORT void XBOXAPI READ_PORT_BUFFER_USHORT(IN DWORD Port, IN PUSHORT Buffer, IN ULONG Count)
	{
		__inwordstring(Port, Buffer, Count);
	}

	inline EXPORTNUM(331) DLLEXPORT void XBOXAPI READ_PORT_BUFFER_ULONG(IN DWORD Port, IN PULONG Buffer, IN ULONG Count)
	{
		__indwordstring(Port, Buffer, Count);
	}

	inline EXPORTNUM(332) DLLEXPORT void XBOXAPI WRITE_PORT_BUFFER_UCHAR(IN DWORD Port, OUT PUCHAR Buffer, IN ULONG Count)
	{
		__outbytestring(Port, Buffer, Count);
	}

	inline EXPORTNUM(331) DLLEXPORT void XBOXAPI WRITE_PORT_BUFFER_USHORT(IN DWORD Port, OUT PUSHORT Buffer, IN ULONG Count)
	{
		__outwordstring(Port, Buffer, Count);
	}

	inline EXPORTNUM(331) DLLEXPORT void XBOXAPI WRITE_PORT_BUFFER_ULONG(IN DWORD Port, OUT PULONG Buffer, IN ULONG Count)
	{
		__outdwordstring(Port, Buffer, Count);
	}
}

#ifdef __cplusplus
}
#endif

VOID HalInitSystem();
