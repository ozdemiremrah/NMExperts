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

        if(ret == ERROR_SUCCESS)
        {
            // Turn off optimizations so we can get display strings for all fields.
            ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser, NmParserOptimizeNone);

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
    // The first parameter is a file, and the second is a frame number.
    if(argc <= 2){
        wprintf(L"Expect a file name and frame number as the only parameters.\n");
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

    ULONG FrameNumber = _wtol(argv[2]) - 1;
    wprintf(L"Iterate the fields of frame #%d\n", FrameNumber+1);

    HANDLE myRawFrame;

    ret = NmGetFrame(myCaptureFile, FrameNumber-1, &myRawFrame);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error getting frame #%d, ret = %d", FrameNumber+1, ret);
        return ret;
    }

    HANDLE myParsedFrame;
    ret = NmParseFrame(myFrameParser, 
                       myRawFrame,
                       FrameNumber, 
                       NmFieldDisplayStringRequired, 
                       &myParsedFrame, 
                       NULL);

    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error: 0x%X, trying to parse frame\n", ret);
        return ret;
    }

    ULONG myFieldCount;
    ret = NmGetFieldCount(myParsedFrame, &myFieldCount);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error getting field count, ret = %d\n", ret);
        return ret;
    }
    else
    {
        // Keeps track of a negative offset so protocols start at zero.
        int IndentOffset = 0;

        wprintf(L"Field count = %d\n", myFieldCount);

        // Iterate through all the fields in this frame.
        for(ULONG j = 0; j < myFieldCount; j++)
        {
            NM_PARSED_FIELD_INFO myParsedDataFieldInfo;
            myParsedDataFieldInfo.Size = sizeof(NM_PARSED_FIELD_INFO);

            // Call with Display String Required as only option.
            ret = NmGetParsedFieldInfo(myParsedFrame, 
                                        j, 
                                        NmFieldDisplayStringRequired,
                                        &myParsedDataFieldInfo);

            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Error getting parsed field #%d info, ret = %d\n", j, ret);
                return ret;
            }

            if(j > 0 && myParsedDataFieldInfo.ValueType == 0)
            {
                IndentOffset--;
                wprintf(L"\n");
            }

            for(int i = 0; i < (int)myParsedDataFieldInfo.FieldIndent + IndentOffset; i++)
            {
                wprintf(L"  ");
            }

            // Allocate space for the Field Name and get it.
            ULONG FieldNameLength = (myParsedDataFieldInfo.NamePathLength + 1) * sizeof(WCHAR);
            WCHAR *FieldName = (WCHAR *)malloc(FieldNameLength);
            ret = NmGetFieldName(myParsedFrame, j, NmFieldNamePath, FieldNameLength, FieldName);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Error %d trying to retreive field name for frame %d, element %d.", ret, FrameNumber, j);
            }

            // Allocate space for the display string and get it.
            ULONG DisplayFormatLength = (myParsedDataFieldInfo.DisplayStringLength + 1) * sizeof(WCHAR);
            WCHAR *DisplayFormat = (WCHAR *)malloc(DisplayFormatLength);
            ret = NmGetFieldName(myParsedFrame, j, NmFieldDisplayString, DisplayFormatLength, DisplayFormat);

            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Error %d tryin to retreive display name for frame %d element %d.", ret, FrameNumber, j);
            } else {
                wprintf(L"%s: %s\n", FieldName, DisplayFormat);
            }

            free(FieldName);
            free(DisplayFormat);
        }
    }

    NmCloseHandle(myParsedFrame);

    NmCloseHandle(myFrameParser);

    NmCloseHandle(myCaptureFile);

    UnLoadNPL();

    return ret;
}

