#pragma once
#include "ke.hpp"
#include "hal_emuio.hpp"

// Type layout of IoRequest
// IoRequestType - DevType - IoFlags - Disposition
// 31 - 28         27 - 23   22 - 3	   2 - 0

#pragma pack(push, 1)
struct IoRequestHeader
{
	ULONG Id; // unique id to identify this request
	ULONG Type; // type of request and flags
};

struct IoRequestXX
{
	ULONG Handle; // file handle (it's the address of the kernel file info object that tracks the file)
};

// Specialized version of IoRequest for read/write requests only
struct IoRequestRw
{
	LONGLONG Offset; // file offset from which to start the I/O
	ULONG Size; // bytes to transfer
	ULONG Address; // virtual address of the data to transfer
	ULONG Handle; // file handle (it's the address of the kernel file info object that tracks the file)
	ULONG Timestamp; // file timestamp
};

// Specialized version of IoRequest for open/create requests only
struct IoRequestOc
{
	LONGLONG InitialSize; // file initial size
	ULONG Size; // size of file path
	ULONG Handle; // file handle (it's the address of the kernel file info object that tracks the file)
	ULONG Path; // file path address
	ULONG Attributes; // file attributes (only uses a single byte really)
	ULONG Timestamp; // file timestamp
	ULONG DesiredAccess; // the kind of access requested for the file
	ULONG CreateOptions; // how the create the file
};

struct IoRequest
{
	IoRequestHeader Header;
	union
	{
		IoRequestOc m_oc;
		IoRequestRw m_rw;
		IoRequestXX m_xx;
	};
};
#pragma pack(pop)
