/*
 * ergo720                Copyright (c) 2023
 */

#include "hal.hpp"
#include "halp.hpp"
#include "rtl.hpp"


 // Global list of routines executed during a reboot
static LIST_ENTRY ShutdownRoutineList = { &ShutdownRoutineList , &ShutdownRoutineList };

#define TRAY_STATE_INVALID 0xFFFF
static DWORD CacheTrayState = TRAY_STATE_INVALID;
static DWORD TrayStateChangedCount = 0;

VOID HalInitSystem()
{
	HalpInitPIC();
	HalpInitPIT();

	TIME_FIELDS TimeFields;
	LARGE_INTEGER SystemTime, OldTime;
	HalpReadCmosTime(&TimeFields);
	if (!RtlTimeFieldsToTime(&TimeFields, &SystemTime)) {
		// If the conversion fails, just set some random date for the system time instead of failing the initialization
		SystemTime.HighPart = 0x80000000;
		SystemTime.LowPart = 0;
	}
	KeSetSystemTime(&SystemTime, &OldTime);

	// Connect the PIT (clock) interrupt (NOTE: this will also enable interrupts)
	KiIdt[IDT_INT_VECTOR_BASE + 0] = ((uint64_t)0x8 << 16) | ((uint64_t)&HalpClockIsr & 0x0000FFFF) | (((uint64_t)&HalpClockIsr & 0xFFFF0000) << 32) | ((uint64_t)0x8E00 << 32);
	HalEnableSystemInterrupt(0, Edge);

	// Connect the SMBUS interrupt
	KeInitializeEvent(&HalpSmbusLock, SynchronizationEvent, 1);
	KeInitializeEvent(&HalpSmbusComplete, NotificationEvent, 0);
	KeInitializeDpc(&HalpSmbusDpcObject, HalpSmbusDpcRoutine, nullptr);
	KiIdt[IDT_INT_VECTOR_BASE + 11] = ((uint64_t)0x8 << 16) | ((uint64_t)&HalpSmbusIsr & 0x0000FFFF) | (((uint64_t)&HalpSmbusIsr & 0xFFFF0000) << 32) | ((uint64_t)0x8E00 << 32);
	HalEnableSystemInterrupt(11, LevelSensitive);

	HalpInitSMCstate();

	if (XboxType == CONSOLE_DEVKIT) {
		XboxHardwareInfo.Flags |= 2;
	}
	else if (XboxType == CONSOLE_CHIHIRO) {
		XboxHardwareInfo.Flags |= 8;
	}
}

VOID HalpInvalidateSMCTrayState()
{
	UCHAR OLdIrql = KeRaiseIrqlToDpcLevel();
	TrayStateChangedCount++;
	CacheTrayState = TRAY_STATE_INVALID;
	KfLowerIrql(OLdIrql);
}

EXPORTNUM(9) NTSTATUS NTAPI HalReadSMCTrayState
(
	ULONG* State,
	ULONG* Count
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG TrayState, TrayCount;

	/* read cached values */
	{
		UCHAR OLdIrql = KeRaiseIrqlToDpcLevel();
		TrayState = CacheTrayState;
		TrayCount = TrayStateChangedCount;
		KfLowerIrql(OLdIrql);
	}

	// read the value if it's not valid
	if (TrayState == TRAY_STATE_INVALID)
	{
		Status = HalReadSMBusValue(SMBUS_ADDRESS_SYSTEM_MICRO_CONTROLLER, SMC_REG_TRAYSTATE, 0, &TrayState);

		if (NT_SUCCESS(Status))
		{
			// check no undefined bits are set in debug builds
			NT_ASSERT((TrayState & ~SMC_REG_TRAYSTATE_VALID_STATES_MASK) == 0);
			TrayState &= SMC_REG_TRAYSTATE_VALID_STATES_MASK; // mask out invalid bits

			// set to open if it's not a known state
			if (!HalpIsDefinedTrayState(TrayState))
			{
				TrayState = SMC_REG_TRAYSTATE_OPEN;
			}
			

			/* write back cached values */
			{
				UCHAR OLdIrql = KeRaiseIrqlToDpcLevel();
				NT_ASSERT(TrayCount <= TrayStateChangedCount);
				// we check if there has been another change since, skip caching if that's the case
				if (TrayStateChangedCount == TrayCount)
				{
					CacheTrayState = TrayState;
				}
				KfLowerIrql(OLdIrql);
			}
		}
	}

	*State = TrayState;
	if (Count)
	{
		*Count = TrayStateChangedCount;
	}
	return Status;
}

EXPORTNUM(40) ULONG HalDiskCachePartitionCount = 3;
EXPORTNUM(41) PANSI_STRING HalDiskModelNumber;
EXPORTNUM(42) PANSI_STRING HalDiskSerialNumber;

EXPORTNUM(45) NTSTATUS XBOXAPI HalReadSMBusValue
(
	UCHAR SlaveAddress,
	UCHAR CommandCode,
	BOOLEAN ReadWordValue,
	ULONG *DataValue
)
{
	KeEnterCriticalRegion(); // prevent suspending this thread while we hold the smbus lock below
	KeWaitForSingleObject(&HalpSmbusLock, Executive, KernelMode, FALSE, nullptr); // prevent concurrent smbus cycles

	HalpBlockAmount = 0;
	HalpExecuteReadSmbusCycle(SlaveAddress, CommandCode, ReadWordValue);

	KeWaitForSingleObject(&HalpSmbusComplete, Executive, KernelMode, FALSE, nullptr); // wait until the cycle is completed by the dpc

	NTSTATUS Status = HalpSmbusStatus;
	*DataValue = *(PULONG)HalpSmbusData;

	KeSetEvent(&HalpSmbusLock, 0, FALSE);
	KeLeaveCriticalRegion();

	return Status;
}

EXPORTNUM(46) VOID XBOXAPI HalReadWritePCISpace
(
	ULONG BusNumber,
	ULONG SlotNumber,
	ULONG RegisterNumber,
	PVOID Buffer,
	ULONG Length,
	BOOLEAN WritePCISpace
)
{
	disable();

	ULONG BufferOffset = 0;
	PBYTE Buffer1 = (PBYTE)Buffer;
	ULONG SizeLeft = Length;
	ULONG CfgAddr = 0x80000000 | ((BusNumber & 0xFF) << 16) | ((SlotNumber & 0x1F) << 11) | ((SlotNumber & 0xE0) << 3) | (RegisterNumber & 0xFC);

	while (SizeLeft > 0) {
		ULONG BytesToAccess = (SizeLeft > 4) ? 4 : (SizeLeft == 3) ? 2 : SizeLeft;
		ULONG RegisterOffset = RegisterNumber % 4;
		outl(PCI_CONFIG_ADDRESS, CfgAddr);

		switch (BytesToAccess)
		{
		case 1:
			if (WritePCISpace) {
				outb(PCI_CONFIG_DATA + RegisterOffset, *(Buffer1 + BufferOffset));
			}
			else {
				*(Buffer1 + BufferOffset) = inb(PCI_CONFIG_DATA + RegisterOffset);
			}
			break;

		case 2:
			if (WritePCISpace) {
				outw(PCI_CONFIG_DATA + RegisterOffset, *PUSHORT(Buffer1 + BufferOffset));
			}
			else {
				*PUSHORT(Buffer1 + BufferOffset) = inw(PCI_CONFIG_DATA + RegisterOffset);
			}
			break;

		default:
			if (WritePCISpace) {
				outl(PCI_CONFIG_DATA + RegisterOffset, *PULONG(Buffer1 + BufferOffset));
			}
			else {
				*PULONG(Buffer1 + BufferOffset) = inl(PCI_CONFIG_DATA + RegisterOffset);
			}
		}

		RegisterNumber += BytesToAccess;
		CfgAddr = (CfgAddr & ~0xFF) | (RegisterNumber >> 2);
		BufferOffset += BytesToAccess;
		SizeLeft -= BytesToAccess;
	}

	enable();
}

// Source: Cxbx-Reloaded
EXPORTNUM(47) VOID XBOXAPI HalRegisterShutdownNotification
(
	PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
	BOOLEAN Register
)
{
	KIRQL OldIrql = KeRaiseIrqlToDpcLevel();

	if (Register) {
		PLIST_ENTRY ListEntry = ShutdownRoutineList.Flink;
		while (ListEntry != &ShutdownRoutineList) {
			if (ShutdownRegistration->Priority > CONTAINING_RECORD(ListEntry, HAL_SHUTDOWN_REGISTRATION, ListEntry)->Priority) {
				InsertTailList(ListEntry, &ShutdownRegistration->ListEntry);
				break;
			}
			ListEntry = ListEntry->Flink;
		}

		if (ListEntry == &ShutdownRoutineList) {
			InsertTailList(ListEntry, &ShutdownRegistration->ListEntry);
		}
	}
	else {
		PLIST_ENTRY ListEntry = ShutdownRoutineList.Flink;
		while (ListEntry != &ShutdownRoutineList) {
			if (ShutdownRegistration == CONTAINING_RECORD(ListEntry, HAL_SHUTDOWN_REGISTRATION, ListEntry)) {
				RemoveEntryList(&ShutdownRegistration->ListEntry);
				break;
			}
			ListEntry = ListEntry->Flink;
		}
	}

	KfLowerIrql(OldIrql);
}

EXPORTNUM(50) NTSTATUS XBOXAPI HalWriteSMBusValue
(
	UCHAR SlaveAddress,
	UCHAR CommandCode,
	BOOLEAN WriteWordValue,
	ULONG DataValue
)
{
	KeEnterCriticalRegion(); // prevent suspending this thread while we hold the smbus lock below
	KeWaitForSingleObject(&HalpSmbusLock, Executive, KernelMode, FALSE, nullptr); // prevent concurrent smbus cycles

	HalpBlockAmount = 0;
	HalpExecuteWriteSmbusCycle(SlaveAddress, CommandCode, WriteWordValue, DataValue);

	KeWaitForSingleObject(&HalpSmbusComplete, Executive, KernelMode, FALSE, nullptr); // wait until the cycle is completed by the dpc

	NTSTATUS Status = HalpSmbusStatus;

	KeSetEvent(&HalpSmbusLock, 0, FALSE);
	KeLeaveCriticalRegion();

	return Status;
}

EXPORTNUM(356) ULONG HalBootSMCVideoMode = SMC_REG_AVPACK_NONE;

EXPORTNUM(365) VOID NTAPI HalEnableSecureTrayEject
(
)
{
	// TODO: figure out what global this checks
	//if (g_SecureTrayEjectAllowed)
	{
		//g_SecureTrayEjectAllowed = false;

		// why does this loop????
		NTSTATUS Status;
		do
		{
			Status = HalWriteSMBusValue(SMBUS_ADDRESS_SYSTEM_MICRO_CONTROLLER, SMC_REG_RESETONEJECT, FALSE, 0);
		} while (!NT_SUCCESS(Status));
	}
}

EXPORTNUM(366) NTSTATUS NTAPI HalWriteSMCScratchRegister
(
	IN DWORD ScratchRegister
)
{
	HalpSMCScratchRegister = ScratchRegister;

	NTSTATUS Status = HalWriteSMBusValue(SMBUS_ADDRESS_SYSTEM_MICRO_CONTROLLER, SMC_REG_SCRATCH, /*WordFlag:*/false, ScratchRegister);

	return Status;
}
