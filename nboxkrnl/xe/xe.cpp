/*
 * ergo720                Copyright (c) 2022
 * LukeUsher              Copyright (c) 2017
 */

#include "ke.hpp"
#include "xe.hpp"
#include "ex.hpp"
#include "nt.hpp"
#include "mi.hpp"
#include "obp.hpp"
#include "hal.hpp"
#include "dbg.hpp"
#include "image.hpp"
#include "string.h"
#include "xc.hpp"

#define XBE_BASE_ADDRESS 0x10000
#define GetXbeAddress() ((XBE_HEADER *)XBE_BASE_ADDRESS)

// XOR keys for Chihiro/Devkit/Retail XBEs
#define XOR_EP_RETAIL 0xA8FC57AB
#define XOR_EP_CHIHIRO 0x40B5C16E
#define XOR_EP_DEBUG 0x94859D4B
#define XOR_KT_RETAIL 0x5B6D40B6
#define XOR_KT_CHIHIRO 0x2290059D
#define XOR_KT_DEBUG 0xEFB1F152

// Required to make sure that XBE_CERTIFICATE has the correct size
static_assert(sizeof(wchar_t) == 2);


EXPORTNUM(164) PLAUNCH_DATA_PAGE LaunchDataPage = nullptr;

EXPORTNUM(325) XBOX_KEY_DATA XboxSignatureKey;

EXPORTNUM(326) OBJECT_STRING XeImageFileName = { 0, 0, nullptr };

EXPORTNUM(354) XBOX_KEY_DATA XboxAlternateSignatureKeys[ALTERNATE_SIGNATURE_COUNT];

// We are allowed to use the real RSA public key since it cannot be used to sign data, only verify it -> it's not secret/private
constexpr static UCHAR XePublicKeyDataRetail[284] ={
	0x52,0x53,0x41,0x31, 0x08,0x01,0x00,0x00, 0x00,0x08,0x00,0x00, 0xff,0x00,0x00,0x00,
	0x01,0x00,0x01,0x00,
	// Public Modulus "m"
	0xd3,0xd7,0x4e,0xe5, 0x66,0x3d,0xd7,0xe6, 0xc2,0xd4,0xa3,0xa1, 0xf2,0x17,0x36,0xd4,
	0x2e,0x52,0xf6,0xd2, 0x02,0x10,0xf5,0x64, 0x9c,0x34,0x7b,0xff, 0xef,0x7f,0xc2,0xee,
	0xbd,0x05,0x8b,0xde, 0x79,0xb4,0x77,0x8e, 0x5b,0x8c,0x14,0x99, 0xe3,0xae,0xc6,0x73,
	0x72,0x73,0xb5,0xfb, 0x01,0x5b,0x58,0x46, 0x6d,0xfc,0x8a,0xd6, 0x95,0xda,0xed,0x1b,
	0x2e,0x2f,0xa2,0x29, 0xe1,0x3f,0xf1,0xb9, 0x5b,0x64,0x51,0x2e, 0xa2,0xc0,0xf7,0xba,
	0xb3,0x3e,0x8a,0x75, 0xff,0x06,0x92,0x5c, 0x07,0x26,0x75,0x79, 0x10,0x5d,0x47,0xbe,
	0xd1,0x6a,0x52,0x90, 0x0b,0xae,0x6a,0x0b, 0x33,0x44,0x93,0x5e, 0xf9,0x9d,0xfb,0x15,
	0xd9,0xa4,0x1c,0xcf, 0x6f,0xe4,0x71,0x94, 0xbe,0x13,0x00,0xa8, 0x52,0xca,0x07,0xbd,
	0x27,0x98,0x01,0xa1, 0x9e,0x4f,0xa3,0xed, 0x9f,0xa0,0xaa,0x73, 0xc4,0x71,0xf3,0xe9,
	0x4e,0x72,0x42,0x9c, 0xf0,0x39,0xce,0xbe, 0x03,0x76,0xfa,0x2b, 0x89,0x14,0x9a,0x81,
	0x16,0xc1,0x80,0x8c, 0x3e,0x6b,0xaa,0x05, 0xec,0x67,0x5a,0xcf, 0xa5,0x70,0xbd,0x60,
	0x0c,0xe8,0x37,0x9d, 0xeb,0xf4,0x52,0xea, 0x4e,0x60,0x9f,0xe4, 0x69,0xcf,0x52,0xdb,
	0x68,0xf5,0x11,0xcb, 0x57,0x8f,0x9d,0xa1, 0x38,0x0a,0x0c,0x47, 0x1b,0xb4,0x6c,0x5a,
	0x53,0x6e,0x26,0x98, 0xf1,0x88,0xae,0x7c, 0x96,0xbc,0xf6,0xbf, 0xb0,0x47,0x9a,0x8d,
	0xe4,0xb3,0xe2,0x98, 0x85,0x61,0xb1,0xca, 0x5f,0xf7,0x98,0x51, 0x2d,0x83,0x81,0x76,
	0x0c,0x88,0xba,0xd4, 0xc2,0xd5,0x3c,0x14, 0xc7,0x72,0xda,0x7e, 0xbd,0x1b,0x4b,0xa4,
	// Padding? Unused, by the way
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

// Debug Public Key
constexpr static UCHAR XePublicKeyDataDebug[284] ={
	0x52, 0x53, 0x41, 0x31, 0x08, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x9B, 0x83, 0xD4, 0xD5,
	0xDE, 0x16, 0x25, 0x8E, 0xE5, 0x15, 0xF2, 0x18, 0x9D, 0x19, 0x1C, 0xF8,
	0xFE, 0x91, 0xA5, 0x83, 0xAE, 0xA5, 0xA8, 0x95, 0x3F, 0x01, 0xB2, 0xC9,
	0x34, 0xFB, 0xC7, 0x51, 0x2D, 0xAC, 0xFF, 0x38, 0xE6, 0xB6, 0x7B, 0x08,
	0x4A, 0xDF, 0x98, 0xA3, 0xFD, 0x31, 0x81, 0xBF, 0xAA, 0xD1, 0x62, 0x58,
	0xC0, 0x6C, 0x8F, 0x8E, 0xCD, 0x96, 0xCE, 0x6D, 0x03, 0x44, 0x59, 0x93,
	0xCE, 0xEA, 0x8D, 0xF4, 0xD4, 0x6F, 0x6F, 0x34, 0x5D, 0x50, 0xF1, 0xAE,
	0x99, 0x7F, 0x1D, 0x92, 0x15, 0xF3, 0x6B, 0xDB, 0xF9, 0x95, 0x8B, 0x3F,
	0x54, 0xAD, 0x37, 0xB5, 0x4F, 0x0A, 0x58, 0x7B, 0x48, 0xA2, 0x9F, 0x9E,
	0xA3, 0x16, 0xC8, 0xBD, 0x37, 0xDA, 0x9A, 0x37, 0xE6, 0x3F, 0x10, 0x1B,
	0xA8, 0x4F, 0xA3, 0x14, 0xFA, 0xBE, 0x12, 0xFB, 0xD7, 0x19, 0x4C, 0xED,
	0xAD, 0xA2, 0x95, 0x8F, 0x39, 0x8C, 0xC4, 0x69, 0x0F, 0x7D, 0xB8, 0x84,
	0x0A, 0x99, 0x5C, 0x53, 0x2F, 0xDE, 0xF2, 0x1B, 0xC5, 0x1D, 0x4C, 0x43,
	0x3C, 0x97, 0xA7, 0xBA, 0x8F, 0xC3, 0x22, 0x67, 0x39, 0xC2, 0x62, 0x74,
	0x3A, 0x0C, 0xB5, 0x57, 0x01, 0x3A, 0x67, 0xC6, 0xDE, 0x0C, 0x0B, 0xF6,
	0x08, 0x01, 0x64, 0xDB, 0xBD, 0x81, 0xE4, 0xDC, 0x09, 0x2E, 0xD0, 0xF1,
	0xD0, 0xD6, 0x1E, 0xBA, 0x38, 0x36, 0xF4, 0x4A, 0xDD, 0xCA, 0x39, 0xEB,
	0x76, 0xCF, 0x95, 0xDC, 0x48, 0x4C, 0xF2, 0x43, 0x8C, 0xD9, 0x44, 0x26,
	0x7A, 0x9E, 0xEB, 0x99, 0xA3, 0xD8, 0xFB, 0x30, 0xA8, 0x14, 0x42, 0x82,
	0x8D, 0xB4, 0x31, 0xB3, 0x1A, 0xD5, 0x2B, 0xF6, 0x32, 0xBC, 0x62, 0xC0,
	0xFE, 0x81, 0x20, 0x49, 0xE7, 0xF7, 0x58, 0x2F, 0x2D, 0xA6, 0x1B, 0x41,
	0x62, 0xC7, 0xE0, 0x32, 0x02, 0x5D, 0x82, 0xEC, 0xA3, 0xE4, 0x6C, 0x9B,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Chihiro public key (Segaboot)
constexpr static UCHAR XePublicKeyDataChihiro[284] ={
	0x52, 0x53, 0x41, 0x31, 0x08, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x6B, 0x7B, 0x38, 0x78,
	0xE3, 0x16, 0x04, 0x88, 0x1D, 0xAF, 0x63, 0x4E, 0x23, 0xDB, 0x10, 0x14,
	0xB4, 0x52, 0x87, 0xEB, 0xE3, 0x37, 0xC2, 0x35, 0x6E, 0x38, 0x08, 0xDC,
	0x07, 0xC5, 0x92, 0x17, 0xC0, 0xBF, 0xDB, 0xF5, 0x9E, 0x72, 0xDB, 0x2F,
	0x98, 0x93, 0x6E, 0x98, 0x5E, 0xD9, 0x69, 0x80, 0x17, 0xFC, 0x0C, 0x72,
	0x2E, 0xE6, 0x75, 0x85, 0x48, 0xEA, 0xBD, 0xDA, 0x6E, 0x82, 0xD9, 0xFB,
	0x10, 0xAA, 0x11, 0xEE, 0xB5, 0xC1, 0xF2, 0x53, 0xD6, 0xF0, 0xD3, 0x4B,
	0xC2, 0x11, 0x4F, 0x8A, 0x18, 0xFB, 0xB7, 0x36, 0xFC, 0xDD, 0xD0, 0xBF,
	0x5C, 0x32, 0x44, 0x40, 0xEB, 0x92, 0x70, 0xA4, 0xEF, 0x3A, 0xAB, 0x78,
	0x66, 0x1A, 0x03, 0x0A, 0x9E, 0xC5, 0x3A, 0xB7, 0x8F, 0xE5, 0xB1, 0x5E,
	0x44, 0x15, 0xBA, 0x42, 0xD9, 0x10, 0x2A, 0x60, 0x93, 0x47, 0x4C, 0x5B,
	0xE1, 0x24, 0x04, 0x1E, 0x5C, 0x95, 0xB2, 0x17, 0x34, 0xD2, 0x37, 0x5F,
	0x85, 0x83, 0x62, 0x8D, 0x6E, 0x90, 0x69, 0x06, 0xB9, 0xFB, 0x7A, 0x24,
	0x8A, 0xE6, 0xCC, 0x77, 0x1E, 0x0A, 0x8C, 0x2B, 0x3B, 0xA2, 0x33, 0x79,
	0x24, 0x8C, 0xD3, 0x88, 0x01, 0x3A, 0x38, 0x7F, 0xF0, 0xAB, 0x9E, 0x2F,
	0x74, 0xCE, 0x50, 0xD1, 0xC2, 0x00, 0x57, 0xD3, 0xA7, 0x09, 0x45, 0x36,
	0xFA, 0xC1, 0xC7, 0x1B, 0x65, 0xAD, 0x98, 0x9C, 0x63, 0xED, 0xBA, 0x99,
	0x9B, 0x07, 0x3E, 0x57, 0xBD, 0xB5, 0x52, 0x44, 0x72, 0x09, 0x43, 0xE0,
	0x5C, 0x35, 0xCC, 0xE4, 0xE0, 0x85, 0x6A, 0x61, 0xAA, 0xF5, 0x0D, 0x1E,
	0xE7, 0x8F, 0xB0, 0xB9, 0xE3, 0xC3, 0x83, 0x10, 0x6C, 0x2F, 0x5D, 0xD4,
	0xAB, 0x2D, 0xAB, 0x4D, 0xCE, 0xC9, 0x7F, 0x52, 0x39, 0x13, 0xED, 0x44,
	0x06, 0x23, 0x2F, 0x62, 0xFF, 0xA1, 0x2B, 0xEE, 0x07, 0x98, 0x7D, 0xBC,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

EXPORTNUM(355) UCHAR XePublicKeyData[284];

static INITIALIZE_GLOBAL_CRITICAL_SECTION(XepXbeLoaderLock);

XBE_CERTIFICATE* XeQueryCertificate()
{
	return (XBE_CERTIFICATE*)(GetXbeAddress()->dwCertificateAddr);
}

static void XeSetupPerTitleKeys()
{
	// Generate per-title keys from the XBE Certificate
	UCHAR Digest[XC_HMAC_OUTPUT_LEN] ={};

	// Set the LAN Key
	XcHMAC(XboxCERTKey, XBOX_KEY_LENGTH, XeQueryCertificate()->bzLanKey, XBOX_KEY_LENGTH, NULL, 0, Digest);
	memcpy(XboxLANKey, Digest, sizeof(XboxLANKey));

	// Signature Key
	XcHMAC(XboxCERTKey, XBOX_KEY_LENGTH, XeQueryCertificate()->bzSignatureKey, XBOX_KEY_LENGTH, NULL, 0, Digest);
	memcpy(XboxSignatureKey, Digest, sizeof(XboxSignatureKey));

	// Alternate Signature Keys
	for (int i = 0; i < ALTERNATE_SIGNATURE_COUNT; i++)
	{
		XcHMAC(XboxCERTKey, XBOX_KEY_LENGTH, XeQueryCertificate()->bzTitleAlternateSignatureKey[i], XBOX_KEY_LENGTH, NULL, 0, Digest);
		memcpy(XboxAlternateSignatureKeys[i], Digest, sizeof(XboxAlternateSignatureKeys[i]));
	}

	switch (XboxType)
	{
	case CONSOLE_XBOX:
		memcpy(XePublicKeyData, XePublicKeyDataRetail, sizeof(XePublicKeyData));
		break;
	case CONSOLE_CHIHIRO:
		memcpy(XePublicKeyData, XePublicKeyDataChihiro, sizeof(XePublicKeyData));
		break;
	case CONSOLE_DEVKIT:
		memcpy(XePublicKeyData, XePublicKeyDataDebug, sizeof(XePublicKeyData));
		break;
	default:
		UNREACHABLE;
		break;
	}
}

// Source: partially from Cxbx-Reloaded
static NTSTATUS XeLoadXbe()
{
	if (LaunchDataPage) {
		// TODO: XBE reboot support, by reading the path of the new XBE from the LaunchDataPage
		RIP_API_MSG("XBE reboot not supported");
	}
	else
	{
		// NOTE: we cannot just assume that the XBE name from the DVD drive is called "default.xbe",
		// because the user might have renamed it. Read it in from host instead.
		// TODO: different logic here for physical hardware? Is this the right place to decide what should go in 
		// XeImageFileName or should that be moved up the call stack?
		if (NTSTATUS Status = HalEmuQueryXBEPath(&XeImageFileName); !NT_SUCCESS(Status))
		{
			#if DBG
			// that shouldn't really fail, log if it does
			DbgPrint("Unable to query XBE path from host - %x", Status);
			#endif
			return Status;
		}
	}

	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, &XeImageFileName, OBJ_CASE_INSENSITIVE, nullptr);
	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE XbeHandle;
	if (NTSTATUS Status = NtOpenFile(&XbeHandle, GENERIC_READ, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE); !NT_SUCCESS(Status)) {
		return Status;
	}

	PXBE_HEADER XbeHeader = (PXBE_HEADER)ExAllocatePoolWithTag(PAGE_SIZE, 'hIeX');
	if (!XbeHeader) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	LARGE_INTEGER XbeOffset{ .QuadPart = 0 };
	if (NTSTATUS Status = NtReadFile(XbeHandle, nullptr, nullptr, nullptr, &IoStatusBlock,
		XbeHeader, PAGE_SIZE, &XbeOffset); !NT_SUCCESS(Status)) {
		ExFreePool(XbeHeader);
		NtClose(XbeHandle);
		return Status;
	}

	// Sanity checks: make sure that the file looks like an XBE
	if ((IoStatusBlock.Information < sizeof(XBE_HEADER)) ||
		(XbeHeader->dwMagic != *(PULONG)"XBEH") ||
		(XbeHeader->dwSizeofHeaders > XbeHeader->dwSizeofImage) ||
		(XbeHeader->dwBaseAddr != XBE_BASE_ADDRESS)) {
		ExFreePool(XbeHeader);
		NtClose(XbeHandle);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	PVOID Address = GetXbeAddress();
	ULONG Size = XbeHeader->dwSizeofImage;
	NTSTATUS Status = NtAllocateVirtualMemory(&Address, 0, &Size, MEM_RESERVE, PAGE_READWRITE);
	if (!NT_SUCCESS(Status)) {
		ExFreePool(XbeHeader);
		NtClose(XbeHandle);
		return Status;
	}

	Address = GetXbeAddress();
	Size = XbeHeader->dwSizeofHeaders;
	Status = NtAllocateVirtualMemory(&Address, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
	if (!NT_SUCCESS(Status)) {
		Address = GetXbeAddress();
		Size = 0;
		NtFreeVirtualMemory(&Address, &Size, MEM_RELEASE);
		ExFreePool(XbeHeader);
		NtClose(XbeHandle);
		return Status;
	}

	memcpy(GetXbeAddress(), XbeHeader, PAGE_SIZE);
	ExFreePool(XbeHeader);
	XbeHeader = nullptr;

	if (GetXbeAddress()->dwSizeofHeaders > PAGE_SIZE) {
		LARGE_INTEGER XbeOffset{ .QuadPart = PAGE_SIZE };
		if (NTSTATUS Status = NtReadFile(XbeHandle, nullptr, nullptr, nullptr, &IoStatusBlock,
			(PCHAR)XbeHeader + PAGE_SIZE, GetXbeAddress()->dwSizeofHeaders - PAGE_SIZE, &XbeOffset); !NT_SUCCESS(Status)) {
			Address = GetXbeAddress();
			Size = 0;
			NtFreeVirtualMemory(&Address, &Size, MEM_RELEASE);
			NtClose(XbeHandle);
			return Status;
		}
	}

	// Unscramble XBE entry point and kernel thunk addresses
	static constexpr ULONG SEGABOOT_EP_XOR = 0x40000000;
	if ((GetXbeAddress()->dwEntryAddr & WRITE_COMBINED_BASE) == SEGABOOT_EP_XOR) {
		GetXbeAddress()->dwEntryAddr ^= XOR_EP_CHIHIRO;
		GetXbeAddress()->dwKernelImageThunkAddr ^= XOR_KT_CHIHIRO;
	}
	else if ((GetXbeAddress()->dwKernelImageThunkAddr & PHYSICAL_MAP_BASE) > 0) {
		GetXbeAddress()->dwEntryAddr ^= XOR_EP_DEBUG;
		GetXbeAddress()->dwKernelImageThunkAddr ^= XOR_KT_DEBUG;
	}
	else {
		GetXbeAddress()->dwEntryAddr ^= XOR_EP_RETAIL;
		GetXbeAddress()->dwKernelImageThunkAddr ^= XOR_KT_RETAIL;
	}

	// Disable security checks. TODO: is this necessary?
	XBE_CERTIFICATE *XbeCertificate = (XBE_CERTIFICATE *)(GetXbeAddress()->dwCertificateAddr);
	XbeCertificate->dwAllowedMedia |= (
		XBEIMAGE_MEDIA_TYPE_HARD_DISK |
		XBEIMAGE_MEDIA_TYPE_DVD_X2 |
		XBEIMAGE_MEDIA_TYPE_DVD_CD |
		XBEIMAGE_MEDIA_TYPE_CD |
		XBEIMAGE_MEDIA_TYPE_DVD_5_RO |
		XBEIMAGE_MEDIA_TYPE_DVD_9_RO |
		XBEIMAGE_MEDIA_TYPE_DVD_5_RW |
		XBEIMAGE_MEDIA_TYPE_DVD_9_RW);
	if (XbeCertificate->dwSize >= offsetof(XBE_CERTIFICATE, bzCodeEncKey)) {
		XbeCertificate->dwSecurityFlags &= ~1;
	}

	if (GetXbeAddress()->dwInitFlags.bMountUtilityDrive)
	{
		// this seems to fail because XAPI thinks the drive needs to be formatted - which then fails
		DbgPrint("HACK: disabling utility drive mount");
		GetXbeAddress()->dwInitFlags.bMountUtilityDrive = 0;
	}

	// Load all sections marked as "preload"
	PXBE_SECTION SectionHeaders = (PXBE_SECTION)GetXbeAddress()->dwSectionHeadersAddr;
	for (unsigned i = 0; i < GetXbeAddress()->dwSections; ++i) {
		if (SectionHeaders[i].Flags & XBEIMAGE_SECTION_PRELOAD) {
			Status = XeLoadSection(&SectionHeaders[i]);
			if (!NT_SUCCESS(Status)) {
				Address = GetXbeAddress();
				Size = 0;
				NtFreeVirtualMemory(&Address, &Size, MEM_RELEASE);
				NtClose(XbeHandle);
				return Status;
			}
		}
	}

	Status = STATUS_SUCCESS;

	// Map the kernel thunk table to the XBE kernel imports
	PULONG XbeKrnlThunk = (PULONG)GetXbeAddress()->dwKernelImageThunkAddr;
	unsigned i = 0;
	while (XbeKrnlThunk[i]) {
		ULONG Ordinal = XbeKrnlThunk[i] & 0x7FFFFFFF;

		NT_ASSERT(Ordinal != 0);

		PVOID FunctionAddress = NULL;
		Status = RtlGetProcedureAddress(KernelBase, NULL, Ordinal, &FunctionAddress);

		if (!NT_SUCCESS(Status))
			break;

		XbeKrnlThunk[i] = (ULONG)FunctionAddress;
		++i;
	}

	if ((XboxType == CONSOLE_DEVKIT) && !(GetXbeAddress()->dwInitFlags.bLimit64MB)) {
		MiAllowNonDebuggerOnTop64MiB = TRUE;
	}

	if (XboxType == CONSOLE_CHIHIRO) {
		GetXbeAddress()->dwInitFlags.bDontSetupHarddisk = 1;
	}

	XeSetupPerTitleKeys();

	NtClose(XbeHandle);

	return Status;
}

VOID XBOXAPI XbeStartupThread(PVOID Opaque)
{
	NTSTATUS Status = XeLoadXbe();
	if (!NT_SUCCESS(Status)) {
		// TODO: this should probably either reboot to the dashboard with an error setting, or display the fatal error message to the screen
		KeBugCheck(XBE_LAUNCH_FAILED);
	}

	// Call the entry point of the XBE!
	using PXBE_ENTRY_POINT = VOID(__cdecl *)();
	PXBE_ENTRY_POINT XbeEntryPoint = (PXBE_ENTRY_POINT)(GetXbeAddress()->dwEntryAddr);
	XbeEntryPoint();
}

// Source: Cxbx-Reloaded
EXPORTNUM(327) NTSTATUS XBOXAPI XeLoadSection
(
	PXBE_SECTION Section
)
{
	RtlEnterCriticalSectionAndRegion(&XepXbeLoaderLock);

	// If the reference count was zero, load the section
	if (Section->SectionReferenceCount == 0) {
		// REMARK: Some titles have sections less than PAGE_SIZE, which will cause an overlap with the next section
		// since both will have the same aligned starting address.
		// Test case: Dead or Alive 3, section XGRPH has a size of 764 bytes
		// XGRPH										DSOUND
		// 1F18A0 + 2FC -> aligned_start = 1F1000		1F1BA0 -> aligned_start = 1F1000 <- collision

		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, &XeImageFileName, OBJ_CASE_INSENSITIVE, nullptr);
		IO_STATUS_BLOCK IoStatusBlock;
		HANDLE XbeHandle;
		if (NTSTATUS Status = NtOpenFile(&XbeHandle, GENERIC_READ, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE); !NT_SUCCESS(Status)) {
			RtlLeaveCriticalSectionAndRegion(&XepXbeLoaderLock);
			return Status;
		}

		PVOID BaseAddress = Section->VirtualAddress;
		ULONG SectionSize = Section->VirtualSize;
		NTSTATUS Status = NtAllocateVirtualMemory(&BaseAddress, 0, &SectionSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!NT_SUCCESS(Status)) {
			NtClose(XbeHandle);
			RtlLeaveCriticalSectionAndRegion(&XepXbeLoaderLock);
			return Status;
		}

		// Clear the memory the section requires
		memset(Section->VirtualAddress, 0, Section->VirtualSize);

		// Copy the section data
		LARGE_INTEGER XbeOffset{ .QuadPart = Section->FileAddress };
		if (NTSTATUS Status = NtReadFile(XbeHandle, nullptr, nullptr, nullptr, &IoStatusBlock,
			Section->VirtualAddress, Section->FileSize, &XbeOffset); !NT_SUCCESS(Status) || (IoStatusBlock.Information != Section->FileSize)) {
			if (IoStatusBlock.Information != Section->FileSize) {
				Status = STATUS_FILE_CORRUPT_ERROR;
			}
			BaseAddress = Section->VirtualAddress;
			SectionSize = Section->VirtualSize;
			NtFreeVirtualMemory(&BaseAddress, &SectionSize, MEM_DECOMMIT);
			NtClose(XbeHandle);
			RtlLeaveCriticalSectionAndRegion(&XepXbeLoaderLock);
			return Status;
		}

		NtClose(XbeHandle);

		// Increment the head/tail page reference counters
		(*Section->HeadReferenceCount)++;
		(*Section->TailReferenceCount)++;
	}

	// Increment the reference count
	Section->SectionReferenceCount++;

	RtlLeaveCriticalSectionAndRegion(&XepXbeLoaderLock);

	return STATUS_SUCCESS;
}

// Source: Cxbx-Reloaded
EXPORTNUM(328) NTSTATUS XBOXAPI XeUnloadSection
(
	PXBE_SECTION Section
)
{
	NTSTATUS Status = STATUS_INVALID_PARAMETER;
	RtlEnterCriticalSectionAndRegion(&XepXbeLoaderLock);

	// If the section was loaded, process it
	if (Section->SectionReferenceCount > 0) {
		// Decrement the reference count
		Section->SectionReferenceCount -= 1;

		// Free the section and the physical memory in use if necessary
		if (Section->SectionReferenceCount == 0) {
			// REMARK: the following can be tested with Broken Sword - The Sleeping Dragon, RalliSport Challenge, ...
			// Test-case: Apex/Racing Evoluzione requires the memory NOT to be zeroed

			ULONG BaseAddress = (ULONG)Section->VirtualAddress;
			ULONG EndingAddress = (ULONG)Section->VirtualAddress + Section->VirtualSize;

			// Decrement the head/tail page reference counters
			(*Section->HeadReferenceCount)--;
			(*Section->TailReferenceCount)--;

			if ((*Section->TailReferenceCount) != 0) {
				EndingAddress = ROUND_DOWN_4K(EndingAddress);
			}

			if ((*Section->HeadReferenceCount) != 0) {
				BaseAddress = ROUND_UP_4K(BaseAddress);
			}

			if (EndingAddress > BaseAddress) {
				PVOID BaseAddress2 = (PVOID)BaseAddress;
				ULONG RegionSize = EndingAddress - BaseAddress;
				NtFreeVirtualMemory(&BaseAddress2, &RegionSize, MEM_DECOMMIT);
			}
		}

		Status = STATUS_SUCCESS;
	}

	RtlLeaveCriticalSectionAndRegion(&XepXbeLoaderLock);

	return Status;
}
