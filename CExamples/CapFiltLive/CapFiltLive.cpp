#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NMApi.h"

HANDLE myNplParser;
HANDLE myFrameParserConfig;
HANDLE myFrameParser;

ULONG myHTTPFilterID;
ULONG myHTTPFieldID;

BOOL CaptureDone = FALSE;
ULONG FrameCount = 0;

void __stdcall
MyParserBuild(PVOID Context, ULONG StatusCode, LPCWSTR lpDescription, ULONG ErrorType)
{
    wprintf(L"%s\n", lpDescription);
}

// Returns a Frame Parser with a filter and one data field.  If there's a failure, 
// INVALID_HANDLE_VALUE is returned.
HANDLE
MyLoadNPL(void)
{
	HANDLE myFrameParser = INVALID_HANDLE_VALUE;
    ULONG ret;

    // Use NULL to load default NPL set
    ret = NmLoadNplParser(NULL, NmAppendRegisteredNplSets, MyParserBuild, 0, &myNplParser);

	if(ret == ERROR_SUCCESS){
		ret = NmCreateFrameParserConfiguration(myNplParser, MyParserBuild, 0, &myFrameParserConfig);

		if(ret == ERROR_SUCCESS){
			ret = NmAddFilter(myFrameParserConfig, L"http.request.command == \"GET\"", &myHTTPFilterID);
			if(ret != 0){
				wprintf(L"Failed to add fitler, error 0x%X\n", ret);
			}

			ret = NmAddField(myFrameParserConfig, L"http.request.uri", &myHTTPFieldID);
			if(ret != ERROR_SUCCESS){
				wprintf(L"Failed to add field, error 0x%X\n", ret);
			}

			ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser);
			if(ret != ERROR_SUCCESS)
			{
				wprintf(L"Failed to create frame parser, error 0x%X\n", ret);
				NmCloseHandle(myFrameParserConfig);
				NmCloseHandle(myNplParser);
				return INVALID_HANDLE_VALUE;
			}
		} else {
			wprintf(L"Unable to load parser config, error 0x%X\n", ret);
			NmCloseHandle(myNplParser);
			return INVALID_HANDLE_VALUE;
		}

	} else {
        wprintf(L"Unable to load NPL\n");
		return INVALID_HANDLE_VALUE;
    }

    return(myFrameParser);
}

void
UnLoadNPL(void)
{
    NmCloseHandle(myNplParser);
    NmCloseHandle(myFrameParserConfig);
}

DWORD WINAPI  
myFrameEval(PVOID pContext)
{
    HANDLE capFile = (HANDLE)pContext;
    HANDLE hRawFrame;
    ULONG ret;
    ULONG curFrame = 0;

    while(!CaptureDone)
    {
        if(FrameCount > curFrame)
        {
            for(;curFrame < FrameCount; curFrame++)
            {
                ret = NmGetFrame(capFile, curFrame, &hRawFrame);

                if(ret == ERROR_SUCCESS)
                {
                    HANDLE myParsedFrame;

                    ret = NmParseFrame(myFrameParser, hRawFrame, curFrame, 0, &myParsedFrame, NULL);
                    if(ret != ERROR_SUCCESS)
                    {
                        wprintf(L"Error: 0x%X, trying to parse frame\n", ret);
                    } else {

                        // Test to see if this frame passed our filter.
                        BOOL passed;
                        NmEvaluateFilter(myParsedFrame, myHTTPFilterID, &passed);
                        if(passed)
                        {
                            // Obtain the value of http.request.uri from frame.  We
                            // know that strings are passed as bstrVal in the variant.
                            WCHAR value[256];
                            ret = NmGetFieldValueString(myParsedFrame, myHTTPFieldID, 256, value);
                            if(ret == ERROR_SUCCESS)
                            {
                                wprintf(L"HTTP: %s\n", value);
                            }
                        }

                        NmCloseHandle(myParsedFrame);
                    }
                }

                NmCloseHandle(hRawFrame);
            }

            Sleep(5000);
        }
    }

    return 0;
}

// Frame indication callback.  Get's called each time a frame appears.
void __stdcall 
myFrameIndication(HANDLE hCapEng, ULONG AdpIdx, PVOID pContext, HANDLE hRawFrame)
{
    HANDLE capFile = (HANDLE)pContext;
    NmAddFrame(capFile, hRawFrame);

    FrameCount++;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    ULONG ret;
	ULONG adapterIndex = 0;

	if(2 == argc)
		adapterIndex = _wtol(argv[1]);

    // Open capture engine.
    HANDLE myCaptureEngine;
    ret = NmOpenCaptureEngine(&myCaptureEngine);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error openning capture engine, 0x%X\n", ret);
        return ret;
    }

    // Initialize the parser engine and return a frame parser.
    myFrameParser = MyLoadNPL();
    if(myFrameParser == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Errors creating frame parser\n");
        return -1;
    }

    HANDLE myTempCap;
	ULONG CapSize;
    ret = NmCreateCaptureFile(L"temp_capfiltlive.cap", 20000000, NmCaptureFileWrapAround, &myTempCap, &CapSize);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error creating the capture file.\n");
        NmCloseHandle(myCaptureEngine);
        NmCloseHandle(myFrameParser);
        UnLoadNPL();
        return ret;
    }
    
    // Configure the adapters callback and pass capture handle as context value.
    ret = NmConfigAdapter(myCaptureEngine, adapterIndex, myFrameIndication, myTempCap);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error configuring the adapter.\n");
        NmCloseHandle(myCaptureEngine);
        NmCloseHandle(myFrameParser);
        NmCloseHandle(myTempCap);
        UnLoadNPL();       
		return ret;
    }

    // Create thread to handle to handle asychronously capture frames.
    HANDLE eThreadHandle = CreateThread(NULL, 0, myFrameEval, myTempCap, 0, NULL);

    if(NULL != eThreadHandle)
    {
        wprintf(L"Capturing for 20 seconds\n");
        ret = NmStartCapture(myCaptureEngine, adapterIndex, NmLocalOnly);
        if(ret == ERROR_SUCCESS)
        {
            Sleep(20000);

            wprintf(L"Stopping capture\n");
            NmStopCapture(myCaptureEngine, adapterIndex);
        }
        else
        {
            wprintf(L"Failed to start capture engine\n");
        }	

        // Terminate evaluation thread
        CaptureDone = TRUE;
        ret = WaitForSingleObject(eThreadHandle, INFINITE);
        if(0 != ret)
        {
            wprintf(L"Failed to terminate evaluation thread (%d)\n", ret);
        }

        if(!CloseHandle(eThreadHandle))
        {
            wprintf(L"Failed to close thread handle\n");
        }
    }

    NmCloseHandle(myCaptureEngine);
    NmCloseHandle(myFrameParser);

    UnLoadNPL();

    return 0;
}