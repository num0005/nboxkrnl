/*
 * PatrickvL              Copyright (c) 2018
 * x1nixmzeng             Copyright (c) 2018
 */

#include "rtl.hpp"
#include <string.h>
#include "ex.hpp"

#define TAG_STRING 'gtrS'
#define NlsOemDefaultChar '¬'

// Source: Cxbx-Reloaded
EXPORTNUM(279) BOOLEAN XBOXAPI RtlEqualString
(
	PSTRING String1,
	PSTRING String2,
	BOOLEAN CaseInSensitive
)
{
	BOOLEAN bRet = TRUE;

	USHORT l1 = String1->Length;
	USHORT l2 = String2->Length;
	if (l1 != l2) {
		return FALSE;
	}

	CHAR *p1 = String1->Buffer;
	CHAR *p2 = String2->Buffer;
	CHAR *last = p1 + l1;

	if (CaseInSensitive) {
		while (p1 < last) {
			CHAR c1 = *p1++;
			CHAR c2 = *p2++;
			if (c1 != c2) {
				c1 = RtlUpperChar(c1);
				c2 = RtlUpperChar(c2);
				if (c1 != c2) {
					return FALSE;
				}
			}
		}

		return TRUE;
	}

	while (p1 < last) {
		if (*p1++ != *p2++) {
			bRet = FALSE;
			break;
		}
	}

	return bRet;
}

// Source: Cxbx-Reloaded
EXPORTNUM(289) VOID XBOXAPI RtlInitAnsiString
(
	PANSI_STRING DestinationString,
	PCSZ SourceString
)
{
	DestinationString->Buffer = const_cast<PCHAR>(SourceString);
	if (SourceString) {
		DestinationString->Length = (USHORT)strlen(DestinationString->Buffer);
		DestinationString->MaximumLength = DestinationString->Length + 1;
	}
	else {
		DestinationString->Length = DestinationString->MaximumLength = 0;
	}
}

DWORD RtlAnsiStringToUnicodeSize(const STRING* str)
{
	// assumes no multibyte encoding, TODO: verify this is a correct upper bound with multibyte encoding
	return (str->Length + 1) * sizeof(WCHAR);
}

// ******************************************************************
// * 0x012B - RtlMultiByteToUnicodeN()
// ******************************************************************
EXPORTNUM(299) NTSTATUS NTAPI RtlMultiByteToUnicodeN
(
	IN     PWSTR  UnicodeString,
	IN     ULONG  MaxBytesInUnicodeString,
	IN     PULONG BytesInUnicodeString,
	IN     PCHAR  MultiByteString,
	IN     ULONG  BytesInMultiByteString
)
{
	ULONG maxUnicodeChars = MaxBytesInUnicodeString / sizeof(WCHAR);
	ULONG numChars = (maxUnicodeChars < BytesInMultiByteString) ? maxUnicodeChars : BytesInMultiByteString;

	if (BytesInUnicodeString != NULL)
	{
		*BytesInUnicodeString = numChars * sizeof(WCHAR);
	}

	while (numChars)
	{
		*UnicodeString = (WCHAR)(*MultiByteString);

		UnicodeString++;
		MultiByteString++;
		numChars--;
	}

	return STATUS_SUCCESS;
}

// source: cxbx-reloaded/reactos
EXPORTNUM(260) NTSTATUS NTAPI RtlAnsiStringToUnicodeString
(
	PUNICODE_STRING DestinationString,
	PSTRING         SourceString,
	UCHAR           AllocateDestinationString
)
{
	DWORD total = RtlAnsiStringToUnicodeSize(SourceString);

	if (total > 0xffff)
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	DestinationString->Length = (USHORT)(total - sizeof(WCHAR));
	if (AllocateDestinationString)
	{
		DestinationString->MaximumLength = (USHORT)total;
		if (!(DestinationString->Buffer = (wchar_t*)ExAllocatePoolWithTag(total, TAG_STRING)))
		{
			return STATUS_NO_MEMORY;
		}
	}
	else
	{
		if (total > DestinationString->MaximumLength)
		{
			return STATUS_BUFFER_OVERFLOW;
		}
	}

	NT_ASSERT(!(DestinationString->MaximumLength & 1) && DestinationString->Length <= DestinationString->MaximumLength);

	RtlMultiByteToUnicodeN((PWSTR)DestinationString->Buffer,
		(ULONG)DestinationString->Length,
		NULL,
		SourceString->Buffer,
		SourceString->Length);

	DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;

	return STATUS_SUCCESS;
}

// ******************************************************************
// * 0x0105 - RtlAppendStringToString()
// ******************************************************************
EXPORTNUM(261) NTSTATUS NTAPI RtlAppendStringToString
(
	IN PSTRING Destination,
	IN PSTRING Source
)
{

	NTSTATUS result = STATUS_SUCCESS;

	USHORT dstLen = Destination->Length;
	USHORT srcLen = Source->Length;
	if (srcLen > 0)
	{
		if ((srcLen + dstLen) > Destination->MaximumLength)
		{
			result = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			CHAR* dstBuf = Destination->Buffer + Destination->Length;
			CHAR* srcBuf = Source->Buffer;
			memmove(dstBuf, srcBuf, srcLen);
			Destination->Length += srcLen;
		}
	}

	return result;
}

// ******************************************************************
// * 0x0106 - RtlAppendUnicodeStringToString()
// ******************************************************************
EXPORTNUM(262) NTSTATUS NTAPI RtlAppendUnicodeStringToString
(
	IN PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source
)
{

	NTSTATUS result = STATUS_SUCCESS;

	USHORT dstLen = Destination->Length;
	USHORT srcLen = Source->Length;
	if (srcLen > 0)
	{
		if ((srcLen + dstLen) > Destination->MaximumLength)
		{
			result = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			WCHAR* dstBuf = (WCHAR*)(Destination->Buffer + (Destination->Length / sizeof(WCHAR)));
			memmove(dstBuf, Source->Buffer, srcLen);
			Destination->Length += srcLen;
			if (Destination->Length < Destination->MaximumLength)
			{
				dstBuf[srcLen / sizeof(WCHAR)] = L'\0';
			}
		}
	}

	return result;
}

// ******************************************************************
// * 0x0107 - RtlAppendUnicodeToString()
// ******************************************************************
EXPORTNUM(263) NTSTATUS NTAPI RtlAppendUnicodeToString
(
	IN OUT PUNICODE_STRING Destination,
	IN PCWSTR Source
)
{

	NTSTATUS result = STATUS_SUCCESS;
	if (Source != NULL)
	{
		UNICODE_STRING unicodeString;
		RtlInitUnicodeString(&unicodeString, Source);

		result = RtlAppendUnicodeStringToString(Destination, &unicodeString);
	}

	return result;
}

// source: reactos
EXPORTNUM(267) NTSTATUS NTAPI RtlCharToInteger
(
	IN     PCSZ    String,
	IN     ULONG   Base OPTIONAL,
	OUT    PULONG  Value
)
{
	char bMinus = 0;

	// skip leading whitespaces
	while (*String != '\0' && *String <= ' ')
	{
		String++;
	}

	// Check for +/-
	if (*String == '+')
	{
		String++;
	}
	else if (*String == '-')
	{
		bMinus = 1;
		String++;
	}

	// base = 0 means autobase
	if (Base == 0)
	{
		Base = 10;
		if (String[0] == '0')
		{
			if (String[1] == 'b')
			{
				String += 2;
				Base = 2;
			}
			else if (String[1] == 'o')
			{
				String += 2;
				Base = 8;
			}
			else if (String[1] == 'x')
			{
				String += 2;
				Base = 16;
			}
		}
	}
	else if (Base != 2 && Base != 8 && Base != 10 && Base != 16)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (Value == NULL)
	{
		return STATUS_ACCESS_VIOLATION;
	}

	ULONG RunningTotal = 0;
	while (*String != '\0')
	{
		CHAR chCurrent = *String;
		int digit;

		if (chCurrent >= '0' && chCurrent <= '9')
		{
			digit = chCurrent - '0';
		}
		else if (chCurrent >= 'A' && chCurrent <= 'Z')
		{
			digit = chCurrent - 'A' + 10;
		}
		else if (chCurrent >= 'a' && chCurrent <= 'z')
		{
			digit = chCurrent - 'a' + 10;
		}
		else
		{
			digit = -1;
		}

		if (digit < 0 || digit >= (int)Base)
			break;

		RunningTotal = RunningTotal * Base + digit;
		String++;
	}

	*Value = bMinus ? (0 - RunningTotal) : RunningTotal;
	return STATUS_SUCCESS;
}

// Source: Cxbx-Reloaded
EXPORTNUM(272) void XBOXAPI RtlCopyString
(
	OUT PSTRING DestinationString,
	IN PSTRING SourceString OPTIONAL
)
{

	if (SourceString == NULL)
	{
		DestinationString->Length = 0;
		return;
	}

	CHAR* pd = DestinationString->Buffer;
	CHAR* ps = SourceString->Buffer;
	USHORT len = SourceString->Length;
	if ((USHORT)len > DestinationString->MaximumLength)
	{
		len = DestinationString->MaximumLength;
	}

	DestinationString->Length = (USHORT)len;
	memcpy(pd, ps, len);
}

EXPORTNUM(273) void NTAPI RtlCopyUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString OPTIONAL
)
{

	if (SourceString == NULL)
	{
		DestinationString->Length = 0;
		return;
	}

	CHAR* pd = (CHAR*)(DestinationString->Buffer);
	CHAR* ps = (CHAR*)(SourceString->Buffer);
	USHORT len = SourceString->Length;
	if ((USHORT)len > DestinationString->MaximumLength)
	{
		len = DestinationString->MaximumLength;
	}

	DestinationString->Length = (USHORT)len;
	memcpy(pd, ps, len);
}

// ******************************************************************
// * 0x0112 - RtlCreateUnicodeString()
// ******************************************************************
EXPORTNUM(274) BOOLEAN NTAPI RtlCreateUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PCWSTR SourceString
)
{
	BOOLEAN result = TRUE;

	SIZE_T string_length = wcslen(SourceString);

	ULONG bufferSize = (string_length + 1) * sizeof(WCHAR);
	DestinationString->Buffer = (WCHAR*)ExAllocatePoolWithTag(bufferSize, TAG_STRING);
	if (!DestinationString->Buffer)
	{
		result = FALSE;
	}
	else
	{
		RtlMoveMemory(DestinationString->Buffer, SourceString, bufferSize);
		DestinationString->MaximumLength = (USHORT)bufferSize;
		DestinationString->Length = string_length * sizeof(WCHAR);
	}

	return result;
}

// ******************************************************************
// * 0x0114 - RtlDowncaseUnicodeString()
// ******************************************************************
 EXPORTNUM(276) NTSTATUS NTAPI RtlDowncaseUnicodeString
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString,
	IN BOOLEAN AllocateDestinationString
)
{
	if (AllocateDestinationString)
	{
		DestinationString->MaximumLength = SourceString->Length;
		DestinationString->Buffer = (WCHAR*)ExAllocatePoolWithTag((ULONG)DestinationString->MaximumLength, TAG_STRING);
		if (DestinationString->Buffer == NULL)
		{
			return STATUS_NO_MEMORY;
		}
	}
	else
	{
		if (SourceString->Length > DestinationString->MaximumLength)
		{
			return STATUS_BUFFER_OVERFLOW;
		}
	}

	ULONG length = ((ULONG)SourceString->Length) / sizeof(WCHAR);
	WCHAR* pDst = (WCHAR*)(DestinationString->Buffer);
	WCHAR* pSrc = (WCHAR*)(SourceString->Buffer);
	for (ULONG i = 0; i < length; i++)
	{
		pDst[i] = (WCHAR)RtlDowncaseUnicodeChar(pSrc[i]);
	}

	DestinationString->Length = SourceString->Length;

	return STATUS_SUCCESS;
}

// ******************************************************************
// * 0x0122 - RtlInitUnicodeString()
// ******************************************************************
EXPORTNUM(290) void NTAPI RtlInitUnicodeString
(
	IN OUT PUNICODE_STRING DestinationString,
	IN     PCWSTR         SourceString
)
{

	DestinationString->Buffer = (PWCHAR)SourceString;
	if (SourceString != NULL)
	{
		DestinationString->Length = (USHORT)wcslen(SourceString) * 2;
		DestinationString->MaximumLength = DestinationString->Length + 2;
	}
	else
	{
		DestinationString->Length = DestinationString->MaximumLength = 0;
	}
}

/*
 * @implemented
 *
 * NOTES
 *  Writes at most length characters to the string str.
 *  Str is nullterminated when length allowes it.
 *  When str fits exactly in length characters the nullterm is ommitted.
 */
EXPORTNUM(292) NTSTATUS NTAPI RtlIntegerToChar(
	ULONG value,   /* [I] Value to be converted */
	ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
	ULONG length,  /* [I] Length of the str buffer in bytes */
	PCHAR str)     /* [O] Destination for the converted value */
{
	CHAR buffer[33];
	PCHAR pos;
	CHAR digit;
	SIZE_T len;

	if (base == 0)
	{
		base = 10;
	}
	else if (base != 2 && base != 8 && base != 10 && base != 16)
	{
		return STATUS_INVALID_PARAMETER;
	}

	pos = &buffer[32];
	*pos = '\0';

	do
	{
		pos--;
		digit = (CHAR)(value % base);
		value = value / base;

		if (digit < 10)
		{
			*pos = '0' + digit;
		}
		else
		{
			*pos = 'A' + digit - 10;
		}
	} while (value != 0L);

	len = &buffer[32] - pos;

	if (len > length)
	{
		return STATUS_BUFFER_OVERFLOW;
	}
	else if (str == NULL)
	{
		return STATUS_ACCESS_VIOLATION;
	}
	else if (len == length)
	{
		RtlCopyMemory(str, pos, len);
	}
	else
	{
		RtlCopyMemory(str, pos, len + 1);
	}

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlIntegerToUnicodeString(
	IN ULONG Value,
	IN ULONG Base OPTIONAL,
	IN OUT PUNICODE_STRING String
)
{
	ANSI_STRING AnsiString;
	CHAR Buffer[33];
	NTSTATUS Status;

	Status = RtlIntegerToChar(Value, Base, sizeof(Buffer), Buffer);
	if (NT_SUCCESS(Status))
	{
		AnsiString.Buffer = Buffer;
		AnsiString.Length = (USHORT)strlen(Buffer);
		AnsiString.MaximumLength = sizeof(Buffer);

		Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
	}

	return Status;
}

/*
 * @implemented
 */
EXPORTNUM(286) VOID NTAPI RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
	if (AnsiString->Buffer)
	{
		ExFreePool(AnsiString->Buffer);
		RtlZeroMemory(AnsiString, sizeof(*AnsiString));
	}
}

EXPORTNUM(287) VOID NTAPI RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
	if (UnicodeString->Buffer)
	{
		ExFreePool(UnicodeString->Buffer);
		RtlZeroMemory(UnicodeString, sizeof(*UnicodeString));
	}
}

EXPORTNUM(300) NTSTATUS NTAPI RtlMultiByteToUnicodeSize
(
	_Out_ PULONG     UnicodeSize,
	_In_ const CHAR* MbString,
	_In_ ULONG       MbSize
)
{
	/* single-byte code page */
	*UnicodeSize = MbSize * sizeof(WCHAR);

	return STATUS_SUCCESS;
}

EXPORTNUM(308) NTSTATUS NTAPI RtlUnicodeStringToAnsiString(
	IN OUT PANSI_STRING AnsiDest,
	IN const PUNICODE_STRING UniSource,
	IN BOOLEAN AllocateDestinationString)
{
	NTSTATUS Status = STATUS_SUCCESS;
	NTSTATUS RealStatus;
	ULONG Length = (UniSource->Length + sizeof(WCHAR)) / sizeof(WCHAR);
	ULONG Index;

	if (Length > 0xFFFF) return STATUS_INVALID_PARAMETER_2;

	AnsiDest->Length = (USHORT)Length - sizeof(CHAR);

	if (AllocateDestinationString)
	{
		AnsiDest->Buffer = (PCHAR)ExAllocatePoolWithTag(Length, TAG_STRING);
		AnsiDest->MaximumLength = (USHORT)Length;

		if (!AnsiDest->Buffer) return STATUS_NO_MEMORY;
	}
	else if (AnsiDest->Length >= AnsiDest->MaximumLength)
	{
		if (!AnsiDest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

		Status = STATUS_BUFFER_OVERFLOW;
		AnsiDest->Length = AnsiDest->MaximumLength - 1;
	}

	RealStatus = RtlUnicodeToMultiByteN(AnsiDest->Buffer,
		AnsiDest->Length,
		&Index,
		UniSource->Buffer,
		UniSource->Length);

	if (!NT_SUCCESS(RealStatus) && AllocateDestinationString)
	{
		ExFreePoolWithTag(AnsiDest->Buffer, TAG_STRING);
		return RealStatus;
	}

	AnsiDest->Buffer[Index] = '\0';
	return Status;
}

/*
 * @implemented
 */
EXPORTNUM(309) NTSTATUS NTAPI RtlUnicodeStringToInteger(
	const UNICODE_STRING* str, /* [I] Unicode string to be converted */
	ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
	ULONG* value               /* [O] Destination for the converted value */
)
{
	const WCHAR *lpwstr = str->Buffer;
	USHORT CharsRemaining = str->Length / sizeof(WCHAR);
	WCHAR wchCurrent;
	int digit;
	ULONG RunningTotal = 0;
	char bMinus = 0;

	while (CharsRemaining >= 1 && *lpwstr <= ' ')
	{
		lpwstr++;
		CharsRemaining--;
	}

	if (CharsRemaining >= 1)
	{
		if (*lpwstr == '+')
		{
			lpwstr++;
			CharsRemaining--;
		}
		else if (*lpwstr == '-')
		{
			bMinus = 1;
			lpwstr++;
			CharsRemaining--;
		}
	}

	if (base == 0)
	{
		base = 10;

		if (CharsRemaining >= 2 && lpwstr[0] == '0')
		{
			if (lpwstr[1] == 'b')
			{
				lpwstr += 2;
				CharsRemaining -= 2;
				base = 2;
			}
			else if (lpwstr[1] == 'o')
			{
				lpwstr += 2;
				CharsRemaining -= 2;
				base = 8;
			}
			else if (lpwstr[1] == 'x')
			{
				lpwstr += 2;
				CharsRemaining -= 2;
				base = 16;
			}
		}
	}
	else if (base != 2 && base != 8 && base != 10 && base != 16)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (value == NULL)
	{
		return STATUS_ACCESS_VIOLATION;
	}

	while (CharsRemaining >= 1)
	{
		wchCurrent = *lpwstr;

		if (wchCurrent >= '0' && wchCurrent <= '9')
		{
			digit = wchCurrent - '0';
		}
		else if (wchCurrent >= 'A' && wchCurrent <= 'Z')
		{
			digit = wchCurrent - 'A' + 10;
		}
		else if (wchCurrent >= 'a' && wchCurrent <= 'z')
		{
			digit = wchCurrent - 'a' + 10;
		}
		else
		{
			digit = -1;
		}

		if (digit < 0 || (ULONG)digit >= base) break;

		RunningTotal = RunningTotal * base + digit;
		lpwstr++;
		CharsRemaining--;
	}

	*value = bMinus ? (0 - RunningTotal) : RunningTotal;
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXPORTNUM(310) NTSTATUS NTAPI RtlUnicodeToMultiByteN
(
	_Out_ PCHAR MbString,
	_In_ ULONG MbSize,
	_Out_opt_ PULONG ResultSize,
	_In_ const WCHAR* UnicodeString,
	_In_ ULONG  UnicodeSize
)
{
	ULONG Size = 0;
	ULONG i;

	/* single-byte code page */
	Size = (UnicodeSize > (MbSize * sizeof(WCHAR)))
		? MbSize : (UnicodeSize / sizeof(WCHAR));

	if (ResultSize)
		*ResultSize = Size;

	for (i = 0; i < Size; i++)
	{
		/* Check for characters that cannot be trivially demoted to ANSI */
		if (*((PCHAR)UnicodeString + 1) == 0)
		{
			*MbString++ = (CHAR)*UnicodeString++;
		}
		else
		{
			/* Invalid character, use default */
			*MbString++ = NlsOemDefaultChar;
			UnicodeString++;
		}
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXPORTNUM(311) NTSTATUS NTAPI RtlUnicodeToMultiByteSize
(
	_Out_ PULONG MbSize,
	_In_ const WCHAR* UnicodeString,
	_In_ ULONG UnicodeSize
)
{
	ULONG UnicodeLength = UnicodeSize / sizeof(WCHAR);

	/* single-byte code page */
	*MbSize = UnicodeLength;

	return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
EXPORTNUM(314) NTSTATUS NTAPI RtlUpcaseUnicodeString
(
	IN OUT PUNICODE_STRING   UniDest,
	IN const PUNICODE_STRING UniSource,
	IN BOOLEAN  AllocateDestinationString
)
{
	ULONG i, j;

	if (AllocateDestinationString)
	{
		UniDest->MaximumLength = UniSource->Length;
		UniDest->Buffer = (PWCHAR)ExAllocatePoolWithTag(UniDest->MaximumLength, TAG_STRING);
		if (UniDest->Buffer == NULL) return STATUS_NO_MEMORY;
	}
	else if (UniSource->Length > UniDest->MaximumLength)
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	j = UniSource->Length / sizeof(WCHAR);

	for (i = 0; i < j; i++)
	{
		UniDest->Buffer[i] = RtlUpcaseUnicodeChar(UniSource->Buffer[i]);
	}

	UniDest->Length = UniSource->Length;
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EXPORTNUM(315) NTSTATUS NTAPI RtlUpcaseUnicodeToMultiByteN
(
	_Out_ PCHAR MbString,
	_In_ ULONG MbSize,
	_Out_opt_ PULONG  ResultSize,
	_In_ const WCHAR *UnicodeString,
	_In_ ULONG UnicodeSize
)
{
	WCHAR UpcaseChar;
	ULONG Size = 0;
	ULONG i;

	/* single-byte code page */
	if (UnicodeSize > (MbSize * sizeof(WCHAR)))
		Size = MbSize;
	else
		Size = UnicodeSize / sizeof(WCHAR);

	if (ResultSize)
		*ResultSize = Size;

	for (i = 0; i < Size; i++)
	{
		UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);

		/* Check for characters that cannot be trivially demoted to ANSI */
		if (*((PCHAR)&UpcaseChar + 1) == 0)
		{
			*MbString = (CHAR)UpcaseChar;
		}
		else
		{
			/* Invalid character, use default */
			*MbString = NlsOemDefaultChar;
		}

		MbString++;
		UnicodeString++;
	}

	return STATUS_SUCCESS;
}

EXPORTNUM (317) VOID NTAPI RtlUpperString
(
	PSTRING DestinationString,
	const STRING* SourceString
)
{
	USHORT Length = SourceString->Length;
	if (Length > DestinationString->MaximumLength)
		Length = DestinationString->MaximumLength;

	PCHAR Src = SourceString->Buffer;
	PCHAR Dest = DestinationString->Buffer;
	DestinationString->Length = Length;

	while (Length)
	{
		*Dest++ = RtlUpperChar(*Src++);
		Length--;
	}
}
