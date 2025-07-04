#include "av.hpp"
#include "ke.hpp"
#include "mm.hpp"

/*
* AvSetSavedDataAddress/AvGetSavedDataAddress presists memory across a quick reboot using the sticky page
* and peristent memory. Used to save a framebuffer.
*/

STICKY_PAGE_START
PVOID AvSavedDataAddress = nullptr;
STICKY_PAGE_END

// ******************************************************************
// * 0x0001 - AvGetSavedDataAddress()
// ******************************************************************
EXPORTNUM(1) PVOID NTAPI AvGetSavedDataAddress(void)
{

	return AvSavedDataAddress;
}

// ******************************************************************
// * 0x0004 - AvSetSavedDataAddress()
// ******************************************************************
EXPORTNUM(4) void NTAPI AvSetSavedDataAddress
(
	IN  PVOID   Address
)
{

	AvSavedDataAddress = Address;
}

EXPORTNUM(2) void NTAPI AvSendTVEncoderOption
(
	IN  PVOID  RegisterBase,
	IN  ULONG  Option,
	IN  ULONG  Param,
	OUT ULONG* Result
)
{
	RIP_UNIMPLEMENTED();
}

EXPORTNUM(3) ULONG NTAPI AvSetDisplayMode
(
	IN  PVOID   RegisterBase,
	IN  ULONG   Step,
	IN  ULONG   Mode,
	IN  ULONG   Format,
	IN  ULONG   Pitch,
	IN  ULONG   FrameBuffer
)
{
	RIP_UNIMPLEMENTED();
}
