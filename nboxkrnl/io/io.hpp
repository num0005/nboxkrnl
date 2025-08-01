/*
 * ergo720                Copyright (c) 2023
 */

#pragma once

#include "iop.hpp"
#include "ob.hpp"


#define IoLock() KeRaiseIrqlToDpcLevel()
#define IoUnlock(OldIrql) KfLowerIrql(OldIrql)

#define IO_CHECK_CREATE_PARAMETERS 0x0200

#define ACCESS_SYSTEM_SECURITY 0x01000000L

#define FILE_VALID_OPTION_FLAGS 0x00ffffff

#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_WRITE_THROUGH              0x00000002
#define FILE_SEQUENTIAL_ONLY            0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_CREATE_TREE_CONNECTION     0x00000080
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_NO_EA_KNOWLEDGE            0x00000200
#define FILE_OPEN_FOR_RECOVERY          0x00000400
#define FILE_RANDOM_ACCESS              0x00000800
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_BY_FILE_ID            0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000
#define FILE_NO_COMPRESSION             0x00008000
#define FILE_RESERVE_OPFILTER           0x00100000
#define FILE_OPEN_REPARSE_POINT         0x00200000
#define FILE_OPEN_NO_RECALL             0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

#define FILE_SHARE_READ         0x00000001
#define FILE_SHARE_WRITE        0x00000002
#define FILE_SHARE_DELETE       0x00000004
#define FILE_SHARE_VALID_FLAGS  0x00000007

#define DELETE        0x00010000L
#define READ_CONTROL  0x00020000L
#define WRITE_DAC     0x00040000L
#define WRITE_OWNER   0x00080000L
#define SYNCHRONIZE   0x00100000L

#define FILE_SUPERSEDE            0x00000000
#define FILE_OPEN                 0x00000001
#define FILE_CREATE               0x00000002
#define FILE_OPEN_IF              0x00000003
#define FILE_OVERWRITE            0x00000004
#define FILE_OVERWRITE_IF         0x00000005
#define FILE_MAXIMUM_DISPOSITION  FILE_OVERWRITE_IF

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)
#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define FILE_READ_DATA         0x0001
#define FILE_LIST_DIRECTORY    0x0001
#define FILE_WRITE_DATA        0x0002
#define FILE_ADD_FILE          0x0002
#define FILE_APPEND_DATA       0x0004
#define FILE_ADD_SUBDIRECTORY  0x0004
#define FILE_EXECUTE           0x0020
#define FILE_TRAVERSE          0x0020
#define FILE_DELETE_CHILD      0x0040
#define FILE_READ_ATTRIBUTES   0x0080
#define FILE_WRITE_ATTRIBUTES  0x0100
#define FILE_READ_EA           0x0008
#define FILE_WRITE_EA          0x0010
#define FILE_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define GENERIC_READ     0x80000000L
#define GENERIC_WRITE    0x40000000L
#define GENERIC_EXECUTE  0x20000000L
#define GENERIC_ALL      0x10000000L

#define FILE_SUPERSEDED      0x00000000
#define FILE_OPENED          0x00000001
#define FILE_CREATED         0x00000002
#define FILE_OVERWRITTEN     0x00000003
#define FILE_EXISTS          0x00000004
#define FILE_DOES_NOT_EXIST  0x00000005

#define FILE_ATTRIBUTE_READONLY      0x00000001
#define FILE_ATTRIBUTE_HIDDEN        0x00000002
#define FILE_ATTRIBUTE_SYSTEM        0x00000004
#define FILE_ATTRIBUTE_DIRECTORY     0x00000010
#define FILE_ATTRIBUTE_ARCHIVE       0x00000020
#define FILE_ATTRIBUTE_NORMAL        0x00000080
#define FILE_ATTRIBUTE_VALID_FLAGS   0x00007FB7


struct GENERIC_MAPPING {
	ACCESS_MASK GenericRead;
	ACCESS_MASK GenericWrite;
	ACCESS_MASK GenericExecute;
	ACCESS_MASK GenericAll;
};
using PGENERIC_MAPPING = GENERIC_MAPPING *;

inline GENERIC_MAPPING IoFileMapping = {
	STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE,
	STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_EXECUTE,
	FILE_ALL_ACCESS
};

#ifdef __cplusplus
extern "C" {
#endif

EXPORTNUM(59) PIRP XBOXAPI IoAllocateIrp
(
	CCHAR StackSize
);

/*
 * @implemented
 */
EXPORTNUM(60) PIRP NTAPI IoBuildAsynchronousFsdRequest
(
	IN ULONG MajorFunction,
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER StartingOffset,
	IN PIO_STATUS_BLOCK IoStatusBlock
);

/*
 * @implemented
 */
EXPORTNUM(61) PIRP NTAPI IoBuildDeviceIoControlRequest
(
	IN ULONG IoControlCode,
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	IN PVOID OutputBuffer,
	IN ULONG OutputBufferLength,
	IN BOOLEAN InternalDeviceIoControl,
	IN PKEVENT Event,
	IN PIO_STATUS_BLOCK IoStatusBlock
);

/*
 * @implemented
 */
EXPORTNUM(62) PIRP NTAPI IoBuildSynchronousFsdRequest
(
	IN ULONG MajorFunction,
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER StartingOffset,
	IN PKEVENT Event,
	IN PIO_STATUS_BLOCK IoStatusBlock
);



EXPORTNUM(63) NTSTATUS XBOXAPI IoCheckShareAccess
(
	ACCESS_MASK DesiredAccess,
	ULONG DesiredShareAccess,
	PFILE_OBJECT FileObject,
	PSHARE_ACCESS ShareAccess,
	BOOLEAN Update
);

EXPORTNUM(64) extern OBJECT_TYPE IoCompletionObjectType;

EXPORTNUM(65) NTSTATUS XBOXAPI IoCreateDevice
(
	PDRIVER_OBJECT DriverObject,
	ULONG DeviceExtensionSize,
	PSTRING DeviceName,
	ULONG DeviceType,
	BOOLEAN Exclusive,
	PDEVICE_OBJECT *DeviceObject
);

EXPORTNUM(66) NTSTATUS XBOXAPI IoCreateFile
(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG Disposition,
	ULONG CreateOptions,
	ULONG Options
);

EXPORTNUM(67) NTSTATUS XBOXAPI IoCreateSymbolicLink
(
	PSTRING SymbolicLinkName,
	PSTRING DeviceName
);

EXPORTNUM(68) VOID XBOXAPI IoDeleteDevice
(
	PDEVICE_OBJECT DeviceObject
);

/*
 * @implemented
 */
EXPORTNUM(69) NTSTATUS NTAPI IoDeleteSymbolicLink
(
	IN PSTRING SymbolicLinkName
);

EXPORTNUM(70) extern OBJECT_TYPE IoDeviceObjectType;

EXPORTNUM(71) extern OBJECT_TYPE IoFileObjectType;

EXPORTNUM(72) VOID XBOXAPI IoFreeIrp
(
	PIRP Irp
);

EXPORTNUM(73) VOID XBOXAPI IoInitializeIrp
(
	PIRP Irp,
	USHORT PacketSize,
	CCHAR StackSize
);

EXPORTNUM(74) NTSTATUS XBOXAPI IoInvalidDeviceRequest
(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
);

EXPORTNUM(75) NTSTATUS NTAPI IoQueryFileInformation
(
	IN PFILE_OBJECT FileObject,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN ULONG Length,
	OUT PVOID FileInformation,
	OUT PULONG ReturnedLength
);

EXPORTNUM(76) NTSTATUS NTAPI IoQueryVolumeInformation
(
	IN PFILE_OBJECT FileObject,
	IN FS_INFORMATION_CLASS FsInformationClass,
	IN ULONG Length,
	OUT PVOID FsInformation,
	OUT PULONG ReturnedLength
);

EXPORTNUM(77) VOID XBOXAPI IoQueueThreadIrp
(
	IN PIRP Irp
);

EXPORTNUM(78) VOID XBOXAPI IoRemoveShareAccess
(
	PFILE_OBJECT FileObject,
	PSHARE_ACCESS ShareAccess
);

EXPORTNUM(79) NTSTATUS NTAPI IoSetIoCompletion
(
	IN PVOID IoCompletion,
	IN PVOID KeyContext,
	IN PVOID ApcContext,
	IN NTSTATUS IoStatus,
	IN ULONG_PTR IoStatusInformation
);

EXPORTNUM(80) VOID XBOXAPI IoSetShareAccess
(
	ULONG DesiredAccess,
	ULONG DesiredShareAccess,
	PFILE_OBJECT FileObject,
	PSHARE_ACCESS ShareAccess
);

EXPORTNUM(81) VOID NTAPI IoStartNextPacket
(
	IN PDEVICE_OBJECT DeviceObject
);

EXPORTNUM(82) VOID NTAPI IoStartNextPacketByKey
(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG Key
);

EXPORTNUM(83) VOID NTAPI IoStartPacket
(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PULONG Key
);

EXPORTNUM(84) NTSTATUS NTAPI IoSynchronousDeviceIoControlRequest
(
	IN ULONG IoControlCode,
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnedOutputBufferLength OPTIONAL,
	IN BOOLEAN InternalDeviceIoControl
);

EXPORTNUM(85) NTSTATUS NTAPI IoSynchronousFsdRequest
(
	IN ULONG          MajorFunction,
	IN PDEVICE_OBJECT DeviceObject,
	OUT PVOID         Buffer OPTIONAL,
	IN ULONG          Length,
	IN PLARGE_INTEGER StartingOffset OPTIONAL
);

EXPORTNUM(86) NTSTATUS FASTCALL IofCallDriver
(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
);

#define IoCallDriver(DeviceObject, Irp) IofCallDriver(DeviceObject, Irp)

EXPORTNUM(87) VOID FASTCALL IofCompleteRequest
(
	PIRP Irp,
	CCHAR PriorityBoost
);

EXPORTNUM(90) NTSTATUS NTAPI IoDismountVolume
(
	IN PDEVICE_OBJECT DeviceObject
);

EXPORTNUM(91) NTSTATUS NTAPI IoDismountVolumeByName
(
	IN PSTRING VolumeName
);

EXPORTNUM(359) VOID XBOXAPI IoMarkIrpMustComplete
(
	IN PIRP Irp
);

#ifdef __cplusplus
}
#endif

BOOLEAN IoInitSystem();
NTSTATUS XBOXAPI IoParseDevice(PVOID ParseObject, POBJECT_TYPE ObjectType, ULONG Attributes, POBJECT_STRING Name, POBJECT_STRING RemainingName,
	PVOID Context, PVOID *Object);
PIRP IoAllocateIrpNoFail(CCHAR StackSize);
