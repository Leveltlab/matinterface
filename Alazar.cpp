
#include "stdafx.h"
#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "matinterface.h"
//#include <conio.h>
#include "alazar.h"

#define bufferCount 16
#define ERRSTRLEN 200

HANDLE g_boardHandle;
U16* g_Buffer[bufferCount];
LPVOID g_BaseAdress = NULL;

U32  g_bytesPerBuffer;
U32  g_samplesPerBuffer;

U32 g_buffersCompleted;
U32 g_idx;

void __stdcall ErrorDisplay(RETURN_CODE retCode, LPCWSTR Mssg)
{
	//char ErrMsg[ERRSTRLEN];
	WCHAR wErrMsg[ERRSTRLEN];

	//cbGetErrMsg(ErrCode, ErrMsg);
	mbstowcs(wErrMsg, AlazarErrorToText(retCode), ERRSTRLEN);

	MessageBox(NULL, wErrMsg, Mssg, MB_OK);

}

UINT16* getBuf()
{
	return g_Buffer[g_idx];
}


void AlazarGetBoardHandle()
{
	U32 systemId = 1;
	U32 boardId = 1;

	// Get a handle to the board and return a copy
	g_boardHandle = AlazarGetBoardBySystemID(systemId, boardId);

}

BOOL ConfigureBoard()
{
	RETURN_CODE retCode;


	U32 systemId = 1;
	U32 boardId = 1;

	// Get a handle to the board and return a copy
	g_boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
	if (g_boardHandle == NULL){
		ErrorDisplay(ApiFailed, L"Error: Unable to open board system ");
		return false;
	}
		
	//retCode = AlazarSetCaptureClock(g_boardHandle,
	//	FAST_EXTERNAL_CLOCK,
	//	SAMPLE_RATE_USER_DEF,
	//	CLOCK_EDGE_RISING,
	//	0);

	retCode = AlazarSetCaptureClock(g_boardHandle,
		INTERNAL_CLOCK,
		SAMPLE_RATE_100MSPS,
		CLOCK_EDGE_RISING,
		0);

	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetCaptureClock failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetCaptureClock failed");
		return FALSE;
	}
	cout << "AlazarSetCaptureClock" << endl;

	retCode = AlazarSetExternalClockLevel(g_boardHandle, (U32)65);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetExternalClockLevel failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetExternalClockLevel failed");
		return FALSE;
	}
	cout << "AlazarSetExternalClockLevel" << endl;


	retCode =	AlazarInputControl(
			g_boardHandle,            // HANDLE -- board handle
			CHANNEL_A,              // U8 -- input channel
			DC_COUPLING,            // U32 -- input coupling id
			INPUT_RANGE_PM_200_MV,     // U32 -- input range id
			IMPEDANCE_50_OHM        // U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarInputControl A failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarInputControl A failed");
		return FALSE;
	}
	cout << "AlazarInputControl A" << endl;


	retCode = 	AlazarInputControl(
			g_boardHandle,            // HANDLE -- board handle
			CHANNEL_B,              // U8 -- input channel
			DC_COUPLING,            // U32 -- input coupling id
			INPUT_RANGE_PM_200_MV,     // U32 -- input range id
			IMPEDANCE_50_OHM        // U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarInputControl B failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarInputControl B failed");
		return FALSE;
	}
	cout << "AlazarInputControl B " << endl;

	retCode = AlazarSetTriggerOperation(g_boardHandle,
		TRIG_ENGINE_OP_J,
		TRIG_ENGINE_J,
		TRIG_EXTERNAL,
		TRIGGER_SLOPE_POSITIVE, //input from scanbox needed + 1 for negative output
		160,  //160-128 = 32 => /127 =  0.25 * input 5 volt => 1.25 volt threshold
		TRIG_ENGINE_K,
		TRIG_DISABLE,
		TRIGGER_SLOPE_POSITIVE,
		128);

	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetTriggerOperation failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetTriggerOperation failed");
		return FALSE;
	}
	cout << "AlazarSetTriggerOperation" << endl;

	retCode = AlazarSetExternalTrigger(g_boardHandle, DC_COUPLING, ETR_1V);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetExternalTrigger failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetExternalTrigger failed");
		return FALSE;
	}
	cout << "AlazarSetExternalTrigger" << endl;

	// NOTE:
	// The board will wait for a for this amount of time for a trigger event.
	// If a trigger event does not arrive, then the board will automatically
	// trigger. Set the trigger timeout value to 0 to force the board to wait
	// forever for a trigger event.
	//
	retCode = AlazarSetTriggerDelay(g_boardHandle, 0);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetTriggerDelay failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetTriggerDelay failed");
		return FALSE;
	}
	cout << "AlazarSetTriggerDelay" << endl;

	//Trigger timeout in 10 μs units, or 0 to wait forever for a trigger event.
	retCode = AlazarSetTriggerTimeOut(g_boardHandle, 0);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetTriggerTimeOut failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetTriggerTimeOut failed");
		return FALSE;
	}
	cout << "AlazarSetTriggerTimeOut" << endl;

	//U32 offset = 29;
	//U32 value = U32(0);
	//U32 password = U32(840194166); //hex2dec('32145876')
	//retCode = AlazarReadRegister(g_boardHandle, offset, &value, password);
	//if (retCode != ApiSuccess)
	//{
	//	cout << "Error: AlazarReadRegisterfailed -- %s\n" << AlazarErrorToText(retCode);
	//	return FALSE;
	//}
	//cout << "AlazarReadRegister" << endl;

	//U32 Mask = 3 << 12;
	//value = value & !Mask;
	////value = value | 0 << 12;

	//Mask = 3 << 14;
	//value = value & !Mask;
	//value = value | 3 << 14;

	//retCode = AlazarWriteRegister(g_boardHandle, offset, value, password);
	//if (retCode != ApiSuccess)
	//{
	//	cout << "Error: AlazarWriteRegister -- %s\n" << AlazarErrorToText(retCode);
	//	return FALSE;
	//}
	//cout << "AlazarWriteRegister" << endl;



	return TRUE;
}

BOOL PrepAcquire(BOOL bScan, UINT lines)
{
	BOOL success = TRUE;

	U32 channelMask = CHANNEL_A | CHANNEL_B;
	int channelCount = 2;

	U32 samplesPerRecord;
	U32 recordsPerBuffer;

	U8 bitsPerSample;
	U32 maxSamplesPerChannel;

	AlazarGetBoardHandle();

	RETURN_CODE retCode = AlazarGetChannelInfo(g_boardHandle, &maxSamplesPerChannel, &bitsPerSample);
	if (retCode != ApiSuccess)
	{
		//printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
		ErrorDisplay(retCode, L"Error: AlazarGetChannelInfo failed");
		return FALSE;
	}

	//512 lines / Resfreq = 7930 => 64,6ms   => 15 Hz
	// 1/Resfreq * laser freq (80180000) => max 10111 samples per line for full cycle of resonant mirror

	if (bScan) //scanmode uni 1, bi 0
	{
		samplesPerRecord = 5000; //half cycle
		recordsPerBuffer = lines;
	}
	else
	{
		samplesPerRecord = 9960;  // 0.9 cycle
		recordsPerBuffer = lines / 2;
	}

	U32 bytesPerSample = 2;
	g_samplesPerBuffer = samplesPerRecord * channelCount * recordsPerBuffer;
	g_bytesPerBuffer = bytesPerSample * g_samplesPerBuffer;

	

	// Configure the board to make an NPT AutoDMA acquisition
	retCode = AlazarSetRecordSize(g_boardHandle, (U32)0, samplesPerRecord);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarSetRecordSize failed -- %s\n" << AlazarErrorToText(retCode);
		ErrorDisplay(retCode, L"Error: AlazarSetRecordSize failed");
		success = FALSE;
	}


	// Calculate the number of buffers in the acquisition
	U32 buffersPerAcquisition = (U32)INFINITE;

	if (success)
	{


		// TODO: Select AutoDMA flags as required
		// ADMA_NPT - Acquire multiple records with no - pretrigger samples
		// ADMA_EXTERNAL_STARTCAPTURE - call AlazarStartCapture to begin the acquisition
		// ADMA_INTERLEAVE_SAMPLES - interleave samples for highest throughput
		U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT | ADMA_INTERLEAVE_SAMPLES;

		retCode = AlazarBeforeAsyncRead(g_boardHandle,
			channelMask,
			0, // Must be 0 not using pretrigger acquisation
			samplesPerRecord,
			recordsPerBuffer, //not including channelCount ??
			buffersPerAcquisition, // Ignored. Behave as if infinite
			admaFlags);

		if (retCode != ApiSuccess)
		{
			//cout << "Error: AlazarBeforeAsyncRead failed -- %s\n" << AlazarErrorToText(retCode);
			ErrorDisplay(retCode, L"Error: AlazarBeforeAsyncRead ");
			return FALSE;
		}

		clearBuffers();

		g_BaseAdress = VirtualAlloc(NULL, g_bytesPerBuffer * bufferCount, MEM_RESERVE, PAGE_READWRITE);

		char* pCharBuf = (char*)VirtualAlloc(g_BaseAdress, g_bytesPerBuffer, MEM_COMMIT, PAGE_READWRITE);
		g_Buffer[0] = (UINT16*)pCharBuf;

		for (UINT i = 1; i < bufferCount; i++)
		{
			//U16* pBuf = (U16 *)malloc(g_bytesPerBuffer);
			//U16* pBuf = AlazarAllocBufferU16(g_boardHandle, g_samplesPerBuffer); //samples
			pCharBuf = (char*)VirtualAlloc(pCharBuf + g_bytesPerBuffer, g_bytesPerBuffer, MEM_COMMIT, PAGE_READWRITE);
			if (pCharBuf == NULL)
			{
				//cout << "Error: alloc %d bytes failed" << g_bytesPerBuffer;
				ErrorDisplay(ApiFailed, L"Error: alloc bytes failed ");
				success = FALSE;
				break;
			}
			g_Buffer[i] = (UINT16*)pCharBuf;
		}

		for (UINT i = 0; i < bufferCount; i++)
			{
			LPVOID pBuf = g_Buffer[i];
			retCode = AlazarPostAsyncBuffer(g_boardHandle, pBuf, g_bytesPerBuffer); //bytes
			if (retCode != ApiSuccess)
			{
				//cout << "Error: AlazarPostAsyncBuffer failed\n" << AlazarErrorToText(retCode);
				ErrorDisplay(retCode, L"Error: AlazarPostAsyncBuffer failed ");
				success = FALSE;
				break;
			}
		}


		if (!success){ //clean up
			clearBuffers();

			return FALSE;
		}

	//	cout << "AlazarPostAsyncBuffers set" << endl;


		retCode = AlazarStartCapture(g_boardHandle);
		if (retCode != ApiSuccess)
		{
			//cout << "Error: AlazarStartCapture failed -- %s\n" << AlazarErrorToText(retCode);
			ErrorDisplay(retCode, L"Error: AlazarStartCapture failed  ");
			return FALSE;
		}
		g_buffersCompleted = 0;
	}

	return TRUE;
}

BOOL GetCapBuf()
{
	BOOL success = TRUE;
	RETURN_CODE retCode;
	U32 timeout_ms = 5000;


	g_idx = g_buffersCompleted % bufferCount;
	U16* pBuf = g_Buffer[g_idx];

	// Wait for a buffer to be filled by the board.
	retCode = AlazarWaitAsyncBufferComplete(g_boardHandle, pBuf, timeout_ms);
	if (retCode == ApiSuccess)
	{
		// This buffer is complete, but there are more buffers in the acquisition.
	}
	else if (retCode == ApiTransferComplete)
	{
		// This buffer is complete, and it is the last buffer of the acqusition.
		//cout << "Acquisation completed" << endl;
		success = FALSE;
	}
	else
	{
		//cout << "Error: AlazarWaitNextAsyncBufferComplete failed -- " << AlazarErrorToText(retCode) << endl;
		ErrorDisplay(retCode, L"Error: AlazarWaitNextAsyncBufferComplete  ");
		success = FALSE;
	}

	return success;
}

BOOL PostBuf()
{
	U16* pBuf = g_Buffer[g_idx];

		RETURN_CODE retCode = AlazarPostAsyncBuffer(g_boardHandle, pBuf, g_bytesPerBuffer);
		if (retCode != ApiSuccess)
		{
			//cout << "Error: AlazarPostAsyncBuffer failed\n" << AlazarErrorToText(retCode);
			ErrorDisplay(retCode, L"Error: AlazarPostAsyncBuffer failed ");

			return FALSE;
		}

		g_buffersCompleted += 1;

	
	return TRUE;
}

void clearBuffers()
{
	if (g_BaseAdress != NULL){
		VirtualFree(g_BaseAdress, 0, MEM_RELEASE);
		g_BaseAdress = NULL;
	}

}

BOOL AbortCapture()
{
	BOOL success = TRUE;
	RETURN_CODE retCode;

	retCode = AlazarAbortAsyncRead(g_boardHandle);
	if (retCode != ApiSuccess)
	{
		//cout << "Error: AlazarAbortAsyncRead failed -- %s\n" << AlazarErrorToText(retCode);
		success = FALSE;
	}

	return success;
}
