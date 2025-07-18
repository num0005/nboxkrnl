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

#define IO_NO_INCREMENT        0
#define THREAD_ALERT_INCREMENT 2

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

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN NTAPI EXPORTNUM(92) KeAlertResumeThread
(
	IN PKTHREAD Thread
);

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN NTAPI EXPORTNUM(93) KeAlertThread
(
	IN PKTHREAD Thread,
	IN KPROCESSOR_MODE AlertMode
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID NTAPI EXPORTNUM(94) KeBoostPriorityThread
(
	IN PKTHREAD Thread,
	IN KPRIORITY Increment
);

[[noreturn]] EXPORTNUM(95) DLLEXPORT VOID XBOXAPI KeBugCheck
(
	ULONG BugCheckCode
);

// This routine terminates the system. It should be used when a failure is not expected, for example those caused by bugs
[[noreturn]] EXPORTNUM(96) DLLEXPORT VOID XBOXAPI KeBugCheckEx
(
	ULONG BugCheckCode,
	ULONG_PTR BugCheckParameter1,
	ULONG_PTR BugCheckParameter2,
	ULONG_PTR BugCheckParameter3,
	ULONG_PTR BugCheckParameter4
);

EXPORTNUM(98) DLLEXPORT BOOLEAN XBOXAPI KeConnectInterrupt
(
	PKINTERRUPT  InterruptObject
);

EXPORTNUM(99) DLLEXPORT NTSTATUS XBOXAPI KeDelayExecutionThread
(
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Interval
);

EXPORTNUM(100) BOOLEAN XBOXAPI KeDisconnectInterrupt
(
	PKINTERRUPT  InterruptObject
);

EXPORTNUM(101) DLLEXPORT VOID XBOXAPI KeEnterCriticalRegion();

inline EXPORTNUM(103) DLLEXPORT KIRQL XBOXAPI KeGetCurrentIrql()
{
	return KeGetPcr()->Irql;
}

inline EXPORTNUM(104) DLLEXPORT PKTHREAD XBOXAPI KeGetCurrentThread(VOID)
{
	return KeGetCurrentPrcb()->CurrentThread;
}

EXPORTNUM(105) DLLEXPORT VOID XBOXAPI KeInitializeApc
(
	PKAPC Apc,
	PKTHREAD Thread,
	PKKERNEL_ROUTINE KernelRoutine,
	PKRUNDOWN_ROUTINE RundownRoutine,
	PKNORMAL_ROUTINE NormalRoutine,
	KPROCESSOR_MODE ApcMode,
	PVOID NormalContext
);

EXPORTNUM(106) DLLEXPORT VOID XBOXAPI KeInitializeDeviceQueue
(
	PKDEVICE_QUEUE DeviceQueue
);

EXPORTNUM(107) DLLEXPORT VOID XBOXAPI KeInitializeDpc
(
	PKDPC Dpc,
	PKDEFERRED_ROUTINE DeferredRoutine,
	PVOID DeferredContext
);

EXPORTNUM(108) DLLEXPORT VOID XBOXAPI KeInitializeEvent
(
	PKEVENT Event,
	EVENT_TYPE Type,
	BOOLEAN SignalState
);

EXPORTNUM(109) DLLEXPORT VOID XBOXAPI KeInitializeInterrupt
(
	PKINTERRUPT Interrupt,
	PKSERVICE_ROUTINE ServiceRoutine,
	PVOID ServiceContext,
	ULONG Vector,
	KIRQL Irql,
	KINTERRUPT_MODE InterruptMode,
	BOOLEAN ShareVector
);

EXPORTNUM(110) DLLEXPORT VOID XBOXAPI KeInitializeMutant
(
	PKMUTANT Mutant,
	BOOLEAN InitialOwner
);

EXPORTNUM(112) DLLEXPORT VOID XBOXAPI KeInitializeSemaphore
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

EXPORTNUM(113) DLLEXPORT VOID XBOXAPI KeInitializeTimerEx
(
	PKTIMER Timer,
	TIMER_TYPE Type
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

EXPORTNUM(118) DLLEXPORT BOOLEAN XBOXAPI KeInsertQueueApc
(
	PKAPC Apc,
	PVOID SystemArgument1,
	PVOID SystemArgument2,
	KPRIORITY Increment
);

EXPORTNUM(119) DLLEXPORT BOOLEAN XBOXAPI KeInsertQueueDpc
(
	PKDPC Dpc,
	PVOID SystemArgument1,
	PVOID SystemArgument2
);

EXPORTNUM(120) DLLEXPORT extern volatile KSYSTEM_TIME KeInterruptTime;

EXPORTNUM(122) DLLEXPORT VOID XBOXAPI KeLeaveCriticalRegion();

EXPORTNUM(125) DLLEXPORT ULONGLONG XBOXAPI KeQueryInterruptTime();

EXPORTNUM(126) DLLEXPORT ULONGLONG XBOXAPI KeQueryPerformanceCounter();

EXPORTNUM(127) DLLEXPORT ULONGLONG XBOXAPI KeQueryPerformanceFrequency();

EXPORTNUM(128) DLLEXPORT VOID XBOXAPI KeQuerySystemTime
(
	PLARGE_INTEGER CurrentTime
);

EXPORTNUM(129) DLLEXPORT KIRQL XBOXAPI KeRaiseIrqlToDpcLevel();

EXPORTNUM(130) DLLEXPORT KIRQL XBOXAPI KeRaiseIrqlToSynchLevel();

EXPORTNUM(131) DLLEXPORT LONG XBOXAPI KeReleaseMutant
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

EXPORTNUM(132) DLLEXPORT LONG XBOXAPI KeReleaseSemaphore
(
	PKSEMAPHORE Semaphore,
	KPRIORITY Increment,
	LONG Adjustment,
	BOOLEAN Wait
);

EXPORTNUM(136) PLIST_ENTRY NTAPI KeRemoveQueue
(
	IN PKQUEUE Queue,
	IN KPROCESSOR_MODE WaitMode,
	IN PLARGE_INTEGER Timeout OPTIONAL
);

EXPORTNUM(140) DLLEXPORT ULONG XBOXAPI KeResumeThread
(
	PKTHREAD Thread
);

EXPORTNUM(141) PLIST_ENTRY NTAPI KeRundownQueue
(
	IN PKQUEUE Queue
);

EXPORTNUM(145) DLLEXPORT LONG XBOXAPI KeSetEvent
(
	PKEVENT Event,
	KPRIORITY Increment,
	BOOLEAN	Wait
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(148) DLLEXPORT KPRIORITY XBOXAPI KeSetPriorityThread
(
	PKTHREAD Thread,
	LONG Priority
);

EXPORTNUM(149) DLLEXPORT BOOLEAN XBOXAPI KeSetTimer
(
	PKTIMER Timer,
	LARGE_INTEGER DueTime,
	PKDPC Dpc
);

EXPORTNUM(150) DLLEXPORT BOOLEAN XBOXAPI KeSetTimerEx
(
	PKTIMER Timer,
	LARGE_INTEGER DueTime,
	LONG Period,
	PKDPC Dpc
);

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(152) DLLEXPORT ULONG XBOXAPI KeSuspendThread
(
	PKTHREAD Thread
);

EXPORTNUM(154) DLLEXPORT extern volatile KSYSTEM_TIME KeSystemTime;

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORTNUM(155) DLLEXPORT BOOLEAN XBOXAPI KeTestAlertThread
(
	KPROCESSOR_MODE AlertMode
);

EXPORTNUM(156) DLLEXPORT extern volatile DWORD KeTickCount;

EXPORTNUM(157) DLLEXPORT extern ULONG KeTimeIncrement;

EXPORTNUM(159) DLLEXPORT NTSTATUS XBOXAPI KeWaitForSingleObject
(
	PVOID Object,
	KWAIT_REASON WaitReason,
	KPROCESSOR_MODE WaitMode,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
);

_IRQL_raises_(NewIrql)
_IRQL_saves_
EXPORTNUM(160) DLLEXPORT KIRQL FASTCALL KfRaiseIrql
(
	KIRQL NewIrql
);


EXPORTNUM(161) DLLEXPORT VOID FASTCALL KfLowerIrql
(
	_IRQL_restores_ KIRQL NewIrql
);

// this is called KiExitDispatcher on reactos, but we use the xbox name since we export it
EXPORTNUM(163) DLLEXPORT VOID FASTCALL KiUnlockDispatcherDatabase
(
	KIRQL NewIrql
);

EXPORTNUM(238) NTSTATUS NTAPI NtYieldExecution(VOID);

// Reactos OS name for KiUnlockDispatcherDatabase
#define KiExitDispatcher(OldIrql) KiUnlockDispatcherDatabase(OldIrql)

EXPORTNUM(321) DLLEXPORT extern XBOX_KEY_DATA XboxEEPROMKey;

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