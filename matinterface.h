#pragma once

#ifdef MATINTERFACE_EXPORTS
#define MATLIB_API __declspec(dllexport) 
#else
#define MATLIB_API __declspec(dllimport) 
#endif

extern "C"  MATLIB_API int InitWindow(UINT* DisplayDim, UINT FilterLen, UINT nLineSamples, UINT* Sfilter, bool bUni, UINT SavePmt, const char* strShader, const char* strFilename);

extern "C"  MATLIB_API int RunSample(UINT16* DataOut, UINT8* Trg);  //processes the data and returns output

extern "C"  MATLIB_API void Clearwin(); //clears device and implementation

extern "C"  MATLIB_API void Closewin(); //close window 

extern "C" MATLIB_API void SetSO(float Scl, bool bTyp, bool bCol); //sets scale or offset

extern "C" MATLIB_API void Shift(short Shift); ////shifts sampling filter

