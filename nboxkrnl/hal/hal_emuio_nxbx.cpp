#include  "hal.hpp"
#include  "halp_nxbx.hpp"
#include "ex.hpp"
#include "halp_nxbx_emuio.hpp"

static_assert(sizeof(IoRequest) == 44);
static_assert(sizeof(IoInfoBlockOc) == 36);

static LONG IoHostFileHandle = FIRST_FREE_HANDLE;

static NTSTATUS HostToNtStatus(IoStatus Status)
{
	switch (Status)
	{
	case IoStatus::Success:
		return STATUS_SUCCESS;

	case IoStatus::Pending:
		return STATUS_PENDING;

	case IoStatus::Error:
		return STATUS_IO_DEVICE_ERROR;

	case IoStatus::Failed:
		return STATUS_ACCESS_DENIED;

	case IoStatus::IsADirectory:
		return STATUS_FILE_IS_A_DIRECTORY;

	case IoStatus::NotADirectory:
		return STATUS_NOT_A_DIRECTORY;

	case IoStatus::NameNotFound:
		return STATUS_OBJECT_NAME_NOT_FOUND;

	case IoStatus::PathNotFound:
		return STATUS_OBJECT_PATH_NOT_FOUND;

	case IoStatus::Corrupt:
		return STATUS_FILE_CORRUPT_ERROR;

	case IoStatus::Full:
		return STATUS_DISK_FULL;

	case CannotDelete:
		return STATUS_CANNOT_DELETE;

	case NotEmpty:
		return STATUS_DIRECTORY_NOT_EMPTY;
	}

	KeBugCheckLogEip(UNREACHABLE_CODE_REACHED);
}

inline void InfoBlockProcess(IoInfoBlock& Block)
{
	// convert status to xbox status
	const IoStatus HostStatus = Block.HostStatus;
	Block.Status = HostToNtStatus(HostStatus);
}

IoInfoBlock SubmitIoRequestToHost(ULONG Type, ULONG Handle)
{
	IoInfoBlockOc InfoBlockOc;
	IoRequest Packet;
	Packet.Header.Id = InterlockedIncrement(&IoHostFileHandle);
	Packet.Header.Type = Type;
	Packet.m_xx.Handle = Handle;
	SubmitIoRequestToHost(&Packet);
	RetrieveIoRequestFromHost(&InfoBlockOc, Packet.Header.Id);

	InfoBlockProcess(InfoBlockOc.Header);
	return InfoBlockOc.Header;
}

IoInfoBlock SubmitIoRequestToHost(ULONG Type, LONGLONG Offset, ULONG Size, ULONG Address, ULONG Handle)
{
	IoInfoBlockOc InfoBlockOc;
	IoRequest Packet;
	Packet.Header.Id = InterlockedIncrement(&IoHostFileHandle);
	Packet.Header.Type = Type;
	Packet.m_rw.Offset = Offset;
	Packet.m_rw.Size = Size;
	Packet.m_rw.Address = Address;
	Packet.m_rw.Handle = Handle;
	Packet.m_rw.Timestamp = 0;
	SubmitIoRequestToHost(&Packet);
	RetrieveIoRequestFromHost(&InfoBlockOc, Packet.Header.Id);

	InfoBlockProcess(InfoBlockOc.Header);
	return InfoBlockOc.Header;
}

IoInfoBlock SubmitIoRequestToHost(ULONG Type, LONGLONG Offset, ULONG Size, ULONG Address, ULONG Handle, ULONG Timestamp)
{
	IoInfoBlockOc InfoBlockOc;
	IoRequest Packet;
	Packet.Header.Id = InterlockedIncrement(&IoHostFileHandle);
	Packet.Header.Type = Type;
	Packet.m_rw.Offset = Offset;
	Packet.m_rw.Size = Size;
	Packet.m_rw.Address = Address;
	Packet.m_rw.Handle = Handle;
	Packet.m_rw.Timestamp = Timestamp;
	SubmitIoRequestToHost(&Packet);
	RetrieveIoRequestFromHost(&InfoBlockOc, Packet.Header.Id);

	InfoBlockOc.Header.Status = HostToNtStatus(InfoBlockOc.Header.HostStatus);

	InfoBlockProcess(InfoBlockOc.Header);
	return InfoBlockOc.Header;
}

IoInfoBlockOc SubmitIoRequestToHost(ULONG Type, LONGLONG InitialSize, ULONG Size, ULONG Handle, ULONG Path, ULONG Attributes,
	ULONG Timestamp, ULONG DesiredAccess, ULONG CreateOptions)
{
	IoInfoBlockOc InfoBlock;
	IoRequest Packet;
	Packet.Header.Id = InterlockedIncrement(&IoHostFileHandle);
	Packet.Header.Type = Type;
	Packet.m_oc.InitialSize = InitialSize;
	Packet.m_oc.Size = Size;
	Packet.m_oc.Handle = Handle;
	Packet.m_oc.Path = Path;
	Packet.m_oc.Attributes = Attributes;
	Packet.m_oc.Timestamp = Timestamp;
	Packet.m_oc.DesiredAccess = DesiredAccess;
	Packet.m_oc.CreateOptions = CreateOptions;
	SubmitIoRequestToHost(&Packet);
	RetrieveIoRequestFromHost(&InfoBlock, Packet.Header.Id);

	InfoBlockProcess(InfoBlock.Header);
	return InfoBlock;
}
