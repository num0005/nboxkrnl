#pragma once
#include "..\types.hpp"

#define CONTEXT_i386                0x00010000
#define CONTEXT_CONTROL             (CONTEXT_i386 | 0x00000001L)
#define CONTEXT_INTEGER             (CONTEXT_i386 | 0x00000002L)
#define CONTEXT_SEGMENTS            (CONTEXT_i386 | 0x00000004L)
#define CONTEXT_FLOATING_POINT      (CONTEXT_i386 | 0x00000008L)
#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L)

#define SIZE_OF_FPU_REGISTERS        128

#pragma pack(1)
struct FLOATING_SAVE_AREA
{
	USHORT  ControlWord;
	USHORT  StatusWord;
	USHORT  TagWord;
	USHORT  ErrorOpcode;
	ULONG   ErrorOffset;
	ULONG   ErrorSelector;
	ULONG   DataOffset;
	ULONG   DataSelector;
	ULONG   MXCsr;
	ULONG   Reserved1;
	UCHAR   RegisterArea[SIZE_OF_FPU_REGISTERS];
	UCHAR   XmmRegisterArea[SIZE_OF_FPU_REGISTERS];
	UCHAR   Reserved2[224];
	ULONG   Cr0NpxState;
};
#pragma pack()

struct  FX_SAVE_AREA
{
	FLOATING_SAVE_AREA FloatSave;
	ULONG Align16Byte[3];
};
using PFX_SAVE_AREA = FX_SAVE_AREA*;

struct CONTEXT
{
	DWORD ContextFlags;
	FLOATING_SAVE_AREA FloatSave;
	DWORD Edi;
	DWORD Esi;
	DWORD Ebx;
	DWORD Edx;
	DWORD Ecx;
	DWORD Eax;
	DWORD Ebp;
	DWORD Eip;
	DWORD SegCs;
	DWORD EFlags;
	DWORD Esp;
	DWORD SegSs;
};
using PCONTEXT = CONTEXT*;