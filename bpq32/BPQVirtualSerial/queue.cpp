/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/


#include "internal.h"

VOID __cdecl Debugprintf(const WCHAR * format, ...);

VOID ProcessWriteBytes(CMyDevice *pDevice, __in_bcount(Length) PUCHAR Characters, __in SIZE_T Length);

CRingBuffer    m_RingBuffer;        // Ring buffer for pending data
IWDFIoQueue    *m_FxReadQueue;      // Manual queue for pending reads
IWDFIoQueue    *m_FxWaitQueue;      // Manual queue for pending IOCTL Wait on Mask

//
// IUnknown implementation
//

//
// Queue destructor.
// Free up the buffer, wait for thread to terminate and
// delete critical section.
//


CMyQueue::~CMyQueue(
    VOID
    )
/*++

Routine Description:


    IUnknown implementation of Release

Arguments:


Return Value:

--*/
{
        WUDF_TEST_DRIVER_ASSERT(m_Device);

        m_Device->Release();
}


//
// Initialize
HRESULT
CMyQueue::CreateInstance(
    __in  CMyDevice *pDevice,
    __in IWDFDevice *FxDevice,
    __out PCMyQueue *Queue
    )
/*++

Routine Description:


    CreateInstance creates an instance of the queue object.

Arguments:

    ppUkwn - OUT parameter is an IUnknown interface to the queue object

Return Value:

    HRESULT indicating success or failure

--*/
{
    CMyQueue *pMyQueue = new CMyQueue(pDevice);
    HRESULT hr;

    if (pMyQueue == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pMyQueue->Initialize(FxDevice);

    if (SUCCEEDED(hr))
        *Queue = pMyQueue;
    else
        pMyQueue->Release();

    return hr;
}

DWORD WINAPI ThreadProc(VOID * p)
{
	CMyDevice *pDevice = (CMyDevice *)p;

	Debugprintf(L"Background Thread Started %x", pDevice);

	while(p)
	{
		DWORD Available, Resp, Err = 0;

		if (pDevice->PipeConnected)
		{
			Resp = PeekNamedPipe(pDevice->PipeHandle, NULL, 0, NULL, &Available, NULL);

			if (Resp == 0)
			{
				Err = GetLastError();

				if (Err == ERROR_BROKEN_PIPE)
				{
					pDevice->PipeConnected = FALSE;
					Resp = DisconnectNamedPipe(pDevice->PipeHandle);
					Err = GetLastError();

					Debugprintf(L"Pipe Connection Lost");
				}
			}

			if (Available)
			{
				char Buffer[512];
	
				if (Available > 512)
					Available = 512;
		
				ReadFile(pDevice->PipeHandle, Buffer, Available, &Available, NULL);

				ProcessWriteBytes(pDevice, (PUCHAR)Buffer, Available);
			}
		}
		Sleep(100);
	}

	OutputDebugString(L"Background Thread Terminated");
	return 0;
}



HRESULT CMyQueue::Initialize(__in IWDFDevice *FxDevice)
{
    IWDFIoQueue *fxQueue;
    IUnknown *unknown = NULL;
    HRESULT hr;

    //
    // Initialize ring buffer
    //

    hr = m_RingBuffer.Initialize(DATA_BUFFER_SIZE);

    if (FAILED(hr))
    {
        goto Exit;
    }

    unknown = QueryIUnknown();

    //
    // Create the default queue
    //

    {
        hr = FxDevice->CreateIoQueue(unknown,
                                     TRUE,
                                     WdfIoQueueDispatchParallel,
                                     TRUE,
                                     FALSE,
                                     &fxQueue);
    }

    if (FAILED(hr))
    {
        goto Exit;
    }

    m_FxQueue = fxQueue;

    fxQueue->Release();

    //
    // Create a manual queue to hold pending read requests. By keeping
    // them in the queue, framework takes care of cancelling them if the app
    // exits
    //

	hr = FxDevice->CreateIoQueue(NULL, FALSE, WdfIoQueueDispatchManual, TRUE, FALSE, &fxQueue);

    if (FAILED(hr))
        goto Exit;

    m_FxReadQueue = fxQueue;

    fxQueue->Release();

	// Create Q for IOCTL WAIT_ON_MASK

	hr = FxDevice->CreateIoQueue(NULL, FALSE, WdfIoQueueDispatchManual, TRUE, FALSE, &fxQueue);

    if (FAILED(hr))
        goto Exit;

    m_FxWaitQueue = fxQueue;

    fxQueue->Release();

Exit:
    SAFE_RELEASE(unknown);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMyQueue::QueryInterface(__in REFIID InterfaceId, __out PVOID *Object)
/*++

Routine Description:

    Query Interface

Arguments:

    Follows COM specifications

Return Value:

    HRESULT indicating success or failure

--*/
{
    HRESULT hr;


    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite))) {
        *Object = QueryIQueueCallbackWrite();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackCreate))) {
        *Object = QueryIQueueCallbackCreate();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackRead))) {
        *Object = QueryIQueueCallbackRead();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDeviceIoControl))) {
        *Object = QueryIQueueCallbackDeviceIoControl();
        hr = S_OK;
    } else {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnDeviceIoControl(
    __in IWDFIoQueue *pWdfQueue,
    __in IWDFIoRequest *pWdfRequest,
    __in ULONG ControlCode,
    __in SIZE_T InputBufferSizeInBytes,
    __in SIZE_T OutputBufferSizeInBytes
    )
/*++

Routine Description:

    DeviceIoControl dispatch routine

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    ControlCode - IO Control Code
    InputBufferSizeInBytes - Length of input buffer
    OutputBufferSizeInBytes - Length of output buffer

    Always succeeds DeviceIoIoctl
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(InputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(pWdfQueue);

    HRESULT hr = S_OK;
    SIZE_T reqCompletionInfo = 0;
    IWDFMemory *inputMemory = NULL;
    IWDFMemory *outputMemory = NULL;
    UINT i;

    WUDF_TEST_DRIVER_ASSERT(pWdfRequest);
    WUDF_TEST_DRIVER_ASSERT(m_Device);


    switch (ControlCode)
    {
		case IOCTL_SERIAL_GET_COMMSTATUS:
		{
            SERIAL_STATUS Status;
			SIZE_T availableData = 0;

            ZeroMemory(&Status, sizeof(SERIAL_STATUS));

			m_RingBuffer.GetAvailableData(&availableData);

            Status.AmountInInQueue = availableData;

            pWdfRequest->GetOutputMemory(&outputMemory);

            if (NULL == outputMemory)
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

            if (SUCCEEDED(hr))
                hr = outputMemory->CopyFromBuffer(0, (void*) &Status, sizeof(SERIAL_STATUS));

            if (SUCCEEDED(hr))
                reqCompletionInfo = sizeof(SERIAL_STATUS);

            break;
        }

		case IOCTL_SERIAL_SET_WAIT_MASK:
        {
            //
            //
            ULONG *pMask = NULL;

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pMask = m_Device->GetWaitMaskPtr();
                WUDF_TEST_DRIVER_ASSERT(pMask);

                hr = inputMemory->CopyToBuffer(0,
                                               (void*) pMask,
                                               sizeof(ULONG));
            }

			Debugprintf(L"Set Wait Mask %x", m_Device->GetWaitMask());

            break;
        }
        case IOCTL_SERIAL_GET_WAIT_MASK:
        {
            ULONG *pMask = NULL;

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pMask = m_Device->GetWaitMaskPtr();
                WUDF_TEST_DRIVER_ASSERT(pMask);

                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) pMask,
                                                  sizeof(ULONG));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(ULONG);
            }

			Debugprintf(L"Get Wait Mask %x", m_Device->GetWaitMask());

            break;
        }

        case IOCTL_SERIAL_WAIT_ON_MASK:
        {
            ULONG Mask = m_Device->GetWaitMask();
			ULONG n;
			ULONG DAV = 1;

			Debugprintf(L"Wait On Mask - Mask is %x", Mask);

			if (Mask & 1)				// RX Avail
			{
				m_RingBuffer.GetAvailableData(&n);

				if (n)
				{
					Debugprintf(L"%d Avail - Completing", n);
					
					pWdfRequest->GetOutputMemory(&outputMemory);

					if (NULL == outputMemory)
				        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
					
					if (SUCCEEDED(hr))
						hr = outputMemory->CopyFromBuffer(0, (void*) &DAV, sizeof(ULONG));

					if (SUCCEEDED(hr))
						reqCompletionInfo = sizeof(ULONG);

					Debugprintf(L"Wait On Mask Returned %x", DAV);

					break;
				}
			}
			// Save till data avail

			pWdfRequest->ForwardToIoQueue(m_FxWaitQueue);

			Debugprintf(L"No Data  - Wait On Mask Queued");
		
			return;
		}
        case IOCTL_SERIAL_SET_BAUD_RATE:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the baud rate and don't do
            // anything with it.
            //
			
			SERIAL_BAUD_RATE baudRateBuffer;
            
			Debugprintf(L"IOCTL_SERIAL_SET_BAUD_RATE");

			ZeroMemory(&baudRateBuffer, sizeof(SERIAL_BAUD_RATE));

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &baudRateBuffer,
                                               sizeof(SERIAL_BAUD_RATE));
            }

            if (SUCCEEDED(hr))
            {
                m_Device->SetBaudRate(baudRateBuffer.BaudRate);
            }

            break;
        }
        case IOCTL_SERIAL_GET_BAUD_RATE:
        {
            SERIAL_BAUD_RATE baudRateBuffer;
 			Debugprintf(L"IOCTL_SERIAL_GET_BAUD_RATE");
           ZeroMemory(&baudRateBuffer, sizeof(SERIAL_BAUD_RATE));

            baudRateBuffer.BaudRate = m_Device->GetBaudRate();

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &baudRateBuffer,
                                                  sizeof(SERIAL_BAUD_RATE));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_BAUD_RATE);
            }

            break;
        }
        case IOCTL_SERIAL_SET_MODEM_CONTROL:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the modem control register
            // configuration and don't do anything with it.
            //
            ULONG *pModemControlRegister = NULL;

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pModemControlRegister = m_Device->GetModemControlRegisterPtr();
                WUDF_TEST_DRIVER_ASSERT(pModemControlRegister);

                hr = inputMemory->CopyToBuffer(0,
                                               (void*) pModemControlRegister,
                                               sizeof(ULONG));
            }

            break;
        }
        case IOCTL_SERIAL_GET_MODEM_CONTROL:
        {
            ULONG *pModemControlRegister = NULL;

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pModemControlRegister = m_Device->GetModemControlRegisterPtr();
                WUDF_TEST_DRIVER_ASSERT(pModemControlRegister);

                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) pModemControlRegister,
                                                  sizeof(ULONG));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(ULONG);
            }

            break;
        }
        case IOCTL_SERIAL_SET_FIFO_CONTROL:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the FIFO control register
            // configuration and don't do anything with it.
            //
            ULONG *pFifoControlRegister = NULL;

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pFifoControlRegister = m_Device->GetFifoControlRegisterPtr();

                hr = inputMemory->CopyToBuffer(0,
                                               (void*) pFifoControlRegister,
                                               sizeof(ULONG));
            }

            break;
        }
        case IOCTL_SERIAL_GET_LINE_CONTROL:
        {
            ULONG *pLineControlRegister = NULL;
            SERIAL_LINE_CONTROL lineControl;
            ULONG lineControlSnapshot;

            ZeroMemory(&lineControl, sizeof(SERIAL_LINE_CONTROL));

            pLineControlRegister = m_Device->GetLineControlRegisterPtr();
            WUDF_TEST_DRIVER_ASSERT(pLineControlRegister);

            //
            // Take a snapshot of the line control register variable
            //
            lineControlSnapshot = *pLineControlRegister;

            //
            // Decode the word length
            //
            if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_5_DATA)
            {
                lineControl.WordLength = 5;
            }
            else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_6_DATA)
            {
                lineControl.WordLength = 6;
            }
            else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_7_DATA)
            {
                lineControl.WordLength = 7;
            }
            else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_8_DATA)
            {
                lineControl.WordLength = 8;
            }

            //
            // Decode the parity
            //
            if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_NONE_PARITY)
            {
                lineControl.Parity = NO_PARITY;
            }
            else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_ODD_PARITY)
            {
                lineControl.Parity = ODD_PARITY;
            }
            else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_EVEN_PARITY)
            {
                lineControl.Parity = EVEN_PARITY;
            }
            else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_MARK_PARITY)
            {
                lineControl.Parity = MARK_PARITY;
            }
            else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_SPACE_PARITY)
            {
                lineControl.Parity = SPACE_PARITY;
            }

            //
            // Decode the length of the stop bit
            //
            if (lineControlSnapshot & SERIAL_2_STOP)
            {
                if (lineControl.WordLength == 5)
                {
                    lineControl.StopBits = STOP_BITS_1_5;
                }
                else
                {
                    lineControl.StopBits = STOP_BITS_2;
                }
            }
            else
            {
                lineControl.StopBits = STOP_BIT_1;
            }

            //
            // Copy the information that was decoded to the caller's buffer
            //
            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &lineControl,
                                                  sizeof(SERIAL_LINE_CONTROL));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_LINE_CONTROL);
            }

            break;
        }
        case IOCTL_SERIAL_SET_LINE_CONTROL:
        {
            ULONG *pLineControlRegister = NULL;
            SERIAL_LINE_CONTROL lineControl;
            UCHAR lineControlData = 0;
            UCHAR lineControlStop = 0;
            UCHAR lineControlParity = 0;
            ULONG lineControlSnapshot;
            ULONG lineControlNew;
            ULONG lineControlPrevious;

            ZeroMemory(&lineControl, sizeof(SERIAL_LINE_CONTROL));

            pLineControlRegister = m_Device->GetLineControlRegisterPtr();
            WUDF_TEST_DRIVER_ASSERT(pLineControlRegister);

            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the line control register
            // configuration and don't do anything with it.
            //
            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &lineControl,
                                               sizeof(SERIAL_LINE_CONTROL));
            }

            //
            // Bits 0 and 1 of the line control register
            //
            if (SUCCEEDED(hr))
            {
                switch (lineControl.WordLength)
                {
                    case 5:
                        lineControlData = SERIAL_5_DATA;
                        m_Device->SetValidDataMask(0x1f);
                        break;

                    case 6:
                        lineControlData = SERIAL_6_DATA;
                        m_Device->SetValidDataMask(0x3f);
                        break;

                    case 7:
                        lineControlData = SERIAL_7_DATA;
                        m_Device->SetValidDataMask(0x7f);
                        break;

                    case 8:
                        lineControlData = SERIAL_8_DATA;
                        m_Device->SetValidDataMask(0xff);
                        break;

                    default:
                        hr = E_INVALIDARG;
                }
            }

            //
            // Bit 2 of the line control register
            //
            if (SUCCEEDED(hr))
            {
                switch (lineControl.StopBits)
                {
                    case STOP_BIT_1:
                        lineControlStop = SERIAL_1_STOP;
                        break;

                    case STOP_BITS_1_5:
                        if (lineControlData != SERIAL_5_DATA)
                        {
                            hr = E_INVALIDARG;
                            break;
                        }
                        lineControlStop = SERIAL_1_5_STOP;
                        break;

                    case STOP_BITS_2:
                        if (lineControlData == SERIAL_5_DATA)
                        {
                            hr = E_INVALIDARG;
                            break;
                        }
                        lineControlStop = SERIAL_2_STOP;
                        break;

                    default:
                        hr = E_INVALIDARG;
                }
            }

            //
            // Bits 3, 4 and 5 of the line control register
            //
            if (SUCCEEDED(hr))
            {
                switch (lineControl.Parity)
                {
                    case NO_PARITY:
                        lineControlParity = SERIAL_NONE_PARITY;
                        break;

                    case EVEN_PARITY:
                        lineControlParity = SERIAL_EVEN_PARITY;
                        break;

                    case ODD_PARITY:
                        lineControlParity = SERIAL_ODD_PARITY;
                        break;

                    case SPACE_PARITY:
                        lineControlParity = SERIAL_SPACE_PARITY;
                        break;

                    case MARK_PARITY:
                        lineControlParity = SERIAL_MARK_PARITY;
                        break;

                    default:
                        hr = E_INVALIDARG;
                }
            }

            //
            // Update our line control register variable atomically
            //
            i=0;
            do
            {
                i++;
                if ((i & 0xf) == 0)
                {
                    //
                    // We've been spinning in a loop for a while trying to
                    // update the line control register variable atomically.
                    // Yield the CPU for other threads for a while.
                    //
                    SwitchToThread();
                }

                lineControlSnapshot = *pLineControlRegister;

                lineControlNew = (lineControlSnapshot & SERIAL_LCR_BREAK) |
                                    (lineControlData |
                                     lineControlParity |
                                     lineControlStop);

                lineControlPrevious = InterlockedCompareExchange((LONG *) pLineControlRegister,
                                                                 lineControlNew,
                                                                 lineControlSnapshot);

            } while (lineControlPrevious != lineControlSnapshot);

            break;
        }
        case IOCTL_SERIAL_GET_TIMEOUTS:
        {
            SERIAL_TIMEOUTS timeoutValues;

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                m_Device->GetTimeouts(&timeoutValues);

                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &timeoutValues,
                                                  sizeof(timeoutValues));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_TIMEOUTS);
            }

            break;
        }
        case IOCTL_SERIAL_SET_TIMEOUTS:
        {
            SERIAL_TIMEOUTS timeoutValues;

            ZeroMemory(&timeoutValues, sizeof(SERIAL_LINE_CONTROL));

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &timeoutValues,
                                               sizeof(timeoutValues));
            }

            if (SUCCEEDED(hr))
            {
                if ((timeoutValues.ReadIntervalTimeout == MAXULONG) &&
                    (timeoutValues.ReadTotalTimeoutMultiplier == MAXULONG) &&
                    (timeoutValues.ReadTotalTimeoutConstant == MAXULONG))
                {
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                m_Device->SetTimeouts(timeoutValues);
            }

            break;
        }
		default:
			
			Debugprintf(L"Unimplemented IOCTL %x", ControlCode);

    }

    //
    // clean up
    //
    if (inputMemory)
    {
        inputMemory->Release();
    }

    if (outputMemory)
    {
        outputMemory->Release();
    }

    //
    // complete the request
    //
    pWdfRequest->CompleteWithInformation(hr, reqCompletionInfo);

    return;
}

VOID STDMETHODCALLTYPE CMyQueue::OnWrite(IWDFIoQueue *pWdfQueue, IWDFIoRequest *pWdfRequest, SIZE_T BytesToWrite)
/*++

Routine Description:


    Write dispatch routine
    IQueueCallbackWrite

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToWrite - Length of bytes in the write buffer

    Allocate and copy data to local buffer
Return Value:

    VOID

--*/
{
    IWDFMemory* pRequestMemory = NULL;
    IWDFIoRequest* pSavedRequest = NULL;
    SIZE_T availableData = 0;
    SIZE_T savedRequestBufferSize = 0;
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pWdfQueue);

    //
    // Get memory object
    //

	Debugprintf(L"OnWrite Called %d", BytesToWrite);

    pWdfRequest->GetInputMemory(&pRequestMemory);

    //
    // Process input. Send to Pipe if connected
    //

	if (this->m_Device->PipeConnected)
	{
		// Have to escape all oxff chars, as these are used to report status info 

		UCHAR NewMessage[1000];
		UCHAR * ptr1 = (PUCHAR)pRequestMemory->GetDataBuffer(NULL);
		UCHAR * ptr2 = NewMessage;
		UCHAR c;
		DWORD Written, NewLen = 0;
		int Length = BytesToWrite;
		int Resp;

		while (Length != 0)
		{
			c = *(ptr1++);
			*(ptr2++) = c;
			NewLen++;

			if (c == 0xff)
			{
				*(ptr2++) = c;
				NewLen++;
			}
			Length--;

			if (NewLen > 998)
			{
				// About to overflow temp buffer - write this, and reset pointers
				
				Resp = WriteFile(this->m_Device->PipeHandle, NewMessage, NewLen, &Written, NULL);

				NewLen = 0;
				ptr2 = NewMessage;
			}
		}

		Resp = WriteFile(this->m_Device->PipeHandle, NewMessage, NewLen, &Written, NULL);
	}

 //   ProcessWriteBytes((PUCHAR)pRequestMemory->GetDataBuffer(NULL), BytesToWrite);

    //
    // Release memory object and complete request
    //
    SAFE_RELEASE(pRequestMemory);
    pWdfRequest->CompleteWithInformation(hr, BytesToWrite);

    //
    // Get the amount of data available in the ring buffer
    //
    m_RingBuffer.GetAvailableData(&availableData);

    if (availableData > 0)
    {
        //
        // Continue with the next request, if there is one pending
        //
        hr = m_FxReadQueue->RetrieveNextRequest(&pSavedRequest);
        if ((pSavedRequest == NULL) || (FAILED(hr)))
        {
            goto Exit;
        }

        pSavedRequest->GetReadParameters(&savedRequestBufferSize, NULL, NULL);

        OnRead(m_FxQueue,
               pSavedRequest,
               savedRequestBufferSize);

        //
        // RetrieveNextRequest from a manual queue increments the reference
        // counter by 1. We need to decrement it, otherwise the request will
        // not be released and there will be an object leak.
        //
        SAFE_RELEASE(pSavedRequest);
    }

Exit:
    return;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnRead(
    __in IWDFIoQueue *pWdfQueue,
    __in IWDFIoRequest *pWdfRequest,
    __in SIZE_T SizeInBytes
    )
/*++

Routine Description:


    Read dispatch routine
    IQueueCallbackRead

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    SizeInBytes - Length of bytes in the read buffer

    Copy available data into the read buffer
Return Value:

    VOID

--*/
{
    IWDFMemory* pRequestMemory = NULL;
    SIZE_T BytesCopied = 0;
    HRESULT     hr;

    UNREFERENCED_PARAMETER(pWdfQueue);

	Debugprintf(L"Onread Called %d", SizeInBytes);

    //
    // Get memory object
    //
    pWdfRequest->GetOutputMemory(&pRequestMemory);

    hr = m_RingBuffer.Read((PBYTE)pRequestMemory->GetDataBuffer(NULL),
                            SizeInBytes,
                            &BytesCopied);

    //
    // Release memory object.
    //
    SAFE_RELEASE(pRequestMemory);

    if (FAILED(hr))
    {
        //
        // Error reading buffer
        //
        pWdfRequest->Complete(hr);
        goto Exit;
    }

    if (BytesCopied > 0)
    {
        //
        // Data was read from buffer succesfully
        //
        pWdfRequest->CompleteWithInformation(hr, BytesCopied);
    }
    else
    {
        //
        // No data to read. Queue the request for later processing.
        //
        pWdfRequest->ForwardToIoQueue(m_FxReadQueue);
    }

Exit:
    return;
}

VOID ProcessWriteBytes(CMyDevice *pDevice, __in_bcount(Length) PUCHAR Characters, __in SIZE_T Length)


/*++
Routine Description:

    This function is called when data is received from the pipe


    Characters - Bytes from pipe

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

   VOID

--*/

{
	IWDFIoRequest* pSavedRequest = NULL;
	SIZE_T availableData = 0;
	SIZE_T savedRequestBufferSize = 0;
	HRESULT hr = S_OK;
	UCHAR currentCharacter;
	UCHAR Reply[3] = {0xff};

	while (Length != 0)
	{
        currentCharacter = *(Characters++);
        Length--;

		if (currentCharacter == 0xff)			// Command Excape
		{
			currentCharacter = *(Characters++);
			Length--;

			switch(currentCharacter)
			{
				case 0xff:			// FF FF means FF
					
									
					if (pDevice->COMConnected)
						m_RingBuffer.Write(&currentCharacter, sizeof(currentCharacter));
					
					break;

				case 01:

						//Request for current connect status

						Reply[1] = pDevice->COMConnected;			
						DWORD Written;
		
						WriteFile(pDevice->PipeHandle, Reply, 2, &Written, NULL);
						break;
			}
		}
		else
		{
			if (pDevice->COMConnected)
				m_RingBuffer.Write(&currentCharacter, sizeof(currentCharacter));
		}
	}

	// See if we can complete any read requests
				 
	// Get the amount of data available in the ring buffer
				
	m_RingBuffer.GetAvailableData(&availableData);

	Debugprintf(L"avdata %d", availableData);

	if (availableData > 0)
	{        
		hr = m_FxReadQueue->RetrieveNextRequest(&pSavedRequest);

		Debugprintf(L"Retrieve Read Request Returned %d", hr);

		if ((pSavedRequest == NULL) || (FAILED(hr)))
		{
			// See if a pending Wait on Mask

			hr = m_FxWaitQueue->RetrieveNextRequest(&pSavedRequest);

			Debugprintf(L"Look for Queued Wait on Mask");

			if ((pSavedRequest == NULL) || (FAILED(hr)))
			{
				Debugprintf(L"Retrieve Wait Request Returned %d", hr);
				return;
			}
			
			// Post the WAIT request

			Debugprintf(L"Got Wait on Mask");

		    SIZE_T reqCompletionInfo = 0;
			IWDFMemory *outputMemory = NULL;
			ULONG DAV = 1;

			pSavedRequest->GetOutputMemory(&outputMemory);

			if (NULL == outputMemory)
				      hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
					
			if (SUCCEEDED(hr))
				hr = outputMemory->CopyFromBuffer(0, (void*) &DAV, sizeof(ULONG));

			if (SUCCEEDED(hr))
				reqCompletionInfo = sizeof(ULONG);

			Debugprintf(L"Wait On Mask Posted %x", DAV);

		    if (outputMemory)
				
				outputMemory->Release();
  
			pSavedRequest->CompleteWithInformation(hr, reqCompletionInfo);
			return;

		}

		pSavedRequest->GetReadParameters(&savedRequestBufferSize, NULL, NULL);

//		CMyQueue::OnRead(m_FxQueue, pSavedRequest, savedRequestBufferSize);

		IWDFMemory* pRequestMemory = NULL;
		SIZE_T BytesCopied = 0;
   
		pSavedRequest->GetOutputMemory(&pRequestMemory);

		hr = m_RingBuffer.Read((PBYTE)pRequestMemory->GetDataBuffer(NULL),
                            savedRequestBufferSize,
                            &BytesCopied);

		//
		// Release memory object.
		//
		SAFE_RELEASE(pRequestMemory);

	  if (FAILED(hr))
		{
			//
		  // Error reading buffer
			//
			pSavedRequest->Complete(hr);
			goto Exit;
		}

         pSavedRequest->CompleteWithInformation(hr, BytesCopied);

		// RetrieveNextRequest from a manual queue increments the reference
		// counter by 1. We need to decrement it, otherwise the request will
		// not be released and there will be an object leak.
Exit:
		SAFE_RELEASE(pSavedRequest);
	}

    return;
}

STDMETHODIMP_(void)
CMyQueue::OnCreateFile(
    __in IWDFIoQueue* pWdfQueue,
    __in IWDFIoRequest* pWdfRequest,
    __in IWDFFile* pWdfFileObject
    )

/*++

Routine Description:

    Create callback from the framework for this default parallel queue 
    
    The create request will create a socket connection , create a file i/o target associated 
    with the socket handle for this connection and store in the file object context.

Aruments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    pWdfFileObject - WDF file object for this create

 Return Value:

    VOID

--*/
{
	HRESULT hr = S_OK;
	UCHAR Reply[3] = {0xff, 1};
	DWORD Written;

	UNREFERENCED_PARAMETER(pWdfRequest);
    UNREFERENCED_PARAMETER(pWdfFileObject);
    UNREFERENCED_PARAMETER(pWdfQueue);

	Debugprintf(L"OnCreate Called");
	pWdfRequest->Complete(hr);

	this->m_Device->COMConnected = TRUE;

	if (this->m_Device->PipeConnected)
		WriteFile(this->m_Device->PipeHandle, Reply, 2, &Written, NULL);

	Debugprintf(L"Status Sent");

}
