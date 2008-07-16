//
//	G8BPQ Serial Port Emulator
//
//	Provides a virtual COM port linked to an API
//

//	Based on ComEmulDrv serial port bridge driver
//
// Copyright (c) 2002 
// MixW team
// http://www.mixw.net
//

//		We dynamically add all COM devices, using a private IOCTL

/		So we need to create an API COntrol Device, to pass the "Add Device" requests over


//		When we add a COM port, we create two NT devices - one to emulate a real COM port using normal COM api,
//		the other to support the private BPQ API

#ifndef OLDVER
#include <ntddk.h>

#else
#include <wdm.h>
#endif

#include <ntddser.h >


#include "BPQVirtualCOM.h"

#include "BPQVCOMMsgs.h"

#define SERIAL_DEVICE_MAP L"SERIALCOMM"

#define DEVICENAME L"\\Device\\BPQSerial"
#define APIDEVICENAME L"\\Device\\BPQSerialAPI"
#define CONTROLDEVICENAME L"\\Device\\BPQSerialControl"

#define APIDOSDEVICENAME L"\\DosDevices\\Global\\BPQ"
#define CONTROLDOSDEVICENAME L"\\DosDevices\\Global\\BPQControl"
#define DOSDEVICENAME L"\\DosDevices\\Global\\COM"

#define REGNAME L"COM"


#define UNIBUFLEN 200

WCHAR NameBuf[MAXDEVICES][UNIBUFLEN];
WCHAR DosNameBuf[MAXDEVICES][UNIBUFLEN];

WCHAR APINameBuf[MAXDEVICES][UNIBUFLEN];
WCHAR APIDosNameBuf[MAXDEVICES][UNIBUFLEN];

WCHAR RegBuf[MAXDEVICES][UNIBUFLEN];


PDRIVER_OBJECT DriverObject;
UNICODE_STRING RegistryPath;

WCHAR SaveRegBuffer[UNIBUFLEN];


NTSTATUS DriverDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS DriverWriteDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp);
NTSTATUS DriverReadDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp);
NTSTATUS DriverCreateDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS DriverCloseDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);


NTSTATUS APIDriverDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS APIDriverWriteDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp);
NTSTATUS APIDriverReadDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp);

NTSTATUS AddDeviceRoutine(int iValue);


VOID SerialReadTimeout(IN PKDPC Dpc,IN PVOID DeferredContext,IN PVOID SystemContext1,IN PVOID SystemContext2);

VOID DriverCancelWaitIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp);
VOID DriverCheckEvent(IN PSERIAL_DEVICE_EXTENSION pExtension, IN ULONG events);

VOID LogMessage(IN PVOID pObj, IN ULONG val1, IN ULONG val2, IN ULONG val3,
                      IN ULONG val4, IN PVOID dump, IN USHORT dumplength);

VOID SerialLogError(
              IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT DeviceObject OPTIONAL,
              IN ULONG SequenceNumber,
              IN UCHAR MajorFunctionCode,
              IN UCHAR RetryCount,
              IN ULONG UniqueErrorValue,
              IN NTSTATUS FinalStatus,
              IN NTSTATUS SpecificIOStatus,
              IN ULONG LengthOfInsert1,
              IN PWCHAR Insert1,
              IN ULONG LengthOfInsert2,
              IN PWCHAR Insert2);


#define UNIBUFLEN 200

WCHAR NameBuf[MAXDEVICES][UNIBUFLEN];
WCHAR DosNameBuf[MAXDEVICES][UNIBUFLEN];

WCHAR APINameBuf[MAXDEVICES][UNIBUFLEN];
WCHAR APIDosNameBuf[MAXDEVICES][UNIBUFLEN];

WCHAR RegBuf[MAXDEVICES][UNIBUFLEN];


int DevicePresent[MAXDEVICES];

int GetStringForCom(int iPair, PWSTR pBuffer, PCWSTR pRegistryPath, PCWSTR pStrPrefix)
{
	UNICODE_STRING sString;

    NTSTATUS ntStatus;
    UNICODE_STRING Value;
	int iValue;
    RTL_QUERY_REGISTRY_TABLE Table[2];

	WCHAR swKey[UNIBUFLEN];
	UNICODE_STRING sKey, sNr;

	// Prepare output buffer

	sString.MaximumLength = UNIBUFLEN;
	sString.Length = 0;
	sString.Buffer = pBuffer;

	RtlZeroMemory(sString.Buffer, UNIBUFLEN*sizeof(WCHAR));

	RtlAppendUnicodeToString(&sString, pStrPrefix);

	// Prepare key name

	sKey.MaximumLength = UNIBUFLEN;
	sKey.Length = 0;
	sKey.Buffer = swKey;

	sNr.MaximumLength = UNIBUFLEN;
	sNr.Length = 0;
	sNr.Buffer = swKey+4;

	RtlZeroMemory(sKey.Buffer, UNIBUFLEN*sizeof(WCHAR));

	RtlAppendUnicodeToString(&sKey, L"Port");
	RtlIntegerToUnicodeString(iPair+1, 10, &sNr);
	sKey.Length += sNr.Length;

	// Read from registry

    RtlInitUnicodeString( &Value, NULL );
    RtlZeroMemory( Table, sizeof(Table) );

    Table[0].Flags        = RTL_QUERY_REGISTRY_DIRECT;
    Table[0].Name         = sKey.Buffer/*(PWSTR)pKey*/;
    Table[0].EntryContext = &Value;
    Table[0].DefaultType  = REG_SZ;
	Table[0].DefaultData = L"0";

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE,
                pRegistryPath,
                Table,
                NULL,
                NULL );

	if ( ntStatus == STATUS_SUCCESS )
	{
		RtlAppendUnicodeStringToString(&sString, &Value);
		RtlUnicodeStringToInteger(&Value, 10, &iValue);
		return iValue;
	}
	else
	{
		RtlAppendUnicodeToString(&sString, L"0");
		return 0;
	}
}

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	
	UNICODE_STRING uniDOSString;

	int i;
	PDEVICE_OBJECT p, k;

	// Delete sym links and registry

	for (i=0; i<MAXDEVICES; i++)
	{
		if (!DevicePresent[i])
			continue;

		RtlInitUnicodeString(&uniDOSString, DosNameBuf[i]);

		IoDeleteSymbolicLink (&uniDOSString);

		RtlInitUnicodeString(&uniDOSString, APIDosNameBuf[i]);

		IoDeleteSymbolicLink (&uniDOSString);

		RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP, NameBuf[i]/*DEVICENAME COMNUMBER1*/);

	}

	// Delete devices

	p = DriverObject->DeviceObject;

	while (p)
	{
		k = p->NextDevice;
		IoDeleteDevice(p);
		p = k;
	}
}


NTSTATUS PortDriverCreateDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{

	//	Called when createfile is called by user app

	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Create COM Called"),
                L"Debug Create Called",0,NULL);


	KeInitializeTimer(&pExtension->ReadRequestTotalTimer);

	KeInitializeDpc(&pExtension->TotalReadTimeoutDpc, SerialReadTimeout, pExtension);


	pExtension->BaudRate = 1200;
	pExtension->RTSstate = 0;
	pExtension->DTRstate = 0;

	pExtension->Timeouts.ReadIntervalTimeout = 0;
	pExtension->Timeouts.ReadTotalTimeoutMultiplier = 0;
	pExtension->Timeouts.ReadTotalTimeoutConstant = 0;
	pExtension->Timeouts.WriteTotalTimeoutMultiplier = 0;
	pExtension->Timeouts.WriteTotalTimeoutConstant = 0;


	//pExtension->Lc

	pExtension->BufHead = 0;
	pExtension->BufTail = 0;
	pExtension->OutBufHead = 0;
	pExtension->OutBufTail = 0;

 

	//pExtension->IsOpen = TRUE;

	KeInitializeSpinLock(&pExtension->ReadSpinLock);
	KeInitializeSpinLock(&pExtension->WriteSpinLock);
	KeInitializeSpinLock(&pExtension->IoctlSpinLock);

	pExtension->EventMask = 0;
	pExtension->HistoryEvents = 0;

	pExtension->pWaitIrp = NULL;
	pExtension->pReadIrp = NULL;

	pExtension->COMOpen = TRUE;


	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS APIDriverCreateDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{

	//	Called when createfile is called to open internal API

	PAPI_DEVICE_EXTENSION pExtension = (PAPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	PSERIAL_DEVICE_EXTENSION pComExtension = pExtension->pCOMDevice;

	
	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Create API Called"),
                L"Debug Create APICalled",0,NULL);



	//pExtension->IsOpen = TRUE;

	KeInitializeSpinLock(&pExtension->ReadSpinLock);
	KeInitializeSpinLock(&pExtension->WriteSpinLock);
	KeInitializeSpinLock(&pExtension->IoctlSpinLock);

	pExtension->EventMask = 0;
	pExtension->HistoryEvents = 0;

	pExtension->pWaitIrp = NULL;
	pExtension->pReadIrp = NULL;

	pComExtension->APIOpen = TRUE;

	pComExtension->CTSstate = 0;
	pComExtension->DSRstate = 0;
	pComExtension->DCDstate = 0;


	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS PortDriverCloseDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{

	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Close COMM Called"),
                L"Debug Close COMM Called",0,NULL);


	if (pExtension->pWaitIrp != NULL)

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
				sizeof(L"Debug Close Called with Wait Pending"),
				L"Debug Close Called with Waitad Pending",0,NULL);
		
		
	pExtension->pWaitIrp = NULL;

	if (pExtension->pReadIrp != NULL)

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
				sizeof(L"Debug Close Called with Read Pending"),
				L"Debug Close Called with Read Pending",0,NULL);


	pExtension->pReadIrp = NULL;

	pExtension->COMOpen = FALSE;

	
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS APIDriverCloseDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{

	PAPI_DEVICE_EXTENSION pExtension = (PAPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PSERIAL_DEVICE_EXTENSION pComExtension = pExtension->pCOMDevice;


	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Close API Called"),
                L"Debug Close API Called",0,NULL);


	if (pExtension->pWaitIrp != NULL)

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
				sizeof(L"Debug Close Called with Wait Pending"),
				L"Debug Close Called with Waitad Pending",0,NULL);
		
		
	pExtension->pWaitIrp = NULL;

	if (pExtension->pReadIrp != NULL)

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
				sizeof(L"Debug Close Called with Read Pending"),
				L"Debug Close Called with Read Pending",0,NULL);


	pExtension->pReadIrp = NULL;

	pComExtension->APIOpen = FALSE;

	
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}




NTSTATUS DriverEntry(IN PDRIVER_OBJECT ParamDriverObject,IN PUNICODE_STRING ParamRegistryPath)
{
	PDEVICE_OBJECT deviceObject1,deviceObject2;

	NTSTATUS status;

	UNICODE_STRING uniNameString, uniDOSString;
	UNICODE_STRING uniAPINameString, uniAPIDOSString;

	PSERIAL_DEVICE_EXTENSION pExtension1;

	int i;
	int cnt;

	DriverObject = ParamDriverObject;

	RegistryPath.MaximumLength = UNIBUFLEN;
	RegistryPath.Length = 0;
	RegistryPath.Buffer = SaveRegBuffer;

	RtlAppendUnicodeStringToString(&RegistryPath, ParamRegistryPath);



	for (i=0; i<MAXDEVICES; i++)
			DevicePresent[i] = FALSE;


	for (i=0; i<MAXDEVICES; i++)
	{
		cnt = 0;

		// Prepare strings

		cnt += GetStringForCom(i, NameBuf[i], RegistryPath.Buffer, DEVICENAME);

		cnt += GetStringForCom(i, DosNameBuf[i], RegistryPath.Buffer, DOSDEVICENAME);

		cnt += GetStringForCom(i, APINameBuf[i], RegistryPath.Buffer, APIDEVICENAME);

		cnt += GetStringForCom(i, APIDosNameBuf[i], RegistryPath.Buffer, APIDOSDEVICENAME);	
		
		cnt += GetStringForCom(i, RegBuf[i], RegistryPath.Buffer, REGNAME);

		if (cnt < 2)
		{
			continue;
		}


		RtlInitUnicodeString(&uniNameString, NameBuf[i]);

		RtlInitUnicodeString(&uniDOSString, DosNameBuf[i]);

		RtlInitUnicodeString(&uniAPINameString, APINameBuf[i]);

		RtlInitUnicodeString(&uniAPIDOSString, APIDosNameBuf[i]);

		status = IoCreateDevice(DriverObject, 
					sizeof(SERIAL_DEVICE_EXTENSION),
					&uniNameString,
					FILE_DEVICE_SERIAL_PORT,
					FILE_DEVICE_SECURE_OPEN, 
					TRUE, 
					&deviceObject1);

		if(!NT_SUCCESS(status))
		{
			DriverUnload(DriverObject);
			return status;
		}

		status = IoCreateDevice(DriverObject, 
					sizeof(API_DEVICE_EXTENSION),
					&uniAPINameString,
					FILE_DEVICE_SERIAL_PORT,
					FILE_DEVICE_SECURE_OPEN, 
					TRUE, 
					&deviceObject2);

		if(!NT_SUCCESS(status))
		{
			DriverUnload(DriverObject);
			return status;
		}

		DevicePresent[i] = TRUE;

		deviceObject1->Flags |= DO_BUFFERED_IO; // Buffered I/O only!
		deviceObject2->Flags |= DO_BUFFERED_IO; // Buffered I/O only!

		status = IoCreateSymbolicLink (&uniDOSString, &uniNameString);
		status = IoCreateSymbolicLink (&uniAPIDOSString, &uniAPINameString);


		status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP, NameBuf[i], REG_SZ,
									   RegBuf[i], (wcslen(RegBuf[i]) + 1) * sizeof(WCHAR));


		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->APIOpen = FALSE;

		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->COMOpen = FALSE;


		((PAPI_DEVICE_EXTENSION)deviceObject2->DeviceExtension)->pCOMDevice = ((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension);

		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->COMDevice = TRUE;

		((PSERIAL_DEVICE_EXTENSION)deviceObject2->DeviceExtension)->COMDevice = FALSE;

	}


	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCloseDispatch;
	
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DriverWriteDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = DriverReadDispatch;
	
	DriverObject->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}


NTSTATUS PortDriverDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{

	UNICODE_STRING          numberString;
	WCHAR                   numberBuffer[100];

    NTSTATUS ntStatus;

	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );
    KIRQL OldIrql;

	pIrp->IoStatus.Information = 0;
	ntStatus = STATUS_SUCCESS;


    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )

     {
        case IOCTL_SERIAL_SET_BAUD_RATE: 
		{
			pExtension->BaudRate = ((PSERIAL_BAUD_RATE)(pIrp->AssociatedIrp.SystemBuffer))->BaudRate;

			pIrp->IoStatus.Information = 0;
			break;
		}

        case IOCTL_SERIAL_GET_BAUD_RATE: 
		{
            PSERIAL_BAUD_RATE Br = (PSERIAL_BAUD_RATE)pIrp->AssociatedIrp.SystemBuffer;
			Br->BaudRate = pExtension->BaudRate;

            pIrp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);
			break;
		}

        case IOCTL_SERIAL_SET_RTS:
		{
			pExtension->RTSstate = 1;
			break;
		}

        case IOCTL_SERIAL_CLR_RTS: 
		{
			pExtension->RTSstate = 0;
			break;
		}

        case IOCTL_SERIAL_SET_DTR:
		{
			pExtension->DTRstate = 1;
			break;
		}

        case IOCTL_SERIAL_CLR_DTR: 
		{
			pExtension->DTRstate = 0;
			break;
		}

 
		case IOCTL_SERIAL_GET_DTRRTS: 
		{
            ULONG ModemControl;
            ModemControl = pExtension->DTRstate + (pExtension->RTSstate<<1);
            
			*(PULONG)pIrp->AssociatedIrp.SystemBuffer = ModemControl;
            pIrp->IoStatus.Information = sizeof(ULONG);
			
			break;
		}

        case IOCTL_SERIAL_GET_MODEMSTATUS:
		{
			ULONG Cts, Dsr, Dcd;

			Cts = pExtension->CTSstate;
			Dsr = pExtension->DSRstate;
			Dcd = pExtension->DCDstate;

			*(PULONG)pIrp->AssociatedIrp.SystemBuffer = 
				(Cts ? SERIAL_CTS_STATE : 0) | (Dsr ? SERIAL_DSR_STATE : 0) | 
				(Dcd ? SERIAL_DCD_STATE : 0);
            pIrp->IoStatus.Information = sizeof(ULONG);

			break;
		}

        case IOCTL_SERIAL_SET_TIMEOUTS: 
		{
			pExtension->Timeouts = *((PSERIAL_TIMEOUTS)(pIrp->AssociatedIrp.SystemBuffer));
			break;
		}

        case IOCTL_SERIAL_GET_TIMEOUTS: 
		{
            *((PSERIAL_TIMEOUTS)pIrp->AssociatedIrp.SystemBuffer) = pExtension->Timeouts;
            pIrp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
			break;
		}

        case IOCTL_SERIAL_RESET_DEVICE: 
		{
			break;
		}

        case IOCTL_SERIAL_PURGE:
        {
			// ???
			break;
		}

        case IOCTL_SERIAL_SET_LINE_CONTROL: 
		{
			pExtension->Lc = *((PSERIAL_LINE_CONTROL)(pIrp->AssociatedIrp.SystemBuffer));
			break;
		}

        case IOCTL_SERIAL_GET_LINE_CONTROL: 
		{
			*((PSERIAL_LINE_CONTROL)(pIrp->AssociatedIrp.SystemBuffer)) = pExtension->Lc;

            pIrp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);
			break;
		}

        case IOCTL_SERIAL_SET_WAIT_MASK:
        {
            PIRP            pOldWaitIrp;
            PDRIVER_CANCEL  pOldCancelRoutine;

			pExtension->EventMask = *(PULONG)pIrp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);
           
			// Is there an Irp queued for this device?
			
			pOldWaitIrp = pExtension->pWaitIrp;

            if (pOldWaitIrp != NULL)
            {
                // yes, we have an Irp
				
				pOldCancelRoutine = IoSetCancelRoutine(pOldWaitIrp, NULL);

                // Did the old Irp have a cancel routine?
                
				if (pOldCancelRoutine != NULL)
                {
                    // Yes

                    pOldWaitIrp->IoStatus.Information = sizeof(ULONG);
                    *(PULONG)pOldWaitIrp->AssociatedIrp.SystemBuffer = 0;

                    pOldWaitIrp->IoStatus.Status = STATUS_SUCCESS;

                    pExtension->pWaitIrp = NULL;
                }
                else
                {
                    // No, so just drop it (why?)
                    pOldWaitIrp = NULL;
                }
            }


            KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);

            // Complete the old Wait Irp if present and had a cancel routine
			
			if (pOldWaitIrp != NULL)
                IoCompleteRequest(pOldWaitIrp, IO_NO_INCREMENT);

			break;
		}

        case IOCTL_SERIAL_GET_WAIT_MASK:
		{
			
            *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pExtension->EventMask;

            pIrp->IoStatus.Information = sizeof(ULONG);
			break;
		}

        case IOCTL_SERIAL_WAIT_ON_MASK:
        {
            PDRIVER_CANCEL  pOldCancelRoutine;

            KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);

            if ((pExtension->pWaitIrp != NULL) || (pExtension->EventMask == 0))
                ntStatus = STATUS_INVALID_PARAMETER;
            else 
			if ((pExtension->EventMask & pExtension->HistoryEvents) != 0)
            {
                // One of requested events has already occured, so complete now

                pIrp->IoStatus.Information = sizeof(ULONG);
                *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pExtension->EventMask & pExtension->HistoryEvents;
                pExtension->HistoryEvents = 0;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {

                // Queue the Irp to be completed when the event occurs
				
				pExtension->pWaitIrp = pIrp;

                ntStatus = STATUS_PENDING;

                IoSetCancelRoutine(pIrp, DriverCancelWaitIrp);

                // soll IRP abgebrochen werden?
                if (pIrp->Cancel)
                {
                    pOldCancelRoutine = IoSetCancelRoutine(pIrp, NULL);

                    // wurde Cancel-Routine schon aufgerufen?
                    if (pOldCancelRoutine != NULL)
                    {
                        // Nein, also IRP hier abbrechen
                        ntStatus = STATUS_CANCELLED;

                        pExtension->pWaitIrp = NULL;
                    }
                    else
                    {
                        // Ja, Cancel-Routine wird Request beenden
                        IoMarkIrpPending(pIrp);
                    }
                }
                else
                    IoMarkIrpPending(pIrp);
            }

            KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);
			break;
		}

        case IOCTL_SERIAL_GET_COMMSTATUS:
		{
            PSERIAL_STATUS  pStatus = (PSERIAL_STATUS)pIrp->AssociatedIrp.SystemBuffer;

			ULONG InputLen,OutputLen;

            KeAcquireSpinLock(&pExtension->ReadSpinLock, &OldIrql);

			InputLen = pExtension->OutBufHead - pExtension->OutBufTail;
			if (pExtension->OutBufHead < pExtension->OutBufTail)
				InputLen += COMBUFLEN;

            KeReleaseSpinLock(&pExtension->ReadSpinLock, OldIrql);

            KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

			OutputLen = pExtension->BufHead - pExtension->BufTail;
			if (pExtension->BufHead < pExtension->BufTail)
				OutputLen += COMBUFLEN;

            KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);

            RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sizeof(SERIAL_STATUS));
            pStatus->AmountInInQueue  = InputLen;
            pStatus->AmountInOutQueue = OutputLen;

            pIrp->IoStatus.Information = sizeof(SERIAL_STATUS);
			break;
		}

        case IOCTL_SERIAL_GET_CHARS:
		{
            RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sizeof(SERIAL_CHARS));
            pIrp->IoStatus.Information = sizeof(SERIAL_CHARS);
			break;
		}

        case IOCTL_SERIAL_GET_HANDFLOW:
		{
			// ?
			break;
		}


		case IOCTL_SERIAL_GETDATA:
		{
			
// In a METHOD_BUFFERED IOCTL, like a buffered read or write request, data transfer
// is performed through a copy of the user's buffer passed in the
// Irp- >AssociatedIrp.SystemBuffer field. The lengths of the input and output buffers
// are passed in the driver's IO_STACK_LOCATION structure in the
// Parameters.DeviceIoControl.InputBufferLength field,
// and the Parameters.DeviceIoControl.OutputBufferLength field. 
// These values represent the maximum number of bytes the driver should read or write
// in response to the buffered IOCTL.


			ULONG BufLen;
			ULONG i = 0;

			BufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

			KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

			while (i<BufLen && pExtension->BufHead!=pExtension->BufTail)
			{
				((CHAR*)pIrp->AssociatedIrp.SystemBuffer)[i] = 
					pExtension->Buffer[pExtension->BufTail];
				i++;
				pExtension->BufTail++;
				pExtension->BufTail %= COMBUFLEN;
			}

 			KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);
 
			pIrp->IoStatus.Information = i;			
		}

			break;

		
		case IOCTL_SERIAL_SETDATA:
		{

			ULONG DataLen;
			ULONG i = 0;
			PCHAR pData = pIrp->AssociatedIrp.SystemBuffer;
			PIRP pOldReadIrp = NULL;
			PDRIVER_CANCEL pOldCancelRoutine;

			DataLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

			KeAcquireSpinLock(&pExtension->ReadSpinLock, &OldIrql);

			for (i=0; i<DataLen; i++)
			{
				pExtension->OutBuffer[pExtension->OutBufHead] = pData[i];
				pExtension->OutBufHead++;
				pExtension->OutBufHead %= COMBUFLEN;
			}


			// if there is a queued read, complete it


			if (pExtension->pReadIrp != NULL) 
			{

				pOldReadIrp = pExtension->pReadIrp;

				pOldCancelRoutine = IoSetCancelRoutine(pOldReadIrp, NULL);

				// wurde Cancel-Routine schon aufgerufen?
				if (pOldCancelRoutine != NULL)
				{

					// Copy data to read irp

				
					PIO_STACK_LOCATION irpOldReadSp = IoGetCurrentIrpStackLocation( pOldReadIrp );

					ULONG BufLen = irpOldReadSp->Parameters.Read.Length;
					PCHAR pBuf = pOldReadIrp->AssociatedIrp.SystemBuffer;

					i=0;

					while ( i<BufLen && 
						(pExtension->OutBufHead != pExtension->OutBufTail) )
					{
						pBuf[i] = pExtension->OutBuffer[pExtension->OutBufTail];
						i++;
						pExtension->OutBufTail++;
						pExtension->OutBufTail %= COMBUFLEN;
					}

					pOldReadIrp->IoStatus.Information = i;
					
					// Nein, also Request beenden

					pOldReadIrp->IoStatus.Status = STATUS_SUCCESS;
	
					pExtension->pReadIrp = NULL;
				}
			else
				{
					// Ja, Cancel-Routine wird Request beenden
					pOldReadIrp = NULL;
				}

			}


			KeReleaseSpinLock(&pExtension->ReadSpinLock, OldIrql);

			if (pOldReadIrp != NULL)
			    IoCompleteRequest(pOldReadIrp, IO_NO_INCREMENT);


			pIrp->IoStatus.Information = i;	
			
			
			if (pExtension->OutBufHead != pExtension->OutBufTail)
				DriverCheckEvent(pExtension, SERIAL_EV_RXCHAR);

		}

			break;
			
		default:
		{

            numberString.Length = 0;
			numberString.MaximumLength = 20;
			numberString.Buffer = &numberBuffer[0];

			RtlIntegerToUnicodeString (
				(irpSp->Parameters.DeviceIoControl.IoControlCode & 0x3fff) >> 2, 10, &numberString);
 			
			SerialLogError(
				0, // pExtension->DeviceObject->DriverObject,
				DeviceObject,
				0,
				0,//(UCHAR)irpSp->Parameters.DeviceIoControl.IoControlCode,
                          0,
                          15,
                          STATUS_SUCCESS,
                          BPQ_DEBUG_MESSAGE,
                          sizeof(L"Unimplemented IOCTL Code"),
                          L"Unimplemented IOCTL Code",
                          numberString.Length,
						  numberString.Buffer);

			break;
		}
		 
	}

    pIrp->IoStatus.Status = ntStatus;

	if (ntStatus!=STATUS_PENDING)
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}


NTSTATUS
PortDriverWriteDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )

{

    NTSTATUS ntStatus = STATUS_SUCCESS;// Assume success

	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

	ULONG DataLen = irpSp->Parameters.Write.Length;
	PCHAR pData = pIrp->AssociatedIrp.SystemBuffer;

	ULONG i;
    KIRQL OldIrql;

 //   PIRP            pOldReadIrp = NULL;
 //   PDRIVER_CANCEL  pOldCancelRoutine;

	pIrp->IoStatus.Information = 0;
	ntStatus = STATUS_SUCCESS;

	if (DataLen == 0)
    {
		ntStatus = STATUS_SUCCESS;
    }
	else
	{        
		KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

		for (i=0; i<DataLen; i++)
		{
			pExtension->Buffer[pExtension->BufHead] = pData[i];
			pExtension->BufHead++;
			pExtension->BufHead %= COMBUFLEN;
		}

//		DriverCheckEvent(pExtension, SERIAL_EV_TXEMPTY);


        
		KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);

	}

    pIrp->IoStatus.Status = ntStatus;
    pIrp->IoStatus.Information = DataLen;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}

VOID DriverCancelCurrentReadIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                   OldIrql;


    IoReleaseCancelSpinLock(pIrp->CancelIrql);

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Read Cancel Called"),
                L"Debug Cancel Read Called",0,NULL);




    KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

    pExtension->pReadIrp = NULL;        

    KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}


NTSTATUS
PortDriverReadDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )

{

    NTSTATUS ntStatus = STATUS_SUCCESS;// Assume success

	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

	ULONG BufLen = irpSp->Parameters.Read.Length;
	PCHAR pBuf = pIrp->AssociatedIrp.SystemBuffer;

	ULONG i = 0;
    KIRQL OldIrql;

    PDRIVER_CANCEL              pOldCancelRoutine;
	LARGE_INTEGER TotalTime;
	BOOLEAN set;

	pIrp->IoStatus.Information = 0;

	if (BufLen == 0)
    {
		ntStatus = STATUS_SUCCESS;
    }
	else
	{

        KeAcquireSpinLock(&pExtension->ReadSpinLock, &OldIrql);

		//LogMessage(DeviceObject,0x33,DataLen,0,0,pData,DataLen);

		while ( i<BufLen && 
			(pExtension->OutBufHead != pExtension->OutBufTail) )
		{
			pBuf[i] = pExtension->OutBuffer[pExtension->OutBufTail];
			i++;
			pExtension->OutBufTail++;
			pExtension->OutBufTail %= COMBUFLEN;
		}

		pIrp->IoStatus.Information = i;

		if (i==0 && pExtension->pReadIrp==NULL)
		{
			
			// Nothing available. If timeouts are specified, kick off completion timer


			if (pExtension->Timeouts.ReadTotalTimeoutMultiplier != 0 || pExtension->Timeouts.ReadTotalTimeoutConstant != 0)
			{
               
				TotalTime.QuadPart =
					
					((LONGLONG)(UInt32x32To64(BufLen,pExtension->Timeouts.ReadTotalTimeoutMultiplier)
                                          
                                          + pExtension->Timeouts.ReadTotalTimeoutConstant)) * -10000;


				set = KeSetTimer(&pExtension->ReadRequestTotalTimer,TotalTime,&pExtension->TotalReadTimeoutDpc);
   
			}

			
			// Nothing available. If no read IRP already queued, queue this one
			//  and return PENDING


			pExtension->pReadIrp = pIrp;
			pIrp->IoStatus.Status = ntStatus = STATUS_PENDING;

			IoSetCancelRoutine(pIrp, DriverCancelCurrentReadIrp);

			if (pIrp->Cancel)		// Has it just been cancelled?
			{
				pOldCancelRoutine = IoSetCancelRoutine(pIrp, NULL);

				if (pOldCancelRoutine != NULL)
				{
					// Nein, also IRP hier abbrechen
					pIrp->IoStatus.Status = ntStatus = STATUS_CANCELLED;

					pExtension->pReadIrp = NULL;
				}
				else
				{
						// Ja, Cancel-Routine wird Request beenden
					IoMarkIrpPending(pIrp);
				}
			}
				else
			{
                    IoMarkIrpPending(pIrp);
			}


		}

        KeReleaseSpinLock(&pExtension->ReadSpinLock, OldIrql);
	
	}

    pIrp->IoStatus.Status = ntStatus;

	if (ntStatus != STATUS_PENDING)
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}

VOID DriverCancelWaitIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                   OldIrql;

    
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

		SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Wait Cancel Called"),
                L"Debug Wait Cancel Called",0,NULL);




    KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);

    pExtension->pWaitIrp = NULL;        

    KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

VOID DriverCheckEvent(IN PSERIAL_DEVICE_EXTENSION pExtension, IN ULONG events)
{
    PIRP            pOldWaitIrp = NULL;
    PDRIVER_CANCEL  pOldCancelRoutine;
    KIRQL           OldIrql;


//    LOCKED_PAGED_CODE();

    KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);

    pExtension->HistoryEvents |= events;

    events &= pExtension->EventMask;

    if ((pExtension->pWaitIrp != NULL) && (events != 0))
    {
        pOldWaitIrp = pExtension->pWaitIrp;

        pOldCancelRoutine = IoSetCancelRoutine(pOldWaitIrp, NULL);

        // wurde Cancel-Routine schon aufgerufen?
        if (pOldCancelRoutine != NULL)
        {
            // Nein, also Request beenden
            pOldWaitIrp->IoStatus.Information = sizeof(ULONG);
            *(PULONG)pOldWaitIrp->AssociatedIrp.SystemBuffer = events;

            pOldWaitIrp->IoStatus.Status = STATUS_SUCCESS;

            pExtension->pWaitIrp      = NULL;
            pExtension->HistoryEvents = 0;
        }
        else
        {
            // Ja, Cancel-Routine wird Request beenden
            pOldWaitIrp = NULL;
        }
    }

    KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);

    if (pOldWaitIrp != NULL)
        IoCompleteRequest(pOldWaitIrp, IO_NO_INCREMENT);
}


VOID LogMessage(IN PVOID pObj, IN ULONG val1, IN ULONG val2, IN ULONG val3,
                      IN ULONG val4, IN PVOID dump, IN USHORT dumplength)
{
    PIO_ERROR_LOG_PACKET    pLogEntry;


    pLogEntry = IoAllocateErrorLogEntry(pObj, (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                              4*sizeof(ULONG) + dumplength));

    if (pLogEntry != NULL )
    {
        pLogEntry->ErrorCode = STATUS_SUCCESS;
        pLogEntry->SequenceNumber = 0;
        pLogEntry->MajorFunctionCode = 0;
        pLogEntry->IoControlCode = 0;
        pLogEntry->RetryCount = 0;
        pLogEntry->FinalStatus = STATUS_SUCCESS;
        pLogEntry->DumpDataSize = 4*sizeof(ULONG) + dumplength;

        pLogEntry->DumpData[0] = val1;
        pLogEntry->DumpData[1] = val2;
        pLogEntry->DumpData[2] = val3;
        pLogEntry->DumpData[3] = val4;

        RtlCopyMemory(&pLogEntry->DumpData[4], dump, dumplength);

        IoWriteErrorLogEntry(pLogEntry);
    }
}

VOID SerialLogError(
              IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT DeviceObject OPTIONAL,
              IN ULONG SequenceNumber,
              IN UCHAR MajorFunctionCode,
              IN UCHAR RetryCount,
              IN ULONG UniqueErrorValue,
              IN NTSTATUS FinalStatus,
              IN NTSTATUS SpecificIOStatus,
              IN ULONG LengthOfInsert1,
              IN PWCHAR Insert1,
              IN ULONG LengthOfInsert2,
              IN PWCHAR Insert2
              )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    P1,P2 - If phyical addresses for the controller ports involved
    with the error are available, put them through as dump data.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET errorLogEntry;

   PVOID objectToUse;
   PUCHAR ptrToFirstInsert;
   PUCHAR ptrToSecondInsert;

   PAGED_CODE();

   if (Insert1 == NULL) {
      LengthOfInsert1 = 0;
   }

   if (Insert2 == NULL) {
      LengthOfInsert2 = 0;
   }


   if (ARGUMENT_PRESENT(DeviceObject)) {

      objectToUse = DeviceObject;

   } else {

      objectToUse = DriverObject;

   }

   errorLogEntry = IoAllocateErrorLogEntry(
                                          objectToUse,
                                          (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                                  LengthOfInsert1 +
                                                  LengthOfInsert2)
                                          );

   if ( errorLogEntry != NULL ) {

      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = MajorFunctionCode;
      errorLogEntry->RetryCount = RetryCount;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = 0;

      ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if (LengthOfInsert1) {

         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory(
                      ptrToFirstInsert,
                      Insert1,
                      LengthOfInsert1
                      );

         if (LengthOfInsert2) {

            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory(
                         ptrToSecondInsert,
                         Insert2,
                         LengthOfInsert2
                         );

         }

      }

      IoWriteErrorLogEntry(errorLogEntry);

   }

}

VOID
SerialReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is used to complete a read because its total
    timer has expired.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PSERIAL_DEVICE_EXTENSION pExtension = DeferredContext;
    KIRQL oldIrql;
	PIRP pOldReadIrp = NULL;
    PDRIVER_CANCEL  pOldCancelRoutine;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

// if there is a queued read, complete it 

	KeAcquireSpinLock(&pExtension->ReadSpinLock, &oldIrql);

	if (pExtension->pReadIrp != NULL) 
	{
		pOldReadIrp = pExtension->pReadIrp;

		pOldCancelRoutine = IoSetCancelRoutine(pOldReadIrp, NULL);

		if (pOldCancelRoutine != NULL)
		{
				pOldReadIrp->IoStatus.Information = 0;
					
				pOldReadIrp->IoStatus.Status = STATUS_SUCCESS;
	
				pExtension->pReadIrp = NULL;
		}
		else
		{
				pOldReadIrp = NULL;
		}

	}


	KeReleaseSpinLock(&pExtension->ReadSpinLock, oldIrql);

	if (pOldReadIrp != NULL)
		IoCompleteRequest(pOldReadIrp, IO_NO_INCREMENT);

	return;

}


// Dispatch Routines

// Call the appropriate driver, depending on device type (COM or API)

NTSTATUS DriverCreateDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (pExtension->COMDevice)

		return PortDriverCreateDispatch(DeviceObject,Irp);
	else
		return APIDriverCreateDispatch(DeviceObject,Irp);
}

NTSTATUS DriverCloseDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (pExtension->COMDevice)

		return PortDriverCloseDispatch(DeviceObject,Irp);
	else
		return APIDriverCloseDispatch(DeviceObject,Irp);
}



NTSTATUS DriverDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (pExtension->COMDevice)

		return PortDriverDeviceControl(DeviceObject,Irp);
	else
		return APIDriverDeviceControl(DeviceObject,Irp);
}

NTSTATUS DriverReadDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (pExtension->COMDevice)

		return PortDriverReadDispatch(DeviceObject,Irp);
	else
		return APIDriverReadDispatch(DeviceObject,Irp);


}


NTSTATUS DriverWriteDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (pExtension->COMDevice)

		return PortDriverWriteDispatch(DeviceObject,Irp);
	else
		return APIDriverWriteDispatch(DeviceObject,Irp);
}


NTSTATUS APIDriverDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{

	UNICODE_STRING          numberString;
	WCHAR                   numberBuffer[100];

    NTSTATUS ntStatus;

	PAPI_DEVICE_EXTENSION pAPIExtension = (PAPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	PSERIAL_DEVICE_EXTENSION pExtension = pAPIExtension->pCOMDevice;


    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );
    KIRQL OldIrql;

	pIrp->IoStatus.Information = 0;
	ntStatus = STATUS_SUCCESS;


    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )
	{

		case IOCTL_BPQ_ADD_DEVICE:
        {
            NTSTATUS status;
			int Port;
			
			Port = *(PULONG)pIrp->AssociatedIrp.SystemBuffer;

			status = AddDeviceRoutine(Port);

			*(PULONG)pIrp->AssociatedIrp.SystemBuffer = status;

			pIrp->IoStatus.Information = sizeof(ULONG);

			break;
		}
		case IOCTL_SERIAL_IS_COM_OPEN:
		{
            *((ULONG*)pIrp->AssociatedIrp.SystemBuffer) = pExtension->COMOpen;
 
			pIrp->IoStatus.Information = sizeof(ULONG);

			break;
		}

        case IOCTL_SERIAL_SET_CTS:
		{
			pExtension->CTSstate = 1;
			DriverCheckEvent(pExtension, SERIAL_EV_CTS);

			break;
		}

        case IOCTL_SERIAL_CLR_CTS: 
		{
			pExtension->CTSstate = 0;
			DriverCheckEvent(pExtension, SERIAL_EV_CTS);
			break;
		}

		
        case IOCTL_SERIAL_SET_DSR:
		{
			pExtension->DSRstate = 1;
			DriverCheckEvent(pExtension, SERIAL_EV_DSR);
			break;
		}

        case IOCTL_SERIAL_CLR_DSR: 
		{
			pExtension->DSRstate = 0;
			DriverCheckEvent(pExtension, SERIAL_EV_DSR);
			break;
		}

        case IOCTL_SERIAL_SET_DCD:
		{
			pExtension->DCDstate = 1;
			DriverCheckEvent(pExtension, SERIAL_EV_RLSD);
			break;
		}

        case IOCTL_SERIAL_CLR_DCD: 
		{
			pExtension->DCDstate = 0;
			DriverCheckEvent(pExtension, SERIAL_EV_RLSD);
			break;
		}

		case IOCTL_SERIAL_GET_DTRRTS: 
		{
            ULONG ModemControl;
            ModemControl = pExtension->DTRstate + (pExtension->RTSstate<<1);
            
			*(PULONG)pIrp->AssociatedIrp.SystemBuffer = ModemControl;
            pIrp->IoStatus.Information = sizeof(ULONG);
			
			break;
		}

        case IOCTL_SERIAL_GET_MODEMSTATUS:
		{
			ULONG Cts, Dsr, Dcd;

			Cts = pExtension->CTSstate;
			Dsr = pExtension->DSRstate;
			Dcd = pExtension->DCDstate;

			*(PULONG)pIrp->AssociatedIrp.SystemBuffer = 
				(Cts ? SERIAL_CTS_STATE : 0) | (Dsr ? SERIAL_DSR_STATE : 0) | 
				(Dcd ? SERIAL_DCD_STATE : 0);
            pIrp->IoStatus.Information = sizeof(ULONG);

			break;
		}

 
        case IOCTL_SERIAL_SET_WAIT_MASK:
        {
            PIRP            pOldWaitIrp;
            PDRIVER_CANCEL  pOldCancelRoutine;

			pAPIExtension->EventMask = *(PULONG)pIrp->AssociatedIrp.SystemBuffer;

/*            KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);
           
			// Is there an Irp queued for this device?
			
			pOldWaitIrp = pExtension->pWaitIrp;

            if (pOldWaitIrp != NULL)
            {
                // yes, we have an Irp
				
				pOldCancelRoutine = IoSetCancelRoutine(pOldWaitIrp, NULL);

                // Did the old Irp have a cancel routine?
                
				if (pOldCancelRoutine != NULL)
                {
                    // Yes

                    pOldWaitIrp->IoStatus.Information = sizeof(ULONG);
                    *(PULONG)pOldWaitIrp->AssociatedIrp.SystemBuffer = 0;

                    pOldWaitIrp->IoStatus.Status = STATUS_SUCCESS;

                    pExtension->pWaitIrp = NULL;
                }
                else
                {
                    // No, so just drop it (why?)
                    pOldWaitIrp = NULL;
                }
            }


            KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);

            // Complete the old Wait Irp if present and had a cancel routine
			
			if (pOldWaitIrp != NULL)
                IoCompleteRequest(pOldWaitIrp, IO_NO_INCREMENT);
*/
			break;
		}

  
		case IOCTL_SERIAL_GET_WAIT_MASK:
		{
			
            *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pAPIExtension->EventMask;

            pIrp->IoStatus.Information = sizeof(ULONG);
			break;
		}

 /*       case IOCTL_SERIAL_WAIT_ON_MASK:
        {
            PDRIVER_CANCEL  pOldCancelRoutine;

            KeAcquireSpinLock(&pExtension->IoctlSpinLock, &OldIrql);

            if ((pExtension->pWaitIrp != NULL) || (pExtension->EventMask == 0))
                ntStatus = STATUS_INVALID_PARAMETER;
            else 
			if ((pExtension->EventMask & pExtension->HistoryEvents) != 0)
            {
                // One of requested events has already occured, so complete now

                pIrp->IoStatus.Information = sizeof(ULONG);
                *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pExtension->EventMask & pExtension->HistoryEvents;
                pExtension->HistoryEvents = 0;
                ntStatus = STATUS_SUCCESS;
            }
            else
            {

                // Queue the Irp to be completed when the event occurs
				
				pExtension->pWaitIrp = pIrp;

                ntStatus = STATUS_PENDING;

                IoSetCancelRoutine(pIrp, DriverCancelWaitIrp);

                // soll IRP abgebrochen werden?
                if (pIrp->Cancel)
                {
                    pOldCancelRoutine = IoSetCancelRoutine(pIrp, NULL);

                    // wurde Cancel-Routine schon aufgerufen?
                    if (pOldCancelRoutine != NULL)
                    {
                        // Nein, also IRP hier abbrechen
                        ntStatus = STATUS_CANCELLED;

                        pExtension->pWaitIrp = NULL;
                    }
                    else
                    {
                        // Ja, Cancel-Routine wird Request beenden
                        IoMarkIrpPending(pIrp);
                    }
                }
                else
                    IoMarkIrpPending(pIrp);
            }

            KeReleaseSpinLock(&pExtension->IoctlSpinLock, OldIrql);
			break;
		}

   */
     case IOCTL_SERIAL_GET_COMMSTATUS:
		{
            PSERIAL_STATUS  pStatus = (PSERIAL_STATUS)pIrp->AssociatedIrp.SystemBuffer;

			ULONG InputLen,OutputLen;

            KeAcquireSpinLock(&pExtension->ReadSpinLock, &OldIrql);

			InputLen = pExtension->OutBufHead - pExtension->OutBufTail;
			if (pExtension->OutBufHead < pExtension->OutBufTail)
				InputLen += COMBUFLEN;

            KeReleaseSpinLock(&pExtension->ReadSpinLock, OldIrql);

            KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

			OutputLen = pExtension->BufHead - pExtension->BufTail;
			if (pExtension->BufHead < pExtension->BufTail)
				OutputLen += COMBUFLEN;

            KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);

            RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sizeof(SERIAL_STATUS));
            pStatus->AmountInInQueue  = OutputLen;
            pStatus->AmountInOutQueue = InputLen;

            pIrp->IoStatus.Information = sizeof(SERIAL_STATUS);
			break;
		}

		case IOCTL_SERIAL_GETDATA:
		{
			
// In a METHOD_BUFFERED IOCTL, like a buffered read or write request, data transfer
// is performed through a copy of the user's buffer passed in the
// Irp- >AssociatedIrp.SystemBuffer field. The lengths of the input and output buffers
// are passed in the driver's IO_STACK_LOCATION structure in the
// Parameters.DeviceIoControl.InputBufferLength field,
// and the Parameters.DeviceIoControl.OutputBufferLength field. 
// These values represent the maximum number of bytes the driver should read or write
// in response to the buffered IOCTL.


			ULONG BufLen;
			ULONG i = 0;

			BufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

			KeAcquireSpinLock(&pExtension->WriteSpinLock, &OldIrql);

			while (i<BufLen && pExtension->BufHead!=pExtension->BufTail)
			{
				((CHAR*)pIrp->AssociatedIrp.SystemBuffer)[i] = 
					pExtension->Buffer[pExtension->BufTail];
				i++;
				pExtension->BufTail++;
				pExtension->BufTail %= COMBUFLEN;
			}

 			KeReleaseSpinLock(&pExtension->WriteSpinLock, OldIrql);
 
			pIrp->IoStatus.Information = i;			
		}

			break;

		
		case IOCTL_SERIAL_SETDATA:
		{

			ULONG DataLen;
			ULONG i = 0;
			PCHAR pData = pIrp->AssociatedIrp.SystemBuffer;
			PIRP pOldReadIrp = NULL;
			PDRIVER_CANCEL pOldCancelRoutine;

			DataLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

			KeAcquireSpinLock(&pExtension->ReadSpinLock, &OldIrql);

			for (i=0; i<DataLen; i++)
			{
				pExtension->OutBuffer[pExtension->OutBufHead] = pData[i];
				pExtension->OutBufHead++;
				pExtension->OutBufHead %= COMBUFLEN;
			}


			// if there is a queued read, complete it


			if (pExtension->pReadIrp != NULL) 
			{

				pOldReadIrp = pExtension->pReadIrp;

				pOldCancelRoutine = IoSetCancelRoutine(pOldReadIrp, NULL);

				// wurde Cancel-Routine schon aufgerufen?
				if (pOldCancelRoutine != NULL)
				{

					// Copy data to read irp

				
					PIO_STACK_LOCATION irpOldReadSp = IoGetCurrentIrpStackLocation( pOldReadIrp );

					ULONG BufLen = irpOldReadSp->Parameters.Read.Length;
					PCHAR pBuf = pOldReadIrp->AssociatedIrp.SystemBuffer;

					i=0;

					while ( i<BufLen && 
						(pExtension->OutBufHead != pExtension->OutBufTail) )
					{
						pBuf[i] = pExtension->OutBuffer[pExtension->OutBufTail];
						i++;
						pExtension->OutBufTail++;
						pExtension->OutBufTail %= COMBUFLEN;
					}

					pOldReadIrp->IoStatus.Information = i;
					
					// Nein, also Request beenden

					pOldReadIrp->IoStatus.Status = STATUS_SUCCESS;
	
					pExtension->pReadIrp = NULL;
				}
			else
				{
					// Ja, Cancel-Routine wird Request beenden
					pOldReadIrp = NULL;
				}

			}


			KeReleaseSpinLock(&pExtension->ReadSpinLock, OldIrql);

			if (pOldReadIrp != NULL)
			    IoCompleteRequest(pOldReadIrp, IO_NO_INCREMENT);


			pIrp->IoStatus.Information = i;	
			
			
			if (pExtension->OutBufHead != pExtension->OutBufTail)
				DriverCheckEvent(pExtension, SERIAL_EV_RXCHAR);

		}

			break;
			
		default:
		{

            numberString.Length = 0;
			numberString.MaximumLength = 20;
			numberString.Buffer = &numberBuffer[0];

			RtlIntegerToUnicodeString (
				(irpSp->Parameters.DeviceIoControl.IoControlCode & 0x3fff) >> 2, 10, &numberString);
 			
			SerialLogError(
				0, // pExtension->DeviceObject->DriverObject,
				DeviceObject,
				0,
				0,//(UCHAR)irpSp->Parameters.DeviceIoControl.IoControlCode,
                          0,
                          15,
                          STATUS_SUCCESS,
                          BPQ_DEBUG_MESSAGE,
                          sizeof(L"Unimplemented IOCTL Code"),
                          L"Unimplemented IOCTL Code",
                          numberString.Length,
						  numberString.Buffer);

			break;
		}
		 
	}

    pIrp->IoStatus.Status = ntStatus;

	if (ntStatus!=STATUS_PENDING)
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}

NTSTATUS APIDriverReadDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{

    NTSTATUS ntStatus = STATUS_SUCCESS;// Assume success

	PAPI_DEVICE_EXTENSION pAPIExtension = (PAPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;	
	PSERIAL_DEVICE_EXTENSION pExtension = pAPIExtension->pCOMDevice;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

	ULONG BufLen = irpSp->Parameters.Read.Length;
	PCHAR pBuf = pIrp->AssociatedIrp.SystemBuffer;

	ULONG i = 0;
    KIRQL OldIrql;

    PDRIVER_CANCEL              pOldCancelRoutine;
	LARGE_INTEGER TotalTime;
	BOOLEAN set;

	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Read Called"),
                L"Debug Read Called",0,NULL);



	pIrp->IoStatus.Information = 0;

	ntStatus = STATUS_SUCCESS;
    
    pIrp->IoStatus.Status = ntStatus;

	if (ntStatus != STATUS_PENDING)
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}

NTSTATUS APIDriverWriteDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{

    NTSTATUS ntStatus = STATUS_SUCCESS;// Assume success

	PAPI_DEVICE_EXTENSION pAPIExtension = (PAPI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;	
	PSERIAL_DEVICE_EXTENSION pExtension = pAPIExtension->pCOMDevice;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

	SerialLogError(0, 
				DeviceObject,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Debug Write Called"),
                L"Debug Read Called",0,NULL);



	pIrp->IoStatus.Information = 0;

	ntStatus = STATUS_SUCCESS;
    
    pIrp->IoStatus.Status = ntStatus;

	if (ntStatus != STATUS_PENDING)
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return ntStatus;
}

int GetStringForAdd(int iValue, PWSTR pBuffer, PCWSTR pStrPrefix);


NTSTATUS AddDeviceRoutine(int iValue)
{
	PDEVICE_OBJECT deviceObject1,deviceObject2;

	NTSTATUS status;

	UNICODE_STRING uniNameString, uniDOSString;
	UNICODE_STRING uniAPINameString, uniAPIDOSString;

	PSERIAL_DEVICE_EXTENSION pExtension1;

	int i;

	if (DriverObject->DriverUnload == DriverUnload)
	{
		SerialLogError(DriverObject, 
				0,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Add Device Obj OK"),
                L"Add Device Obj OK",0,NULL);
	}
	else
	{
		SerialLogError(DriverObject, 
				0,0,0,0,13,STATUS_SUCCESS,BPQ_DEBUG_MESSAGE,
                sizeof(L"Add Device Obj Duff"),
                L"Add Device Obj Duff",0,NULL);
	}


	for (i=0; i<MAXDEVICES; i++)
	{
		if (DevicePresent[i])
			continue;
	
		// We have a free entry
			

		GetStringForAdd(iValue, NameBuf[i], DEVICENAME);

		GetStringForAdd(iValue, DosNameBuf[i], DOSDEVICENAME);

		GetStringForAdd(iValue, APINameBuf[i], APIDEVICENAME);

		GetStringForAdd(iValue, APIDosNameBuf[i], APIDOSDEVICENAME);	
			
		GetStringForAdd(iValue, RegBuf[i], REGNAME);


		RtlInitUnicodeString(&uniNameString, NameBuf[i]);

		RtlInitUnicodeString(&uniDOSString, DosNameBuf[i]);

		RtlInitUnicodeString(&uniAPINameString, APINameBuf[i]);

		RtlInitUnicodeString(&uniAPIDOSString, APIDosNameBuf[i]);

		status = IoCreateDevice(DriverObject, 
					sizeof(SERIAL_DEVICE_EXTENSION),
					&uniNameString,
					FILE_DEVICE_SERIAL_PORT,
					0,//FILE_DEVICE_SECURE_OPEN, 
					TRUE, 
					&deviceObject1);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		status = IoCreateDevice(DriverObject, 
					sizeof(API_DEVICE_EXTENSION),
					&uniAPINameString,
					FILE_DEVICE_SERIAL_PORT,
					0,//FILE_DEVICE_SECURE_OPEN, 
					TRUE, 
					&deviceObject2);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		DevicePresent[i] = TRUE;

		deviceObject1->Flags |= DO_BUFFERED_IO; // Buffered I/O only!
		deviceObject2->Flags |= DO_BUFFERED_IO; // Buffered I/O only!

		status = IoCreateSymbolicLink (&uniDOSString, &uniNameString);
		status = IoCreateSymbolicLink (&uniAPIDOSString, &uniAPINameString);


		status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP, NameBuf[i]/*DEVICENAME COMNUMBER1*/, REG_SZ,
									   RegBuf[i]/*REGNAME COMNUMBER1*/, (wcslen(RegBuf[i]/*REGNAME COMNUMBER1*/) + 1) * sizeof(WCHAR));


		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->APIOpen = FALSE;

		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->COMOpen = FALSE;


		((PAPI_DEVICE_EXTENSION)deviceObject2->DeviceExtension)->pCOMDevice = ((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension);

		((PSERIAL_DEVICE_EXTENSION)deviceObject1->DeviceExtension)->COMDevice = TRUE;

		((PSERIAL_DEVICE_EXTENSION)deviceObject2->DeviceExtension)->COMDevice = FALSE;


		deviceObject1->Flags &= (~DO_DEVICE_INITIALIZING);
		deviceObject2->Flags &= (~DO_DEVICE_INITIALIZING);

		return STATUS_SUCCESS;
	}

	// No free entries

	return STATUS_UNSUCCESSFUL;
}

int GetStringForAdd(int iValue, PWSTR pBuffer, PCWSTR pStrPrefix)
{
	UNICODE_STRING sString;
    UNICODE_STRING Value;
	WCHAR numberBuffer[10];

	// Prepare output buffer

	sString.MaximumLength = UNIBUFLEN;
	sString.Length = 0;
	sString.Buffer = pBuffer;


	Value.Length = 0;
	Value.MaximumLength = 10;
	Value.Buffer = &numberBuffer[0];


	RtlZeroMemory(sString.Buffer, UNIBUFLEN*sizeof(WCHAR));

	RtlAppendUnicodeToString(&sString, pStrPrefix);

	RtlIntegerToUnicodeString(iValue, 10, &Value);

	RtlAppendUnicodeStringToString(&sString, &Value);
	
	return iValue;
}

