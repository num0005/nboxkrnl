#pragma once
#include "..\types.hpp"
#include "context.hpp"

extern "C"
{
	EXPORTNUM(265) VOID XBOXAPI RtlCaptureContext
	(
		PCONTEXT ContextRecord
	);
}
