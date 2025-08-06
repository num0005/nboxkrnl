/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "..\types.hpp"
#include "seh.hpp"
#include "io.hpp"
#include "rtl_assert.hpp"
#include "rtl_context.hpp"
#include <string.h>


struct RTL_CRITICAL_SECTION {
	KEVENT Event;
	LONG LockCount;
	LONG RecursionCount;
	struct KTHREAD* OwningThread;
};
using PRTL_CRITICAL_SECTION = RTL_CRITICAL_SECTION *;

#define INITIALIZE_GLOBAL_CRITICAL_SECTION(CriticalSection)        \
    RTL_CRITICAL_SECTION CriticalSection = {                       \
        SynchronizationEvent,                                      \
        FALSE,                                                     \
        offsetof(RTL_CRITICAL_SECTION, LockCount) / sizeof(LONG),  \
        FALSE,                                                     \
        FALSE,                                                     \
        &CriticalSection.Event.Header.WaitListHead,                       \
        &CriticalSection.Event.Header.WaitListHead,                       \
        -1,                                                        \
        0,                                                         \
        NULL                                                       \
    }

#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(264) VOID XBOXAPI RtlAssert
(
	PVOID FailedAssertion,
	PVOID FileName,
	ULONG LineNumber,
	PCHAR Message
);

EXPORTNUM(266) USHORT XBOXAPI RtlCaptureStackBackTrace
(
	ULONG FramesToSkip,
	ULONG FramesToCapture,
	PVOID *BackTrace,
	PULONG BackTraceHash
);

EXPORTNUM(277) VOID XBOXAPI RtlEnterCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(278) VOID XBOXAPI RtlEnterCriticalSectionAndRegion
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(279) BOOLEAN XBOXAPI RtlEqualString
(
	PSTRING String1,
	PSTRING String2,
	BOOLEAN CaseInSensitive
);

EXPORTNUM(280) BOOLEAN NTAPI RtlEqualUnicodeString
(
	IN PUNICODE_STRING String1,
	IN PUNICODE_STRING String2,
	IN BOOLEAN CaseInSensitive
);

EXPORTNUM(281) LARGE_INTEGER XBOXAPI RtlExtendedIntegerMultiply
(
	LARGE_INTEGER Multiplicand,
	LONG Multiplier
);

EXPORTNUM(282) LARGE_INTEGER NTAPI RtlExtendedLargeIntegerDivide
(
	IN LARGE_INTEGER Dividend,
	IN ULONG Divisor,
	IN OUT PULONG Remainder OPTIONAL
);

EXPORTNUM(283) LARGE_INTEGER XBOXAPI RtlExtendedMagicDivide
(
	LARGE_INTEGER Dividend,
	LARGE_INTEGER MagicDivisor,
	CCHAR ShiftCount
);

EXPORTNUM(285) VOID XBOXAPI RtlFillMemoryUlong
(
	PVOID Destination,
	SIZE_T Length,
	ULONG Pattern
);

EXPORTNUM(289) VOID XBOXAPI RtlInitAnsiString
(
	PANSI_STRING DestinationString,
	PCSZ SourceString
);

EXPORTNUM(260) NTSTATUS NTAPI RtlAnsiStringToUnicodeString
(
	PUNICODE_STRING DestinationString,
	PSTRING         SourceString,
	UCHAR           AllocateDestinationString
);

EXPORTNUM(261) NTSTATUS NTAPI RtlAppendStringToString
(
	IN PSTRING Destination,
	IN PSTRING Source
);

EXPORTNUM(262) NTSTATUS NTAPI RtlAppendUnicodeStringToString
(
	IN PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source
);

EXPORTNUM(263) NTSTATUS NTAPI RtlAppendUnicodeToString
(
	IN OUT PUNICODE_STRING Destination,
	IN PCWSTR Source
);


EXPORTNUM(267) NTSTATUS NTAPI RtlCharToInteger
(
	IN     PCSZ    String,
	IN     ULONG   Base OPTIONAL,
	OUT    PULONG  Value
);

EXPORTNUM(268) SIZE_T NTAPI RtlCompareMemory
(
	IN const void* Source1,
	IN const void* Source2,
	IN SIZE_T      Length
);

EXPORTNUM(269) SIZE_T NTAPI RtlCompareMemoryUlong
(
	IN PVOID Source,
	IN SIZE_T Length,
	IN ULONG Pattern
);

EXPORTNUM(270) LONG NTAPI RtlCompareString
(
	IN PSTRING String1,
	IN PSTRING String2,
	IN BOOLEAN CaseInSensitive
);

EXPORTNUM(271) LONG NTAPI RtlCompareUnicodeString
(
	IN PUNICODE_STRING String1,
	IN PUNICODE_STRING String2,
	IN BOOLEAN CaseInSensitive
);

EXPORTNUM(272) void NTAPI RtlCopyString
(
	OUT PSTRING DestinationString,
	IN PSTRING SourceString OPTIONAL
);

EXPORTNUM(273) void NTAPI RtlCopyUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString OPTIONAL
);

EXPORTNUM(274) BOOLEAN NTAPI RtlCreateUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PCWSTR SourceString
);

EXPORTNUM(276) NTSTATUS NTAPI RtlDowncaseUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString,
	IN BOOLEAN AllocateDestinationString
);

EXPORTNUM(286) VOID NTAPI RtlFreeAnsiString(IN PANSI_STRING AnsiString);

EXPORTNUM(287) VOID NTAPI RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString);

EXPORTNUM(288) __declspec(noinline) VOID NTAPI RtlGetCallersAddress(
	OUT PVOID* CallersAddress,
	OUT PVOID* CallersCaller
);

EXPORTNUM(290) void NTAPI RtlInitUnicodeString
(
	IN OUT PUNICODE_STRING DestinationString,
	IN     PCWSTR         SourceString
);

EXPORTNUM(291) VOID XBOXAPI RtlInitializeCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(292) NTSTATUS NTAPI RtlIntegerToChar
(
	ULONG value,   /* [I] Value to be converted */
	ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
	ULONG length,  /* [I] Length of the str buffer in bytes */
	PCHAR str     /* [O] Destination for the converted value */
);

EXPORTNUM(293) NTSTATUS NTAPI RtlIntegerToUnicodeString(
	IN ULONG Value,
	IN ULONG Base OPTIONAL,
	IN OUT PUNICODE_STRING String
);

EXPORTNUM(294) VOID XBOXAPI RtlLeaveCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(295) VOID XBOXAPI RtlLeaveCriticalSectionAndRegion
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(297) VOID XBOXAPI RtlMapGenericMask
(
	PACCESS_MASK AccessMask,
	PGENERIC_MAPPING GenericMapping
);

EXPORTNUM(299) NTSTATUS NTAPI RtlMultiByteToUnicodeN
(
	IN     PWSTR  UnicodeString,
	IN     ULONG  MaxBytesInUnicodeString,
	IN     PULONG BytesInUnicodeString,
	IN     PCHAR  MultiByteString,
	IN     ULONG  BytesInMultiByteString
);

EXPORTNUM(300) NTSTATUS NTAPI RtlMultiByteToUnicodeSize
(
	_Out_ PULONG     UnicodeSize,
	_In_ const CHAR* MbString,
	_In_ ULONG       MbSize
);

EXPORTNUM(301) ULONG XBOXAPI RtlNtStatusToDosError
(
	NTSTATUS Status
);

EXPORTNUM(302) VOID XBOXAPI RtlRaiseException
(
	PEXCEPTION_RECORD ExceptionRecord
);

EXPORTNUM(303) VOID XBOXAPI RtlRaiseStatus
(
	NTSTATUS Status
);

EXPORTNUM(304) BOOLEAN XBOXAPI RtlTimeFieldsToTime
(
	PTIME_FIELDS TimeFields,
	PLARGE_INTEGER Time
);

EXPORTNUM(305) VOID XBOXAPI RtlTimeToTimeFields
(
	PLARGE_INTEGER Time,
	PTIME_FIELDS TimeFields
);

/*++
 * RtlTryEnterCriticalSection
 *
 *     Attemps to gain ownership of the critical section without waiting.
 *
 * Params:
 *     CriticalSection - Critical section to attempt acquiring.
 *
 * Returns:
 *     TRUE if the critical section has been acquired, FALSE otherwise.
 *
 * Remarks:
 *     None
 *
 *--*/
EXPORTNUM(306) BOOLEAN NTAPI RtlTryEnterCriticalSection
(
	PRTL_CRITICAL_SECTION CriticalSection
);

EXPORTNUM(308) NTSTATUS NTAPI RtlUnicodeStringToAnsiString(
	IN OUT PANSI_STRING AnsiDest,
	IN const PUNICODE_STRING UniSource,
	IN BOOLEAN AllocateDestinationString);

EXPORTNUM(309) NTSTATUS NTAPI RtlUnicodeStringToInteger(
	const UNICODE_STRING* str, /* [I] Unicode string to be converted */
	ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
	ULONG* value               /* [O] Destination for the converted value */
);

EXPORTNUM(310) NTSTATUS NTAPI RtlUnicodeToMultiByteN
(
	_Out_ PCHAR MbString,
	_In_ ULONG MbSize,
	_Out_opt_ PULONG ResultSize,
	_In_ const WCHAR* UnicodeString,
	_In_ ULONG  UnicodeSize
);

EXPORTNUM(311) NTSTATUS NTAPI RtlUnicodeToMultiByteSize
(
	_Out_ PULONG MbSize,
	_In_ const WCHAR* UnicodeString,
	_In_ ULONG UnicodeSize
);

EXPORTNUM(312) VOID XBOXAPI RtlUnwind
(
	PVOID TargetFrame,
	PVOID TargetIp,
	PEXCEPTION_RECORD ExceptionRecord,
	PVOID ReturnValue
);

EXPORTNUM(314) NTSTATUS NTAPI RtlUpcaseUnicodeString
(
	IN OUT PUNICODE_STRING   UniDest,
	IN const PUNICODE_STRING UniSource,
	IN BOOLEAN  AllocateDestinationString
);

EXPORTNUM(315) NTSTATUS NTAPI RtlUpcaseUnicodeToMultiByteN
(
	_Out_ PCHAR MbString,
	_In_ ULONG MbSize,
	_Out_opt_ PULONG  ResultSize,
	_In_ const WCHAR* UnicodeString,
	_In_ ULONG UnicodeSize
);

EXPORTNUM(316) CHAR XBOXAPI RtlUpperChar
(
	CHAR Character
);

EXPORTNUM(317) VOID NTAPI RtlUpperString
(
	PSTRING DestinationString,
	const STRING* SourceString
);

EXPORTNUM(319) ULONG XBOXAPI RtlWalkFrameChain
(
	PVOID *Callers,
	ULONG Count,
	ULONG Flags
);

EXPORTNUM(320) VOID XBOXAPI RtlZeroMemory
(
	PVOID Destination,
	SIZE_T Length
);

EXPORTNUM(361) int CDECL RtlSnprintf
(
	IN PCHAR       string,
	IN SIZE_T      count,
	IN const char* format,
	...
);

EXPORTNUM(362) int CDECL RtlSprintf
(
	IN PCHAR string,
	IN const char* format,
	...
);

EXPORTNUM(363) int CDECL RtlVsnprintf
(
	IN PCHAR string,
	IN SIZE_T count,
	IN const char* format,
	va_list ap
);

EXPORTNUM(364) int CDECL RtlVsprintf
(
	IN PCHAR string,
	IN const char* format,
	va_list ap
);

#ifdef __cplusplus
}
#endif

#define UNREACHABLE NT_ASSERT_MESSAGE(FALSE, "unreachable code reached!")

#include <intrin.h>

inline UCHAR RtlpBitScanForward(ULONG *Index, ULONG Mask)
{
	auto value = _BitScanForward(reinterpret_cast<unsigned long*>(Index), Mask);
	return value;
}
#define BitScanForward RtlpBitScanForward

inline UCHAR RtlpBitScanReverse(ULONG *Index, ULONG Mask)
{
	auto value = _BitScanReverse(reinterpret_cast<unsigned long*>(Index), Mask);
	return value;
}
#define BitScanReverse RtlpBitScanReverse

extern "C" {

	EXPORTNUM(318) USHORT inline FASTCALL RtlUshortByteSwap
	(
		IN USHORT Source
	)
	{
		return _byteswap_ushort(Source);
	}

	EXPORTNUM(307) ULONG inline FASTCALL RtlUlongByteSwap
	(
		IN ULONG Source
	)
	{
		return _byteswap_ulong(Source);
	}

	// ******************************************************************
	// * 0x0128 - RtlLowerChar()
	// ******************************************************************
	inline EXPORTNUM(296) CHAR NTAPI RtlLowerChar
	(
		CHAR Character
	)
	{

		BYTE CharCode = (BYTE)Character;

		if (CharCode >= 'A' && CharCode <= 'Z')
		{
			CharCode ^= 0x20;
		}

		// Latin alphabet (ISO 8859-1)
		else if (CharCode >= 0xc0 && CharCode <= 0xde && CharCode != 0xd7)
		{
			CharCode ^= 0x20;
		}

		return (CHAR)(CharCode);
	}

	//#define tolower RtlLowerChar

	// Source: Cxbx-Reloaded
	inline EXPORTNUM(316) CHAR XBOXAPI RtlUpperChar
	(
		CHAR Character
	)
	{
		BYTE CharCode = (BYTE)Character;

		if (CharCode >= 'a' && CharCode <= 'z')
		{
			CharCode ^= 0x20;
		}
		// Latin alphabet (ISO 8859-1)
		else if (CharCode >= 0xe0 && CharCode <= 0xfe && CharCode != 0xf7)
		{
			CharCode ^= 0x20;
		}
		else if (CharCode == 0xFF)
		{
			CharCode = '?';
		}

		return CharCode;
	}

	//#define toupper RtlUpperChar

	// from reactos nlsboot.c
	// basic latin-only implementation, should be replaced with a full unicode implementation if that's actually needed
	inline EXPORTNUM(313) WCHAR NTAPI RtlUpcaseUnicodeChar
	(
			_In_ WCHAR Source
	)
	{
		USHORT Offset = 0;

		if (Source < 'a')
			return Source;

		if (Source <= 'z')
			return (Source - ('a' - 'A'));

		return Source + (short)Offset;
	}


	inline EXPORTNUM(275) WCHAR NTAPI RtlDowncaseUnicodeChar
	(
			_In_ WCHAR Source
	)
	{
		USHORT Offset = 0;

		if (Source < L'A')
			return Source;

		if (Source <= L'Z')
			return Source + (L'a' - L'A');

		#if 0
		if (Source < 0x80)
			return Source;
		#endif

		return Source + (short)Offset;
	}

	inline EXPORTNUM(284) void NTAPI RtlFillMemory
	(
		IN void* Destination,
		IN DWORD Length,
		IN BYTE  Fill
	)
	{
		memset(Destination, Fill, Length);
	}

	inline EXPORTNUM(298) void NTAPI RtlMoveMemory
	(
		PVOID       Destination,
		const VOID* Source,
		SIZE_T      Length
	)
	{
		memmove(Destination, Source, Length);
	}

}

#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))

inline static size_t wcslen(const WCHAR* s)
{
	size_t rc = 0;

	while (s[rc])
	{
		++rc;
	}

	return rc;
}
