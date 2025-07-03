/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "..\types.hpp"
#include "ki.hpp"
#include "hw_exp.hpp"

 // PIC i/o ports
#define PIC_MASTER_CMD          0x20
#define PIC_MASTER_DATA         0x21
#define PIC_MASTER_ELCR         0x4D0
#define PIC_SLAVE_CMD           0xA0
#define PIC_SLAVE_DATA          0xA1
#define PIC_SLAVE_ELCR          0x4D1

// PIC icw1 command bytes
#define ICW1_ICW4_NEEDED        0x01
#define ICW1_CASCADE            0x00
#define ICW1_INTERVAL8          0x00
#define ICW1_EDGE               0x00
#define ICW1_INIT               0x10

// PIC icw4 command bytes
#define ICW4_8086               0x01
#define ICW4_NORNAL_EOI         0x00
#define ICW4_NON_BUFFERED       0x00
#define ICW4_NOT_FULLY_NESTED   0x00

// PIC ocw2 command bytes
#define OCW2_EOI_IRQ            0x60

// PIC ocw3 command bytes
#define OCW3_READ_ISR           0x0B

// PIC irq base (icw2)
#define PIC_MASTER_VECTOR_BASE  IDT_INT_VECTOR_BASE
#define PIC_SLAVE_VECTOR_BASE   (IDT_INT_VECTOR_BASE + 8)

// PCI ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// SMBUS ports
#define SMBUS_STATUS 0xC000
#define SMBUS_CONTROL 0xC002
#define SMBUS_ADDRESS 0xC004
#define SMBUS_DATA 0xC006
#define SMBUS_COMMAND 0xC008
#define SMBUS_FIFO 0xC009

// SMBUS constants
#define GS_PRERR_STS (1 << 2) // cycle failed
#define GS_HCYC_STS (1 << 4) // cycle succeeded
#define GE_RW_BYTE 2
#define GE_RW_WORD 3
#define GE_RW_BLOCK 5
#define GE_HOST_STC (1 << 3) // set to start a cycle
#define GE_HCYC_EN (1 << 4) // set to enable interrupts
#define HA_RC (1 << 0) // set to specify a read/receive cycle

// SMBUS sw addresses
#define EEPROM_WRITE_ADDR 0xA8
#define EEPROM_READ_ADDR 0xA9
#define SMC_WRITE_ADDR 0x20
#define SMC_READ_ADDR 0x21

#define SMBUS_ADDRESS_SYSTEM_MICRO_CONTROLLER 0x20

/*
 * Hardware is a PIC16LC
 * http://www.xbox-linux.org/wiki/PIC
 * Directly from XEMU
 */

#define SMC_REG_VER                 0x01
#define SMC_REG_POWER               0x02
#define     SMC_REG_POWER_RESET         0x01
#define     SMC_REG_POWER_CYCLE         0x40
#define     SMC_REG_POWER_SHUTDOWN      0x80
#define SMC_REG_TRAYSTATE           0x03
#define     SMC_REG_TRAYSTATE_OPEN               0x10
#define     SMC_REG_TRAYSTATE_NO_MEDIA_DETECTED  0x40
#define     SMC_REG_TRAYSTATE_MEDIA_DETECTED     0x60
#define     SMC_REG_TRAYSTATE_VALID_STATES_MASK  (SMC_REG_TRAYSTATE_OPEN|SMC_REG_TRAYSTATE_NO_MEDIA_DETECTED|SMC_REG_TRAYSTATE_MEDIA_DETECTED)
#define SMC_REG_AVPACK              0x04
#define     SMC_REG_AVPACK_SCART        0x00
#define     SMC_REG_AVPACK_HDTV         0x01
#define     SMC_REG_AVPACK_VGA          0x02
#define     SMC_REG_AVPACK_RFU          0x03
#define     SMC_REG_AVPACK_SVIDEO       0x04
#define     SMC_REG_AVPACK_COMPOSITE    0x06
#define     SMC_REG_AVPACK_NONE         0x07
#define SMC_REG_FANMODE             0x05
#define SMC_REG_FANSPEED            0x06
#define SMC_REG_LEDMODE             0x07
#define SMC_REG_LEDSEQ              0x08
#define SMC_REG_CPUTEMP             0x09
#define SMC_REG_BOARDTEMP           0x0a
#define SMC_REG_TRAYEJECT           0x0c
#define SMC_REG_INTACK              0x0d
#define SMC_REG_ERROR_WRITE         0x0e
#define SMC_REG_ERROR_READ          0x0f
#define SMC_REG_INTSTATUS           0x11
#define     SMC_REG_INTSTATUS_POWER         0x01
#define     SMC_REG_INTSTATUS_TRAYCLOSED    0x02
#define     SMC_REG_INTSTATUS_TRAYOPENING   0x04
#define     SMC_REG_INTSTATUS_AVPACK_PLUG   0x08
#define     SMC_REG_INTSTATUS_AVPACK_UNPLUG 0x10
#define     SMC_REG_INTSTATUS_EJECT_BUTTON  0x20
#define     SMC_REG_INTSTATUS_TRAYCLOSING   0x40
#define SMC_REG_RESETONEJECT        0x19
#define SMC_REG_INTEN               0x1a
#define SMC_REG_SCRATCH             0x1b
#define     SMC_REG_SCRATCH_SHORT_ANIMATION 0x04

#define SMC_VERSION_LENGTH 3

inline bool HalpIsDefinedTrayState(ULONG TrayState)
{
	switch (TrayState)
	{
		case SMC_REG_TRAYSTATE_OPEN:
		case SMC_REG_TRAYSTATE_NO_MEDIA_DETECTED:
		case SMC_REG_TRAYSTATE_MEDIA_DETECTED:
			return true;
		default:
			return false;
	}
}


extern KDPC HalpSmbusDpcObject;
extern NTSTATUS HalpSmbusStatus;
extern BYTE HalpSmbusData[32];
extern UCHAR HalpBlockAmount;
extern KEVENT HalpSmbusLock;
extern KEVENT HalpSmbusComplete;
inline ULONG HalpSMCScratchRegister;

// macro copied from reactos source code then modified to work in a header

#if defined(__GNUC__)

#define HalpHwInt(x)                                                \
    static inline                                                   \
    VOID                                                            \
    XBOXAPI                                                         \
    HalpHwInt##x(VOID)                                              \
    {                                                               \
        asm volatile ("int $%c0\n"::"i"(PRIMARY_VECTOR_BASE + x));  \
    }

#elif defined(_MSC_VER)

#define HalpHwInt(x)                                                \
    static inline                                                   \
    VOID                                                            \
    XBOXAPI                                                         \
    HalpHwInt##x(VOID)                                              \
    {                                                               \
        __asm                                                       \
        {                                                           \
            int IDT_INT_VECTOR_BASE + x                             \
        }                                                           \
    }

#else
#error Unsupported compiler
#endif

HalpHwInt(0);
HalpHwInt(1);
HalpHwInt(2);
HalpHwInt(3);
HalpHwInt(4);
HalpHwInt(5);
HalpHwInt(6);
HalpHwInt(7);
HalpHwInt(8);
HalpHwInt(9);
HalpHwInt(10);
HalpHwInt(11);
HalpHwInt(12);
HalpHwInt(13);
HalpHwInt(14);
HalpHwInt(15);

VOID XBOXAPI HalpSwIntApc();
VOID XBOXAPI HalpSwIntDpc();


VOID XBOXAPI HalpInterruptCommon();
VOID XBOXAPI HalpClockIsr();
VOID XBOXAPI HalpSmbusIsr();

inline constexpr VOID(XBOXAPI *const SwIntHandlers[])() = {
	&KiUnexpectedInterrupt,
	&HalpSwIntApc,
	&HalpSwIntDpc,
	&KiUnexpectedInterrupt,
	&HalpHwInt0,
	&HalpHwInt1,
	&HalpHwInt2,
	&HalpHwInt3,
	&HalpHwInt4,
	&HalpHwInt5,
	&HalpHwInt6,
	&HalpHwInt7,
	&HalpHwInt8,
	&HalpHwInt9,
	&HalpHwInt10,
	&HalpHwInt11,
	&HalpHwInt12,
	&HalpHwInt13,
	&HalpHwInt14,
	&HalpHwInt15
};

// todo: replace usages of this with HalReturnToFirmware?
[[noreturn]] VOID HalpShutdownSystem();
VOID HalpInitPIC();
VOID HalpInitPIT();
VOID HalpInitSMCstate();
VOID HalpReadCmosTime(PTIME_FIELDS TimeFields);
// check for unmasked interrupts, must be called with interrupts disabled
VOID HalpCheckUnmaskedInt();
VOID XBOXAPI HalpSmbusDpcRoutine(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
VOID HalpExecuteReadSmbusCycle(UCHAR SlaveAddress, UCHAR CommandCode, BOOLEAN ReadWordValue);
VOID HalpExecuteWriteSmbusCycle(UCHAR SlaveAddress, UCHAR CommandCode, BOOLEAN WriteWordValue, ULONG DataValue);
VOID HalpExecuteBlockReadSmbusCycle(UCHAR SlaveAddress, UCHAR CommandCode);
VOID HalpExecuteBlockWriteSmbusCycle(UCHAR SlaveAddress, UCHAR CommandCode, PBYTE Data);
NTSTATUS HalpReadSMBusBlock(UCHAR SlaveAddress, UCHAR CommandCode, UCHAR ReadAmount, BYTE *Buffer);
NTSTATUS HalpWriteSMBusBlock(UCHAR SlaveAddress, UCHAR CommandCode, UCHAR WriteAmount, BYTE *Buffer);
// fire off if we have reason to think cached SMC tray state is invalid 
VOID HalpInvalidateSMCTrayState();
_IRQL_requires_min_(DISPATCH_LEVEL)
NTSTATUS NTAPI HalpRunShutdownRoutines();
