#pragma once
#include "ke.hpp"
#include "nt.hpp"

typedef void(*PIDE_INTERRUPT_ROUTINE) (void);

typedef void(*PIDE_FINISHIO_ROUTINE) (void);

typedef BOOLEAN(*PIDE_POLL_RESET_COMPLETE_ROUTINE) (void);

typedef void(*PIDE_TIMEOUT_EXPIRED_ROUTINE) (void);

typedef void(*PIDE_START_PACKET_ROUTINE) (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

typedef void(*PIDE_START_NEXT_PACKET_ROUTINE) (void);


typedef struct _IDE_CHANNEL_OBJECT
{
	PIDE_INTERRUPT_ROUTINE InterruptRoutine;
	PIDE_FINISHIO_ROUTINE FinishIoRoutine;
	PIDE_POLL_RESET_COMPLETE_ROUTINE PollResetCompleteRoutine;
	PIDE_TIMEOUT_EXPIRED_ROUTINE TimeoutExpiredRoutine;
	PIDE_START_PACKET_ROUTINE StartPacketRoutine;
	PIDE_START_NEXT_PACKET_ROUTINE StartNextPacketRoutine;
	KIRQL InterruptIrql;
	BOOLEAN ExpectingBusMasterInterrupt;
	BOOLEAN StartPacketBusy;
	BOOLEAN StartPacketRequested;
	UCHAR Timeout;
	UCHAR IoRetries;
	UCHAR MaximumIoRetries;
	PIRP CurrentIrp;
	KDEVICE_QUEUE DeviceQueue;
	LONG PhysicalRegionDescriptorTablePhysical;
	KDPC TimerDpc;
	KDPC FinishDpc;
	KTIMER Timer;
	KINTERRUPT InterruptObject;
} IDE_CHANNEL_OBJECT, * PIDE_CHANNEL_OBJECT;

extern EXPORTNUM(357) IDE_CHANNEL_OBJECT IdexChannelObject;
