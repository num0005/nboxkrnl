#include  "hal.hpp"
#include  "halp_nxbx.hpp"
#include "ex.hpp"

void HalpWriteToDebugOutput(char* buffer, size_t max_length)
{
	// send string address to NXBX host
	outl(DBG_OUTPUT_STR_PORT, reinterpret_cast<ULONG>(buffer));
}

[[noreturn]] VOID HalpShutdownSystem()
{
	outl(KE_ABORT, 0);

	while (true)
	{
		_disable();
		__halt();
	}
}

ConsoleType HalQueryConsoleType()
{
	// read NXBX console mode configuration register
	return static_cast<ConsoleType>(inl(KE_SYSTEM_TYPE));
}

ULONGLONG HalQueryPerformanceCounter()
{
	/* Save EFlags and disable interrupts */
	ULONG EFlags = __readeflags();
	_disable();

	// get timestamp from NXBX host
	ULONG time_low  = inl(KE_ACPI_TIME_LOW);
	ULONG time_high = inl(KE_ACPI_TIME_HIGH);

	__writeeflags(EFlags);

	ULONGLONG time = ((ULONGLONG)time_high) << 32 | time_low;

	return time;
}

NTSTATUS HalEmuQueryXBEPath(PSTRING XBEPath)
{
	if (!XBEPath)
	{
		return STATUS_INVALID_PARAMETER;
	}

	// read size from host
	const ULONG PathSize = inl(XE_XBE_PATH_LENGTH);

	// max path length for UDF
	if (PathSize > 1023)
	{
		return STATUS_IO_DEVICE_ERROR;
	}

	const PCHAR PathBuffer = (PCHAR)ExAllocatePoolWithTag(PathSize, 'PebX');
	if (!PathBuffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	// ask host to write path to memory address
	outl(XE_XBE_PATH_ADDR, reinterpret_cast<ULONG>(PathBuffer));

	XBEPath->Buffer = PathBuffer;
	XBEPath->Length = PathSize;
	XBEPath->MaximumLength = PathSize;

	return STATUS_SUCCESS;
}
