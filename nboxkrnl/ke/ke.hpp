/*
 * ergo720                Copyright (c) 2022
 */

#pragma once

#include "..\types.hpp"
#include "..\driverspecs.h"
#include "rtl_assert.hpp"

#define XBOX_KEY_LENGTH 16

#define THREAD_QUANTUM 60 // ms that a thread is allowed to run before being preempted, in multiples of CLOCK_QUANTUM_DECREMENT
#define CLOCK_QUANTUM_DECREMENT 3 // subtracts 1 ms after every clock interrupt, in multiples of CLOCK_QUANTUM_DECREMENT
#define WAIT_QUANTUM_DECREMENT 9 // subtracts 3 ms after a wait completes, in multiples of CLOCK_QUANTUM_DECREMENT
#define NORMAL_BASE_PRIORITY 8
#define TIME_CRITICAL_BASE_PRIORITY 14
#define LOW_PRIORITY 0
#define LOW_REALTIME_PRIORITY 16
#define HIGH_PRIORITY 31

#define PRIORITY_BOOST_EVENT 1
#define PRIORITY_BOOST_MUTANT 1
#define PRIORITY_BOOST_SEMAPHORE PRIORITY_BOOST_EVENT
#define PRIORITY_BOOST_IO 0
#define PRIORITY_BOOST_TIMER 0

#define HIGH_LEVEL 31
#define CLOCK_LEVEL 28
#define SYNC_LEVEL CLOCK_LEVEL
#define SMBUS_LEVEL 15
#define DISPATCH_LEVEL 2
#define APC_LEVEL 1
#define PASSIVE_LEVEL 0

// SMP: this is is changed on SMP NT kernels, try to not assume SYNCH is always DISPATCH 
#define SYNCH_LEVEL DISPATCH_LEVEL

#define IRQL_OFFSET_FOR_IRQ 4
#define DISPATCH_LENGTH 22

#define IDT_SERVICE_VECTOR_BASE 0x20
#define IDT_INT_VECTOR_BASE 0x30
#define MAX_BUS_INTERRUPT_LEVEL 26

#define SYNCHRONIZATION_OBJECT_TYPE_MASK 7

#define MUTANT_LIMIT 0x80000000

#define MAX_TIMER_DPCS 16

#define MAXIMUM_SUSPEND_COUNT 0x7F

#define EVENT_INCREMENT                   1
#define IO_NO_INCREMENT                   0
#define IO_CD_ROM_INCREMENT               1
#define IO_DISK_INCREMENT                 1
#define IO_KEYBOARD_INCREMENT             6
#define IO_MAILSLOT_INCREMENT             2
#define IO_MOUSE_INCREMENT                6
#define IO_NAMED_PIPE_INCREMENT           2
#define IO_NETWORK_INCREMENT              2
#define IO_PARALLEL_INCREMENT             1
#define IO_SERIAL_INCREMENT               2
#define IO_SOUND_INCREMENT                8
#define IO_VIDEO_INCREMENT                1
#define SEMAPHORE_INCREMENT               1
#define THREAD_ALERT_INCREMENT            2

//
// EFlags (x86)
//
#define EFLAGS_CF               0x01L
#define EFLAGS_ZF               0x40L
#define EFLAGS_TF               0x100L
#define EFLAGS_INTERRUPT_MASK   0x200L
#define EFLAGS_DF               0x400L
#define EFLAGS_IOPL             0x3000L
#define EFLAGS_NESTED_TASK      0x4000L
#define EFLAGS_RF               0x10000
#define EFLAGS_V86_MASK         0x20000
#define EFLAGS_ALIGN_CHECK      0x40000
#define EFLAGS_VIF              0x80000
#define EFLAGS_VIP              0x100000
#define EFLAGS_ID               0x200000
#define EFLAGS_USER_SANITIZE    0x3F4DD7
#define EFLAG_SIGN              0x8000
#define EFLAG_ZERO              0x4000

// These macros (or equivalent assembly code) should be used to access the members of KiPcr when the irql is below dispatch level, to make sure that
// the accesses are atomic and thus thread-safe
#define KeGetStackBase(var) \
	ASM_BEGIN \
		ASM(mov eax, [KiPcr].NtTib.StackBase); \
		ASM(mov var, eax); \
	ASM_END

#define KeGetStackLimit(var) \
	ASM_BEGIN \
		ASM(mov eax, [KiPcr].NtTib.StackLimit); \
		ASM(mov var, eax); \
	ASM_END

#define KeGetExceptionHead(var) \
	ASM_BEGIN \
		ASM(mov eax, [KiPcr].NtTib.ExceptionList); \
		ASM(mov dword ptr var, eax); \
	ASM_END

#define KeSetExceptionHead(var) \
	ASM_BEGIN \
		ASM(mov eax, dword ptr var); \
		ASM(mov [KiPcr].NtTib.ExceptionList, eax); \
	ASM_END

#define KeResetExceptionHead(var) \
	ASM_BEGIN \
		ASM(mov eax, dword ptr var); \
		ASM(mov [KiPcr].NtTib.ExceptionList, eax); \
	ASM_END

#define KeGetDpcStack(var) \
	ASM_BEGIN \
		ASM(mov eax, [KiPcr].PrcbData.DpcStack); \
		ASM(mov var, eax); \
	ASM_END

#define KeGetDpcActive(var) \
	ASM_BEGIN \
		ASM(mov eax, [KiPcr].PrcbData.DpcRoutineActive); \
		ASM(mov var, eax); \
	ASM_END

#define INITIALIZE_GLOBAL_KEVENT(Event, Type, State) \
    KEVENT Event = {                                 \
        Type,                                        \
        FALSE,                                       \
        sizeof(KEVENT) / sizeof(LONG),               \
        FALSE,                                       \
        State,                                       \
        &Event.Header.WaitListHead,                  \
        &Event.Header.WaitListHead                   \
    }


using KIRQL = UCHAR;
using PKIRQL = KIRQL *;
using KPROCESSOR_MODE = CCHAR;
using XBOX_KEY_DATA = uint8_t[XBOX_KEY_LENGTH];

enum MODE {
	KernelMode,
	UserMode,
	MaximumMode
};

enum KOBJECTS {
	EventNotificationObject = 0,
	EventSynchronizationObject = 1,
	MutantObject = 2,
	ProcessObject = 3,
	QueueObject = 4,
	SemaphoreObject = 5,
	ThreadObject = 6,
	Spare1Object = 7,
	TimerNotificationObject = 8,
	TimerSynchronizationObject = 9,
	Spare2Object = 10,
	Spare3Object = 11,
	Spare4Object = 12,
	Spare5Object = 13,
	Spare6Object = 14,
	Spare7Object = 15,
	Spare8Object = 16,
	Spare9Object = 17,
	ApcObject,
	DpcObject,
	DeviceQueueObject,
	EventPairObject,
	InterruptObject,
	ProfileObject
};

enum TIMER_TYPE {
	NotificationTimer,
	SynchronizationTimer
};

enum WAIT_TYPE {
	WaitAll,
	WaitAny
};

enum KTHREAD_STATE {
	Initialized,
	Ready,
	Running,
	Standby,
	Terminated,
	Waiting,
	Transition
};

enum KINTERRUPT_MODE: UCHAR {
	LevelSensitive,
	Edge
};

enum KWAIT_REASON {
	Executive = 0,
	FreePage = 1,
	PageIn = 2,
	PoolAllocation = 3,
	DelayExecution = 4,
	Suspended = 5,
	UserRequest = 6,
	WrExecutive = 7,
	WrFreePage = 8,
	WrPageIn = 9,
	WrPoolAllocation = 10,
	WrDelayExecution = 11,
	WrSuspended = 12,
	WrUserRequest = 13,
	WrEventPair = 14,
	WrQueue = 15,
	WrLpcReceive = 16,
	WrLpcReply = 17,
	WrVirtualMemory = 18,
	WrPageOut = 19,
	WrRendezvous = 20,
	WrFsCacheIn = 21,
	WrFsCacheOut = 22,
	WrDispatchInt = 23,
	WrQuantumEnd = 24,
	WrYieldExecution = 25,
	WrKernel = 26,
	MaximumWaitReason = 27
};

enum EVENT_TYPE {
	NotificationEvent = 0,
	SynchronizationEvent
};

struct KAPC_STATE {
	LIST_ENTRY ApcListHead[MaximumMode];
	struct KPROCESS *Process;
	BOOLEAN KernelApcInProgress;
	BOOLEAN KernelApcPending;
	BOOLEAN UserApcPending;
	BOOLEAN ApcQueueable;
};
using PKAPC_STATE = KAPC_STATE *;

struct KWAIT_BLOCK {
	LIST_ENTRY WaitListEntry;
	struct KTHREAD *Thread;
	PVOID Object;
	struct KWAIT_BLOCK *NextWaitBlock;
	USHORT WaitKey;
	USHORT WaitType;
};
using PKWAIT_BLOCK = KWAIT_BLOCK *;

struct KQUEUE {
	DISPATCHER_HEADER Header;
	LIST_ENTRY EntryListHead;
	ULONG CurrentCount;
	ULONG MaximumCount;
	LIST_ENTRY ThreadListHead;
};
using PKQUEUE = KQUEUE *;

struct KTIMER {
	DISPATCHER_HEADER Header;
	ULARGE_INTEGER DueTime;
	LIST_ENTRY TimerListEntry;
	struct KDPC *Dpc;
	LONG Period;
};
using PKTIMER = KTIMER *;

using PKNORMAL_ROUTINE = VOID(XBOXAPI *)(
	PVOID NormalContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
	);

using PKKERNEL_ROUTINE = VOID(XBOXAPI *)(
	struct KAPC *Apc,
	PKNORMAL_ROUTINE *NormalRoutine,
	PVOID *NormalContext,
	PVOID *SystemArgument1,
	PVOID *SystemArgument2
	);

using PKRUNDOWN_ROUTINE = VOID(XBOXAPI *)(
	struct KAPC *Apc
	);

using PKSTART_ROUTINE = VOID(XBOXAPI *)(
	VOID *StartContext
	);

using PKSYSTEM_ROUTINE = VOID(XBOXAPI *)(
	PKSTART_ROUTINE StartRoutine,
	VOID *StartContext
	);

struct KAPC {
	CSHORT Type;
	CSHORT Size;
	ULONG Reserved;
	struct KTHREAD *Thread;
	LIST_ENTRY ApcListEntry;
	PKKERNEL_ROUTINE KernelRoutine;
	PKRUNDOWN_ROUTINE RundownRoutine;
	PKNORMAL_ROUTINE NormalRoutine;
	PVOID NormalContext;
	PVOID SystemArgument1;
	PVOID SystemArgument2;
	// might be zero on xbox? todo: test with original software, apcstateindex is used for handling threads being attached to a different process
	CCHAR ApcStateIndex;
	KPROCESSOR_MODE ApcMode;
	BOOLEAN Inserted;
};
using PKAPC = KAPC *;

struct KSEMAPHORE {
	DISPATCHER_HEADER Header;
	LONG Limit;
};
using PKSEMAPHORE = KSEMAPHORE *;

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	LONG CurrentCount;
	LONG MaximumCount;
}
SEMAPHORE_BASIC_INFORMATION, * PSEMAPHORE_BASIC_INFORMATION;

struct KEVENT {
	DISPATCHER_HEADER Header;
};
using PKEVENT = KEVENT *;

struct KDEVICE_QUEUE {
	CSHORT Type;
	UCHAR Size;
	BOOLEAN Busy;
	LIST_ENTRY DeviceListHead;
};
using PKDEVICE_QUEUE = KDEVICE_QUEUE *;

typedef struct _KDEVICE_QUEUE_ENTRY
{
	LIST_ENTRY DeviceListEntry;
	ULONG SortKey;
	BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, * PKDEVICE_QUEUE_ENTRY, * RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

struct KMUTANT {
	DISPATCHER_HEADER Header;
	LIST_ENTRY MutantListEntry;
	struct KTHREAD *OwnerThread;
	BOOLEAN Abandoned;
};
using PKMUTANT = KMUTANT *;

struct KSTART_FRAME {
	PVOID RetAddrFatal;
	PKSYSTEM_ROUTINE SystemRoutine;
	PKSTART_ROUTINE StartRoutine;
	PVOID StartContext;
};
using PKSTART_FRAME = KSTART_FRAME *;

struct KSWITCHFRAME {
	PVOID ExceptionList;
	DWORD Eflags;
	PVOID RetAddr;
};
using PKSWITCHFRAME = KSWITCHFRAME *;

struct KTHREAD {
	DISPATCHER_HEADER Header;
	LIST_ENTRY MutantListHead;
	ULONG KernelTime;
	PVOID StackBase;
	PVOID StackLimit;
	PVOID KernelStack;
	PVOID TlsData;
	UCHAR State;
	BOOLEAN Alerted[MaximumMode];
	BOOLEAN Alertable;
	UCHAR NpxState;
	CHAR Saturation;
	SCHAR Priority;
	UCHAR Padding;
	KAPC_STATE ApcState;
	ULONG ContextSwitches;
	LONG_PTR WaitStatus;
	KIRQL WaitIrql;
	KPROCESSOR_MODE WaitMode;
	BOOLEAN WaitNext;
	UCHAR WaitReason;
	PKWAIT_BLOCK WaitBlockList;
	LIST_ENTRY WaitListEntry;
	ULONG WaitTime;
	ULONG KernelApcDisable;
	LONG Quantum;
	SCHAR BasePriority;
	UCHAR DecrementCount;
	SCHAR PriorityDecrement;
	BOOLEAN DisableBoost;
	UCHAR NpxIrql;
	CCHAR SuspendCount;
	BOOLEAN Preempted;
	BOOLEAN HasTerminated;
	PKQUEUE Queue;
	LIST_ENTRY QueueListEntry;
	KTIMER Timer;
	KWAIT_BLOCK TimerWaitBlock;
	KAPC SuspendApc;
	KSEMAPHORE SuspendSemaphore;
	LIST_ENTRY ThreadListEntry;
};
using PKTHREAD = KTHREAD *;

struct ETHREAD {
	KTHREAD Tcb;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	NTSTATUS ExitStatus;
	LIST_ENTRY ReaperLink;
	HANDLE UniqueThread;
	PVOID StartAddress;
	LIST_ENTRY IrpList;
};
using PETHREAD = ETHREAD *;

struct KPROCESS {
	LIST_ENTRY ReadyListHead;
	LIST_ENTRY ThreadListHead;
	ULONG StackCount;
	LONG ThreadQuantum;
	SCHAR BasePriority;
	BOOLEAN DisableBoost;
	BOOLEAN DisableQuantum;
};
using PKPROCESS = KPROCESS *;

using PKDEFERRED_ROUTINE = VOID(XBOXAPI *)(
	struct KDPC *Dpc,
	PVOID DeferredContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
	);

struct KDPC {
	CSHORT Type;
	BOOLEAN Inserted;
	UCHAR Padding;
	LIST_ENTRY DpcListEntry;
	PKDEFERRED_ROUTINE DeferredRoutine;
	PVOID DeferredContext;
	PVOID SystemArgument1;
	PVOID SystemArgument2;
	PULONG_PTR Lock;
};
using PKDPC = KDPC *;

struct KSYSTEM_TIME {
	ULONG LowTime;
	LONG HighTime;
	LONG High2Time;
};

struct DPC_QUEUE_ENTRY {
	PKDPC Dpc;
	PKDEFERRED_ROUTINE Routine;
	PVOID Context;
};
using PDPC_QUEUE_ENTRY = DPC_QUEUE_ENTRY *;

using PKSERVICE_ROUTINE = BOOLEAN(XBOXAPI *)(
	struct KINTERRUPT *Interrupt,
	PVOID ServiceContext
	);

struct KINTERRUPT {
	PKSERVICE_ROUTINE ServiceRoutine;
	PVOID ServiceContext;
	ULONG BusInterruptLevel;
	ULONG Irql;
	BOOLEAN Connected;
	BOOLEAN ShareVector;
	KINTERRUPT_MODE Mode;
	/*UCHAR Padding;*/
	ULONG ServiceCount;
	ULONG DispatchCode[DISPATCH_LENGTH];
};
static_assert(offsetof(KINTERRUPT, ServiceContext) == 4);
static_assert(offsetof(KINTERRUPT, BusInterruptLevel) == 8);
static_assert(offsetof(KINTERRUPT, Irql) == 12);
static_assert(offsetof(KINTERRUPT, Connected) == 16);
static_assert(offsetof(KINTERRUPT, ShareVector) == 17);
static_assert(offsetof(KINTERRUPT, Mode) == 18);
static_assert(offsetof(KINTERRUPT, ServiceCount) == 20);
static_assert(offsetof(KINTERRUPT, DispatchCode) == 24);
using PKINTERRUPT = KINTERRUPT *;

#define SIZE_OF_FPU_REGISTERS        128
#define NPX_STATE_NOT_LOADED (CR0_TS | CR0_MP) // x87 fpu, XMM, and MXCSR registers not loaded on fpu
#define NPX_STATE_LOADED 0                     // x87 fpu, XMM, and MXCSR registers loaded on fpu

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

struct KPRCB
{
	struct KTHREAD* CurrentThread;
	struct KTHREAD* NextThread;
	struct KTHREAD* IdleThread;
	struct KTHREAD* NpxThread;
	ULONG InterruptCount;
	ULONG DpcTime;
	ULONG InterruptTime;
	ULONG DebugDpcTime;
	ULONG KeContextSwitches;
	ULONG DpcInterruptRequested;
	LIST_ENTRY DpcListHead;
	ULONG DpcRoutineActive;
	PVOID DpcStack;
	volatile ULONG QuantumEnd;
	// NOTE: if this is used with a fxsave instruction to save the float state, then this buffer must be 16-bytes aligned.
	// At the moment, we only use the Npx area in the thread's stack
	FX_SAVE_AREA NpxSaveArea;
	VOID* DmEnetFunc;
	VOID* DebugMonitorData;
};
using PKPRCB = KPRCB*;

struct NT_TIB
{
	struct EXCEPTION_REGISTRATION_RECORD* ExceptionList;
	PVOID StackBase;
	PVOID StackLimit;
	PVOID SubSystemTib;
	union
	{
		PVOID FiberData;
		DWORD Version;
	};
	PVOID ArbitraryUserPointer;
	NT_TIB* Self;
};
using PNT_TIB = NT_TIB*;

struct KPCR
{
	NT_TIB NtTib;
	KPCR* SelfPcr;
	KPRCB* Prcb;
	volatile KIRQL Irql;
	KPRCB PrcbData;
};
using PKPCR = KPCR*;

extern KPCR KiPcr;

using PKFLOATING_SAVE = void*;

#if defined(CONFIG_SMP)
#define KeGetPcr()              ((KPCR *)__readfsdword(offsetof(KPCR, SelfPcr)))
inline PKPRCB KeGetCurrentPrcb(VOID)
{
	return (PKPRCB)(ULONG_PTR)__readfsdword(offsetof(KPCR, Prcb));
}
#else
// xbox is a single core system so this is mostly for reactos/windows compat
// compiles down to just acccessing the global
#define KeGetPcr()              (&KiPcr)
inline PKPRCB KeGetCurrentPrcb(VOID)
{
	return &KeGetPcr()->PrcbData;
}

#endif

BOOLEAN FASTCALL KiInsertQueueApc(PKAPC Apc, KPRIORITY Increment);

#ifdef __cplusplus
extern "C" {
#endif


/*
 * @implemented
 */
EXPORTNUM(121) inline BOOLEAN NTAPI KeIsExecutingDpc(VOID)
{
    /* Return if the Dpc Routine is active */
    return KeGetCurrentPrcb()->DpcRoutineActive != 0;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(92) BOOLEAN NTAPI KeAlertResumeThread
(
	IN PKTHREAD Thread
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(93) BOOLEAN NTAPI KeAlertThread
(
	IN PKTHREAD Thread,
	IN KPROCESSOR_MODE AlertMode
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(94) VOID NTAPI KeBoostPriorityThread
(
	IN PKTHREAD Thread,
	IN KPRIORITY Increment
);

[[noreturn]] EXPORTNUM(95) VOID XBOXAPI KeBugCheck
(
	ULONG BugCheckCode
);

// This routine terminates the system. It should be used when a failure is not expected, for example those caused by bugs
[[noreturn]] EXPORTNUM(96) VOID XBOXAPI KeBugCheckEx
(
	ULONG BugCheckCode,
	ULONG_PTR BugCheckParameter1,
	ULONG_PTR BugCheckParameter2,
	ULONG_PTR BugCheckParameter3,
	ULONG_PTR BugCheckParameter4
);

EXPORTNUM(98) BOOLEAN XBOXAPI KeConnectInterrupt
(
	PKINTERRUPT  InterruptObject
);

EXPORTNUM(99) NTSTATUS XBOXAPI KeDelayExecutionThread
(
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Interval
);

EXPORTNUM(100) BOOLEAN XBOXAPI KeDisconnectInterrupt
(
	PKINTERRUPT  InterruptObject
);

EXPORTNUM(101) VOID XBOXAPI KeEnterCriticalRegion();

inline EXPORTNUM(103) KIRQL XBOXAPI KeGetCurrentIrql()
{
	return KeGetPcr()->Irql;
}

inline EXPORTNUM(104) PKTHREAD XBOXAPI KeGetCurrentThread(VOID)
{
	return KeGetCurrentPrcb()->CurrentThread;
}

EXPORTNUM(105) VOID XBOXAPI KeInitializeApc
(
	PKAPC Apc,
	PKTHREAD Thread,
	PKKERNEL_ROUTINE KernelRoutine,
	PKRUNDOWN_ROUTINE RundownRoutine,
	PKNORMAL_ROUTINE NormalRoutine,
	KPROCESSOR_MODE ApcMode,
	PVOID NormalContext
);

EXPORTNUM(106) VOID XBOXAPI KeInitializeDeviceQueue
(
	PKDEVICE_QUEUE DeviceQueue
);

EXPORTNUM(107) VOID XBOXAPI KeInitializeDpc
(
	PKDPC Dpc,
	PKDEFERRED_ROUTINE DeferredRoutine,
	PVOID DeferredContext
);

EXPORTNUM(108) VOID XBOXAPI KeInitializeEvent
(
	PKEVENT Event,
	EVENT_TYPE Type,
	BOOLEAN SignalState
);

EXPORTNUM(109) VOID XBOXAPI KeInitializeInterrupt
(
	PKINTERRUPT Interrupt,
	PKSERVICE_ROUTINE ServiceRoutine,
	PVOID ServiceContext,
	ULONG Vector,
	KIRQL Irql,
	KINTERRUPT_MODE InterruptMode,
	BOOLEAN ShareVector
);

EXPORTNUM(110) VOID XBOXAPI KeInitializeMutant
(
	PKMUTANT Mutant,
	BOOLEAN InitialOwner
);

EXPORTNUM(112) VOID XBOXAPI KeInitializeSemaphore
(
	PKSEMAPHORE Semaphore,
	LONG Count,
	LONG Limit
);

EXPORTNUM(97) BOOLEAN NTAPI KeCancelTimer(
	IN OUT PKTIMER Timer
);

EXPORTNUM(111) VOID NTAPI KeInitializeQueue
(
	IN PKQUEUE Queue,
	IN ULONG Count OPTIONAL
);

EXPORTNUM(113) VOID XBOXAPI KeInitializeTimerEx
(
	PKTIMER Timer,
	TIMER_TYPE Type
);

EXPORTNUM(114) BOOLEAN NTAPI KeInsertByKeyDeviceQueue
(
	IN PKDEVICE_QUEUE DeviceQueue,
	IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
	IN ULONG SortKey
);

EXPORTNUM(115) BOOLEAN NTAPI KeInsertDeviceQueue
(
	IN PKDEVICE_QUEUE DeviceQueue,
	IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
);

EXPORTNUM(116) LONG NTAPI KeInsertHeadQueue
(
	IN PKQUEUE Queue,
	IN PLIST_ENTRY Entry
);

EXPORTNUM(117) LONG NTAPI KeInsertQueue
(
	IN PKQUEUE Queue,
	IN PLIST_ENTRY Entry
);

/*
 * @implemented
 *
 * Returns number of entries in the queue
 */
inline LONG NTAPI KeReadStateQueue
(
	IN PKQUEUE Queue
)
{
	/* Returns the Signal State */
	//ASSERT_QUEUE(Queue);
	return Queue->Header.SignalState;
}

EXPORTNUM(118) BOOLEAN XBOXAPI KeInsertQueueApc
(
	PKAPC Apc,
	PVOID SystemArgument1,
	PVOID SystemArgument2,
	KPRIORITY Increment
);

EXPORTNUM(119) BOOLEAN XBOXAPI KeInsertQueueDpc
(
	PKDPC Dpc,
	PVOID SystemArgument1,
	PVOID SystemArgument2
);

EXPORTNUM(120) extern volatile KSYSTEM_TIME KeInterruptTime;

EXPORTNUM(122) VOID XBOXAPI KeLeaveCriticalRegion();

EXPORTNUM(123) LONG NTAPI KePulseEvent
(
	IN PKEVENT Event,
	IN KPRIORITY Increment,
	IN BOOLEAN Wait
);

EXPORTNUM(124) KPRIORITY NTAPI KeQueryBasePriorityThread
(
	IN PKTHREAD Thread
);

EXPORTNUM(125) ULONGLONG XBOXAPI KeQueryInterruptTime();

EXPORTNUM(126) ULONGLONG XBOXAPI KeQueryPerformanceCounter();

EXPORTNUM(127) ULONGLONG XBOXAPI KeQueryPerformanceFrequency();

EXPORTNUM(128) VOID XBOXAPI KeQuerySystemTime
(
	PLARGE_INTEGER CurrentTime
);

EXPORTNUM(129) KIRQL XBOXAPI KeRaiseIrqlToDpcLevel();

EXPORTNUM(130) KIRQL XBOXAPI KeRaiseIrqlToSynchLevel();

EXPORTNUM(131) LONG XBOXAPI KeReleaseMutant
(
	PKMUTANT Mutant,
	KPRIORITY Increment,
	BOOLEAN Abandoned,
	BOOLEAN Wait
);

LONG NTAPI KeReadStateSemaphore
(
	IN PKSEMAPHORE Semaphore
);

EXPORTNUM(132) LONG XBOXAPI KeReleaseSemaphore
(
	PKSEMAPHORE Semaphore,
	KPRIORITY Increment,
	LONG Adjustment,
	BOOLEAN Wait
);

EXPORTNUM(133) PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveByKeyDeviceQueue
(
	IN PKDEVICE_QUEUE DeviceQueue,
	IN ULONG SortKey
);

EXPORTNUM(134) PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveDeviceQueue
(
	IN PKDEVICE_QUEUE DeviceQueue
);

EXPORTNUM(135) BOOLEAN NTAPI KeRemoveEntryDeviceQueue
(
	IN PKDEVICE_QUEUE DeviceQueue,
	IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
);

EXPORTNUM(136) PLIST_ENTRY NTAPI KeRemoveQueue
(
	IN PKQUEUE Queue,
	IN KPROCESSOR_MODE WaitMode,
	IN PLARGE_INTEGER Timeout OPTIONAL
);

EXPORTNUM(137) BOOLEAN NTAPI KeRemoveQueueDpc
(
	IN PKDPC Dpc
);

/*++
 * @name KeRemoveQueueApc
 * @implemented NT4
 *
 *     The KeRemoveQueueApc routine removes a given APC object from the system
 *     APC queue.
 *
 * @param Apc
 *         Pointer to an initialized APC object that was queued by calling
 *         KeInsertQueueApc.
 *
 * @return TRUE if the APC Object is in the APC Queue. Otherwise, no operation
 *         is performed and FALSE is returned.
 *
 * @remarks If the given APC Object is currently queued, it is removed from the
 *          queue and any calls to the registered routines are cancelled.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN NTAPI KeRemoveQueueApc(IN PKAPC Apc);

void KeFlushQueuedDpcs();

EXPORTNUM(138) LONG NTAPI KeResetEvent
(
	IN PKEVENT Event
);

EXPORTNUM(139) NTSTATUS NTAPI KeRestoreFloatingPointState
(
	_In_ PKFLOATING_SAVE Save
);

EXPORTNUM(140) ULONG XBOXAPI KeResumeThread
(
	PKTHREAD Thread
);

EXPORTNUM(141) PLIST_ENTRY NTAPI KeRundownQueue
(
	IN PKQUEUE Queue
);

EXPORTNUM(142) NTSTATUS NTAPI KeSaveFloatingPointState
(
	_Out_ PKFLOATING_SAVE Save
);

EXPORTNUM(143) LONG NTAPI KeSetBasePriorityThread
(
	IN PKTHREAD Thread,
	IN LONG Increment
);

EXPORTNUM(144) BOOLEAN NTAPI KeSetDisableBoostThread
(
	IN OUT PKTHREAD Thread,
	IN BOOLEAN Disable
);

EXPORTNUM(145) LONG XBOXAPI KeSetEvent
(
	PKEVENT Event,
	KPRIORITY Increment,
	BOOLEAN	Wait
);

EXPORTNUM(146) VOID NTAPI KeSetEventBoostPriority
(
	IN PKEVENT Event,
	IN PKTHREAD* WaitingThread OPTIONAL
);

EXPORTNUM(147) KPRIORITY NTAPI KeSetPriorityProcess
(
	IN PKPROCESS Process,
	IN KPRIORITY BasePriority
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(148) KPRIORITY XBOXAPI KeSetPriorityThread
(
	PKTHREAD Thread,
	LONG Priority
);

EXPORTNUM(149) BOOLEAN XBOXAPI KeSetTimer
(
	PKTIMER Timer,
	LARGE_INTEGER DueTime,
	PKDPC Dpc
);

EXPORTNUM(150) BOOLEAN XBOXAPI KeSetTimerEx
(
	PKTIMER Timer,
	LARGE_INTEGER DueTime,
	LONG Period,
	PKDPC Dpc
);

EXPORTNUM(151) VOID NTAPI KeStallExecutionProcessor
(
	ULONG MicroSeconds
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(152) ULONG XBOXAPI KeSuspendThread
(
	PKTHREAD Thread
);

_Function_class_(KSYNCHRONIZE_ROUTINE)
_IRQL_requires_same_
typedef BOOLEAN
(NTAPI KSYNCHRONIZE_ROUTINE)(
	_In_ PVOID SynchronizeContext);
typedef KSYNCHRONIZE_ROUTINE* PKSYNCHRONIZE_ROUTINE;

/*
 * @implemented
 */
EXPORTNUM(153) BOOLEAN NTAPI KeSynchronizeExecution
(
	IN OUT PKINTERRUPT Interrupt,
	IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
	IN PVOID SynchronizeContext OPTIONAL
);

EXPORTNUM(154) extern volatile KSYSTEM_TIME KeSystemTime;

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(155) BOOLEAN XBOXAPI KeTestAlertThread
(
	KPROCESSOR_MODE AlertMode
);

EXPORTNUM(156) extern volatile DWORD KeTickCount;

EXPORTNUM(157) extern ULONG KeTimeIncrement;

EXPORTNUM(159) NTSTATUS XBOXAPI KeWaitForSingleObject
(
	PVOID Object,
	KWAIT_REASON WaitReason,
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
);

_IRQL_raises_(NewIrql)
_IRQL_saves_
EXPORTNUM(160) KIRQL FASTCALL KfRaiseIrql
(
	KIRQL NewIrql
);


EXPORTNUM(161) VOID FASTCALL KfLowerIrql
(
	_IRQL_restores_ KIRQL NewIrql
);

// this is called KiExitDispatcher on reactos, but we use the xbox name since we export it
EXPORTNUM(163) VOID FASTCALL KiUnlockDispatcherDatabase
(
	KIRQL NewIrql
);

EXPORTNUM(238) NTSTATUS NTAPI NtYieldExecution(VOID);

// Reactos OS name for KiUnlockDispatcherDatabase
#define KiExitDispatcher(OldIrql) KiUnlockDispatcherDatabase(OldIrql)

EXPORTNUM(321) extern XBOX_KEY_DATA XboxEEPROMKey;

#ifdef __cplusplus
}
#endif

extern XBOX_KEY_DATA XboxCERTKey;
extern ULONG KernelThunkTable[379];

VOID XBOXAPI KiSuspendNop(PKAPC Apc, PKNORMAL_ROUTINE *NormalRoutine, PVOID *NormalContext, PVOID *SystemArgument1, PVOID *SystemArgument2);
VOID XBOXAPI KiSuspendThread(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument);
VOID KeInitializeThread(PKTHREAD Thread, PVOID KernelStack, ULONG KernelStackSize, ULONG TlsDataSize, PKSYSTEM_ROUTINE SystemRoutine, PKSTART_ROUTINE StartRoutine,
	PVOID StartContext, PKPROCESS Process);

VOID XBOXAPI KeInitializeTimer(PKTIMER Timer);
VOID FASTCALL KiCheckExpiredTimers(DWORD OldKeTickCount);
VOID KeScheduleThread(PKTHREAD Thread);
VOID KiScheduleThread(PKTHREAD Thread);
VOID FASTCALL KeAddThreadToTailOfReadyList(PKTHREAD Thread);

[[noreturn]] VOID CDECL KeBugCheckLogEip(ULONG BugCheckCode);
// place at top of stack to catch superious returns
[[noreturn]] VOID CDECL KeBugCheckNoReturnGuard
(
	ULONG_PTR BugCheckParameter1,
	ULONG_PTR BugCheckParameter2,
	ULONG_PTR BugCheckParameter3,
	ULONG_PTR BugCheckParameter4
);

// no need for memory barriers on a single core/single cpu system
#include <intrin.h>
#define KeMemoryBarrier() _ReadWriteBarrier()
#define KeMemoryBarrierWithoutFence() _ReadWriteBarrier()

// not exported on XBOX, we still want this for easier reactos/windows driver compatibility
static inline VOID KeRaiseIrql(KIRQL NewIrql,
	PKIRQL OldIrql)
{
	/* Call the fastcall function */
	*OldIrql = KfRaiseIrql(NewIrql);
};
// exactly the same as KfLowerIrql, as above, reactos/windows compat
#define KeLowerIrql(new_irql) KfLowerIrql(new_irql)

// no-op on Pentium 3, fancy no-op on later CPUs
#define YieldProcessor _mm_pause

// currently unexported spinlock functions, esentially no-op and for debugging but kept in ke* to match
// reactos/windows
// may be exported as functions beyond the standard xbox API at some point

void NTAPI KeInitializeSpinLock(PKSPIN_LOCK SpinLock);

PKDEVICE_QUEUE_ENTRY NTAPI KeRemoveByKeyDeviceQueueIfBusy
(
	IN PKDEVICE_QUEUE DeviceQueue,
	IN ULONG SortKey
);

_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
KIRQL
FASTCALL
KfAcquireSpinLock(
	_Inout_ PKSPIN_LOCK SpinLock);
#define KeAcquireSpinLock(a,b) *(b) = KfAcquireSpinLock(a)


_Requires_lock_held_(*SpinLock)
_Releases_lock_(*SpinLock)
_IRQL_requires_(DISPATCH_LEVEL)
VOID
FASTCALL
KfReleaseSpinLock(
	_Inout_ PKSPIN_LOCK SpinLock,
	_In_ _IRQL_restores_ KIRQL NewIrql);
#define KeReleaseSpinLock(a,b) KfReleaseSpinLock(a,b)


_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_requires_min_(DISPATCH_LEVEL)
VOID
NTAPI
KeAcquireSpinLockAtDpcLevel(
	_Inout_ PKSPIN_LOCK SpinLock);

_Requires_lock_held_(*SpinLock)
_Releases_lock_(*SpinLock)
_IRQL_requires_min_(DISPATCH_LEVEL)
VOID
NTAPI
KeReleaseSpinLockFromDpcLevel(
	_Inout_ PKSPIN_LOCK SpinLock);

_Must_inspect_result_
_IRQL_requires_min_(DISPATCH_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(
	_Inout_ _Requires_lock_not_held_(*_Curr_)
	_When_(return != 0, _Acquires_lock_(*_Curr_))
	PKSPIN_LOCK SpinLock);

// SMP: change this
struct KLOCK_QUEUE_HANDLE
{
	KIRQL OldIRQL;
};

/* Diable interrupts and return whether they were enabled before */
static inline BOOLEAN KeDisableInterrupts()
{
	ULONG Flags;
	BOOLEAN Return;

	/* Get EFLAGS and check if the interrupt bit is set */
	Flags = __readeflags();
	Return = (Flags & EFLAGS_INTERRUPT_MASK) ? TRUE : FALSE;

	/* Disable interrupts */
	_disable();
	return Return;
}

/* Restore previous interrupt state */
static inline void KeRestoreInterrupts(BOOLEAN WereEnabled)
{
	if (WereEnabled) _enable();
}

