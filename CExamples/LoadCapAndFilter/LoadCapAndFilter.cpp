// LoadCapAndFilter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NMApi.h"

HANDLE myNplParser = INVALID_HANDLE_VALUE;
HANDLE myFrameParserConfig = INVALID_HANDLE_VALUE;

ULONG myHTTPFilterID = 0;    // Global for the Filter.
ULONG myHTTPFieldID = 0;     // Global ID for the HTTP data field.

void __stdcall
MyParserBuild(PVOID Context, ULONG StatusCode, LPCWSTR lpDescription, ULONG ErrorType)
{
    wprintf(L"%s\n", lpDescription);
}

// Returns a Frame Paser with a filter and one data field.
HANDLE
MyLoadNPL(void)
{
    ULONG ret;

    // Load default NPL parser sets
    ret = NmLoadNplParser(NULL, NmAppendRegisteredNplSets, MyParserBuild, 0, &myNplParser);
	if(ERROR_SUCCESS == ret)
	{
		//Create frame parser configuration
		ret = NmCreateFrameParserConfiguration(myNplParser, MyParserBuild, 0, &myFrameParserConfig);
		if(ERROR_SUCCESS == ret)
		{
			//Add filter
			ret = NmAddFilter(myFrameParserConfig, L"http.request.command == \"GET\"", &myHTTPFilterID);
			if(ret != ERROR_SUCCESS)
			{
				wprintf(L"Fail to load Add fitler, error: \n", ret);
			}

			//Add field
			ret = NmAddField(myFrameParserConfig, L"http.request.uri", &myHTTPFieldID);
			if(ret != ERROR_SUCCESS)
			{
				wprintf(L"Fail to load Add field, error: \n", ret);
			}

			//Create frame parser
			HANDLE myFrameParser = INVALID_HANDLE_VALUE;
			ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser);
			if(ret != ERROR_SUCCESS)
			{
				wprintf(L"Fail to create frame parser, error: \n", ret);
				NmCloseHandle(myFrameParserConfig);
				NmCloseHandle(myNplParser);
				return INVALID_HANDLE_VALUE;
			}
			else
			{
				return myFrameParser;
			}
		}
		else
		{
			wprintf(L"Unable to create frame parser configuration, error: \n", ret);
			NmCloseHandle(myNplParser);
			return INVALID_HANDLE_VALUE;
		}
	}
	else{
		wprintf(L"Unable to load NPL, error: \n", ret);
		return INVALID_HANDLE_VALUE;
    }
}

void
UnLoadNPL(void)
{
    NmCloseHandle(myNplParser);
    NmCloseHandle(myFrameParserConfig);
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    ULONG ret = ERROR_SUCCESS;
    // We expect the first param to be a file.
    if(argc <= 1){
        wprintf(L"Expect a file name as the only command line parameter\n");
        return -1;
    }

    // Open the specified capture file
    HANDLE myCaptureFile = INVALID_HANDLE_VALUE;
    if(ERROR_SUCCESS == NmOpenCaptureFile(argv[1], &myCaptureFile))
    {
        // Initialize the parser engine and return a frame parser.
        HANDLE myFrameParser = MyLoadNPL();
        if(myFrameParser != INVALID_HANDLE_VALUE)
        {
            ULONG myFrameCount = 0;
            ret = NmGetFrameCount(myCaptureFile, &myFrameCount); 
            if(ret == ERROR_SUCCESS)
            {
				HANDLE myRawFrame = INVALID_HANDLE_VALUE;
                for(ULONG i = 0; i < myFrameCount; i++)
                {
					HANDLE myParsedFrame = INVALID_HANDLE_VALUE;
					ret = NmGetFrame(myCaptureFile, i, &myRawFrame); 
                    if(ret == ERROR_SUCCESS)
                    {
                        // The last parameter is for API to return reassembled frame if enabled 
                        // NULL means that API discards reassembled frame.
                        ret = NmParseFrame(myFrameParser, myRawFrame, i, 0, &myParsedFrame, NULL); 
                        if(ret == ERROR_SUCCESS)
                        {
                            // Test to see if this frame passed our filter.
                            BOOL passed = FALSE;
                            ret = NmEvaluateFilter(myParsedFrame, myHTTPFilterID, &passed);
                            if((ret == ERROR_SUCCESS) && (passed == TRUE))
                            {
                                // Obtain the value of http.request.uri from frame.  We
                                // know that strings are passed as word pointer to unicode string in the variant.
                                WCHAR value[256];
                                ret = NmGetFieldValueString(myParsedFrame, myHTTPFieldID, 256, (LPWSTR)value);
                                if(ret == ERROR_SUCCESS)
                                {
                                    // Cast to WCHAR *
                                    wprintf(L"Frame %d: HTTP: %s\n", i+1, (LPWSTR)value);
                                }
                            }

                            // Release current parsed frame
                            NmCloseHandle(myParsedFrame);
                        }
                        else
                        {
                            // Just print error but continue to loop
                            wprintf(L"Error: 0x%X, trying to parse frame %d\n", ret, i+1);
                        }

                        // Release current raw frame
                        NmCloseHandle(myRawFrame);
                    }
                    else
                    {
                        // Just print error but continue to loop
                        wprintf(L"Errors getting raw frame %d\n", i+1);
                    }
                }
            }

            NmCloseHandle(myFrameParser);
        }
        else
        {
            wprintf(L"Errors creating frame parser\n");
        }

        NmCloseHandle(myCaptureFile);
    }
    else
    {
        wprintf(L"Errors openning capture file\n");
    }

    // release global handles
    UnLoadNPL();

    return 0;
}
