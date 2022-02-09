#include "stdafx.h"
#include "Commdlg.h"
#include "Device.h"
#include "GPUCPU.h"
#include "basewin.h"

#include "matinterface.h"
#include "alazar.h"


MainWindow* MWin = NULL;
//ofstream g_fpData;
//FILE* g_fpData;
HANDLE g_hFile;
UINT32 g_bytesPerFrame;
UINT32 g_SamplesPerFrame;
WCHAR StrShaderPath[260];
UINT g_SavePmt;
 

void getPathStr(WCHAR* StrFilenm);

MATLIB_API int InitWindow(UINT* DisplayDim, UINT FilterLen, UINT nLineSamples, UINT* Sfilter, bool bUni, UINT SavePmt, const char* strShader, const char* strFilename = NULL)
{
	GpuParam Par;
	
	Par.NumCol = DisplayDim[2];
	Par.NumRows = DisplayDim[3];
	Par.NumWide = FilterLen;
	Par.pFilter = Sfilter;
	Par.SampleWidth = nLineSamples;
	Par.bUni = bUni;
	g_SavePmt = SavePmt;
	g_SamplesPerFrame = DisplayDim[2] * DisplayDim[3];
	g_bytesPerFrame = Par.NumCol*Par.NumRows*sizeof(UINT);

	if (!MWin){

		MWin = new MainWindow();

		FILE *file;

		//check if shader file exists
		if (strShader != NULL && !fopen_s(&file, strShader, "r") ) {
			fclose(file);
			for (int i = 0; i < strlen(strShader); i++)
				StrShaderPath[i] = strShader[i];

			StrShaderPath[strlen(strShader) + 1] = '\0';
		}
		else if ( !fopen_s(&file, "sample.hlsl", "r")) { // 0 wfopen failed  L"sample.hlsl"
			fclose(file);
			GetCurrentDirectory(MAX_PATH, StrShaderPath);
			wcsncat_s(StrShaderPath, L"\\sample.hlsl", 12);
		}
		else {
			getPathStr(StrShaderPath);
			if (_wfopen_s(&file, StrShaderPath, L"r")){
				MessageBox(NULL, L"Could not open sample.hlsl", L"Error", MB_OK);
				return S_FALSE;
			}
			else fclose(file);
		}


		Par.strShader = StrShaderPath;

		HWND ParenthWnd = GetForegroundWindow(); //gets matlab's figure window of scanbox, make it parent to our window
		if (!IsWindow(ParenthWnd)) {
			ParenthWnd = NULL;
		}

		if (!MWin->Create(NULL, WS_CHILD, 1, 0, DisplayDim[0], DisplayDim[1], DisplayDim[2], DisplayDim[3], ParenthWnd)) //L"ScanboxEx" WS_POPUP WS_BORDER WS_EX_TOPMOST
		{
			MessageBox(NULL, L"Failed to Open a Window.", L"Error", MB_OK);
				return 1;
		}
		


		if (FAILED(MWin->Init(Par))) //Initialize DirectX device 
		{
			MessageBox(NULL, L"Failed to initialize DirectX components.", L"Error", MB_OK);
			return 1;
		}

	}
	else
	{

		Par.strShader = StrShaderPath;

			HWND hwnd = MWin->Window();
			if (hwnd){
				SetWindowPos(hwnd, NULL, DisplayDim[0], DisplayDim[1], DisplayDim[2], DisplayDim[3], SWP_NOZORDER | SWP_SHOWWINDOW);
				//MessageBox(NULL, L"Reopened Window.", L"Error", MB_OK);

				if (FAILED(MWin->Resize(Par)))
				{
					MessageBox(NULL, L"Failed to initialize DirectX components.", L"Error", MB_OK);			
					return 1;
				}
			}
			else {
				MessageBox(NULL, L"Invalid Window", L"Error", MB_OK);
				return 1;
			}

	}


	if (!PrepAcquire(bUni, Par.NumRows)) { //Allocate buffers for Alazar IO
			MessageBox(NULL, L"Failed to initialize Alazar Buffers.", L"Error", MB_OK);
			MWin->m_bWin = false;
			return 1;
	}

	if (strFilename != NULL){ //save the data to a file

		wchar_t  wszDest[160];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strFilename, -1, wszDest, 160);
		
		//g_fpData = ofstream(StrFile, ios::out | ios::binary);
		g_hFile = CreateFile(wszDest, // name of the write
			GENERIC_WRITE,          // open for writing
			0,                      // do not share
			NULL,                   // default security
			CREATE_NEW,             // create new file only
			FILE_ATTRIBUTE_NORMAL,  // normal file
			NULL);                  // no attr. template

	//	g_fpData = fopen(strFilename, "wb");
		//	if (!g_fpData.is_open())
		if( g_hFile == INVALID_HANDLE_VALUE) //(!g_fpData)
			{
				MessageBox(NULL, L"Unable to create data file.", L"Error", MB_OK);
				return 1;
			}
	}
	else g_hFile = INVALID_HANDLE_VALUE;

	   return 0;
}

MATLIB_API int RunSample(UINT16* DataOut, UINT8* Trg) //, ULONG* nmBufEmpty)
{
	BOOL bErrorFlag = FALSE;
	DWORD dwBytesWritten = 0;

	if (GetCapBuf()){
		UINT16* pBuf = getBuf();
		*Trg = (pBuf[0] & 0x03);  //trigger output

		//map data to GPU, run compute shader and render result on screen
		MWin->Render((UINT*)pBuf, (UINT*)DataOut);
		if (!PostBuf())
			return 1;

		if (g_hFile != INVALID_HANDLE_VALUE) //(g_fpData)
		{
		//	// Write buffer to file and save image data
			//g_fpData.write((char*)DataOut, g_bytesPerFrame);
			if (g_SavePmt == 1){

				//green channel
				//fwrite((char*)DataOut, sizeof(char), g_bytesPerFrame / 2, g_fpData);
				bErrorFlag = WriteFile(
					g_hFile,           // open file handle
					DataOut,      // start of data to write
					g_bytesPerFrame / 2,  // number of bytes to write
					&dwBytesWritten, // number of bytes that were written
					NULL);            // no overlapped structure
			}	
			else if(g_SavePmt == 2){
				//red channel
				//DataOut is a UINT32 pointer => two uint16 pages of data; point to the second half 
				//fwrite((char*)(DataOut + g_SamplesPerFrame/2), sizeof(char), g_bytesPerFrame / 2, g_fpData);
				bErrorFlag = WriteFile(
					g_hFile,           // open file handle
					(DataOut + g_SamplesPerFrame),      // start of data to write
					g_bytesPerFrame / 2,  // number of bytes to write
					&dwBytesWritten, // number of bytes that were written
					NULL);            // no overlapped structure
			}
			else {//both channels
				//fwrite((char*)DataOut, sizeof(char), g_bytesPerFrame, g_fpData);
				bErrorFlag = WriteFile(
					g_hFile,           // open file handle
					DataOut,      // start of data to write
					g_bytesPerFrame,  // number of bytes to write
					&dwBytesWritten, // number of bytes that were written
					NULL);            // no overlapped structure
			}
			if (FALSE == bErrorFlag)
			{
				printf("Terminal failure: Unable to write to file.\n");
				return 1;
			}
		}

	}
	else return 1;

	return 0;
}

MATLIB_API void Clearwin()
{

	if (MWin){
		MWin->Clear();

		AbortCapture(); //alazar abort
		clearBuffers(); //alazar MEM_CLEAR buffers

		if (g_hFile != INVALID_HANDLE_VALUE){
			//fclose(g_fpData);
			//g_fpData = NULL;
			CloseHandle(g_hFile);
		}
	}

}

MATLIB_API void Closewin()
{
	if (MWin){
		MWin->Close();
		delete MWin;
		MWin = NULL;
	}

}

MATLIB_API void SetSO(float Scl, bool bTyp, bool bCol){
	MWin->SetScaleOffset(Scl, bTyp, bCol);
}

MATLIB_API void Shift(short Shift){
	MWin->Shift(Shift);
}

void getPathStr(WCHAR* StrFilenm)
{

	OPENFILENAME ofn;       // common dialog box structure
	HWND hwnd = NULL;      // owner window

	StrFilenm[0] = '\0';
	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);

	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = StrFilenm;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"All\0*.hlsl\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = NULL;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&ofn);
	//GetSaveFileName(&ofn);

	//StrFilenm = ofn.lpstrFileTitle;
	//set current to original directory
	//SetCurrentDirectory(strCurrPath);

}