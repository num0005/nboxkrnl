#pragma once
#include "halp.hpp"


// We use i/o instructions to communicate with the host that's running us
// The list of i/o ports used on the xbox is at https://xboxdevwiki.net/Memory, so avoid using one of those

#define NXBX_BASE_PORT 0x0200
#define NXBX_PORT(port) (NXBX_BASE_PORT + (port))
// Output debug strings to the host
#define DBG_OUTPUT_STR_PORT NXBX_PORT(0x00)
// Get the system type that we should use
#define KE_SYSTEM_TYPE NXBX_PORT(0x01)
// Request an abort interrupt to terminate execution
#define KE_ABORT NXBX_PORT(0x02)
// Request the clock increment in 100 ns units since the last clock
#define KE_CLOCK_INCREMENT_LOW NXBX_PORT(0x03)
#define KE_CLOCK_INCREMENT_HIGH NXBX_PORT(0x04)
// Request the total execution time since booting in ms
#define KE_TIME_MS NXBX_PORT(0x05)
// Submit a new I/O request
#define IO_START NXBX_PORT(0x06)
// Retry submitting a I/O request
#define IO_RETRY NXBX_PORT(0x07)
// Request the I/O request with the specified id
#define IO_QUERY NXBX_PORT(0x08)
// Check if a I/O request was submitted successfully
#define IO_CHECK_ENQUEUE NXBX_PORT(0x0A)
// Request the path's length of the XBE to launch when no reboot occured
#define XE_XBE_PATH_LENGTH NXBX_PORT(0x0D)
// Send the address where to put the path of the XBE to launch when no reboot occured
#define XE_XBE_PATH_ADDR NXBX_PORT(0x0E)
// Request the total ACPI time since booting
#define KE_ACPI_TIME_LOW NXBX_PORT(0x0F)
#define KE_ACPI_TIME_HIGH NXBX_PORT(0x10)


static inline VOID SubmitIoRequestToHost(void* RequestAddr)
{
	outl(IO_START, (ULONG_PTR)RequestAddr);
	while (inl(IO_CHECK_ENQUEUE))
	{
		outl(IO_RETRY, 0);
	}
}

static inline VOID RetrieveIoRequestFromHost(volatile IoInfoBlockOc* Info, ULONG Id)
{
	// TODO: instead of polling the IO like this, the host should signal I/O completion by raising a HDD interrupt, so that we can handle the event in the ISR

	Info->Header.Id = Id;
	Info->Header.Ready = 0;

	do
	{
		outl(IO_QUERY, (ULONG_PTR)Info);
	} while (!Info->Header.Ready);
}
