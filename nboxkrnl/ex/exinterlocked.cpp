//SPDX-License-Identifier: GPL-2.0-or-later

#include "ex.hpp"
#include <intrin.h>

__int64 InterlockedCompareExchange64(_Interlocked_operand_ __int64 volatile* _Destination, __int64 _Exchange, __int64 _Comparand)
{
	return _InterlockedCompareExchange64(_Destination, _Exchange, _Comparand);
}

ULONGLONG InterlockedCompareExchange64(_Interlocked_operand_ ULONGLONG volatile* _Destination, ULONGLONG _Exchange, ULONGLONG _Comparand)
{
	return _InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(_Destination), _Exchange, _Comparand);
}


extern "C" {
	// source: reactos
	EXPORTNUM(4) PSLIST_ENTRY FASTCALL InterlockedFlushSList
	(
		_Inout_ PSLIST_HEADER SListHead
	)
	{
		SLIST_HEADER OldHeader, NewHeader;
		ULONGLONG Compare;

		/* Read the header */
		OldHeader = *SListHead;

		do
		{
			/* Check for empty list */
			if (OldHeader.Next.Next == NULL)
			{
				return NULL;
			}

			/* Create a new header (keep the sequence number) */
			NewHeader = OldHeader;
			NewHeader.Next.Next = NULL;
			NewHeader.Depth = 0;

			/* Try to exchange atomically */
			Compare = OldHeader.Alignment;
			OldHeader.Alignment = InterlockedCompareExchange64(&SListHead->Alignment,
				NewHeader.Alignment,
				Compare);
		} while (OldHeader.Alignment != Compare);

		/* Return the old first entry */
		return OldHeader.Next.Next;
	}

	EXPORTNUM(19) LARGE_INTEGER NTAPI ExInterlockedAddLargeInteger
	(
		IN OUT PLARGE_INTEGER Addend,
		IN LARGE_INTEGER Increment
	)
	{
		ULONG Flags = __readeflags();
		_disable();

		LARGE_INTEGER OldValue = *Addend;

		Addend->QuadPart = OldValue.QuadPart + Increment.QuadPart;

		__writeeflags(Flags);

		return OldValue;
	}

	// per https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/intrlfst/addlargestatistic.htm this is not a fully thread safe API
	// 
	EXPORTNUM(20) VOID FASTCALL ExInterlockedAddLargeStatistic
	(
		IN OUT PLARGE_INTEGER Addend,
		IN ULONG Increment
	)
	{
		_InterlockedAddLargeStatistic(&Addend->QuadPart, Increment);
	}

	EXPORTNUM(32) PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList
	(
		IN OUT PLIST_ENTRY ListHead,
		IN PLIST_ENTRY ListEntry
	)
	{
		PLIST_ENTRY FirstEntry;

		/* Disable interrupts */
		ULONG Flags = __readeflags();
		_disable();

		/* Save the first entry */
		FirstEntry = ListHead->Flink;

		/* Insert the new entry */
		InsertHeadList(ListHead, ListEntry);

		/* restore interrupts */
		__writeeflags(Flags);

		/* Return the old first entry or NULL for empty list */
		return (FirstEntry == ListHead) ? NULL : FirstEntry;
	}

	EXPORTNUM(33) PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList
	(
		IN OUT PLIST_ENTRY ListHead,
		IN PLIST_ENTRY ListEntry
	)
	{
		PLIST_ENTRY LastEntry;

		/* Disable interrupts */
		ULONG Flags = __readeflags();
		_disable();

		/* Save the last entry */
		LastEntry = ListHead->Blink;

		/* Insert the new entry */
		InsertTailList(ListHead, ListEntry);

		/* restore interrupts */
		__writeeflags(Flags);

		/* Return the old last entry or NULL for empty list */
		return (LastEntry == ListHead) ? NULL : LastEntry;
	}


	EXPORTNUM(34) PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList
	(
		IN OUT PLIST_ENTRY ListHead
	)
	{
		PLIST_ENTRY ListEntry;

		/* Disable interrupts */
		ULONG Flags = __readeflags();
		_disable();

		/* Check if the list is empty */
		if (IsListEmpty(ListHead))
		{
			/* Return NULL */
			ListEntry = NULL;
		}
		else
		{
			/* Remove the first entry from the list head */
			ListEntry = RemoveHeadList(ListHead);
			#if DBG
			ListEntry->Flink = (PLIST_ENTRY)0x0BADD0FF;
			ListEntry->Blink = (PLIST_ENTRY)0x0BADD0FF;
			#endif
		}

		/* restore interrupts */
		__writeeflags(Flags);

		/* return the entry */
		return ListEntry;
	}

	LONGLONG FASTCALL ExInterlockedCompareExchange64
	(
			IN OUT LONGLONG volatile* Destination,
			IN LONGLONG* Exchange,
			IN LONGLONG* Comparand
	)
	{
		return _InterlockedCompareExchange64(Destination, *Exchange, *Comparand);
	}

	EXPORTNUM(51) LONG FASTCALL InterlockedCompareExchange
	(
		volatile PLONG Destination,
		LONG  Exchange,
		LONG  Comperand
	)
	{
		return _InterlockedCompareExchange(Destination, Exchange, Comperand);
	}

	EXPORTNUM(52) LONG FASTCALL InterlockedDecrement
	(
		volatile PLONG Addend
	)
	{
		return _InterlockedDecrement(Addend);
	}

	EXPORTNUM(53) LONG FASTCALL InterlockedIncrement
	(
		volatile PLONG Addend
	)
	{
		return _InterlockedIncrement(Addend);
	}

	EXPORTNUM(54) LONG FASTCALL InterlockedExchange
	(
		volatile PLONG Destination,
		LONG Value
	)
	{
		return _InterlockedExchange(Destination, Value);
	}

	EXPORTNUM(55) LONG FASTCALL InterlockedExchangeAdd
	(
		volatile PLONG Destination,
		LONG Value
	)
	{
		return _InterlockedExchangeAdd(Destination, Value);
	}

	// source: reactos
	EXPORTNUM(57) PSLIST_ENTRY FASTCALL InterlockedPopEntrySList
	(
		_Inout_ PSLIST_HEADER SListHead
	)
	{
		/* Disable interrupts */
		ULONG Flags = __readeflags();
		_disable();

		SLIST_HEADER OldHeader, NewHeader;
		ULONGLONG Compare;

	restart:

		/* Read the header */
		OldHeader = *SListHead;

		do
		{
			/* Check for empty list */
			if (OldHeader.Next.Next == NULL)
			{
				return NULL;
			}

			/* Create a new header */
			NewHeader = OldHeader;

			/* HACK to let the kernel know that we are doing slist-magic */
			//RtlpExpectSListFault = TRUE;

			/* Wrapped in SEH, since OldHeader.Next.Next can already be freed */
			//_SEH2_TRY
			//{
			NewHeader.Next = *OldHeader.Next.Next;
		//}
		//	_SEH2_EXCEPT((SListHead->Next.Next != OldHeader.Next.Next) ?
		//		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		//{
		//	/* We got an exception and the list head changed.
		//	   Restart the whole operation. */
		//	RtlpExpectSListFault = FALSE;
		//	goto restart;
		//}
		//_SEH2_END;

		/* We are done */
		//RtlpExpectSListFault = FALSE;

		/* Adjust depth */
			NewHeader.Depth--;

			/* Try to exchange atomically */
			Compare = OldHeader.Alignment;
			OldHeader.Alignment = InterlockedCompareExchange64(&SListHead->Alignment,
				NewHeader.Alignment,
				Compare);
		} while (OldHeader.Alignment != Compare);

		__writeeflags(Flags);

		return OldHeader.Next.Next;
	}

	// source: reactos
	EXPORTNUM(58) PSLIST_ENTRY FASTCALL InterlockedPushEntrySList
	(
		_Inout_ PSLIST_HEADER SListHead,
		_Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry
	)
	{
		SLIST_HEADER OldHeader, NewHeader;
		ULONGLONG Compare;

		/* Read the header */
		OldHeader = *SListHead;

		do
		{
			/* Link the list entry */
			SListEntry->Next = OldHeader.Next.Next;

			/* Create a new header */
			NewHeader = OldHeader;
			NewHeader.Next.Next = SListEntry;
			NewHeader.Depth++;
			NewHeader.Sequence++;

			/* Try to exchange atomically */
			Compare = OldHeader.Alignment;
			OldHeader.Alignment = InterlockedCompareExchange64(&SListHead->Alignment,
				NewHeader.Alignment,
				Compare);
		} while (OldHeader.Alignment != Compare);

		/* Return the old first entry */
		return OldHeader.Next.Next;
	}

}
