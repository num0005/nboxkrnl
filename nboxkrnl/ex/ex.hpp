/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "zw.hpp"
#include "ob.hpp"

#define TIME_ZONE_NAME_LENGTH 4

 // No defined value type.
#define REG_NONE 0
// A null - terminated string. This will be either a Unicode or an ANSI string, depending on whether you use the Unicode or ANSI functions.
#define REG_SZ 1
// A null - terminated string that contains unexpanded references to environment variables (for example, "%PATH%").
#define REG_EXPAND_SZ 2
// Binary data in any form.
#define REG_BINARY 3
// A 32 - bit number.
#define REG_DWORD 4
// A 32 - bit number in little - endian format. Windows is designed to run on little - endian computer architectures.
#define REG_DWORD_LITTLE_ENDIAN 4
// A 32 - bit number in big - endian format. Some UNIX systems support big - endian architectures.
#define REG_DWORD_BIG_ENDIAN 5
// A null - terminated Unicode string that contains the target path of a symbolic link that was created by calling the RegCreateKeyEx function with REG_OPTION_CREATE_LINK.
#define REG_LINK 6
// A sequence of null - terminated strings, terminated by an empty string(\0). String1\0String2\0String3\0LastString\0\0.
#define REG_MULTI_SZ 7
// Resource list in the resource map
#define REG_RESOURCE_LIST 8
// Resource list in the hardware description
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10


enum XC_VALUE_INDEX {
	XC_TIMEZONE_BIAS = 0,
	XC_TZ_STD_NAME = 1,
	XC_TZ_STD_DATE = 2,
	XC_TZ_STD_BIAS = 3,
	XC_TZ_DLT_NAME = 4,
	XC_TZ_DLT_DATE = 5,
	XC_TZ_DLT_BIAS = 6,
	XC_LANGUAGE = 7,
	XC_VIDEO = 8,
	XC_AUDIO = 9,
	XC_P_CONTROL_GAMES = 0xA,
	XC_P_CONTROL_PASSWORD = 0xB,
	XC_P_CONTROL_MOVIES = 0xC,
	XC_ONLINE_IP_ADDRESS = 0xD,
	XC_ONLINE_DNS_ADDRESS = 0xE,
	XC_ONLINE_DEFAULT_GATEWAY_ADDRESS = 0xF,
	XC_ONLINE_SUBNET_ADDRESS = 0x10,
	XC_MISC = 0x11,
	XC_DVD_REGION = 0x12,
	XC_MAX_OS = 0xFF,
	// Break
	XC_FACTORY_START_INDEX = 0x100,
	XC_FACTORY_SERIAL_NUMBER = XC_FACTORY_START_INDEX,
	XC_FACTORY_ETHERNET_ADDR = 0x101,
	XC_FACTORY_ONLINE_KEY = 0x102,
	XC_FACTORY_AV_REGION = 0x103,
	XC_FACTORY_GAME_REGION = 0x104,
	XC_MAX_FACTORY = 0x1FF,
	// Break
	XC_ENCRYPTED_SECTION = 0xFFFE,
	XC_MAX_ALL = 0xFFFF
};
using PXC_VALUE_INDEX = XC_VALUE_INDEX *;

struct XBOX_ENCRYPTED_SETTINGS {
	UCHAR Checksum[20];
	UCHAR Confounder[8];
	UCHAR HDKey[XBOX_KEY_LENGTH];
	ULONG GameRegion;
};

struct XBOX_FACTORY_SETTINGS {
	ULONG Checksum;
	UCHAR SerialNumber[12];
	UCHAR EthernetAddr[6];
	UCHAR Reserved1[2];
	UCHAR OnlineKey[16];
	ULONG AVRegion;
	ULONG Reserved2;
};

struct XBOX_TIMEZONE_DATE {
	UCHAR Month;
	UCHAR Day;
	UCHAR DayOfWeek;
	UCHAR Hour;
};

struct XBOX_USER_SETTINGS {
	ULONG Checksum;
	LONG TimeZoneBias;
	CHAR TimeZoneStdName[TIME_ZONE_NAME_LENGTH];
	CHAR TimeZoneDltName[TIME_ZONE_NAME_LENGTH];
	ULONG Reserved1[2];
	XBOX_TIMEZONE_DATE TimeZoneStdDate;
	XBOX_TIMEZONE_DATE TimeZoneDltDate;
	ULONG Reserved2[2];
	LONG TimeZoneStdBias;
	LONG TimeZoneDltBias;
	ULONG Language;
	ULONG VideoFlags;
	ULONG AudioFlags;
	ULONG ParentalControlGames;
	ULONG ParentalControlPassword;
	ULONG ParentalControlMovies;
	ULONG OnlineIpAddress;
	ULONG OnlineDnsAddress;
	ULONG OnlineDefaultGatewayAddress;
	ULONG OnlineSubnetMask;
	ULONG MiscFlags;
	ULONG DvdRegion;
};

struct XBOX_EEPROM {
	XBOX_ENCRYPTED_SETTINGS EncryptedSettings;
	XBOX_FACTORY_SETTINGS FactorySettings;
	XBOX_USER_SETTINGS UserSettings;
	UCHAR Unused[58];
	UCHAR UEMInfo[4];
	UCHAR Reserved1[2];
};

struct ERWLOCK {
	LONG LockCount;
	ULONG WritersWaitingCount;
	ULONG ReadersWaitingCount;
	ULONG ReadersEntryCount;
	KEVENT WriterEvent;
	KSEMAPHORE ReaderSemaphore;
};
using PERWLOCK = ERWLOCK *;

typedef struct _ETIMER
{
	KTIMER KeTimer;
	KAPC TimerApc;
	KDPC TimerDpc;
	LIST_ENTRY ActiveTimerListEntry;
	KSPIN_LOCK Lock;
	LONG Period;
	BOOLEAN ApcAssociated;
	BOOLEAN WakeTimer;
	LIST_ENTRY WakeTimerListEntry;
} ETIMER, * PETIMER;

static_assert(sizeof(XBOX_EEPROM) == 256);

//
// Information Structures for NtQueryEvent
//
typedef struct _EVENT_BASIC_INFORMATION
{
	EVENT_TYPE EventType;
	LONG EventState;
} EVENT_BASIC_INFORMATION, * PEVENT_BASIC_INFORMATION;

inline XBOX_EEPROM CachedEeprom;
inline ULONG XboxFactoryGameRegion;

// ******************************************************************
// * XBOX_REFURB_INFO
// ******************************************************************
// Source : DXBX (Xbox Refurb Info)
typedef struct _XBOX_REFURB_INFO
{
	DWORD Signature;
	DWORD PowerCycleCount;
	FILETIME FirstBootTime;
}
XBOX_REFURB_INFO, * PXBOX_REFURB_INFO;


#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(4) PSLIST_ENTRY FASTCALL InterlockedFlushSList
(
	_Inout_ PSLIST_HEADER SListHead
);

EXPORTNUM(12) VOID XBOXAPI ExAcquireReadWriteLockExclusive
(
	PERWLOCK ReadWriteLock
);

EXPORTNUM(13) VOID XBOXAPI ExAcquireReadWriteLockShared
(
	PERWLOCK ReadWriteLock
);

EXPORTNUM(14) PVOID XBOXAPI ExAllocatePool
(
	SIZE_T NumberOfBytes
);

EXPORTNUM(15) PVOID XBOXAPI ExAllocatePoolWithTag
(
	SIZE_T NumberOfBytes,
	ULONG Tag
);

EXPORTNUM(16) extern OBJECT_TYPE ExEventObjectType;

EXPORTNUM(17) VOID XBOXAPI ExFreePool
(
	PVOID P
);

VOID XBOXAPI ExFreePoolWithTag
(
	PVOID P,
	ULONG Tag
);

EXPORTNUM(18) VOID XBOXAPI ExInitializeReadWriteLock
(
	PERWLOCK ReadWriteLock
);

EXPORTNUM(19) LARGE_INTEGER NTAPI ExInterlockedAddLargeInteger
(
	IN OUT PLARGE_INTEGER Addend,
	IN LARGE_INTEGER Increment
);

// per https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/intrlfst/addlargestatistic.htm this is not a fully thread safe API
// but instead one intended to be very fast
EXPORTNUM(20) VOID FASTCALL ExInterlockedAddLargeStatistic
(
	IN OUT PLARGE_INTEGER Addend,
	IN ULONG Increment
);

EXPORTNUM(22) extern OBJECT_TYPE ExMutantObjectType;

EXPORTNUM(23) ULONG XBOXAPI ExQueryPoolBlockSize
(
	PVOID PoolBlock
);

EXPORTNUM(24) NTSTATUS XBOXAPI ExQueryNonVolatileSetting
(
	DWORD ValueIndex,
	DWORD *Type,
	PVOID Value,
	SIZE_T ValueLength,
	PSIZE_T ResultLength
);

EXPORTNUM(25) NTSTATUS NTAPI ExReadWriteRefurbInfo
(
	IN OUT PXBOX_REFURB_INFO	pRefurbInfo,
	IN ULONG	dwBufferSize,
	IN BOOLEAN	bIsWriteMode
);

EXPORTNUM(26) VOID XBOXAPI ExRaiseException
(
	PEXCEPTION_RECORD ExceptionRecord
);

EXPORTNUM(27) VOID XBOXAPI ExRaiseStatus
(
	NTSTATUS Status
);

EXPORTNUM(28) VOID XBOXAPI ExReleaseReadWriteLock
(
	PERWLOCK ReadWriteLock
);

// ******************************************************************
// * 0x001D - ExSaveNonVolatileSetting()
// ******************************************************************
EXPORTNUM(29) NTSTATUS NTAPI ExSaveNonVolatileSetting
(
	IN  DWORD			   ValueIndex,
	IN  DWORD			   Type,
	IN  PVOID			   Value,
	IN  SIZE_T			   ValueLength
);

EXPORTNUM(32) PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList
(
	IN OUT PLIST_ENTRY ListHead,
	IN PLIST_ENTRY ListEntry
);

EXPORTNUM(33) PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList
(
	IN OUT PLIST_ENTRY ListHead,
	IN PLIST_ENTRY ListEntry
);

EXPORTNUM(34) PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList
(
	IN OUT PLIST_ENTRY ListHead
);

EXPORTNUM(51) LONG FASTCALL InterlockedCompareExchange
(
	volatile PLONG Destination,
	LONG  Exchange,
	LONG  Comparand
);

EXPORTNUM(52) LONG FASTCALL InterlockedDecrement
(
	volatile PLONG Addend
);

EXPORTNUM(53) LONG FASTCALL InterlockedIncrement
(
	volatile PLONG Addend
);

EXPORTNUM(54) LONG FASTCALL InterlockedExchange
(
	volatile PLONG Destination,
	LONG Value
);

EXPORTNUM(57) PSLIST_ENTRY FASTCALL InterlockedPopEntrySList
(
	_Inout_ PSLIST_HEADER SListHead
);

EXPORTNUM(58) PSLIST_ENTRY FASTCALL InterlockedPushEntrySList
(
	_Inout_ PSLIST_HEADER SListHead,
	_Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry
);

EXPORTNUM(185) NTSTATUS NTAPI NtCancelTimer
(
	IN HANDLE TimerHandle,
	OUT BOOLEAN* CurrentState OPTIONAL
);

EXPORTNUM(186) NTSTATUS NTAPI NtClearEvent
(
	IN HANDLE EventHandle
);

EXPORTNUM(189) NTSTATUS NTAPI NtCreateEvent
(
	OUT PHANDLE EventHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	IN EVENT_TYPE EventType,
	IN BOOLEAN InitialState
);

EXPORTNUM(193) NTSTATUS NTAPI NtCreateSemaphore
(
	OUT PHANDLE SemaphoreHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN LONG InitialCount,
	IN LONG MaximumCount
);

EXPORTNUM(194) NTSTATUS NTAPI NtCreateTimer
(
	OUT PHANDLE TimerHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN TIMER_TYPE TimerType
);

NTSTATUS NTAPI NtOpenEvent
(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS NTAPI NtOpenSemaphore
(
	OUT PHANDLE SemaphoreHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

EXPORTNUM(205) NTSTATUS NTAPI NtPulseEvent
(
	IN HANDLE EventHandle,
	OUT PLONG PreviousState OPTIONAL
);

EXPORTNUM(209) NTSTATUS NTAPI NtQueryEvent
(
	IN HANDLE EventHandle,
	OUT PEVENT_BASIC_INFORMATION BasicInfo
);

NTSTATUS NTAPI NtResetEvent
(
	IN HANDLE EventHandle,
	OUT PLONG PreviousState OPTIONAL
);

NTSTATUS NTAPI NtSetEvent
(
	IN HANDLE EventHandle,
	OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS NTAPI NtSetEventBoostPriority
(
	IN HANDLE EventHandle
);

EXPORTNUM(214) NTSTATUS NTAPI NtQuerySemaphore
(
	IN HANDLE SemaphoreHandle,
	OUT PSEMAPHORE_BASIC_INFORMATION BasicInfo
);

//
// Information Structures for NtQueryTimer
//
typedef struct _TIMER_BASIC_INFORMATION
{
	LARGE_INTEGER TimeRemaining;
	BOOLEAN SignalState;
} TIMER_BASIC_INFORMATION, * PTIMER_BASIC_INFORMATION;


EXPORTNUM(216) NTSTATUS NTAPI NtQueryTimer
(
	IN HANDLE TimerHandle,
	OUT PTIMER_BASIC_INFORMATION  TimerInformation
);

EXPORTNUM(222) NTSTATUS NTAPI NtReleaseSemaphore
(
	IN HANDLE SemaphoreHandle,
	IN LONG ReleaseCount,
	OUT PLONG PreviousCount OPTIONAL
);

#ifdef __cplusplus
}
#endif


VOID XBOXAPI ExpDeleteMutant(PVOID Object);
