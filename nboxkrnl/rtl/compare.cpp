#include "rtl.hpp"

// ******************************************************************
// * 0x010C - RtlCompareMemory()
// ******************************************************************
// * compare block of memory, return number of equivalent bytes.
// ******************************************************************
EXPORTNUM(268) SIZE_T NTAPI RtlCompareMemory
(
	IN const void* Source1,
	IN const void* Source2,
	IN SIZE_T      Length
)
{
	SIZE_T result = Length;

	uint8_t* pBytes1 = (uint8_t*)Source1;
	uint8_t* pBytes2 = (uint8_t*)Source2;
	for (uint32_t i = 0; i < Length; i++)
	{
		if (pBytes1[i] != pBytes2[i])
		{
			result = i;
			break;
		}
	}

	return result;
}

// ******************************************************************
// * 0x010D - RtlCompareMemoryUlong()
// ******************************************************************
EXPORTNUM(269) SIZE_T NTAPI RtlCompareMemoryUlong
(
	IN PVOID Source,
	IN SIZE_T Length,
	IN ULONG Pattern
)
{
	PULONG ptr = (PULONG)Source;
	ULONG_PTR len = Length / sizeof(ULONG);
	ULONG_PTR i;

	for (i = 0; i < len; i++)
	{
		if (*ptr != Pattern)
			break;

		ptr++;
	}

	SIZE_T result = (SIZE_T)((PCHAR)ptr - (PCHAR)Source);

	return result;
}

// ******************************************************************
// * 0x010E - RtlCompareString()
// ******************************************************************
EXPORTNUM(270) LONG NTAPI RtlCompareString
(
	IN PSTRING String1,
	IN PSTRING String2,
	IN BOOLEAN CaseInSensitive
)
{
	const USHORT l1 = String1->Length;
	const USHORT l2 = String2->Length;
	const USHORT maxLen = (l1 <= l2 ? l1 : l2);

	const PCHAR str1 = String1->Buffer;
	const PCHAR str2 = String2->Buffer;

	if (CaseInSensitive)
	{
		for (unsigned i = 0; i < maxLen; i++)
		{
			UCHAR char1 = RtlLowerChar(str1[i]);
			UCHAR char2 = RtlLowerChar(str2[i]);
			if (char1 != char2)
			{
				return char1 - char2;
			}
		}
	}
	else
	{
		for (unsigned i = 0; i < maxLen; i++)
		{
			if (str1[i] != str2[i])
			{
				return str1[i] - str2[i];
			}
		}
	}

	return l1 - l2;
}

// ******************************************************************
// * 0x010F - RtlCompareUnicodeString()
// ******************************************************************
EXPORTNUM(271) LONG NTAPI RtlCompareUnicodeString
(
	IN PUNICODE_STRING String1,
	IN PUNICODE_STRING String2,
	IN BOOLEAN CaseInSensitive
)
{
	const USHORT l1 = String1->Length;
	const USHORT l2 = String2->Length;
	const USHORT maxLen = (l1 <= l2 ? l1 : l2) / sizeof(WCHAR);

	const WCHAR* str1 = String1->Buffer;
	const WCHAR* str2 = String2->Buffer;

	if (CaseInSensitive)
	{
		for (unsigned i = 0; i < maxLen; i++)
		{
			WCHAR char1 = RtlUpcaseUnicodeChar(str1[i]);
			WCHAR char2 = RtlUpcaseUnicodeChar(str2[i]);

			if (char1 != char2)
			{
				return char1 - char2;
			}
		}
	}
	else
	{
		for (unsigned i = 0; i < maxLen; i++)
		{
			if (str1[i] != str2[i])
			{
				return str1[i] - str2[i];
			}
		}
	}

	return l1 - l2;
}


// ******************************************************************
// * 0x0118 - RtlEqualUnicodeString()
// ******************************************************************
EXPORTNUM(280) BOOLEAN NTAPI RtlEqualUnicodeString
(
	IN PUNICODE_STRING String1,
	IN PUNICODE_STRING String2,
	IN BOOLEAN CaseInSensitive
)
{
	BOOLEAN bRet = FALSE;
	USHORT l1 = String1->Length;
	USHORT l2 = String2->Length;

	if (l1 == l2)
	{
		WCHAR* p1 = (WCHAR*)(String1->Buffer);
		WCHAR* p2 = (WCHAR*)(String2->Buffer);
		WCHAR* last = p1 + (l1 / sizeof(WCHAR));

		bRet = TRUE;

		if (CaseInSensitive)
		{
			while (p1 < last)
			{
				WCHAR c1 = *p1++;
				WCHAR c2 = *p2++;
				if (c1 != c2)
				{
					c1 = RtlUpcaseUnicodeChar(c1);
					c2 = RtlUpcaseUnicodeChar(c2);
					if (c1 != c2)
					{
						bRet = FALSE;
						break;
					}
				}
			}
		}
		else
		{
			while (p1 < last)
			{
				if (*p1++ != *p2++)
				{
					bRet = FALSE;
					break;
				}
			}
		}
	}

	return bRet;
}
