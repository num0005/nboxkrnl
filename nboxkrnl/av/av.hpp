#pragma once
#include "ke.hpp"

extern "C"
{
	EXPORTNUM(1) PVOID NTAPI AvGetSavedDataAddress(void);

	EXPORTNUM(4) void NTAPI AvSetSavedDataAddress
	(
		IN  PVOID   Address
	);
}