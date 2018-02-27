#include "Stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NMApi.h"

void __stdcall 
MyFrameIndication(HANDLE hCapEng, ULONG ulAdaptIdx, PVOID pContext, HANDLE hRawFrame)
{
    HANDLE capFile = (HANDLE)pContext;
    NmAddFrame(capFile, hRawFrame);
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    ULONG ret;
	ULONG adapterIndex = 0;

	if(2 == argc)
		adapterIndex = _wtol(argv[1]);

    // Open a capture file for saving frames
    HANDLE myCapFile;
    ULONG CapSize;
    ret = NmCreateCaptureFile(L"20sec.cap", 20000000, NmCaptureFileWrapAround, &myCapFile, &CapSize);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error opening capture file, 0x%X\n", ret);
        return ret;
    }

    // Open capture engine.
    HANDLE myCaptureEngine;
    ret = NmOpenCaptureEngine(&myCaptureEngine);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error opening capture engine, 0x%X\n", ret);
        NmCloseHandle(myCapFile);
        return ret;
    }

	//Config adapter
    ret = NmConfigAdapter(myCaptureEngine, adapterIndex, MyFrameIndication, myCapFile);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error configuration adapter, 0x%X\n", ret);
        NmCloseHandle(myCaptureEngine);
        NmCloseHandle(myCapFile);
        return ret;
    }

	//Start to capture frame
    wprintf(L"Capturing for 20 seconds\n");
    NmStartCapture(myCaptureEngine, adapterIndex, NmLocalOnly);

    Sleep(20000);

    wprintf(L"Stopping capture\n");
    NmStopCapture(myCaptureEngine, adapterIndex);

    NmCloseHandle(myCaptureEngine);
    NmCloseHandle(myCapFile);

    return 0;
}

