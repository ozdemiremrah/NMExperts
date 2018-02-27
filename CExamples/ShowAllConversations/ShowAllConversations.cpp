#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NMApi.h"

HANDLE myNplParser;
HANDLE myFrameParserConfig;

void __stdcall
MyParserBuild(PVOID Context, ULONG StatusCode, LPCWSTR lpDescription, ULONG ErrorType)
{
    wprintf(L"%s\n", lpDescription);
}

// Returns a frame parser with a filter and one data field.
// INVALID_HANDLE_VALUE indicates failure.
HANDLE
MyLoadNPL(void)
{
    HANDLE myFrameParser = INVALID_HANDLE_VALUE;
    ULONG ret;

    // Use NULL to load the default NPL set.
    ret = NmLoadNplParser(NULL, NmAppendRegisteredNplSets, MyParserBuild, 0, &myNplParser);

    if(ret == ERROR_SUCCESS)
    {
        ret = NmCreateFrameParserConfiguration(myNplParser, MyParserBuild, 0, &myFrameParserConfig);

            ret = NmConfigConversation(myFrameParserConfig, NmConversationOptionNone, true);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to enable conversations.\n");
                return INVALID_HANDLE_VALUE;
            }

            
        if(ret == ERROR_SUCCESS)
        {
            ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser);

            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Failed to create frame parser, error 0x%X\n", ret);
                NmCloseHandle(myFrameParserConfig);
                NmCloseHandle(myNplParser);
                return INVALID_HANDLE_VALUE;
            }

        }
        else
        {
            wprintf(L"Unable to load parser config, error 0x%X\n", ret);
            NmCloseHandle(myNplParser);
            return INVALID_HANDLE_VALUE;
        }

    }
    else
    {
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

int __cdecl wmain(int argc, WCHAR* argv[])
{
    ULONG ret;
    // The only parameter is a file.
    if(argc <= 1){
        wprintf(L"Expect a file name as the only parameters.\n");
        return ERROR_INVALID_PARAMETER;
    }

    HANDLE myCaptureFile;
    ret = NmOpenCaptureFile(argv[1], &myCaptureFile);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error opening capture file: %s, ret = %d\n", argv[1], ret);
        return ret;
    }

    // Initialize the parser engine and return a frame parser.
    HANDLE myFrameParser = MyLoadNPL();
    if(myFrameParser == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Errors creating frame parser.\n");
        return (int)INVALID_HANDLE_VALUE;
    }

    ULONG NumFrames;
    NmGetFrameCount(myCaptureFile, &NumFrames);

    // Iterate through all the frames
    for(ULONG n=0;n < NumFrames; n++)
    {
        HANDLE myRawFrame;

        ret = NmGetFrame(myCaptureFile, n, &myRawFrame);
        if(ret != ERROR_SUCCESS)
        {
            wprintf(L"Error getting frame #%d, ret = %d", n+1, ret);
            return ret;
        }

        HANDLE myParsedFrame;
        ret = NmParseFrame(myFrameParser, 
                           myRawFrame,
                           n, 
                           NmFrameConversationInfoRequired, 
                           &myParsedFrame, 
                           NULL);
        if(ret != ERROR_SUCCESS)
        {
            wprintf(L"Error: 0x%X, trying to parse frame\n", ret);
            return ret;
        }

        WCHAR ProtName[256];
        ULONG RetProtNameLen = 0;
        ULONG ConvID = 0;
        ProtName[0] = L'\0';

        wprintf(L"Frame %d ", n+1);

        // Get the Top Conversation
        ret = NmGetTopConversation(myParsedFrame, 256, ProtName, &RetProtNameLen, &ConvID);
        if(ret != ERROR_SUCCESS)
        {
            wprintf(L"Error %d getting top conversation.\n", ret);
        } else  {
            // Iterate through all conversations until an error occurs.
            while(ret == ERROR_SUCCESS)
            {
                wprintf(L"(%s %d) ", ProtName, ConvID);
                ret = NmGetParentConversation(myParsedFrame, ConvID, 256, ProtName, &RetProtNameLen, &ConvID);
            }
        }

        wprintf(L"\n");

        NmCloseHandle(myParsedFrame);
        NmCloseHandle(myRawFrame);
    }

    NmCloseHandle(myFrameParser);
    NmCloseHandle(myCaptureFile);

    UnLoadNPL();

    return ERROR_SUCCESS;
}


