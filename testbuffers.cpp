#include "stdafx.h"
#include "alazar.h"

#define bufferCount 16

unsigned short* g_Buffer[bufferCount];
LPVOID g_BaseAdress = NULL;

UINT  g_bytesPerBuffer;
UINT  g_samplesPerBuffer;

UINT g_buffersCompleted;
UINT g_idx;


BOOL PrepAcquire(BOOL bScan, UINT lines)
{
	bool success = true;
	UINT channelCount = 2;

	UINT samplesPerRecord;
	UINT recordsPerBuffer;

//	SYSTEM_INFO sSysInfo;
//	GetSystemInfo(&sSysInfo);


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

	UINT bytesPerSample = 2;
	g_samplesPerBuffer = samplesPerRecord * channelCount * recordsPerBuffer;
	g_bytesPerBuffer = bytesPerSample * g_samplesPerBuffer;

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
			cout << "Error: alloc %d bytes failed" << g_bytesPerBuffer;
			//ErrorDisplay(ApiFailed, L"Error: alloc bytes failed ");
			success = FALSE;
			break;
		}
		g_Buffer[i] = (UINT16*)pCharBuf;
	}

	return success;
}

void clearBuffers()
{
	if (g_BaseAdress != NULL){
		VirtualFree(g_BaseAdress, 0, MEM_RELEASE);
		g_BaseAdress = NULL;
	}

}

