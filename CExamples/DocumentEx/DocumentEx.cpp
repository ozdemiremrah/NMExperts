#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NMApi.h"

#define ADAPTER_INDEX       0

HANDLE myNplParser;
HANDLE myFrameParserConfig;
HANDLE myFrameParser;

ULONG myHTTPFilterID;          // Filter ID

ULONG IPv4NextProtocolID;      // 8 bit
ULONG TCPSrcPortID;            // 16 bit
ULONG TCPSeqNumID;             // 32 bit
ULONG SMBAllocSizeID;          // 64 bit
ULONG EthSrcAddrID;            // Byte Array (6 bytes)
ULONG HttpURIID;               // String
ULONG Desc_PropID;             // Property.Description ID
ULONG ProcName_PropID;         // Conversation.ProcessName ID
ULONG TCPLen_PropID;           // Property.TCPPayloadLength ID
ULONG IPv6Addr_PropID;         // Global.IPConfig.IPv4LocalAddress

void
RawBuffer(HANDLE myCapFile)
{
    BYTE myBuf[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8,
                    0x08, 0x00};

    HANDLE myParsedFrame;
    NmParseBuffer(myFrameParser, myBuf, sizeof(myBuf), 0, NmParsingOptionNone, &myParsedFrame);

    byte EthSrcAdr[6];
    ULONG EthSrcAdrLen;
    ULONG ret = NmGetFieldValueByteArray(myParsedFrame, EthSrcAddrID, sizeof(EthSrcAdr), EthSrcAdr, &EthSrcAdrLen);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"NextPro = %x-%x-%x-%x-%x-%x, len = %d\n", EthSrcAdr[0],
                                                            EthSrcAdr[1], 
                                                            EthSrcAdr[2], 
                                                            EthSrcAdr[3], 
                                                            EthSrcAdr[4], 
                                                            EthSrcAdr[5], 
                                                            EthSrcAdrLen);
    }

    HANDLE myRawFrame;
    NmBuildRawFrameFromBuffer(myBuf, sizeof(myBuf), 1, 0, &myRawFrame);
    NmAddFrame(myCapFile, myRawFrame);

}

void __stdcall
MyParserBuild(PVOID Context, ULONG StatusCode, LPCWSTR lpDescription, ULONG ErrorType)
{
    wprintf(L"%s\n", lpDescription);
}

// Returns a Frame Parser with a filter and one data field.
bool
MyLoadNPL(void)
{
    ULONG ret;

    ret = NmLoadNplParser(NULL, NmAppendRegisteredNplSets, MyParserBuild, 0, &myNplParser);
    if(ret == ERROR_SUCCESS)
    {
        ret = NmCreateFrameParserConfiguration(myNplParser, MyParserBuild, 0, &myFrameParserConfig);
        if(ret == ERROR_SUCCESS)
        {
            ret = NmConfigConversation(myFrameParserConfig, NmConversationOptionNone, TRUE);

            ret = NmConfigReassembly(myFrameParserConfig, NmReassemblyOptionNone, TRUE);

            ret = NmAddFilter(myFrameParserConfig, L"http.request.command == \"GET\"", &myHTTPFilterID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add filter http.request.command == \"GET\"\n");
            }
            
            ret = NmAddField(myFrameParserConfig, L"http.request.uri", &HttpURIID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field http.request.uri\n");
            }
            // 8 bits
            ret = NmAddField(myFrameParserConfig, L"IPv4.NextProtocol", &IPv4NextProtocolID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field IPv4.NextProtocol\n");
            }
            // 16 bits
            ret = NmAddField(myFrameParserConfig, L"TCP.SrcPort", &TCPSrcPortID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field TCP.SrcPort\n");
            }
            // 32 bits
            ret = NmAddField(myFrameParserConfig, L"TCP.SequenceNumber", &TCPSeqNumID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field TCP.SequenceNumber\n");
            }
            // 64 bits
            ret = NmAddField(myFrameParserConfig, L"SMBRequestNTCreateAndX.AllocationSize", &SMBAllocSizeID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field SMBRequestNTCreateAndX.AllocationSize\n");
            }
            // Byte Array, 6 bytes
            ret = NmAddField(myFrameParserConfig, L"ethernet.SourceAddress", &EthSrcAddrID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add field ethernet.SourceAddress\n");
            }

            // Add Properties

            ret = NmAddProperty(myFrameParserConfig, L"Conversation.ProcessName", &ProcName_PropID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add property Conversation.ProcessName\n");
            }

            ret = NmAddProperty(myFrameParserConfig, L"Property.TCPPayloadLength", &TCPLen_PropID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add property Property.TCPPayloadLength\n");
            }

            ret = NmAddProperty(myFrameParserConfig, L"Global.IPConfig.LocalIPv6Address", &IPv6Addr_PropID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add property Global.IPConfig.LocalIPv6Address\n");
            }

            ret = NmAddProperty(myFrameParserConfig, L"Property.Description\n", &Desc_PropID);
            if(ret != ERROR_SUCCESS)
            {
                wprintf(L"Unable to add property Property.Description\n");
            }

            WCHAR StartDataType[256];
            ret = NmGetStartDataType(myFrameParserConfig, sizeof(StartDataType), (LPWSTR)StartDataType);

            ret = NmConfigStartDataType(myFrameParserConfig, (LPWSTR)StartDataType);

            NM_PROTOCOL_SEQUENCE_CONFIG seqConfig;
            seqConfig.Size = sizeof(NM_PROTOCOL_SEQUENCE_CONFIG);
            seqConfig.GroupKeyString = L"SourceNetworkAddress;SourcePort;DestinationNetworkAddress;DestinationPort";
            seqConfig.NextSequencePropertyString = L"TCPSeqNumber";
            seqConfig.SequencePropertyString = L"TCPNextSeqNumber";

            ULONG rIndex = 0;
            ret = NmAddSequenceOrderConfig(myFrameParserConfig, &seqConfig, &rIndex);
            if( ERROR_SUCCESS != ret ) 
            {
                wprintf(L"NmApi: Add TCP protocol sequence config failed!\n");
            }

            ret = NmCreateFrameParser(myFrameParserConfig, &myFrameParser);
            if(ERROR_SUCCESS != ret)
            {
                NmCloseHandle(myNplParser);
                NmCloseHandle(myFrameParserConfig);

                return false;
            }
        } else {
            wprintf(L"Unable to load parser config, error=%d\n", ret);
            NmCloseHandle(myNplParser);

            return false;
        }
    } else {
        wprintf(L"Unable to load NPL, error=%d\n", ret);

        return false;
    }

    return true;
}

void
UnLoadNPL(void)
{
    NmCloseHandle(myFrameParser);
    NmCloseHandle(myNplParser);
    NmCloseHandle(myFrameParserConfig);
}

void
GetFieldFunctions(HANDLE parsedFrame)
{
    ULONG ret;

    byte EthSrcAdr[6];
    ULONG EthSrcAdrLen;
    ret = NmGetFieldValueByteArray(parsedFrame, EthSrcAddrID, sizeof(EthSrcAdr), EthSrcAdr, &EthSrcAdrLen);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"NextPro = %x-%x-%x-%x-%x-%x, len = %d\n", EthSrcAdr[0],
                                                            EthSrcAdr[1], 
                                                            EthSrcAdr[2], 
                                                            EthSrcAdr[3], 
                                                            EthSrcAdr[4], 
                                                            EthSrcAdr[5], 
                                                            EthSrcAdrLen);
    }

    UINT8 NextProt;
    ret = NmGetFieldValueNumber8Bit(parsedFrame, IPv4NextProtocolID, &NextProt);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"NextPro = %d\n", NextProt);
    }

    UINT16 TCPSrcPort;
    ret = NmGetFieldValueNumber16Bit(parsedFrame, TCPSrcPortID, &TCPSrcPort);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"TCPSrcPort = %d\n", TCPSrcPort);
    }

    UINT32 TCPSeqNum;
    ret = NmGetFieldValueNumber32Bit(parsedFrame, TCPSeqNumID, &TCPSeqNum);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"TCPSeqNum = %ld\n", TCPSeqNum);
    }

    UINT64 SMBAllocSize;
    ret = NmGetFieldValueNumber64Bit(parsedFrame, SMBAllocSizeID, &SMBAllocSize);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"SMBAllocSize = %ld\n", SMBAllocSize);
    }

}

void
RawFrameAccess(HANDLE rawFrame)
{
    ULONG ret;

    ULONG RawFrameLength;
    NmGetRawFrameLength(rawFrame, &RawFrameLength);
    wprintf(L"Frame Length = %d\n", RawFrameLength);

    BYTE *RawFrameBuf = (BYTE *)malloc(RawFrameLength);
    ULONG ActFrameLength;
    NmGetRawFrame(rawFrame, RawFrameLength, RawFrameBuf, &ActFrameLength);
    wprintf(L"Actual Raw Frame Buf copied %d\n", ActFrameLength);
    free(RawFrameBuf);

    BYTE PartialBuf[100];
    NmGetPartialRawFrame(rawFrame, 10, sizeof(PartialBuf), PartialBuf, &ActFrameLength);

}

void
PerFramePropertyAccess(HANDLE frameParser)
{
    ULONG ret;
    WCHAR buf[256];
    ULONG retlength;
    NmPropertyValueType type;

    ret = NmGetPropertyValueById(frameParser, ProcName_PropID, sizeof(buf), (PBYTE)buf, &retlength, &type);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error %d trying to get property for Conversation.ProcessName\n", ret);
    }
    else if(type == NmPropertyValueNone)
    {
        wprintf(L"Conversation.ProcessName does not exist in this frame.\n");
    }
    else
    {
        wprintf(L"Conversation.ProcessName = %s\n", buf);
    }

    unsigned int tcplen;
    ret = NmGetPropertyValueById(frameParser, TCPLen_PropID, sizeof(tcplen), (PBYTE)&tcplen, &retlength, &type);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error %d trying to get property for Property.TCPPayloadLength\n", ret);
    }
    else if(type == NmPropertyValueNone)
    {
        wprintf(L"Property.TCPPayloadLength does not exist in this frame.\n");
    }
    else
    {
        wprintf(L"Property.TCPPayloadLength = %d\n", tcplen);
    }
}

void
FrameEval(HANDLE capFile)
{
    HANDLE hRawFrame;
    ULONG ret;

    ULONG FilterCount;
    ret = NmGetFilterCount(myFrameParser, &FilterCount);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"Number of filters in this frame parser = %d\n", FilterCount);
    } else {
        wprintf(L"Unable to get filter count, error=%d\n", ret);
        return;
    }

    ULONG PropertyCount;
    ret = NmGetRequestedPropertyCount(myFrameParser, &PropertyCount);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"Number of properties in this frame parser = %d\n", PropertyCount);
    } else {
        wprintf(L"Unable to get property count, error=%d\n", ret);
        return;
    }

    ULONG NumberOfFrames;
    ret = NmGetFrameCount(capFile, &NumberOfFrames);
    wprintf(L"Number of frames = %d\n", NumberOfFrames);

    for(ULONG curFrame = 0;curFrame < NumberOfFrames; curFrame++)
    {
        wprintf(L"-------------- Frame %d ---------------\n", curFrame+1);
        ret = NmGetFrame(capFile, curFrame, &hRawFrame);

        if(ret == ERROR_SUCCESS)
        {
            // See if there is a comment
            ULONG TitleLen;
            ULONG DescLen;
            PBYTE Title;
            PBYTE Desc;
            ret = NmGetFrameCommentInfo(hRawFrame, &TitleLen, NULL, &DescLen, NULL);
            if(ret == ERROR_INSUFFICIENT_BUFFER)
            {
                wprintf(L"TitleLen = %d, DescLen = %d\n", TitleLen, DescLen);
                Title = (PBYTE)malloc(TitleLen);
                Desc = (PBYTE)malloc(DescLen);

                ret = NmGetFrameCommentInfo(hRawFrame, &TitleLen, Title, &DescLen, Desc);
                if(ret == ERROR_SUCCESS)
                {
                    wprintf(L"Title: %s\n", Title);
                    wprintf(L"---- Description ----\n");
                    wprintf(L"%S\n", Desc);
                    wprintf(L"-- End Description --\n");
                }
                else
                {
                    wprintf(L"Error %d getting frame comment with buffer.\n");
                }
            }
            else if(ret == ERROR_EMPTY)
            {
                wprintf(L"No Frame Comment available.\n");
            }
            else
            {
                wprintf(L"Error %d getting frame comment with no buffer.\n");
            }

            RawFrameAccess(hRawFrame);

            HANDLE myParsedFrame;

            ret = NmParseFrame(myFrameParser, 
                                hRawFrame, 
                                curFrame, 
                                    NmFieldFullNameRequired |
                                    NmContainingProtocolNameRequired |
                                    NmDataTypeNameRequired |
                                    NmFieldDisplayStringRequired |
                                    NmFrameConversationInfoRequired , 
                                &myParsedFrame, 
                                NULL);


            if(ret == ERROR_SUCCESS)
            {
                WCHAR Description[512];
                ULONG DescRetLen;
                NmPropertyValueType proptype;

                ret = NmGetPropertyValueByName(myFrameParser, L"Property.Description", sizeof(Description), (PBYTE)Description, &DescRetLen, &proptype);
                if(ret == ERROR_SUCCESS)
                {
                    wprintf(L"Description: %s\n", Description);
                }

                // Get the Top Converation

                WCHAR ProtName[256];
                ULONG RetProtNameLen = 0;
                ULONG ConvID = 0;
                ProtName[0] = L'\0';

                ret = NmGetTopConversation(myParsedFrame, 256, ProtName, &RetProtNameLen, &ConvID);
                if(ret == ERROR_SUCCESS)
                {
                    // Iterate through all conversations until an error occurs.
                    while(ret == ERROR_SUCCESS)
                    {
                        wprintf(L"(%s %d) ", ProtName, ConvID);
                        ret = NmGetParentConversation(myParsedFrame, ConvID, 256, ProtName, &RetProtNameLen, &ConvID);
                    }
                } 
                else if(ret == ERROR_NOT_FOUND)
                {
                    wprintf(L"No conversation info.\n");
                } 
                else  
                {
                    wprintf(L"Error %d getting top conversation.\n", ret);
                }
                wprintf(L"\n");

                ULONG MacType = 0;
                ret = NmGetFrameMacType(myParsedFrame, &MacType);
                wprintf(L"MacType is %d, ret=%d\n", MacType, ret);

                UINT64 timestamp;
                NmGetFrameTimeStamp(myParsedFrame, &timestamp);
                
                FILETIME ft;
                ft.dwHighDateTime = timestamp >> 32;
                ft.dwLowDateTime = timestamp & 0xFFFFFFFFL;

                SYSTEMTIME mySystemTime;
                FileTimeToSystemTime(&ft, &mySystemTime);

                wprintf(L"Time Stamp is %d/%d/%d %d:%d:%d.%d\n", mySystemTime.wMonth,
                                                    mySystemTime.wDay,
                                                    mySystemTime.wYear,
                                                    mySystemTime.wHour,
                                                    mySystemTime.wMinute,
                                                    mySystemTime.wSecond,
                                                    mySystemTime.wMilliseconds);

                ULONG FieldCount;
                NmGetFieldCount(myParsedFrame, &FieldCount);
                wprintf(L"Frame has %d fields\n", FieldCount);

                NM_FRAGMENTATION_INFO FragInfo;
                FragInfo.Size = sizeof(FragInfo);
                ret = NmGetFrameFragmentInfo(myParsedFrame, &FragInfo);
                wprintf(L"Fragment Type is %d, ret=%d\n", FragInfo.FragmentType, ret);

                // Test to see if this frame passed our filter.
                BOOL passed;
                NmEvaluateFilter(myParsedFrame, myHTTPFilterID, &passed);
                if(passed)
                {
                    // Obtain the value of http.request.uri from frame.  We
                    // know that strings are passed as bstrVal in the variant.
                    WCHAR value[256];
                    ret = NmGetFieldValueString(myParsedFrame, HttpURIID, 256, value);
                    if(ret == ERROR_SUCCESS)
                    {
                        wprintf(L"HTTP: %s\n", value);
                    }

                    WCHAR NameBuffer[512];
                    ret = NmGetFieldName(myParsedFrame, HttpURIID, NmFieldDataTypeName, sizeof(NameBuffer), (LPWSTR)NameBuffer);
                    wprintf(L"Data Type for HttpURIID is %s\n", NameBuffer);

                    ULONG FieldOffset, FieldSize;
                    ret = NmGetFieldOffsetAndSize(myParsedFrame, HttpURIID, &FieldOffset, &FieldSize);
                    wprintf(L"Http URI is at offset %d and size %d\n", FieldOffset, FieldSize);

                    NM_PARSED_FIELD_INFO ParsedFieldInfo;
                    ParsedFieldInfo.Size = sizeof(ParsedFieldInfo);
                    NmGetParsedFieldInfo(myParsedFrame, HttpURIID, 0, &ParsedFieldInfo);
                    wprintf(L"Http size in bits is %d\n", ParsedFieldInfo.FieldBitLength);
                }

                GetFieldFunctions(myParsedFrame);

                PBYTE rawbuf[8];
                ULONG rawbuflen;
                ret = NmGetFieldInBuffer(myParsedFrame, SMBAllocSizeID, 8, (PBYTE)rawbuf, &rawbuflen);
                if(ret == ERROR_SUCCESS)
                {
                    wprintf(L"%X-%X-%X-%X-%X-%X-%X-%X\n", rawbuf[0],
                                                            rawbuf[1],
                                                            rawbuf[2],
                                                            rawbuf[3],
                                                            rawbuf[4],
                                                            rawbuf[5],
                                                            rawbuf[6],
                                                            rawbuf[7]);
                }
                NmCloseHandle(myParsedFrame);

                PerFramePropertyAccess(myFrameParser);
            } else {
                wprintf(L"Error: 0x%X, trying to parse frame\n", ret);
            }
        }

        NmCloseHandle(hRawFrame);
    }

    // Now dump the IPv6 Property info if the NetworkMonitorInfo frame populates it.
    NM_PROPERTY_INFO PropInfo;
    PropInfo.Size = sizeof(PropInfo);
    PropInfo.Name = NULL;
    ret = NmGetPropertyInfo(myFrameParser, IPv6Addr_PropID, &PropInfo);
    if(ret == ERROR_SUCCESS)
    {
        wprintf(L"Number of items is %d\n", PropInfo.ItemCount);
        for(ULONG n = 0; n < PropInfo.ItemCount; n++)
        {
            NM_PROPERTY_STORAGE_KEY PropStorKey;
            PropStorKey.Size = sizeof(PropStorKey);
            PropStorKey.KeyType = NmMvsKeyTypeArrayIndex;
            PropStorKey.Key.ArrayIndex = n;
            ULONG IPv6AddrRetLen;
            NmPropertyValueType PropType;

            BYTE IPv6Address[16];
            ret = NmGetPropertyValueById(myFrameParser, IPv6Addr_PropID, sizeof(IPv6Address), (PBYTE)&IPv6Address, &IPv6AddrRetLen, &PropType, 1, &PropStorKey);
            if(ret == ERROR_SUCCESS)
            {
                wprintf(L"Address %d, length=%d, type=%d: ", n, IPv6AddrRetLen, PropType);
                for(int m = 0; m < 16; m++)
                {
                    wprintf(L"%2.2X ", IPv6Address[m]);
                }
                wprintf(L"\n");
            }
            else
            {
                wprintf(L"Error %d getting property value for item %d.  Returned len was %d\n", ret, n, IPv6AddrRetLen);
            }
        }
    }
    else if(ret = ERROR_NOT_FOUND)
    {
        wprintf(L"No Network Information frame in this trace.\n", ret);
    }
    else
    {
        wprintf(L"Error 0x%x getting Property Info\n", ret);
    }
}

void
NM3ApiInit()
{
    NM_API_CONFIGURATION NmApiConfig;

    NmApiConfig.Size = sizeof(NM_API_CONFIGURATION);
    NmGetApiConfiguration(&NmApiConfig);

    NmApiConfig.CaptureEngineHandleCountLimit = 4;

    NmApiInitialize(&NmApiConfig);

    unsigned short Maj, Min, Build, Revision;
    NmGetApiVersion(&Maj, &Min, &Build, &Revision);
    wprintf(L"Maj: %d, Min: %d, Build: %d, Revision: %d\n", Maj, Min, Build, Revision);    
}

// Frame indicaiton callback.  Get's called each time a frame appears.
void __stdcall 
myFrameIndication(HANDLE hCapEng, ULONG AdpIdx, PVOID pContext, HANDLE hRawFrame)
{
    HANDLE capFile = (HANDLE)pContext;
    NmAddFrame(capFile, hRawFrame);
}

void
NmCaptureEngine(HANDLE myTempCap)
{
    ULONG ret;
    // Open capture engine.
    HANDLE myCaptureEngine;
    ret = NmOpenCaptureEngine(&myCaptureEngine);
    if(ret == ERROR_SUCCESS)
    {
        ULONG AdapterCount;
        ret = NmGetAdapterCount(myCaptureEngine, &AdapterCount);

        NM_NIC_ADAPTER_INFO AdapterInfo;
        AdapterInfo.Size = sizeof(AdapterInfo);
        ret = NmGetAdapter(myCaptureEngine, ADAPTER_INDEX, &AdapterInfo);
        if(ret != ERROR_SUCCESS)
        {
            wprintf(L"Error %d attempted to get adapter information.\n", ret);
        }


        // Configure the adapters callback and pass capture handle as context value.
        ret = NmConfigAdapter(myCaptureEngine, ADAPTER_INDEX, myFrameIndication, myTempCap, NmDiscardRemainFrames);
        if(ret == ERROR_SUCCESS)
        {
            wprintf(L"Capturing for 5 second\n");
            ret = NmStartCapture(myCaptureEngine, ADAPTER_INDEX, NmLocalOnly);
            if(ret == ERROR_SUCCESS)
            {
                Sleep(5000);

                wprintf(L"Pausing capture for 1 second\n");
                ret = NmPauseCapture(myCaptureEngine, ADAPTER_INDEX);
                if(ret != ERROR_SUCCESS)
                {
                    wprintf(L"Error %d pausing the capture\n", ret);
                }

                Sleep(1000);

                wprintf(L"Resuming capture for 5 seconds\n");
                ret = NmResumeCapture(myCaptureEngine, ADAPTER_INDEX);
                if(ret != ERROR_SUCCESS)
                {
                    wprintf(L"Error %d resuming the capture\n", ret);
                }

                Sleep(5000);

                wprintf(L"Stopping capture\n");
                ret = NmStopCapture(myCaptureEngine, ADAPTER_INDEX);
                if(ret != ERROR_SUCCESS)
                {
                    wprintf(L"Error %d stopping the capture\n", ret);
                }
            }
        } else {
            wprintf(L"Error configuring the adapter.\n");
        }
        Sleep(4000);
        NmCloseHandle(myCaptureEngine);
    } else {
        wprintf(L"Error openning capture engine, 0x%X\n", ret);
    }
}

void
NmCaptureFile()
{
    ULONG ret;
    HANDLE myCapFile;

    ULONG ActualSize;
    ret = NmCreateCaptureFile(L"test.cap", 20000000, NmCaptureFileWrapAround, &myCapFile, &ActualSize);
    if(ret == ERROR_SUCCESS)
    {
        NmCaptureEngine(myCapFile);

        NmCloseHandle(myCapFile);
    } else {
        wprintf(L"Error openning capture file, 0x%X\n", ret);
    }

    ret = NmOpenCaptureFile(L"test.cap", &myCapFile);

    if(ret == ERROR_SUCCESS)
    {
        ULONG NumberOfFrames;
        ret = NmGetFrameCount(myCapFile, &NumberOfFrames);
        wprintf(L"%d frames total\n", NumberOfFrames);
    }

    NmCloseHandle(myCapFile);

    NM_ORDER_PARSER_PARAMETER myOrderParserParam;
    myOrderParserParam.Size = sizeof(NM_ORDER_PARSER_PARAMETER);
    myOrderParserParam.FrameParser = myFrameParser;
    ret = NmOpenCaptureFileInOrder(L"test.cap", &myOrderParserParam, &myCapFile);
    if(ret != ERROR_SUCCESS)
    {
        wprintf(L"Error openning capture file in order.\n");
    }
    NmCloseHandle(myCapFile);
}

void
AnalyzeCaptureFile(WCHAR *CaptureFile)
{
    ULONG ret;
    HANDLE myCapFile;

    ret = NmOpenCaptureFile(CaptureFile, &myCapFile);
    if(ret == ERROR_SUCCESS)
    {
        FrameEval(myCapFile);

        RawBuffer(myCapFile);

        NmCloseHandle(myCapFile);
    }
}

int __cdecl 
wmain(int argc, WCHAR* argv[])
{
    NM3ApiInit();

    NmCaptureFile();

    if(MyLoadNPL())
    {
        if(argc > 1)
        {
            AnalyzeCaptureFile(argv[1]);
        }
        else 
        {
            wprintf(L"Analysis not done, no capture file provided\n");
        }

        UnLoadNPL();
    } else {
        wprintf(L"NPL load failed.\n");
    }

    NmApiClose();
    
    return 0;
}
