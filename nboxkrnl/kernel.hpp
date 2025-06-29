/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "types.hpp"

#define KERNEL_STACK_SIZE 12288
#define KERNEL_BASE 0x80010000

// Host device number
#define DEV_CDROM      0
#define DEV_UNUSED     1
#define DEV_PARTITION0 2
#define DEV_PARTITION1 3
#define DEV_PARTITION2 4
#define DEV_PARTITION3 5
#define DEV_PARTITION4 6
#define DEV_PARTITION5 7
#define DEV_PARTITION6 8 // non-standard
#define DEV_PARTITION7 9 // non-standard
#define NUM_OF_DEVS    10
#define DEV_TYPE(n)    ((n) << 23)

// Special host handles NOTE: these numbers must be less than 64 KiB - 1. This is because normal files use the address of a kernel file info struct as the host handle,
// and addresses below 64 KiB are forbidden on the xbox. This avoids collisions with these handles
#define CDROM_HANDLE      DEV_CDROM
#define UNUSED_HANDLE     DEV_UNUSED
#define PARTITION0_HANDLE DEV_PARTITION0
#define PARTITION1_HANDLE DEV_PARTITION1
#define PARTITION2_HANDLE DEV_PARTITION2
#define PARTITION3_HANDLE DEV_PARTITION3
#define PARTITION4_HANDLE DEV_PARTITION4
#define PARTITION5_HANDLE DEV_PARTITION5
#define PARTITION6_HANDLE DEV_PARTITION6
#define PARTITION7_HANDLE DEV_PARTITION7
#define FIRST_FREE_HANDLE NUM_OF_DEVS

#define LOWER_32(A) ((A) & 0xFFFFFFFF)
#define UPPER_32(A) ((A) >> 32)

enum ConsoleType {
	CONSOLE_XBOX,
	CONSOLE_CHIHIRO,
	CONSOLE_DEVKIT
};

enum IoRequestType : ULONG {
	Open       = 1 << 28,
	Remove     = 2 << 28,
	Close      = 3 << 28,
	Read       = 4 << 28,
	Write      = 5 << 28,
};

enum IoFlags : ULONG {
	MustBeADirectory    = 1 << 4,
	MustNotBeADirectory = 1 << 5
};

enum IoStatus : NTSTATUS {
	Success = 0,
	Pending,
	Error,
	Failed,
	IsADirectory,
	NotADirectory,
	NameNotFound,
	PathNotFound,
	Corrupt,
	CannotDelete,
	NotEmpty,
	Full
};

// Same as FILE_ macros of IO
enum IoInfo : ULONG {
	Superseded = 0,
	Opened,
	Created,
	Overwritten,
	Exists,
	NotExists
};

#pragma pack(push, 1)
struct IoInfoBlock {
	ULONG Id; // id of the io request to query
	union
	{
		IoStatus HostStatus; // hal-internal status
		NTSTATUS Status; // the final status of the request
	};
	IoInfo Info; // request-specific information
	ULONG Ready; // set to 0 by the guest, then set to 1 by the host when the io request is complete
};
static_assert(sizeof(IoInfoBlock) == 16);
static_assert(sizeof(IoStatus) == sizeof(NTSTATUS));
static_assert(offsetof(IoInfoBlock, Status) == 4);
static_assert(offsetof(IoInfoBlock, Info) == 8);
static_assert(offsetof(IoInfoBlock, Ready) == 12);

struct IoInfoBlockOc {
	IoInfoBlock Header;
	ULONG FileSize; // actual size of the opened file
	union {
		struct {
			ULONG FreeClusters; // number of free clusters remaining
			ULONG CreationTime; // timestamp when the file was created
			ULONG LastAccessTime; // timestamp when the file was read/written/executed
			ULONG LastWriteTime; // timestamp when the file was written/truncated/overwritten
		} Fatx;
		LONGLONG XdvdfsTimestamp; // only one timestamp for xdvdfs, because xiso is read-only
	};
};
#pragma pack(pop)

struct XBOX_HARDWARE_INFO {
	ULONG Flags;
	UCHAR GpuRevision;
	UCHAR McpRevision;
	UCHAR Unknown3;
	UCHAR Unknown4;
};

struct XBOX_KRNL_VERSION {
	USHORT Major;
	USHORT Minor;
	USHORT Build;
	USHORT Qfe;
};

inline ConsoleType XboxType;

#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(322) DLLEXPORT extern XBOX_HARDWARE_INFO XboxHardwareInfo;

EXPORTNUM(324) DLLEXPORT extern XBOX_KRNL_VERSION XboxKrnlVersion;

#ifdef __cplusplus
}
#endif

IoInfoBlock SubmitIoRequestToHost(ULONG Type, ULONG Handle);
IoInfoBlock SubmitIoRequestToHost(ULONG Type, LONGLONG Offset, ULONG Size, ULONG Address, ULONG Handle);
IoInfoBlock SubmitIoRequestToHost(ULONG Type, LONGLONG Offset, ULONG Size, ULONG Address, ULONG Handle, ULONG Timestamp);
IoInfoBlockOc SubmitIoRequestToHost(ULONG Type, LONGLONG InitialSize, ULONG Size, ULONG Handle, ULONG Path, ULONG Attributes, ULONG Timestamp,
	ULONG DesiredAccess, ULONG CreateOptions);
VOID KeSetSystemTime(PLARGE_INTEGER NewTime, PLARGE_INTEGER OldTime);
