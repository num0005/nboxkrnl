#pragma once
#include "../types.hpp"
#include "../driverspecs.h"

//
// Directory Entry Specifiers
//
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8
#define IMAGE_DIRECTORY_ENTRY_TLS             9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11
#define IMAGE_DIRECTORY_ENTRY_IAT            12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

typedef struct _IMAGE_FILE_HEADER // Size=20
{
	USHORT  Machine;
	USHORT  NumberOfSections;
	UINT    TimeDateStamp;
	UINT    PointerToSymbolTable;
	UINT    NumberOfSymbols;
	USHORT  SizeOfOptionalHeader;
	USHORT  Characteristics;
} IMAGE_FILE_HEADER, * PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY
{
	UINT VirtualAddress;
	UINT Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER32
{
	USHORT  Magic;
	UCHAR   MajorLinkerVersion;
	UCHAR   MinorLinkerVersion;
	UINT    SizeOfCode;
	UINT    SizeOfInitializedData;
	UINT    SizeOfUninitializedData;
	UINT    AddressOfEntryPoint;
	UINT    BaseOfCode;
	UINT    BaseOfData;
	UINT    ImageBase;
	UINT    SectionAlignment;
	UINT    FileAlignment;
	USHORT  MajorOperatingSystemVersion;
	USHORT  MinorOperatingSystemVersion;
	USHORT  MajorImageVersion;
	USHORT  MinorImageVersion;
	USHORT  MajorSubsystemVersion;
	USHORT  MinorSubsystemVersion;
	UINT    Win32VersionValue;
	UINT    SizeOfImage;
	UINT    SizeOfHeaders;
	UINT    CheckSum;
	USHORT  Subsystem;
	USHORT  DllCharacteristics;
	UINT    SizeOfStackReserve;
	UINT    SizeOfStackCommit;
	UINT    SizeOfHeapReserve;
	UINT    SizeOfHeapCommit;
	UINT    LoaderFlags;
	UINT    NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, * PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_NT_HEADERS
{
	UINT Signature; // 0x0
	struct _IMAGE_FILE_HEADER FileHeader; // 0x4
	struct _IMAGE_OPTIONAL_HEADER32 OptionalHeader; // 0x18
} IMAGE_NT_HEADERS, * PIMAGE_NT_HEADERS;

typedef struct _IMAGE_EXPORT_DIRECTORY
{
	ULONG Characteristics;
	ULONG TimeDateStamp;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG Name;
	ULONG Base;
	ULONG NumberOfFunctions;
	ULONG NumberOfNames;
	ULONG AddressOfFunctions;
	ULONG AddressOfNames;
	ULONG AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, * PIMAGE_EXPORT_DIRECTORY;

PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader
(
	IN PVOID Base
);

PIMAGE_SECTION_HEADER NTAPI RtlImageRvaToSection
(
	PIMAGE_NT_HEADERS NtHeader,
	PVOID BaseAddress,
	ULONG Rva
);

PVOID NTAPI RtlImageRvaToVa
(
	PIMAGE_NT_HEADERS NtHeader,
	PVOID BaseAddress,
	ULONG Rva,
	PIMAGE_SECTION_HEADER* SectionHeader
);

PVOID NTAPI RtlImageDirectoryEntryToData
(
	PVOID BaseAddress,
	BOOLEAN MappedAsImage,
	USHORT Directory,
	PULONG Size
);

NTSTATUS RtlGetProcedureAddress
(
	_In_ PVOID BaseAddress,
	_In_opt_ _When_(Ordinal == 0, _Notnull_) PANSI_STRING Name,
	_In_opt_ _When_(Name == NULL, _In_range_(> , 0)) ULONG Ordinal,
	_Out_ PVOID* ProcedureAddress
);
