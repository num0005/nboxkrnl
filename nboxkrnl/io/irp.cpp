#include "io.hpp"
#include "iop.hpp"
#include "rtl.hpp"
#include "ps.hpp"

inline VOID FASTCALL IofQueueIrpToThread
(
    IN PIRP Irp
)
{
    PETHREAD Thread = Irp->Tail.Overlay.Thread;

    /* Disable *all* kernel APCs so we can't race with IopCompleteRequest.
     * IRP's thread must be the current thread */
    KIRQL OldIrql = KfRaiseIrql(APC_LEVEL);

    /* Insert it into the list */
    InsertHeadList(&Thread->IrpList, &Irp->ThreadListEntry);

    /* restore the old IRQL */
    KfLowerIrql(OldIrql);
}

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
)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize);
    if (!Irp) return NULL;

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Write the Major function and then deal with it */
    StackPtr->MajorFunction = (UCHAR)MajorFunction;

    /* Do not handle the following here */
    if ((MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
        (MajorFunction != IRP_MJ_SHUTDOWN)) //&&
        //(MajorFunction != IRP_MJ_PNP) &&
        //(MajorFunction != IRP_MJ_POWER))
    {
        NT_ASSERT((DeviceObject->Flags & DO_DIRECT_IO) == 0);
        if (DeviceObject->Flags & DO_DIRECT_IO)
            KeBugCheckLogEip(UNREACHABLE_CODE_REACHED);
        #if 0
        /* Check if this is Buffered IO */
        if (DeviceObject->Flags & DO_BUFFERED_IO)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPool, Length, TAG_IOBUF);
            if (!Irp->AssociatedIrp.SystemBuffer)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Set flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;

            /* Handle special IRP_MJ_WRITE Case */
            if (MajorFunction == IRP_MJ_WRITE)
            {
                /* Copy the buffer data */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
            }
            else
            {
                /* Set the Input Operation flag and set this as a User Buffer */
                Irp->Flags |= IRP_INPUT_OPERATION;
                Irp->UserBuffer = Buffer;
            }
        }
        else if (DeviceObject->Flags & DO_DIRECT_IO)
        {
            /* Use an MDL for Direct I/O */
            Irp->MdlAddress = IoAllocateMdl(Buffer,
                Length,
                FALSE,
                FALSE,
                NULL);
            if (!Irp->MdlAddress)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Probe and Lock */
            _SEH2_TRY
            {
                /* Do the probe */
                MmProbeAndLockPages(Irp->MdlAddress,
                                    KernelMode,
                                    MajorFunction == IRP_MJ_READ ?
                                    IoWriteAccess : IoReadAccess);
            }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Free the IRP and its MDL */
                IoFreeMdl(Irp->MdlAddress);
                IoFreeIrp(Irp);

                /* Fail */
                _SEH2_YIELD(return NULL);
            }
            _SEH2_END;
        }
        else
        #endif
        {
            /* Neither, use the buffer */
            Irp->UserBuffer = Buffer;
        }

        /* Check if this is a read */
        if (MajorFunction == IRP_MJ_READ)
        {
            /* Set the parameters for a read */
            StackPtr->Parameters.Read.Length = Length;
            StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
        }
        else if (MajorFunction == IRP_MJ_WRITE)
        {
            /* Otherwise, set write parameters */
            StackPtr->Parameters.Write.Length = Length;
            StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
        }
    }

    /* Set the Current Thread and IOSB */
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Return the IRP */
    IOTRACE(IO_IRP_DEBUG,
        "%s - Built IRP %p with Major, Buffer, DO %lx %p %p\n",
        __FUNCTION__,
        Irp,
        MajorFunction,
        Buffer,
        DeviceObject);
    return Irp;
}

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
)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    ULONG BufferLength;

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize);
    if (!Irp) return NULL;

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Set the DevCtl Type */
    StackPtr->MajorFunction = InternalDeviceIoControl ?
        IRP_MJ_INTERNAL_DEVICE_CONTROL :
        IRP_MJ_DEVICE_CONTROL;

/* Set the IOCTL Data */
    StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    StackPtr->Parameters.DeviceIoControl.OutputBufferLength =
        OutputBufferLength;

    /* Handle the Methods */
    #if 0
    switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
    {
        /* Buffered I/O */
    case METHOD_BUFFERED:

        /* Select the right Buffer Length */
        BufferLength = InputBufferLength > OutputBufferLength ?
            InputBufferLength : OutputBufferLength;

/* Make sure there is one */
        if (BufferLength)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPool,
                    BufferLength,
                    TAG_IOBUF);
            if (!Irp->AssociatedIrp.SystemBuffer)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Check if we got a buffer */
            if (InputBuffer)
            {
                /* Copy into the System Buffer */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                    InputBuffer,
                    InputBufferLength);
            }

            /* Write the flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            if (OutputBuffer) Irp->Flags |= IRP_INPUT_OPERATION;

            /* Save the Buffer */
            Irp->UserBuffer = OutputBuffer;
        }
        else
        {
            /* Clear the Flags and Buffer */
            Irp->Flags = 0;
            Irp->UserBuffer = NULL;
        }
        break;

    /* Direct I/O */
    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:

        /* Check if we got an input buffer */
        if (InputBuffer)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPool,
                    InputBufferLength,
                    TAG_IOBUF);
            if (!Irp->AssociatedIrp.SystemBuffer)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Copy into the System Buffer */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                InputBuffer,
                InputBufferLength);

  /* Write the flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        }
        else
        {
            /* Clear the flags */
            Irp->Flags = 0;
        }

        /* Check if we got an output buffer */
        if (OutputBuffer)
        {
            /* Allocate the System Buffer */
            Irp->MdlAddress = IoAllocateMdl(OutputBuffer,
                OutputBufferLength,
                FALSE,
                FALSE,
                Irp);
            if (!Irp->MdlAddress)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Probe and Lock */
            _SEH2_TRY
            {
                /* Do the probe */
                MmProbeAndLockPages(Irp->MdlAddress,
                                    KernelMode,
                                    IO_METHOD_FROM_CTL_CODE(IoControlCode) ==
                                    METHOD_IN_DIRECT ?
                                    IoReadAccess : IoWriteAccess);
            }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Free the MDL */
                IoFreeMdl(Irp->MdlAddress);

                /* Free the input buffer and IRP */
                if (InputBuffer) ExFreePool(Irp->AssociatedIrp.SystemBuffer);
                IoFreeIrp(Irp);

                /* Fail */
                _SEH2_YIELD(return NULL);
            }
            _SEH2_END;
        }
        break;

    case METHOD_NEITHER:

        /* Just save the Buffer */
        Irp->UserBuffer = OutputBuffer;
        StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }
    #endif

    /* Now write the Event and IoSB */
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IofQueueIrpToThread(Irp);

    /* Return the IRP */
    IOTRACE(IO_IRP_DEBUG,
        "%s - Built IRP %p with IOCTL, Buffers, DO %lx %p %p %p\n",
        __FUNCTION__,
        Irp,
        IoControlCode,
        InputBuffer,
        OutputBuffer,
        DeviceObject);
    return Irp;
}

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
)
{
    PIRP Irp;

    /* Do the big work to set up the IRP */
    Irp = IoBuildAsynchronousFsdRequest(MajorFunction,
        DeviceObject,
        Buffer,
        Length,
        StartingOffset,
        IoStatusBlock);
    if (!Irp) return NULL;

    /* Set the Event which makes it Syncronous */
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    IofQueueIrpToThread(Irp);
    return Irp;
}

EXPORTNUM(77) VOID XBOXAPI IoQueueThreadIrp
(
    IN PIRP Irp
)
{
    IofQueueIrpToThread(Irp);
}

EXPORTNUM(359) VOID XBOXAPI IoMarkIrpMustComplete
(
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION NextIrpStackPointer = IoGetNextIrpStackLocation(Irp);
    NextIrpStackPointer->Control |= SL_MUST_COMPLETE;
}
