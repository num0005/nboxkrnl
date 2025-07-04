#pragma once
#include "ke.hpp"

extern "C"
{
	EXPORTNUM(1) PVOID NTAPI AvGetSavedDataAddress(void);

	EXPORTNUM(4) void NTAPI AvSetSavedDataAddress
	(
		IN  PVOID   Address
	);

	EXPORTNUM(2) void NTAPI AvSendTVEncoderOption
	(
		IN  PVOID  RegisterBase,
		IN  ULONG  Option,
		IN  ULONG  Param,
		OUT ULONG* Result
	);

	EXPORTNUM(3) ULONG NTAPI AvSetDisplayMode
	(
		IN  PVOID   RegisterBase,
		IN  ULONG   Step,
		IN  ULONG   Mode,
		IN  ULONG   Format,
		IN  ULONG   Pitch,
		IN  ULONG   FrameBuffer
	);
}