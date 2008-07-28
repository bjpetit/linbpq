/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   IOCTL.C

Abstract:

    This modules contains functions to register/deregsiter a control-
    deviceobject for ioctl purposes and dispatch routine for handling
    ioctl requests from usermode.

Revision History:

Notes:

--*/

#if defined(IOCTL_INTERFACE)


#include "miniport.h"
#include "public.h"

//
// Simple Mutual Exclusion constructs used in preference to
// using KeXXX calls since we don't have Mutex calls in NDIS.
// These can only be called at passive IRQL.
//

typedef struct _NIC_MUTEX
{
    ULONG                   Counter;
    ULONG                   ModuleAndLine;  // useful for debugging

} NIC_MUTEX, *PNIC_MUTEX;

#define NIC_INIT_MUTEX(_pMutex)                                 \
{                                                               \
    (_pMutex)->Counter = 0;                                     \
    (_pMutex)->ModuleAndLine = 0;                               \
}

#define NIC_ACQUIRE_MUTEX(_pMutex)                              \
{                                                               \
    while (NdisInterlockedIncrement((PLONG)&((_pMutex)->Counter)) != 1)\
    {                                                           \
        NdisInterlockedDecrement((PLONG)&((_pMutex)->Counter));        \
        NdisMSleep(10000);                                      \
    }                                                           \
    (_pMutex)->ModuleAndLine = ('I' << 16) | __LINE__;\
}

#define NIC_RELEASE_MUTEX(_pMutex)                              \
{                                                               \
    (_pMutex)->ModuleAndLine = 0;                               \
    NdisInterlockedDecrement((PLONG)&(_pMutex)->Counter);              \
}

#define LINKNAME_STRING     L"\\DosDevices\\BPQ32NET"
#define NTDEVICE_STRING     L"\\Device\\BPQ32NET"

//
// Global variables
//

NDIS_HANDLE        NdisDeviceHandle = NULL; // From NdisMRegisterDevice
LONG               MiniportCount = 0; // Total number of miniports in existance
PDEVICE_OBJECT     ControlDeviceObject = NULL;  // Device for IOCTLs
NIC_MUTEX          ControlDeviceMutex;
extern NDIS_HANDLE NdisWrapperHandle;

#pragma NDIS_PAGEABLE_FUNCTION(NICRegisterDevice)
#pragma NDIS_PAGEABLE_FUNCTION(NICDeregisterDevice)
#pragma NDIS_PAGEABLE_FUNCTION(NICDispatch)


NDIS_STATUS
NICRegisterDevice(
    VOID
    )
/*++

Routine Description:

    Register an ioctl interface - a device object to be used for this
    purpose is created by NDIS when we call NdisMRegisterDevice.

    This routine is called whenever a new miniport instance is
    initialized. However, we only create one global device object,
    when the first miniport instance is initialized. This routine
    handles potential race conditions with NICDeregisterDevice via
    the ControlDeviceMutex.

    NOTE: do not call this from DriverEntry; it will prevent the driver
    from being unloaded (e.g. on uninstall).

Arguments:

    None

Return Value:

    NDIS_STATUS_SUCCESS if we successfully register a device object.

--*/
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING      DeviceName;
    UNICODE_STRING      DeviceLinkUnicodeString;
    PDRIVER_DISPATCH    DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];

    DEBUGP(MP_TRACE, ("==>NICRegisterDevice\n"));

    NIC_ACQUIRE_MUTEX(&ControlDeviceMutex);

    ++MiniportCount;
    
    if (1 == MiniportCount)
    {
        NdisZeroMemory(DispatchTable, (IRP_MJ_MAXIMUM_FUNCTION+1) * sizeof(PDRIVER_DISPATCH));
        
        DispatchTable[IRP_MJ_CREATE] = NICDispatch;
        DispatchTable[IRP_MJ_CLEANUP] = NICDispatch;
        DispatchTable[IRP_MJ_CLOSE] = NICDispatch;
        DispatchTable[IRP_MJ_DEVICE_CONTROL] = NICDispatch;
        

        NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
        NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

        //
        // Create a device object and register our dispatch handlers
        //
        Status = NdisMRegisterDevice(
                    NdisWrapperHandle, 
                    &DeviceName,
                    &DeviceLinkUnicodeString,
                    &DispatchTable[0],
                    &ControlDeviceObject,
                    &NdisDeviceHandle
                    );
    }

    NIC_RELEASE_MUTEX(&ControlDeviceMutex);

    DEBUGP(MP_TRACE, ("<==NICRegisterDevice: %x\n", Status));

    return (Status);
}


NTSTATUS
NICDispatch(
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp
    )
/*++
Routine Description:

    Process IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
    Irp      - pointer to an I/O Request Packet

Return Value:

    NTSTATUS - STATUS_SUCCESS always - change this when adding
    real code to handle ioctls.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               inlen, outlen;
    PUCHAR               buffer;
	PMP_ADAPTER			Adapter; 
	ULONG				i=0;
    PTCB				pTCB;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	PNDIS_PACKET		Packet;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
//    DEBUGP(MP_TRACE, ("==>NICDispatch %d\n", irpStack->MajorFunction));

	Irp->IoStatus.Information = 0;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            break;
        
        case IRP_MJ_CLEANUP:
            break;
        
        case IRP_MJ_CLOSE:
            break;        
        
        case IRP_MJ_DEVICE_CONTROL: 
        {

          buffer = Irp->AssociatedIrp.SystemBuffer;  
          inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
          outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

		  NdisAcquireSpinLock(&GlobalData.Lock);

		  if (IsListEmpty(&GlobalData.AdapterList))
		  {

		
			DEBUGP(MP_TRACE, ("IOCTL Adapter Gone\n"));

		  NdisReleaseSpinLock(&GlobalData.Lock);

		  break;
		  }

		  Adapter = (PMP_ADAPTER) &GlobalData.AdapterList;
		  Adapter = (PMP_ADAPTER) Adapter->List.Flink;

		  NdisReleaseSpinLock(&GlobalData.Lock);

         switch (irpStack->Parameters.DeviceIoControl.IoControlCode) 
          {

		  case IOCTL_BPQdrv_GETMACADDR:
 
				if (outlen >=12)			// Room for 2 mc addresses
				{
					memcpy(buffer, Adapter->PermanentAddress, 6);
					memcpy(&buffer[6], Adapter->CurrentAddress, 6);

					Irp->IoStatus.Information = 12;	

				}
					
				break;

            
		  case IOCTL_BPQdrv_READ_DATA:

			
				// In a METHOD_BUFFERED IOCTL, like a buffered read or write request, data transfer
				// is performed through a copy of the user's buffer passed in the
				// Irp- >AssociatedIrp.SystemBuffer field. The lengths of the input and output buffers
				// are passed in the driver's IO_STACK_LOCATION structure in the
				// Parameters.DeviceIoControl.InputBufferLength field,
				// and the Parameters.DeviceIoControl.OutputBufferLength field. 
				// These values represent the maximum number of bytes the driver should read or write
				// in response to the buffered IOCTL.
  
				if (!MP_IS_READY(Adapter))	// Probably being disabled or hibernating
				{
					break;
				}

				NdisAcquireSpinLock(&Adapter->BPQLock);    

				if(IsListEmpty(&Adapter->TCBForBPQ))
				{
					NdisReleaseSpinLock(&Adapter->BPQLock);    
					break;
				}

				pTCB = (PTCB) RemoveHeadList(&Adapter->TCBForBPQ);   

				NdisReleaseSpinLock(&Adapter->BPQLock);  

				DEBUGP(MP_TRACE, ("<-- Get IOCTL TCB Len %x %d\n", pTCB, pTCB->ulSize));

				i = min(outlen, pTCB->ulSize);

				memcpy(buffer, &pTCB->Data[0], i);

				Irp->IoStatus.Information = i;	

				NICFreeSendTCB(Adapter, pTCB);

				DEBUGP(MP_TRACE, ("Received Read IOCTL\n"));
				break;


			case IOCTL_BPQdrv_WRITE_DATA:
				{
					PTCB           pTCB = NULL;
				    PNDIS_PACKET     RecvPacket = NULL;
					PNDIS_BUFFER     CurrentBuffer = NULL;   
					PLIST_ENTRY      pEntry;
					PRCB             pRCB;

					DEBUGP(MP_TRACE, ("IOCTL Write Adapter= %x\n",Adapter));

					// Get a TCB - just a source of buffer space and buffer descriptor

					if (inlen == 0)
						break;

					NdisAcquireSpinLock(&Adapter->SendLock);    

					if(IsListEmpty(&Adapter->SendFreeList))
					{
						DEBUGP(MP_WARNING, ("TCB not available......!\n")); 
						NdisReleaseSpinLock(&Adapter->SendLock);            
						break;
					}

					pTCB = (PTCB) RemoveHeadList(&Adapter->SendFreeList);
            
					NdisReleaseSpinLock(&Adapter->SendLock);            


					memcpy(&pTCB->Data[0], buffer, inlen);

					NdisAdjustBufferLength(pTCB->Buffer, inlen);        

					// Allocate memory for RCB. 
					//
    
					pRCB = NdisAllocateFromNPagedLookasideList(&Adapter->RecvLookaside);
    
					if(!pRCB)
					{
						DEBUGP(MP_ERROR, ("Failed to allocate memory for RCB\n"));   
						break;
					}  
    
				    //
				    // Get a free recv packet descriptor from the list.
				    //
				    pEntry = (PLIST_ENTRY) NdisInterlockedRemoveHeadList(
					    &Adapter->RecvFreeList, 
						&Adapter->RecvLock);

					if(!pEntry)
					{
					    ++Adapter->RcvResourceErrors;
						NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, pRCB);
						break;
					}
					++Adapter->GoodReceives;
    
					RecvPacket = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);
        
        //
        // Prepare the recv packet
        //
					NdisReinitializePacket(RecvPacket);
					*((PTCB *)RecvPacket->MiniportReserved) = pTCB;

						//
						// Chain the TCB buffers to the packet
						//
        
					NdisChainBufferAtBack(RecvPacket, pTCB->Buffer);               
                  
					NDIS_SET_PACKET_STATUS(RecvPacket, NDIS_STATUS_SUCCESS);
    
					DEBUGP(MP_LOUD, ("RecvPkt= %p\n", RecvPacket));

						//
						// Initialize RCB 
						//
        
					NdisInitializeListHead(&pRCB->List);
					pRCB->Packet = RecvPacket;
             

        //
        // Insert the packet in the recv wait queue to be picked up by
        // the receive indication DPC.
        // 
					NdisAcquireSpinLock(&Adapter->RecvLock);
					InsertTailList(&Adapter->RecvWaitList, 
						    &pRCB->List);
					Adapter->nBusyRecv++;     
					ASSERT(Adapter->nBusyRecv <= NIC_MAX_BUSY_RECVS);
					NdisReleaseSpinLock(&Adapter->RecvLock);

        //
        // Fire a timer DPC. By specifing zero timeout, the DPC will
        // be serviced whenever the next system timer interrupt arrives.
        //
					NdisMSetTimer(&Adapter->RecvTimer, 0);
    
				}

                DEBUGP(MP_TRACE, ("Completed Write IOCTL\n"));
				
                break;


            default:
                status = STATUS_UNSUCCESSFUL;
                break;
          }
          break;  
        }
        default:
            break;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

//    DEBUGP(MP_TRACE, ("<== NIC Dispatch\n"));

    return status;

} 


NDIS_STATUS
NICDeregisterDevice(
    VOID
    )
/*++

Routine Description:

    Deregister the ioctl interface. This is called whenever a miniport
    instance is halted. When the last miniport instance is halted, we
    request NDIS to delete the device object

Arguments:

    NdisDeviceHandle - Handle returned by NdisMRegisterDevice

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(MP_TRACE, ("==>NICDeregisterDevice\n"));

    NIC_ACQUIRE_MUTEX(&ControlDeviceMutex);

    ASSERT(MiniportCount > 0);

    --MiniportCount;
    
    if (0 == MiniportCount)
    {
        //
        // All miniport instances have been halted.
        // Deregister the control device.
        //

        if (NdisDeviceHandle != NULL)
        {
            Status = NdisMDeregisterDevice(NdisDeviceHandle);
            NdisDeviceHandle = NULL;
        }
    }

    NIC_RELEASE_MUTEX(&ControlDeviceMutex);

    DEBUGP(MP_TRACE, ("<== NICDeregisterDevice: %x\n", Status));
    return Status;
    
}

#endif


