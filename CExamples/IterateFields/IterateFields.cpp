// IterateFields.cpp : Opens a capture and prints each field and it's value.
//
#include "stdafx.h"
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

// Returns a Frame Paser with a filter and one data field.
HANDLE
MyLoadNPL(LPCWSTR nplPath)
{
    ULONG ret;

	//Load npl path
    ret = NmLoadNplParser(nplPath, NmAppendRegisteredNplSets, MyParserBuild, 0, &myNplParser);
    if(ret != ERROR_SUCCESS)
	{
        wprintf(L"Unable to load NPL\n");
        return NULL;
    }

    ret = NmCreateFrameParserConfiguration(myNplParser, MyParserBuild, 0, &myFrameParserConfig);
    if(ret != ERROR_SUCCESS)
	{
        wprintf(L"Unable to load parser config\n");
        return NULL;
    }

    HANDLE myFrameParser;
    ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser);
	if(ret != ERROR_SUCCESS)
	{
        wprintf(L"Unable to create frame parser.\n");
        return NULL;
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
    // We expect the first param to be a file and the second to be a frame number.
    if(argc <= 2){
        wprintf(L"Expect a file name, frame number and NPL path(optional) as the only command line parameters\n");
        return -1;
    }

    HANDLE myCaptureFile;
    ret = NmOpenCaptureFile(argv[1], &myCaptureFile);
	if(ret != ERROR_SUCCESS)
	{
		wprintf(L"Errors opening capture file: %s failed, ret = %d\n", argv[1], ret);
        return ret;
	}

    // Initialize the parser engine and return a frame parser.
	WCHAR* nplPath = NULL;
	if(4 == argc)
		nplPath = argv[3];
    HANDLE myFrameParser = MyLoadNPL(nplPath);
    if(myFrameParser == NULL)
    {
        wprintf(L"Errors creating frame parser");
        return -1;
    }

    ULONG FrameNumber = _wtol(argv[2]);
	wprintf(L"Iterate the fields of frame #%d\n", FrameNumber);

    HANDLE myRawFrame;

    ret = NmGetFrame(myCaptureFile, FrameNumber-1, &myRawFrame);
	if(ret != ERROR_SUCCESS)
	{
		wprintf(L"Error getting frame #%d, ret = %d", FrameNumber, ret);
		return ret;
	}

    HANDLE myParsedFrame;
    ret = NmParseFrame(myFrameParser, 
                        myRawFrame,
						FrameNumber, 
                        NmFieldFullNameRequired | NmDataTypeNameRequired | NmContainingProtocolNameRequired, 
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


    for(ULONG j = 0; j < myFieldCount; j++)
    {
        NM_PARSED_FIELD_INFO myParsedDataFieldInfo;
        myParsedDataFieldInfo.Size = sizeof(NM_PARSED_FIELD_INFO);

		ret = NmGetParsedFieldInfo(myParsedFrame, j, 0, &myParsedDataFieldInfo);
		if(ret != ERROR_SUCCESS)
		{
			wprintf(L"Error getting parsed field #%d info, ret = %d\n", j, ret);
			return ret;
		}

        for(int i = 0; i < myParsedDataFieldInfo.FieldIndent; i++){
            wprintf(L" ");
        }

        // Add space for a NULL
        WCHAR *NamePath = (WCHAR *)malloc((myParsedDataFieldInfo.NamePathLength + 1) * sizeof(WCHAR));
        ret = NmGetFieldName(myParsedFrame, j, NmFieldNamePath, myParsedDataFieldInfo.NamePathLength + 1 , NamePath);

        // Add space for a NULL
        WCHAR *NplDataTypeName = (WCHAR *)malloc((myParsedDataFieldInfo.NplDataTypeNameLength + 1) * sizeof(WCHAR));
        ret = NmGetFieldName(myParsedFrame, j, NmFieldDataTypeName, myParsedDataFieldInfo.NplDataTypeNameLength * 2, NplDataTypeName);

		//Add space for a NULL
		WCHAR *ProtocolName = (WCHAR *)malloc((myParsedDataFieldInfo.ProtocolNameLength + 1) * sizeof(WCHAR));
		ret = NmGetFieldName(myParsedFrame, j, NmFieldContainingProtocolName, myParsedDataFieldInfo.ProtocolNameLength * 2, ProtocolName);
		
		wprintf(L"%s.%s (%s) - Offset: %d, Size: %d\n", ProtocolName, NamePath, NplDataTypeName, myParsedDataFieldInfo.FrameBitOffset/8, myParsedDataFieldInfo.FieldBitLength/8);

        free(NamePath);
        free(NplDataTypeName);
		free(ProtocolName);
    }

    NmCloseHandle(myParsedFrame);
    NmCloseHandle(myFrameParser);
    NmCloseHandle(myCaptureFile);

    UnLoadNPL();

	return 0;
}