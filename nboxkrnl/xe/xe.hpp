/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "xbe.hpp"
#include "ob.hpp"


struct LAUNCH_DATA_HEADER {
	DWORD dwLaunchDataType;
	DWORD dwTitleId;
	CHAR szLaunchPath[520];
	DWORD dwFlags;
};
using PLAUNCH_DATA_HEADER = LAUNCH_DATA_HEADER *;

struct LAUNCH_DATA_PAGE {
	LAUNCH_DATA_HEADER Header;
	UCHAR Pad[492];
	UCHAR LaunchData[3072];
};
using PLAUNCH_DATA_PAGE = LAUNCH_DATA_PAGE *;

#ifdef __cplusplus
extern "C" {
#endif

extern EXPORTNUM(164) PLAUNCH_DATA_PAGE LaunchDataPage;

extern EXPORTNUM(326) OBJECT_STRING XeImageFileName;

EXPORTNUM(327) NTSTATUS XBOXAPI XeLoadSection
(
	PXBE_SECTION Section
);

EXPORTNUM(328) NTSTATUS XBOXAPI XeUnloadSection
(
	PXBE_SECTION Section
);

#ifdef __cplusplus
}
#endif

VOID XBOXAPI XbeStartupThread(PVOID Opaque);
