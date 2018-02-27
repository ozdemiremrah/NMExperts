#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "objbase.h"
#include "ntddndis.h"
#include "NmApi.h"

int __cdecl 
wmain(int argc, WCHAR* argv[])
{
    ULONG ret = ERROR_SUCCESS;
    // We expect the first param to be a file.
    if(argc <= 1){
        wprintf(L"Expect a file name as the only command line parameter\n");
        return ERROR_INVALID_PARAMETER;
    }

    // Open given capture file
    HANDLE myCaptureFile = INVALID_HANDLE_VALUE;
    if(ERROR_SUCCESS == NmOpenCaptureFile(argv[1], &myCaptureFile))
    {
        ULONG myFrameCount = 0;
        ret = NmGetFrameCount(myCaptureFile, &myFrameCount); 
        if(ret == ERROR_SUCCESS)
        {
            // Iterate through all the frames and print out comments where they exist.
            HANDLE myRawFrame = INVALID_HANDLE_VALUE;
            for(ULONG i = 0; i < myFrameCount; i++)
            {
                ret = NmGetFrame(myCaptureFile, i, &myRawFrame); 
                if(ret == ERROR_SUCCESS)
                {
                    // Get comment from raw frame, no need to parse the frame
                    ULONG titleByteLength = 0;            //comment title length in bytes
                    ULONG descriptionByteLength = 0;      //comment description length in bytes

                    PBYTE titleBuffer = NULL;
                    PBYTE descriptionBuffer = NULL;

                    // Call NmGetFrameCommentInfo() first time to get comment length info if there is
                    ret = NmGetFrameCommentInfo(myRawFrame, &titleByteLength, titleBuffer, &descriptionByteLength, descriptionBuffer);
                    if (ret == ERROR_NOT_FOUND || ret == ERROR_EMPTY)   //Looks like ERROR_EMPTY is the right one
                    {
                        printf("Frame %d has no comment info\n", i+1);
                    }
                    else if (ret == ERROR_INSUFFICIENT_BUFFER)
                    {
                        if (titleByteLength > 0)
                        {
                            titleBuffer = new BYTE[titleByteLength];
                        }
                        if (descriptionByteLength > 0)
                        {
                            descriptionBuffer = new BYTE[descriptionByteLength];
                        }
                        
                        // Call NmGetFrameCommentInfo() second time to get comment in supplied buffers
                        ret = NmGetFrameCommentInfo(myRawFrame, &titleByteLength, titleBuffer, &descriptionByteLength, descriptionBuffer);
                        if (ret == ERROR_SUCCESS)
                        {
                            wprintf(L"Frame %d Comment Info:\n\tTitleByteLength: %d, Title: %s", i+1, titleByteLength, titleBuffer);
                            
                            // Description is ASCII, though it's encoded with RTF markup.
                            printf("\n\tDescriptionByteLength: %d, Description: %s\n", descriptionByteLength, descriptionBuffer);
                        }
                        else 
                        {
                            printf("Error: 0x%X, trying to get comment info for frame %d\n", ret, i+1);
                        }
                        
                        if( titleBuffer != NULL )
                        {
                            delete[] titleBuffer;
                        }
                        if( descriptionBuffer != NULL )
                        {
                            delete[] descriptionBuffer;
                        }
                    }

                    // Release current raw frame
                    NmCloseHandle(myRawFrame);
                } // NmGetFrame()
                else
                {
                    // Just print error but continue to loop
                    wprintf(L"Errors getting raw frame %d\n", i+1);
                }
            } // for
        } // GetFrameCount()
        NmCloseHandle(myCaptureFile);
    } // NmOpenCaptureFile()
    else
    {
        wprintf(L"Errors opening capture file:%s\n", argv[1]);
    }

    return 0;
}
