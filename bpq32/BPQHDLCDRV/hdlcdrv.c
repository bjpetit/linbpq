/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

	HDLC Card Driver Module for BPQ32 Switch

	Based on WINDDK Sample Generic Port I/O driver for Windows 2000

	Switch calls via IOCIL requests under semaphore, so are single threaded,
	but if queues are also accessed by interrupt routines we need to protect them.

	One Device Instance handles all cards. All IO and interrupt resources must be 
	allocated in the .inf file.


--*/

#include <stdio.h>
#include <STDARG.H>

#include "hdlcdrv.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, GpdAddDevice)
#pragma alloc_text (PAGE, GpdDispatchPnp)
#pragma alloc_text (PAGE, GpdDispatchPower)
#pragma alloc_text (PAGE, GpdDispatchSystemControl)
#pragma alloc_text (PAGE, GpdUnload)
#pragma alloc_text (PAGE, GpdDispatch)
#pragma alloc_text (PAGE, GpdIoctlReadPort)
#pragma alloc_text (PAGE, GpdIoctlWritePort)
#pragma alloc_text (PAGE, GpdStartDevice)
#endif

// Save area for Channel Structures so they may be released on close

int AllocatedChannels = 0;			// Counts number allocated

ULONG AllocatedInterrupts=0;
ULONG AllocatedPorts[32];			// Bitmap of IO Addresses Assigned to us (1024 bits)

VOID PushBUFEntry(PLIST_ENTRY ListHead, PBUF_ENTRY Entry, PLOCAL_DEVICE_INFO deviceInfo)
{
    ExInterlockedInsertTailList(ListHead, &Entry->ListEntry, &deviceInfo->QueueSpinLock);
}

PBUF_ENTRY PopBUFEntry(PLIST_ENTRY ListHead, PLOCAL_DEVICE_INFO deviceInfo)
{
    PLIST_ENTRY ListEntry;
    ListEntry = ExInterlockedRemoveHeadList(ListHead, &deviceInfo->QueueSpinLock);
	if (ListEntry == NULL) return NULL;
    return CONTAINING_RECORD(ListEntry, BUF_ENTRY, ListEntry);
}



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS

--*/
{

    UNREFERENCED_PARAMETER (RegistryPath);

    DebugPrint (("BPQHDLC: Entered Driver Entry V 0.0.2\n"));
    
    //
    // Create dispatch points for the IRPs.
    //
    
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = GpdDispatch;
    DriverObject->DriverUnload                          = GpdUnload;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = GpdDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = GpdDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = GpdDispatchSystemControl;
    DriverObject->DriverExtension->AddDevice           = GpdAddDevice;

    return STATUS_SUCCESS;
}


NTSTATUS
GpdAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    The Plug & Play subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to provide a driver.

    We need to determine if we need to be in the driver stack for the device.
    Create a functional device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually talk to the device UNTIL we have 
    received an IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

    NT status code.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PLOCAL_DEVICE_INFO      deviceInfo; 
    UNICODE_STRING          ntDeviceName;
    UNICODE_STRING          win32DeviceName;

    PAGED_CODE();

    DebugPrint(("BPQHDLC: Entered AddDevice: %p\n", PhysicalDeviceObject));

    RtlInitUnicodeString(&ntDeviceName, GPD_DEVICE_NAME);


    //
    // Create a named deviceobject so that legacy applications can talk to us.
    // Since we are naming the object, we wouldn't able to install multiple
    // instance of this driver. Please note that as per PNP guidelines, we 
    // should not name the FDO or create symbolic links. We are doing it because
    // we have a legacy APP that doesn't know how to open an interface.
    //
        
    status = IoCreateDevice(DriverObject,
                             sizeof (LOCAL_DEVICE_INFO),
                             &ntDeviceName,
                             GPD_TYPE,
                             FILE_DEVICE_SECURE_OPEN, // do security checks on relative open
                             FALSE,
                             &deviceObject);

    
    if (!NT_SUCCESS (status)) {
        //
        // Either not enough memory to create a deviceobject or another
        // deviceobject with the same name exits. This could happen
        // if you install another instance of this device.
        //
        DebugPrint(("BPQHDLC: IoCreateDevice failed: %x\n", status));
        return status;
    }

    RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink( &win32DeviceName, &ntDeviceName );

    if (!NT_SUCCESS(status))    // If we we couldn't create the link then
    {                           //  abort installation.
        IoDeleteDevice(deviceObject);
        return status;
    }

    deviceInfo = (PLOCAL_DEVICE_INFO) deviceObject->DeviceExtension;
    
    deviceInfo->NextLowerDriver = IoAttachDeviceToDeviceStack (
                                       deviceObject,
                                       PhysicalDeviceObject);
    if(NULL == deviceInfo->NextLowerDriver) {
        IoDeleteSymbolicLink(&win32DeviceName);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // We will use remove lock to keep count of IRPs so that 
    // we don't deteach or delete our deviceobject until all pending I/Os
    // are completed. 
    //
    
    IoInitializeRemoveLock (&deviceInfo->RemoveLock , 
                            HDLC_TAG,
                            1, // MaxLockedMinutes 
                            5); // HighWatermark, this parameter is 
                                // used only on checked build.

	KeInitializeSpinLock(&deviceInfo->QueueSpinLock);

    //
    // Set the flag if the device is not holding a pagefile
    // crashdump file or hibernate file. 
    // 
    
    deviceObject->Flags |=  DO_POWER_PAGABLE;

    deviceInfo->DeviceObject = deviceObject;

    //
    // Set the initial state of the FDO
    //

    INITIALIZE_PNP_STATE(deviceInfo);

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // If you provide read/write dispatch handlers, make sure
    // to specify the preferred buffering method for the I/O.
    // deviceObject->Flags |= DO_BUFFERED_IO;


    DebugPrint(("BPQHDLC: AddDevice: %p to %p->%p \n", deviceObject, 
                       deviceInfo->NextLowerDriver,
                       PhysicalDeviceObject));

    return STATUS_SUCCESS;
}

NTSTATUS 
GpdCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    The completion routine for plug & play irps that needs to be
    processed first by the lower drivers. 

Arguments:

   DeviceObject - pointer to a device object.   

   Irp - pointer to an I/O Request Packet.

   Context - pointer to an event object.

Return Value:

      NT status code

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // If the lower driver didn't return STATUS_PENDING, we don't need to 
    // set the event because we won't be waiting on it. 
    // This optimization avoids grabbing the dispatcher lock and improves perf.
    //
    //

    if (Irp->PendingReturned == TRUE) {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }

    //
    // The dispatch routine will have to call IoCompleteRequest
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
GpdDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these the driver will completely ignore.
    In all cases it must pass the IRP to the next lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status = STATUS_SUCCESS;
    KEVENT                      event;        
    UNICODE_STRING              win32DeviceName;
    PLOCAL_DEVICE_INFO          deviceInfo;
 
    PAGED_CODE();

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock (&deviceInfo->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    DebugPrint(("BPQHDLC: PNP Dispatch %s\n",PnPMinorFunctionString(irpStack->MinorFunction)));

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                                  NotificationEvent,
                                  FALSE
                                  );

        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE) GpdCompletionRoutine, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);
                               
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);

        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Must be kernelmode if event memory is in stack
               FALSE, // No allert
               NULL); // No timeout
        }

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {

            status = GpdStartDevice(DeviceObject, Irp);
            if(NT_SUCCESS(status))
            {
                //
                // As we are successfully now back from our start device
                // we can do some device specific work.
                //
                SET_NEW_PNP_STATE(deviceInfo, Started);                
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with STATUS_MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // Fail the query stop to prevent the system from taking away hardware 
        // resources. We do this because our hardware is a legacy hardware and 
        // it can't work with any other resources.
        // If you do support this, you must have a queue to hold
        // incoming requests between stop and subsequent start with new set of
        // resources. Check the toaster sample to learn how to do that.
        //

        SET_NEW_PNP_STATE(deviceInfo, StopPending);

        Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;
        
    case IRP_MN_QUERY_REMOVE_DEVICE:
        //
        // The device can be removed without disrupting the machine. 
        //

       SET_NEW_PNP_STATE(deviceInfo, RemovePending);
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:

        //
        // The device has been unexpectedly removed from the machine 
        // and is no longer available for I/O. Stop all access to the device.
        // Release any resources associated with the device, but leave the 
        // device object attached to the device stack until the PnP Manager 
        // sends a subsequent IRP_MN_REMOVE_DEVICE request. 
        // You should fail any outstanding I/O to the device. You will
        // not get a remove until all the handles open to the device
        // have been closed.
        //

        SET_NEW_PNP_STATE(deviceInfo, SurpriseRemovePending);
       
		RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&win32DeviceName);           
        
        IoSkipCurrentIrpStackLocation(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        break;       
        
    case IRP_MN_REMOVE_DEVICE:

        //
        // Relinquish all resources here.
        // Detach and delete the device object so that
        // your driver can be unloaded. You get remove
        // either after query_remove or surprise_remove.
        //

        if(SurpriseRemovePending != deviceInfo->DevicePnPState)
        {
            //
            // We received query-remove earlier so free up resources.
            //

			RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
            IoDeleteSymbolicLink(&win32DeviceName);           
        }        

        SET_NEW_PNP_STATE(deviceInfo, Deleted);

        //
        // Wait for all outstanding requests to complete
        //
        DebugPrint(("BPQHDLC: Waiting for outstanding requests\n"));
        IoReleaseRemoveLockAndWait(&deviceInfo->RemoveLock, Irp);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);

        IoDetachDevice(deviceInfo->NextLowerDriver); 
        IoDeleteDevice(DeviceObject);
        
        return status;
                
    case IRP_MN_STOP_DEVICE:

        ASSERTMSG("We failed query stop, so we should never get this request\n", FALSE);

        SET_NEW_PNP_STATE(deviceInfo, Stopped);
        
        // No action required in this case. Just pass it down.
        Irp->IoStatus.Status = STATUS_SUCCESS;        
        goto passdown;
        break;
        
    case IRP_MN_CANCEL_REMOVE_DEVICE: 

        RESTORE_PREVIOUS_PNP_STATE(deviceInfo);
        //
        // No action required in this case. Just pass it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto passdown;
        break;
        
    case IRP_MN_CANCEL_STOP_DEVICE: 

        RESTORE_PREVIOUS_PNP_STATE(deviceInfo);
        //
        // No action required in this case. Just pass it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto passdown;
        break;
    default:
        //
        // Please see PnP documentation for use of these IRPs.
        //
        goto passdown;
        break;
    }
    
    IoReleaseRemoveLock(&deviceInfo->RemoveLock, Irp);       
    return status;

passdown:
   
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
    IoReleaseRemoveLock(&deviceInfo->RemoveLock, Irp);       
    return status;
        
}

NTSTATUS
GpdStartDevice (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:
    
    Get the resources, map the resources if required
    and initialize the device.    

Arguments:
    
   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.
   
Return Value:

    NT status code
    

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceTrans;
    PCM_PARTIAL_RESOURCE_LIST   partialResourceList;
    PCM_PARTIAL_RESOURCE_LIST   partialResourceListTranslated;
    PIO_STACK_LOCATION  stack;
    ULONG i, Port, Int;
    PLOCAL_DEVICE_INFO deviceInfo;

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;

    stack = IoGetCurrentIrpStackLocation (Irp);

    PAGED_CODE();

    //
    // Do whatever initialization needed when starting the device: 
    // gather information about it,  update the registry, etc.
    //

    if ((NULL == stack->Parameters.StartDevice.AllocatedResources) &&
        (NULL == stack->Parameters.StartDevice.AllocatedResourcesTranslated)) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Parameters.StartDevice.AllocatedResources points to a 
    // CM_RESOURCE_LIST describing the hardware resources that 
    // the PnP Manager assigned to the device. This list contains 
    // the resources in raw form. Use the raw resources to program 
    // the device.
    //

    partialResourceList = 
    &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;

    resource = &partialResourceList->PartialDescriptors[0];
    
    //
    // Parameters.StartDevice.AllocatedResourcesTranslated points 
    // to a CM_RESOURCE_LIST describing the hardware resources that 
    // the PnP Manager assigned to the device. This list contains 
    // the resources in translated form. Use the translated resources 
    // to connect the interrupt vector, map I/O space, and map memory.
    //

    partialResourceListTranslated = 
    &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;

    resourceTrans = &partialResourceListTranslated->PartialDescriptors[0];

    for (i = 0;
            i < partialResourceList->Count; i++, resource++, resourceTrans++) {

        switch (resource->Type) {

        case CmResourceTypePort:

            switch (resourceTrans->Type) {

            case CmResourceTypePort:

                DebugPrint(("BPQHDLC: Resource UnTranslated Port: (%x) Length: (%d)\n", 
                    resource->u.Port.Start.LowPart, 
                    resource->u.Port.Length));

                DebugPrint(("BPQHDLC: Resource Translated Port: (%x) Length: (%d)\n", 
                    resourceTrans->u.Port.Start.LowPart, 
                    resourceTrans->u.Port.Length));

				// Set Bitmap of Assigned Ports

				for (Port = resource->u.Port.Start.LowPart;
					Port < resource->u.Port.Start.LowPart + resourceTrans->u.Port.Length; Port++)
				{
					AllocatedPorts[(Port & 0x3ff) >> 5] |= (1 << (Port & 0x1f));
				}
                break;

 /*           case CmResourceTypeMemory:

                //
                // We need to map the memory
                //

                deviceInfo->PortBase = (PVOID)
                    MmMapIoSpace (resourceTrans->u.Memory.Start,
                                  resourceTrans->u.Memory.Length,
                                  MmNonCached);

                deviceInfo->PortCount = resourceTrans->u.Memory.Length;
                deviceInfo->PortWasMapped = TRUE;

                DebugPrint(("BPQHDLC: Resource Translated Memory: (%x) Length: (%d)\n", 
                    resourceTrans->u.Memory.Start.LowPart, 
                    resourceTrans->u.Memory.Length));

                break;
*/

            default:
                DebugPrint(("BPQHDLC: Unhandled resource_type (0x%x)\n", resourceTrans->Type));
                status = STATUS_UNSUCCESSFUL;
            }             
            break;

 /*       case CmResourceTypeMemory:

            deviceInfo->PortBase = (PVOID)
                MmMapIoSpace (resourceTrans->u.Memory.Start,
                              resourceTrans->u.Memory.Length,
                              MmNonCached);

            deviceInfo->PortCount = resourceTrans->u.Memory.Length;
            deviceInfo->PortWasMapped = TRUE;

            DebugPrint(("BPQHDLC: Resource Translated Memory: (%x) Length: (%d)\n", 
                resourceTrans->u.Memory.Start.LowPart, 
                resourceTrans->u.Memory.Length));

            break;
*/
        case CmResourceTypeInterrupt:

			AllocatedInterrupts |= (1 << resource->u.Interrupt.Vector);

			Int = resource->u.Interrupt.Vector;

			deviceInfo->Interrupt_Control[Int].Vector = resourceTrans->u.Interrupt.Vector;
			deviceInfo->Interrupt_Control[Int].Level= (UCHAR)resourceTrans->u.Interrupt.Level;
			deviceInfo->Interrupt_Control[Int].Affinity = resourceTrans->u.Interrupt.Affinity;
            
            if (resourceTrans->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
				deviceInfo->Interrupt_Control[Int].InterruptMode = Latched;
			else
				deviceInfo->Interrupt_Control[Int].InterruptMode = LevelSensitive;

			DebugPrint(("BPQHDLC: Resource Untranslated Vector %d Level %d Aff %d\n", 
                    resource->u.Interrupt.Vector,
                    resource->u.Interrupt.Level,
					resource->u.Interrupt.Affinity));

			DebugPrint(("BPQHDLC: Resource Translated Vector %d Level %d Aff %d Latched %d\n", 
                    resourceTrans->u.Interrupt.Vector,
                    resourceTrans->u.Interrupt.Level,
					resourceTrans->u.Interrupt.Affinity,
					deviceInfo->Interrupt_Control[Int].InterruptMode));

			DebugPrint(("BPQHDLC: Avail Interrupts %x\n", AllocatedInterrupts));

			break;

        default:

            DebugPrint(("BPQHDLC: Unhandled resource type (0x%x)\n", resource->Type));
            status = STATUS_UNSUCCESSFUL;
            break;
    
        } // end of switch
    } // end of for

    return status;

}


NTSTATUS
GpdDispatchPower(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps.
    Does nothing except forwarding the IRP to the next device
    in the stack.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT Status code
--*/
{
    PLOCAL_DEVICE_INFO   deviceInfo;
    
    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // If the device has been removed, the driver should not pass 
    // the IRP down to the next lower driver.
    //
    
    if (deviceInfo->DevicePnPState == Deleted) {
        
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status =  STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }
    
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(deviceInfo->NextLowerDriver, Irp);
}

NTSTATUS 
GpdDispatchSystemControl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for WMI irps.
    Does nothing except forwarding the IRP to the next device
    in the stack.
    
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT Status code
--*/
{
    PLOCAL_DEVICE_INFO   deviceInfo;

    PAGED_CODE();

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;

    //
    // If the device has been removed, the driver should not pass 
    // the IRP down to the next lower driver.
    //
    
    if (deviceInfo->DevicePnPState == Deleted) {
        
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status =  STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceInfo->NextLowerDriver, Irp);
}

    
VOID
GpdUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    //
    // The device object(s) should be NULL now
    // (since we unload, all the devices objects associated with this
    // driver must have been deleted.
    //
    ASSERT(DriverObject->DeviceObject == NULL);
    
    DebugPrint (("BPQHDLC: Unload\n"));

    return;
}

NTSTATUS GpdDispatch(IN DEVICE_OBJECT * pDO, IN    PIRP pIrp)
{   
//    pDO - Pointer to device object.

//    pIrp - Pointer to the current IRP.

   PLOCAL_DEVICE_INFO pLDI;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;
	int i;
	PULONG pIOBuffer;
	PUCHAR IOBuffer;
	PBUF_ENTRY Buffer;
	PHDLC_CHANNEL Channel;
	PUCHAR Mapped_IOADDR;


    PAGED_CODE();

    pIrp->IoStatus.Information = 0;
    pLDI = pDO->DeviceExtension;    // Get local info struct

    Status = IoAcquireRemoveLock (&pLDI->RemoveLock, pIrp);

    if (!NT_SUCCESS (Status)) { // may be device is being removed.
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest (pIrp, IO_NO_INCREMENT);
        return Status;
    }
   
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    // Dispatch based on major fcn code.

    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:

			DebugPrint (("BPQHDLC: Entered Create %x \n", pIrpStack->MinorFunction));

			AllocatedChannels=0;

			InitializeListHead(&pLDI->FREE_Q);
		
            Status = STATUS_SUCCESS;
            break;

         case IRP_MJ_CLOSE:

			DebugPrint (("BPQHDLC: Entered Close %x \n", pIrpStack->MinorFunction));
			ReleaseResources(pLDI);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:
            //  Dispatch on IOCTL
            switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
            {

			case IOCTL_BPQHDLC_IOREAD:

				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

				Mapped_IOADDR = ULongToPtr(*pIOBuffer);

				*(pIOBuffer) = READ_PORT_UCHAR(Mapped_IOADDR);
				
				pIrp->IoStatus.Information = 4;		// Bytes Returned

				DebugPrint (("BPQHDLC: IOREAD %x %x\n", Mapped_IOADDR, *(pIOBuffer) ));

				Status = STATUS_SUCCESS;
				break;

			case IOCTL_BPQHDLC_IOWRITE:

				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
				IOBuffer = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;

				Mapped_IOADDR = ULongToPtr(*pIOBuffer);

				DebugPrint (("BPQHDLC: IOWRITE %x %x\n", Mapped_IOADDR, IOBuffer[4]));

				WRITE_PORT_UCHAR(Mapped_IOADDR, IOBuffer[4]);
				
				pIrp->IoStatus.Information = 0;		// Bytes Returned
				Status = STATUS_SUCCESS;

				break;

			case IOCTL_BPQHDLC_POLL:
		
				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

	//			DebugPrint (("BPQHDLC: Poll IOCTL %x \n", *pIOBuffer));

				pIrp->IoStatus.Information = 
					ReceivePacket((PHDLC_CHANNEL)*pIOBuffer, (PUCHAR) pIOBuffer, pLDI);

				Status = STATUS_SUCCESS;
				break;

            case IOCTL_BPQHDLC_TIMER:

				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
				Channel = (PHDLC_CHANNEL)*pIOBuffer;
			
				// See if anything to send. If so, and channel is clear, Key up and start TXDelay timer.
				
				if (Channel->LINKSTS == 0)		// Idle
				{
					if (!IsListEmpty(&Channel->TXMSG_Q))
					{
						Buffer = PopBUFEntry(&Channel->TXMSG_Q, pLDI);
						if (Buffer == NULL) return 0;

						StartTX(pLDI, Channel, Buffer);
					}
				}
	

				pIrp->IoStatus.Information = 0;		// Bytes Returned
				Status = STATUS_SUCCESS;
				break;


            case IOCTL_BPQHDLC_CHECKTX:

				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

//				DebugPrint (("BPQHDLC: CheckTX IOCTL %x \n", *pIOBuffer));
				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

				*(pIOBuffer) = 0;
				
				pIrp->IoStatus.Information = 4;		// Bytes Returned
				Status = STATUS_SUCCESS;
				break;


			case IOCTL_BPQHDLC_SEND:
			
				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

//				DebugPrint (("BPQHDLC: Send IOCTL %x \n", *pIOBuffer));

				SendPacket((PHDLC_CHANNEL)*pIOBuffer, (PUCHAR) pIOBuffer, pLDI);
	            Status = STATUS_SUCCESS;
		
				break;

			
			case IOCTL_BPQHDLC_ADDCHANNEL:
			
				// We return the Address of the Channel Structure to BPQ32 to use as a Handle for IO requests
				
				pIOBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

				*(PULONG)pIOBuffer = (ULONG)InitChannel((PBPQHDLC_ADDCHANNEL_INPUT)pIOBuffer, pLDI);

				pIrp->IoStatus.Information = 4;		// Bytes Returned
				Status = STATUS_SUCCESS;
				break;
			
            default:  
			
				DebugPrint (("BPQHDLC: Invalid IOCTL %x \n", pIrpStack->Parameters.DeviceIoControl.IoControlCode));
                Status = STATUS_INVALID_PARAMETER;

            }
            break;

        default: 
			    
			DebugPrint (("BPQHDLC: Invalid Function %d\n", pIrpStack->MajorFunction));
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    // We're done with I/O request.  Record the status of the I/O action.
    pIrp->IoStatus.Status = Status;

    // Don't boost priority when returning since this took little time.
    IoCompleteRequest(pIrp, IO_NO_INCREMENT );
    IoReleaseRemoveLock(&pLDI->RemoveLock, pIrp);       
    return Status;
}

#if DBG
PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
            
        default:
            return "IRP_MN_?????";
    }
}

#endif

 
BOOLEAN HDLCISR(IN PKINTERRUPT InterruptObject, HDLC_CHANNEL * Channel)
{
    // This function will return TRUE if the serial port is the source
    // of this interrupt, FALSE otherwise.

	// Context (2nd Param) is the first Channel Record on this level.
	// Each Channel Record has a ponter back to the Device Object record.

	MYDEVICE_OBJECT * Device = Channel->Device;

    //
    // Holds the contents of the interrupt identification record.
    // A low bit of zero in this register indicates that there is
    // an interrupt pending on this device.
    //
    UCHAR InterruptIdReg;
	UCHAR Vector;

    //
    // Will hold whether we've serviced any interrupt causes in this
    // routine.
    //
    BOOLEAN ServicedAnInterrupt;

    UCHAR tempLSR;

    UNREFERENCED_PARAMETER(InterruptObject);

    //
    // Make sure we have an interrupt pending.  If we do then
    // we need to make sure that the device is open.  If the
    // device isn't open or powered down then quiet the device.  Note that
    // if the device isn't opened when we enter this routine
    // it can't open while we're in it.
    //


    //
    // Apply lock so if close happens concurrently we don't miss the DPC
    // queueing
    //

//    InterlockedIncrement(&Channel->DpcCount);

	// ENTERED FROM HARDWARE INTERRUPT

	DebugPrint(("BPQHDLC: ISR Entered\n"));

SIOI10:

	WRITE_PORT_UCHAR(Channel->Mapped_ASIOC, 3);		//SELECT RR3

	InterruptIdReg = READ_PORT_UCHAR(Channel->Mapped_ASIOC);

	if (InterruptIdReg == 0) goto NOINTS; 

	ServicedAnInterrupt = TRUE;

	WRITE_PORT_UCHAR(Channel->Mapped_BSIOC, 2);		//SELECT RR3

	Vector = READ_PORT_UCHAR(Channel->Mapped_BSIOC);
		
	if (Vector < 8)
		Channel = Channel->B_PTR;		// GET DATA FOR B CHANNEL
	else
	{
		Channel = Channel->A_PTR;		// GET DATA FOR B CHANNEL
		Vector -=8;
	}

	// Call our char handler

	Channel->VECTOR[Vector<<1](Channel);

	if (Channel->TXComplete)
	{
		KeInsertQueueDpc(&Channel->TXCompleteDpc, Channel->Device, Channel);
		Channel->TXComplete = FALSE;
	}

	WRITE_PORT_UCHAR(Channel->Mapped_ASIOC, 0x38);		// RESET IUS

	goto	SIOI10;			// SEE IF ANY MORE 

NOINTS:

    return ServicedAnInterrupt;

 }

VOID TXComplete(IN PKDPC Dpc, IN PVOID Context, MYDEVICE_OBJECT * Device, HDLC_CHANNEL * Channel)
{
	// Called when TX Complete Occurs. Free the buffer and see if any more to send

	PushBUFEntry(&Device->DeviceExtension->FREE_Q, Channel->TXFRAME, Device->DeviceExtension);
}

VOID RXComplete(IN PKDPC Dpc, IN PVOID Context, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	// Called when RX Complete Occurs. Queue buffer.

	//PushBUFEntry(&DeviceObject->DeviceExtension->FREE_Q, Context->TXFRAME, DeviceObject->DeviceExtension);

}
VOID SDADRX(PHDLC_CHANNEL Channel)
{
	UCHAR Char;

//	Channel->SOFTDCD = 4;			// SET RX ACTIVE
/*
	MOV	ESI,OFFSET32 deviceInfo->FREE_Q
	cli
	CALL	Q_REM
	sti
	JNZ SHORT GETB00

;	CALL	NOBUFFERCHECK		; CHECK IF POOL IS CORRUPT
	JMP SHORT NOBUFFERS

GETB00:

	DEC	QCOUNT

	MOV	DWORD PTR [EDI],OFFSET32 GETB00	; FLAG FOR DUMP ANALYSER
*/
	Char = SIOR;			// GET FIRST BYTE OF ADDRESS
/*
	MOV	CURALP[EBX],EDI		; SAVE ADDR
	ADD	EDI,7
	MOV	[EDI],AL			; ADDR TO BUFFER
	INC	EDI
	MOV	SDRNEXT[EBX],EDI		; AND NEXT BYTE POINTER
;*/
	Channel->FRAMELEN = 7;

	Channel->SDFLAGS |= SDRINP;	// SET RX IN PROGRESS

	SETRVEC	SDIDRX;					// SET VECTOR TO 'GET DATA'

	return;

//NOBUFFERS:

//	NO BUFFER FOR RECEIVE - SET TO DISCARD

	Channel->OLOADS++;

	SETRVEC	SDOVRX;

	SIOR;				// CLEAR INT PENDING

	return;
}

VOID SDIDRX(PHDLC_CHANNEL Channel)
{
//;
//;	NOW READ CHARACTER FROM SIO AND STORE
//;

	Channel->SOFTDCD = 4;			// SET RX ACTIVE
	Channel->FRAMELEN++;

	if (Channel->FRAMELEN > BUFFLEN-10)
		SETRVEC	SDOVRX;				//	CANT TAKE ANY MORE

	*(Channel->SDRNEXT++) = SIOR;	// GET data

	return;
}

VOID SDOVRX(PHDLC_CHANNEL Channel)
{
	//	DISCARD REST OF MESSAGE

	SIOR;				// READ CHAR AND DISCARD

	return;
}

//***	RX SPECIAL CHARACTER INTERRUPT

VOID SPCLINT(PHDLC_CHANNEL Channel)
{
	UCHAR RR1;
	
	SIOR;				// READ CHAR AND DISCARD

	SIOCW(1);			// SELECT RR1

	RR1 = SIOCR;		// INPUT RR1

	SIOCW(0x30);		// RESET SIO ERROR LATCHES

	if (RR1 & 0x80)		// END OF FRAME?
		goto SDEOF;		// YES

//	NOT END OF FRAME - SHOULD BE OVERRUN - DISCARD FRAME

	Channel->L2ORUNC++;	// FOR STATS

	if (Channel->SDFLAGS & SDRINP)
	{
		// BUFFER IS ALLOCATED

		Channel->SDFLAGS &= !SDRINP;

		//MOV	ESI,OFFSET32 deviceInfo->FREE_Q
		//MOV	EDI,CURALP[EBX]
		//CALL	Q_ADDF
	}

	SETRVEC SDOVRX;			// DISCARD REST OF MESSAGE

	goto SPCLINTEXIT;

SDEOF:

//	IF RESIDUE IS NONZERO, IGNORE FRAME

//	MOV	AL,AH
//	AND	AL,1110B		; GET RESIDUE BITS
//	CMP	AL,0110B
//;	JNE SHORT DONTCOUNT		; NOT MULTIPLE OF 8 BITS

//	END OF FRAME - SEE IF FCS OK

	if ((RR1 & 0x40) == 0)
		goto SDSP10;			// J IF GOOD END-OF-FRAME

//	FCS ERROR

//	CMP	FRAMELEN[EBX],14H
//	JB SHORT DONTCOUNT		// TOO SHORT
	Channel->RXERRORS++;


	if ((Channel->SDFLAGS & SDRINP) == 0)
		goto SDSP09;		// IF NOT SET, NO BUFFER IS ALLOCATED

	Channel->SDFLAGS &= !SDRINP;

DISCARDFRAME:

//	MOV	ESI,OFFSET32 deviceInfo->FREE_Q
//	MOV	EDI,CURALP[EBX]
//	CALL	Q_ADDF

SDSP09:

	SETRVEC SDADRX;

	goto SPCLINTEXIT;

//	GOOD FRAME RECEIVED

SDSP10:
	if ((Channel->SDFLAGS & SDRINP) == 0)
		goto SDSP11;			// IF NOT SET, NO BUFFER IS ALLOCATED

	Channel->SDFLAGS &= !SDRINP;

	*(--Channel->SDRNEXT) = 0xfe;	// OVERWRITE FIRSTS FCS BYTE WITH MARKER

	if (Channel->FRAMELEN < 20)
		goto DISCARDFRAME;		// TOO SHORT

//	MOV	EDI,CURALP[EBX]		; BUFFER ADDR
//	MOV	5[EDI],AX		; PUT IN LENGTH

//	LEA	ESI,RXMSG_Q[EBX]
//	CALL	Q_ADD			; QUEUE MSG FOR B/G
SDSP11:

	SETRVEC SDADRX;			// READY FOR NEXT FRAME

SPCLINTEXIT:

	return;
	
}
	


//	EXTERNAL/STATUS INTERRUPT

//		 - TRANSMIT UNDERRUN/EOM - IF AT END OF FRAME IGNORE
//					      ELSE SEND ABORT
//		 - ABORT START/END - IF RECEIVING, CANCEL MESSAGE

VOID EXTINT(PHDLC_CHANNEL Channel)
{
	UCHAR RR0, OldRR0 = Channel->RR0;

	RR0 = SIOCR;
	
	Channel->RR0 = RR0;

	if (((RR0 ^ OldRR0) & 0xc0) == 0)
		goto SDST40;							// NO INTERESTING CHANGES
	
	if ((Channel->SDFLAGS & SDTINP) == 0)
		goto SDST10;							// J IF 'TX IN PROGRESS' NOT SET

//	WE ARE TRANSMITTING - CHECK FOR UNDERRUN

	if ((RR0 & SDUNDER) == 0)		// TX UNDERRUN?
		goto SDST10;				// ONLY INTERESTING ONE

	if ((Channel->SDTXCNT) & 0x8000)	// IS IDP CHAR COUNT -VE?
		goto SDST10;						// J IF YES (NORMAL END-OF-FRAME)

//	UNDERRUN IN MID-FRAME
//	ABORT THE TRANSMISSION

	SIOCW(SDABTX);						// SEND ABORT SEQUENCE
	SETTVEC	SDCMTX;						// SET VECTOR TO 'TX COMPLETE'

	Channel->SDFLAGS |= ABSENT;		// SET ABORT SENT

	Channel->L2URUNC++;				// UNDERRUNS

	goto SDST10;						// SEE IF ANY RX ERROR BITS ALSO SET

//	IS RX IN PROGRESS?

SDST10:
	if ((Channel->SDTXCNT & SDRINP) == 0)
		goto SDST40;					// NO, SO PROBABLY ABORT FOLLOWING MSG 

	if ((RR0 & SDABORT)	== 0)			// IS ABORT STATUS BIT SET?
		goto SDST40;					// J IF NOT - ? ABORT TERMINATON

//	MOV	EDI,CURALP[EBX]		; BUFFER ADDR

//	MOV	ESI,OFFSET32 deviceInfo->FREE_Q
//	CALL	Q_ADDF			; RELEASE BUFFER

	Channel->SDFLAGS &= !SDRINP;	// CLEAR RX IN PROGRESS

	SETRVEC SDADRX;					// READY FOR NEXT FRAME

SDST40:

	SIOCW(SDEXTR);					// RESET EXTERNAL/STATUS INTERRUPT

	return ;

}

//***	TX DATA

VOID SDDTTX(PHDLC_CHANNEL Channel)
{	
	Channel->SDTXCNT--;			// DECREMENT CURRENT IDP COUNT
;
	if (Channel->SDTXCNT >= 0)
	{
		SIOW (*Channel->SDTNEXT++);	// TRANSMIT NEXT BYTE
		return;
	}

//	NO MORE DATA TO TRANSMIT

	SIOCW (10);							// Select Wr10  
	SIOCW(Channel->WR10 |  0x80);		// SET TO SEND CRC ON UNDERRUN

	SIOCW(SDRPEND);			// RESET TX UNDERRUN/EOM LATCH
	SETTVEC	SDCMTX;			// SET TX VECTOR TO 'TX COMPLETION'

	return;
}

//	*** F11 - TX COMPLETION

VOID SDCMTX(PHDLC_CHANNEL Channel)
{
	SIOCW(SDRPEND);			// RESET TX INT PENDING

	if ((Channel->SDFLAGS) & ABSENT)
	{
		//	FRAME WAS ABORTED - SEND AGAIN

		Channel->SDFLAGS &= !ABSENT;
		//	MOV	EDI,TXFRAME[EBX]
		//	JMP SHORT SENDAGAIN
	}

	Channel->TXComplete = TRUE;
	Channel->SDFLAGS &= !SDTINP;

//	SEE IF MORE TO SEND

//	LEA	ESI,PCTX_Q[EBX]
//	CALL	Q_REM

	goto NOMORETOSEND;				// J IF QUEUE EMPTY

//	MOV	TXFRAME[EBX],EDI		; SAVE ADDRESS OF FRAME

//SENDAGAIN:

//	MOV	AX,5[EDI]
//	SUB	AX,8
//	MOV	SDTXCNT[EBX],AX			; GET MESSAGE LENGTH FROM BUFFER

//	LEA	EDI,8[EDI]
//	MOV	SDTNEXT[EBX],EDI		; SET NEXT BYTE POINTER

	Channel->SDFLAGS |= SDTINP;
	SETTVEC	SDDTTX;				// SET VECTOR TO 'TX DATA'

	SIOCW(SDTXCRC);					// RESET TX CRC GENERATOR

	SIOW (*Channel->SDTNEXT++);	// TRANSMIT First BYTE

	SIOCW(SDTXUND);					// RESET TX UNDERRUN LATCH

	Channel->RR0 &= !SDUNDER;		// KEEP STORE COPY

	SIOCW(10);						// Select WR10 
	SIOCW(Channel->WR10 | 0x84);	// SET TO SEND CRC ON UNDERRUN
 
	return;

NOMORETOSEND:

//	SEND A FEW PADDING CHARS TO ENSURE FLAG IS CLEAR OF SCC

	SETTVEC	SENDDUMMY1;

	SIOW(0);		// FIRST DUMMY

	return;
}

VOID SENDDUMMY1(PHDLC_CHANNEL Channel)
{
	SIOW(0);		// FIRST DUMMY

	SETTVEC	SENDDUMMY2;

	return;
}

VOID SENDDUMMY2(PHDLC_CHANNEL Channel)
{
	SIOW(0);		// SECOND DUMMY

	SETTVEC	SENDDUMMY3;

	return;
}

VOID SENDDUMMY3(PHDLC_CHANNEL Channel)
{
	SIOW(0);		// THIRD DUMMY

	SETTVEC	SENDDUMMY4;

	return;
}
VOID SENDDUMMY4(PHDLC_CHANNEL Channel)
{
	SIOW(0);		// 4TH DUMMY

	SETTVEC	DROPRTS;

	return;
}

VOID DROPRTS(PHDLC_CHANNEL Channel)
{
	Channel->LINKSTS &= 0xfe;	// SET NOT TRANSMITTING

	SIOCW(5);
	
	SIOCW(0xe1);			// DROP RTS AND TXEN

	SETTVEC	IGNORE;

	if (Channel->RXBRG)
	{
		// NEED TO RESET BRG FOR RECEIVE

		SIOCW(12);			// Select WR12
		SIOCW(Channel->RXBRG &0xff);	// SET LSB
		SIOCW(13);			// Select WR12
		SIOCW(Channel->RXBRG >> 8);	// SET MSB
	}

	return;
}


//***	TX INTERRUPT IGNORE

VOID IGNORE(PHDLC_CHANNEL Channel)
{
	SIOCW(SDRPEND);					// RESET TX INTERRUPT PENDING

	return;
}


PHDLC_CHANNEL InitChannel(PBPQHDLC_ADDCHANNEL_INPUT Params, PLOCAL_DEVICE_INFO deviceInfo)
{
	// Create and Initialise Kernel Channel Entry for a new Channel

	PHDLC_CHANNEL Channel = NULL;
	PBUF_ENTRY Buffer;
	int i, AddChannels;
	ULONG Port;
	NTSTATUS status;
	PHDLC_INTERRUPTS Interrupt;
    UCHAR InterruptIdReg;
	UCHAR Vector;


	DebugPrint(("BPQHDLC: InitChannel Interrupt %d IOBASE %x IOLEN %d Chan %c\n",
		Params->Interrupt, Params->IOBASE, Params->IOLEN, Params->Channel));

	// Check that IO address range and Interrupt are available

	if ((AllocatedInterrupts & (1 << Params->Interrupt)) == 0)
	{
		DebugPrint(("BPQHDLC: InitChannel Interrupt %d not allocated\n", Params->Interrupt));
		return 0;
	}
				
	for (Port = Params->IOBASE; Port < Params->IOBASE + Params->IOLEN; Port++)
	{
		if ((AllocatedPorts[(Port & 0x3ff) >> 5] & (1 << (Port & 0x1f))) == 0)
		{
			DebugPrint(("BPQHDLC: InitChannel Port %x not allocated\n", Port));
			return 0;
		}
	}
 
	Channel = ExAllocatePool(NonPagedPool, sizeof(HDLC_CHANNEL));

	if (Channel == NULL)
	{
        DebugPrint(("BPQHDLC: Unable to allocate memory for Channel Structure\n"));
		return Channel;
	}
	
	DebugPrint(("BPQHDLC: Channel Structure allocated at %x Len %d\n", Channel, sizeof(HDLC_CHANNEL)));

	Channel->ChannelPointer = AllocatedChannels; // Save our position in AllocatedChannels List

	deviceInfo->ChannelPointers[AllocatedChannels++] = Channel;	

	InitializeListHead(&Channel->TXMSG_Q);
	InitializeListHead(&Channel->RXMSG_Q);

	Channel->IOBASE = Params->IOBASE;
	Channel->Mapped_ASIOC = ULongToPtr(Params->ASIOC);
	Channel->Mapped_BSIOC = ULongToPtr(Params->BSIOC);
	Channel->Mapped_SIOC = ULongToPtr(Params->SIOC);
	Channel->Mapped_SIO = ULongToPtr(Params->SIO);
	Channel->TXBRG = Params->TXBRG;
	Channel->RXBRG = Params->RXBRG;
	Channel->WR10 = Params->WR10;
	Channel->CHANNELNUM = Params->Channel;
	Channel->SOFTDCD = Params->SOFTDCD;
	Channel->TXDELAY = Params->TXDELAY;

	Channel->IOTXCA = IGNORE;			// TX CHANNEL A
	Channel->IOTXEA = EXTINT;
	Channel->IORXCA = SDADRX;			// RX CHANNEL A
	Channel->IORXEA = SPCLINT;


	// Add a few buffers to the pool (More if first channel)

	AddChannels = (AllocatedChannels == 1) ? 5:2;

	for (i = 0; i < AddChannels; i++)
	{
		Buffer = ExAllocatePool(NonPagedPool, sizeof(BUF_ENTRY));
		DebugPrint(("BPQHDLC: Buffer allocated at %x Len %d\n", Buffer, sizeof(BUF_ENTRY)));
		if (Buffer) PushBUFEntry(&deviceInfo->FREE_Q, Buffer, deviceInfo);
	}

    // Create Timer and Interrupt Stuff - 3  * Dpc and a Timer Object

	KeInitializeDpc(&Channel->TXDelayDpc, TXDelay, (PVOID)Channel );
	KeInitializeDpc(&Channel->TXCompleteDpc, TXComplete, (PVOID)Channel );
	KeInitializeDpc(&Channel->RXCompleteDpc, RXComplete, (PVOID)Channel );

    // Initialize the timer object

    KeInitializeTimer(&Channel->TXDelayTimer);

	// Clear any Pending ints

SIOI10:

	WRITE_PORT_UCHAR(Channel->Mapped_ASIOC, 3);		//SELECT RR3

	InterruptIdReg = READ_PORT_UCHAR(Channel->Mapped_ASIOC);

	if (InterruptIdReg == 0 || InterruptIdReg == 255) goto NOINTS; 

	WRITE_PORT_UCHAR(Channel->Mapped_BSIOC, 2);		//SELECT RR2

	Vector = READ_PORT_UCHAR(Channel->Mapped_BSIOC);

	DebugPrint(("BPQHDLC: Clear Ints Vector = %d\n", Vector));
	
	if (Vector < 8)
		Channel = Channel->B_PTR;		// GET DATA FOR B CHANNEL
	else
	{
		Channel = Channel->A_PTR;		// GET DATA FOR B CHANNEL
		Vector -=8;
	}

	// Call our char handler

	Channel->VECTOR[Vector<<1](Channel);

	WRITE_PORT_UCHAR(Channel->Mapped_ASIOC, 0x38);		// RESET IUS

	goto	SIOI10;			// SEE IF ANY MORE 

NOINTS:


	// Hook the interrupt if this is the first device on the level

	Interrupt = &deviceInfo->Interrupt_Control[Params->Interrupt];

	if (Interrupt->Interrupt == NULL)
	{
		DebugPrint(("BPQHDLC: Connecting Interrupt %d Vector %d Level %d Mode %d Aff %d\n",
			Params->Interrupt, Interrupt->Vector,
			Interrupt->Level,
			Interrupt->InterruptMode,
			Interrupt->Affinity, FALSE));
		
		status = IoConnectInterrupt(&Interrupt->Interrupt,
						HDLCISR,
						Channel, NULL,
						Interrupt->Vector,
						Interrupt->Level,
						Interrupt->Level,
						Interrupt->InterruptMode,
						FALSE,		// Sharable
						Interrupt->Affinity, FALSE);

		if (!NT_SUCCESS(status)) 
			DebugPrint(("Couldn't connect to interrupt"));

	}

 	//RXAINIT(Channel);

	return Channel;
}

VOID ReleaseResources(PLOCAL_DEVICE_INFO deviceInfo)
{
	int i;
	PBUF_ENTRY Buffer;
	PHDLC_CHANNEL Channel;
	PHDLC_INTERRUPTS Interrupt;

	for (i=0; i< AllocatedChannels; i++)
	{
		Channel = deviceInfo->ChannelPointers[i];

		KeCancelTimer(&Channel->TXDelayTimer);
		
		ExFreePool(Channel);
		DebugPrint(("BPQHDLC: Releasing Channel Structure allocated at %x\n", Channel));
		deviceInfo->ChannelPointers[i] = NULL;
	}

	while (!IsListEmpty(&deviceInfo->FREE_Q))
	{
		Buffer = PopBUFEntry(&deviceInfo->FREE_Q, deviceInfo);
		DebugPrint(("BPQHDLC: Releasing Buffer allocated at %x\n", Buffer));
		ExFreePool(Buffer);
	}

	// Release Interupts

	for (i=0; i < 17; i++)
	{
		Interrupt = &deviceInfo->Interrupt_Control[i];
		{
			if (Interrupt->Interrupt)
			{
				DebugPrint(("BPQHDLC: Disconnecting Interrupt %d\n", i));
				IoDisconnectInterrupt(Interrupt->Interrupt);
				Interrupt->Interrupt = NULL;
			}
		}
	}
}

VOID SendPacket(PHDLC_CHANNEL Channel, UCHAR * Msg, PLOCAL_DEVICE_INFO deviceInfo)
{
	int Len;
	PBUF_ENTRY Buffer;

	Len = (Msg[6]<<8) + Msg[5];

	DebugPrint(("BPQHDLC: Sending Msg Len %d to %x Chan %c\n", Len, Channel->IOBASE, Channel->CHANNELNUM));

	if (Len > BUFFLEN) return;

	if (IsListEmpty(&deviceInfo->FREE_Q)) Buffer = NULL; else Buffer = PopBUFEntry(&deviceInfo->FREE_Q, deviceInfo);

//	DebugPrint(("BPQHDLC: Sending  - Buffer %x\n", Buffer));

	if (Buffer == NULL)
	{
		DebugPrint(("Sending  - No Buffer\n"));

	} else{

		Buffer->MsgLen = Len;
		memcpy(Buffer->Message, Msg, Len);

		PushBUFEntry(&Channel->TXMSG_Q, Buffer, deviceInfo);
	}

	return;
}


int ReceivePacket(PHDLC_CHANNEL Channel, UCHAR * Msg, PLOCAL_DEVICE_INFO deviceInfo)
{
	int Len;
	PBUF_ENTRY Buffer;

	if (Channel == NULL) return 0;

	if (IsListEmpty(&Channel->RXMSG_Q)) return 0;

	Buffer = PopBUFEntry(&Channel->RXMSG_Q, deviceInfo);

	if (Buffer == NULL) return 0;

	Len = Buffer->MsgLen;

//	DebugPrint(("BPQHDLC: RX Msg Len %d\n", Len));

	if (Len > BUFFLEN)
		return 0;

	memcpy(Msg, Buffer->Message, Len);
		
	PushBUFEntry(&deviceInfo->FREE_Q, Buffer, deviceInfo);

	return Len;
}

VOID StartTX(PLOCAL_DEVICE_INFO pLDI, PHDLC_CHANNEL Channel, PBUF_ENTRY Buffer)
{
	LARGE_INTEGER Interval;
	
	Channel->LINKSTS |= 1;		// Set Active

	// Kick off TXDelay 

	DebugPrint(("BPQHDLC: Starting TXDelay %d\n", Channel->TXDELAY));

	//PushBUFEntry(&pLDI->FREE_Q, Buffer, pLDI);

	Channel->TXFRAME = Buffer;
	Channel->SDTXCNT = Buffer->MsgLen;
	Channel->SDTNEXT = &Buffer->Message[8];

	Channel->SDFLAGS |= SDTINP;

	Channel->SDFLAGS |= SDTINP;
	SETTVEC	SDDTTX;				// SET VECTOR TO 'TX DATA'

	SIOCW(SDTXCRC);					// RESET TX CRC GENERATOR

	SIOW (*Channel->SDTNEXT++);	// TRANSMIT First BYTE

	SIOCW(SDTXUND);					// RESET TX UNDERRUN LATCH

	Channel->RR0 &= !SDUNDER;		// KEEP STORE COPY

	SIOCW(10);						// Select WR10 
	SIOCW(Channel->WR10 | 0x84);	// SET TO SEND CRC ON UNDERRUN

	if (Channel->TXBRG)
	{
	//;	NEED TO RESET BRG FOR TRANSMIT

		SIOCW(12);						// Select WR12
		SIOCW(Channel->TXBRG & 0xff);	// SET LSB
		SIOCW(13);						// Select WR13
		SIOCW(Channel->TXBRG >> 8);	// SET MSB
	}

//
//	Start TXDelay Timer
//

	WRITE_PORT_UCHAR(Channel->Mapped_SIOC, 5);
	WRITE_PORT_UCHAR(Channel->Mapped_SIOC, 0xeb); //RAISE RTS TO START SENDING FLAGS

	//MOV	WR10[BX],00100000B	; NRZI

	WRITE_PORT_UCHAR(Channel->Mapped_SIOC, 10);
	WRITE_PORT_UCHAR(Channel->Mapped_SIOC, Channel->WR10 | 0xA4); //Abort on underrun

	Interval.QuadPart = Int32x32To64(Channel->TXDELAY, -10000);

	KeSetTimer(&Channel->TXDelayTimer, Interval , &Channel->TXDelayDpc);

	return;
}

VOID TXDelay(IN PKDPC Dpc, IN PVOID Context, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
 //   PDEVICE_OBJECT          deviceObject;
 //   PDEVICE_Channel       deviceChannel;

 //   deviceObject = (PDEVICE_OBJECT)Context;
 //   deviceChannel = deviceObject->DeviceExtension;

	PHDLC_CHANNEL Channel = (PHDLC_CHANNEL)Context;

	DebugPrint(("BPQHDLC: TXDelay Expired - starting TX, DPC %x Context %x\n", Dpc, Context));

	Channel->SDFLAGS |= SDTINP;
	SETTVEC	SDDTTX;				// SET VECTOR TO 'TX DATA'

	SIOCW(SDTXCRC);					// RESET TX CRC GENERATOR

	SIOW (*Channel->SDTNEXT++);	// TRANSMIT First BYTE

	SIOCW(SDTXUND);					// RESET TX UNDERRUN LATCH

	Channel->RR0 &= !SDUNDER;		// KEEP STORE COPY

	SIOCW(10);						// Select WR10 
	SIOCW(Channel->WR10 | 0x84);	// SET TO SEND CRC ON UNDERRUN

}
