/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

// WARNING: apparently, the inline assembler of MSVC has a problem when accessing struct members as memory operands with instructions such as MOV and POP.
// In this case, the assembler sometimes emits isntructions that access them as if they were bytes, instead of dwords, which can then cause subtle bugs. To
// avoid the problem, be sure to always specify the memory operand size in the instructions, instead of letting the assembler guess it

// Workaround fix for using clang-format with WhitespaceSensitiveMacros option
// However, clang-format will not format inside the parentheses bracket.
#define ASM(...) __asm __VA_ARGS__
#define ASM_BEGIN __asm {
#define ASM_END }

// segment tag for code, not currently used but will eventually be used to discard INIT after boot
#define CODE_SEG(segment) __declspec(code_seg(segment))

// mark data within (STICKY_PAGE_START, STICKY_PAGE_END) as being part of the sticky page
#define STICKY_PAGE_START __pragma(data_seg(push, STICKY, "STICKY")) __pragma(bss_seg(push, STICKY, "STICKY"))
#define STICKY_PAGE_END __pragma(data_seg(pop, STICKY)) __pragma(bss_seg(pop, STICKY))

// segment tag for initalized data, not currently used
#define DATA_SEG(segment) __declspec(code_seg(segment)) 
// segment tag for uninitialized data, used for sticky page
// TODO: should sticky page be using DATA_SEG instead?
#define BSS_SEG(segment) __declspec(bss_seg (segment)) 

#include <stdint.h>
#include <stddef.h>
#include "bug_codes.hpp"

#define TRUE 1
#define FALSE 0

#define MAXLONG     0x7fffffff

#define XBOXAPI  __stdcall
#define FASTCALL __fastcall
#define CDECL    __cdecl
#define NTAPI XBOXAPI
#define OUT
#define IN
#define OPTIONAL
#define PAGED_CODE()
#define RESTRICTED_POINTER

#define DLLEXPORT __declspec(dllexport)
#define EXPORTNUM(n) DLLEXPORT

#define ANSI_NULL ((CHAR)0)
#define UNICODE_NULL ((WCHAR)0)

#define _SEH2_TRY __try
#define _SEH2_EXCEPT __except
#define _SEH2_END
#define _SEH2_GetExceptionCode GetExceptionCode
//#define ExSystemExceptionFilter() 1


using VOID = void;
using PVOID = void *;
using BYTE = uint8_t;
using UCHAR = unsigned char;
using CHAR = char;
using WCHAR = wchar_t;
using BOOL = bool;
using SCHAR = CHAR;
using CCHAR = CHAR;
using BOOLEAN = uint8_t;
using PBOOLEAN = BOOLEAN*;
using USHORT = uint16_t;
using CSHORT = int16_t;
using WORD = uint16_t;
using DWORD = uint32_t;
using ULONG = unsigned long;
using UINT = uint32_t;
using LONG = long;
using INT = int32_t;
using LONGLONG = int64_t;
using ULONGLONG = uint64_t;
using ULONG64 = uint64_t;
using QUAD = ULONGLONG;
using PCHAR = CHAR *;
using PBYTE = BYTE *;
using PWCHAR = WCHAR *;
using PWSTR = PWCHAR;
using PUSHORT = USHORT *;
using PULONG = ULONG *;
using PULONGLONG = ULONGLONG *;
using PULONG64 = ULONG64*;
using LPDWORD = DWORD *;
using PUCHAR = UCHAR *;
using PWSTR = WCHAR *;
using PCWSTR = const WCHAR *;
using PLONG = LONG *;
using PCSZ = const CHAR *;
using ULONG_PTR = ULONG;
using DWORD_PTR = ULONG;
using LONG_PTR = LONG;
using PULONG_PTR = ULONG_PTR *;
using SIZE_T = ULONG_PTR;
using PSIZE_T = SIZE_T *;
using DWORDLONG = uint64_t;
using NTSTATUS = LONG;
using KPRIORITY = LONG;
using HANDLE = PVOID;
using PHANDLE = HANDLE *;
using HRESULT = LONG;
using ACCESS_MASK = ULONG;
using PACCESS_MASK = ACCESS_MASK *;

#include "ntstatus.hpp"
#include <stddef.h>

#define CONTAINING_RECORD(address, type, field) ((type *)((PCHAR)(address) - (ULONG_PTR)offsetof(type, field)))

#define LOWORD(l) ((WORD)((DWORD_PTR)(l)))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xFFFF))

#define MAXULONG_PTR (~((ULONG_PTR)0))
#define MAXLONG_PTR  ((LONG_PTR)(MAXULONG_PTR >> 1))
#define MINLONG_PTR  (~MAXLONG_PTR)

struct LIST_ENTRY {
	LIST_ENTRY *Flink;
	LIST_ENTRY *Blink;
};
using PLIST_ENTRY = LIST_ENTRY *;

// Source: Cxbx-Reloaded
inline VOID InitializeListHead(PLIST_ENTRY pListHead)
{
	pListHead->Flink = pListHead->Blink = pListHead;
}

// Source: Cxbx-Reloaded
inline BOOLEAN IsListEmpty(PLIST_ENTRY pListHead)
{
	return (pListHead->Flink == pListHead);
}

// Source: Cxbx-Reloaded
inline VOID InsertTailList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
	PLIST_ENTRY _EX_ListHead = pListHead;
	PLIST_ENTRY _EX_Blink = _EX_ListHead->Blink;

	pEntry->Flink = _EX_ListHead;
	pEntry->Blink = _EX_Blink;
	_EX_Blink->Flink = pEntry;
	_EX_ListHead->Blink = pEntry;
}

// Source: Cxbx-Reloaded
inline VOID InsertHeadList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
	PLIST_ENTRY _EX_ListHead = pListHead;
	PLIST_ENTRY _EX_Flink = _EX_ListHead->Flink;

	pEntry->Flink = _EX_Flink;
	pEntry->Blink = _EX_ListHead;
	_EX_Flink->Blink = pEntry;
	_EX_ListHead->Flink = pEntry;
}


// source: reactos
inline BOOLEAN RemoveEntryList(PLIST_ENTRY Entry)
{
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;

	OldFlink = Entry->Flink;
	OldBlink = Entry->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;
	return (BOOLEAN)(OldFlink == OldBlink); // almost free is-empty
}

// Source: Cxbx-Reloaded
inline PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY pList = ListHead->Blink;
	RemoveEntryList(pList);
	return pList;
}

// Source: Cxbx-Reloaded
inline PLIST_ENTRY RemoveHeadList(PLIST_ENTRY pListHead)
{
	PLIST_ENTRY pList = pListHead->Flink;
	RemoveEntryList(pList);
	return pList;
}

struct SINGLE_LIST_ENTRY {
	struct SINGLE_LIST_ENTRY *Next;
};
using SLIST_ENTRY = SINGLE_LIST_ENTRY;
using PSINGLE_LIST_ENTRY = SINGLE_LIST_ENTRY *;
using PSLIST_ENTRY = SLIST_ENTRY *;

union SLIST_HEADER {
	ULONGLONG Alignment;
	struct {
		SINGLE_LIST_ENTRY Next;
		USHORT Depth;
		USHORT Sequence;
	};
};
using PSLIST_HEADER = SLIST_HEADER *;

struct STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR Buffer;
};
using ANSI_STRING = STRING;
using PSTRING = STRING *;
using PANSI_STRING = PSTRING;

struct UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWCHAR Buffer;
};
using PUNICODE_STRING = UNICODE_STRING *;

struct DISPATCHER_HEADER {
	UCHAR Type;
	UCHAR Absolute;
	UCHAR Size;
	UCHAR Inserted;
	LONG SignalState;
	LIST_ENTRY WaitListHead;
};

union ULARGE_INTEGER {
	struct {
		DWORD LowPart;
		DWORD HighPart;
	};

	struct {
		DWORD LowPart;
		DWORD HighPart;
	} u;

	DWORDLONG QuadPart;
};
using PULARGE_INTEGER = ULARGE_INTEGER *;

union LARGE_INTEGER {
	struct {
		DWORD LowPart;
		LONG HighPart;
	};

	struct {
		DWORD LowPart;
		LONG HighPart;
	} u;

	LONGLONG QuadPart;
};
using PLARGE_INTEGER = LARGE_INTEGER *;

struct TIME_FIELDS {
	USHORT Year;
	USHORT Month;
	USHORT Day;
	USHORT Hour;
	USHORT Minute;
	USHORT Second;
	USHORT Millisecond;
	USHORT Weekday;
};
using PTIME_FIELDS = TIME_FIELDS *;

typedef ULONG KSPIN_LOCK;  // winnt ntndis
typedef KSPIN_LOCK* PKSPIN_LOCK;

// ******************************************************************
// * FILETIME
// ******************************************************************
// Source : DXBX
typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
}
FILETIME, * PFILETIME;
