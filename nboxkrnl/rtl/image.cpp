#include "image.hpp"
#include "rtl.hpp"
#include "dbg.hpp"

PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader
(
	IN PVOID Base
)
{
	PIMAGE_DOS_HEADER DOSHeader = (PIMAGE_DOS_HEADER)Base;
	if (RtlUlongByteSwap(DOSHeader->e_magic) != 'MZ')
		return NULL;

	const PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)(((UCHAR*)Base) + DOSHeader->e_lfanew);

	if (RtlUlongByteSwap(NtHeaders->Signature) != 'PE\0\0')
		return NULL;

	return NtHeaders;
}

//
// Retreives the first image section header from the Nt Header
//
#define IMAGE_FIRST_SECTION( NtHeader )                \
  ((PIMAGE_SECTION_HEADER) ((ULONG_PTR)(NtHeader) +    \
   offsetof( IMAGE_NT_HEADERS, OptionalHeader ) +  \
   ((NtHeader))->FileHeader.SizeOfOptionalHeader))

/*
 * @implemented
 */
PIMAGE_SECTION_HEADER NTAPI RtlImageRvaToSection
(
	PIMAGE_NT_HEADERS NtHeader,
	PVOID BaseAddress,
	ULONG Rva
)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG Va;
	ULONG Count;

	Count = NtHeader->FileHeader.NumberOfSections;
	Section = IMAGE_FIRST_SECTION(NtHeader);

	while (Count--)
	{
		Va = Section->VirtualAddress;
		if ((Va <= Rva) && (Rva < Va + (Section->SizeOfRawData)))
			return Section;
		Section++;
	}

	return NULL;
}


/*
 * @implemented
 */
PVOID NTAPI RtlImageRvaToVa
(
	PIMAGE_NT_HEADERS NtHeader,
	PVOID BaseAddress,
	ULONG Rva,
	PIMAGE_SECTION_HEADER* SectionHeader
)
{
	PIMAGE_SECTION_HEADER Section = NULL;

	if (SectionHeader)
		Section = *SectionHeader;

	if ((Section == NULL) ||
		(Rva < Section->VirtualAddress) ||
		(Rva >= Section->VirtualAddress + Section->SizeOfRawData))
	{
		Section = RtlImageRvaToSection(NtHeader, BaseAddress, Rva);
		if (Section == NULL)
			return NULL;

		if (SectionHeader)
			*SectionHeader = Section;
	}

	return (PVOID)((ULONG_PTR)BaseAddress + Rva +
		(ULONG_PTR)(Section->PointerToRawData) -
		(ULONG_PTR)(Section->VirtualAddress));
}

PVOID NTAPI RtlImageDirectoryEntryToData
(
	PVOID BaseAddress,
	BOOLEAN MappedAsImage,
	USHORT Directory,
	PULONG Size
)
{
	PIMAGE_NT_HEADERS NtHeader;
	ULONG Va;

	/* Magic flag for non-mapped images. */
	if ((ULONG_PTR)BaseAddress & 1)
	{
		BaseAddress = (PVOID)((ULONG_PTR)BaseAddress & ~1);
		MappedAsImage = FALSE;
	}

	NtHeader = RtlImageNtHeader(BaseAddress);
	if (NtHeader == NULL)
		return NULL;

	PIMAGE_OPTIONAL_HEADER32 OptionalHeader = (PIMAGE_OPTIONAL_HEADER32)&NtHeader->OptionalHeader;

	if (Directory >= OptionalHeader->NumberOfRvaAndSizes)
		return NULL;

	Va = OptionalHeader->DataDirectory[Directory].VirtualAddress;
	if (Va == 0)
		return NULL;

	*Size = OptionalHeader->DataDirectory[Directory].Size;

	if (MappedAsImage || Va < OptionalHeader->SizeOfHeaders)
		return (PVOID)((ULONG_PTR)BaseAddress + Va);

	/* Image mapped as ordinary file, we must find raw pointer */
	return RtlImageRvaToVa(NtHeader, BaseAddress, Va, NULL);
}

static void  __declspec(naked) RtlBadImport()
{
	__asm
	{
		push IMPORTED_FUNCTION_MISSING
		jmp KeBugCheckLogEip
	}
}

NTSTATUS RtlGetProcedureAddress
(
	_In_ PVOID BaseAddress,
	_In_opt_ _When_(Ordinal == 0, _Notnull_) PANSI_STRING Name,
	_In_opt_ _When_(Name == NULL, _In_range_(> , 0)) ULONG Ordinal,
	_Out_ PVOID* ProcedureAddress
)
{
	NT_ASSERT_MESSAGE(Name == nullptr, "Lookup by name is not implemented!");
	NT_ASSERT(ProcedureAddress);

	if (!ProcedureAddress)
		return STATUS_INVALID_PARAMETER_4;

	*ProcedureAddress = &RtlBadImport;

	if (Ordinal != 0 && Name != NULL)
		return STATUS_INVALID_PARAMETER_2;
	if (Ordinal == 0 && Name == NULL)
		return STATUS_INVALID_PARAMETER_3;
	if (Name)
		return STATUS_NOT_IMPLEMENTED;

	/* Get the pointer to the export directory */
	ULONG ExportDirSize;
	PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(BaseAddress,
		TRUE,
		IMAGE_DIRECTORY_ENTRY_EXPORT,
		&ExportDirSize);

	if (!ExportDir)
	{
		DbgPrint("Export table not found for %p!", BaseAddress);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	Ordinal -= ExportDir->Base;

	// ordinal too high to be valid
	if (Ordinal >= ExportDir->NumberOfFunctions)
	{
		return STATUS_ORDINAL_NOT_FOUND;
	}

	PULONG AddressOfFunctions = (PULONG)((ULONG)BaseAddress + ExportDir->AddressOfFunctions);
	ULONG FunctionRVA = AddressOfFunctions[Ordinal];
	if (!FunctionRVA)
		DbgPrint("ordinal seems invalid");
	PVOID Function = (PVOID)((ULONG)BaseAddress + FunctionRVA);

	*ProcedureAddress = Function;

	return STATUS_SUCCESS;
}
