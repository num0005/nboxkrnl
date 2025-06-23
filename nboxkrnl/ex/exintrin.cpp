//SPDX-License-Identifier: GPL-2.0-or-later

#include "ex.hpp"

extern "C" {

	LONGLONG
		FASTCALL
		ExInterlockedCompareExchange64(
			IN OUT LONGLONG volatile* Destination,
			IN LONGLONG* Exchange,
			IN LONGLONG* Comparand)
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
}