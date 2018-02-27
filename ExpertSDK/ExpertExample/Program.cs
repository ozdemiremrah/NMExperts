//-----------------------------------------------------------------------
// <copyright file="Program.cs" company="Microsoft">
//     Copyright (c) 2009 Microsoft. All rights reserved.
// </copyright>
// <author>Michael A. Hawker</author>
//-----------------------------------------------------------------------
using System;

[assembly: CLSCompliant(false)]
namespace ExpertExample
{
    using System.Collections.Generic;
    using System.Runtime.InteropServices.ComTypes;
    using System.Text;
    using Microsoft.NetworkMonitor;
    using Microsoft.NetworkMonitor.Automation;

    /// <summary>
    /// Provides a quick example of accessing parameters received from the Network Monitor UI
    /// and processing a capture file looking only at the frames selected from the user.
    /// </summary>
    [CLSCompliant(false)]
    public sealed class Program
    {
        /// <summary>
        /// Returned by API functions when they are successful
        /// </summary>
        private const ulong ERROR_SUCCESS = 0;

        /// <summary>
        /// Reference to a file, file path, handle, or data field is incorrect.
        /// </summary>
        private const ulong ERROR_NOT_FOUND = 1168;

        /// <summary>
        /// The field is a container, so the content is not available.
        /// </summary>
        private const ulong ERROR_RESOURCE_NOT_AVAILABLE = 5006;

        /// <summary>
        /// Size of Buffer to use when retrieving field values
        /// </summary>
        private const uint BUFFER_SIZE = 512;

        /// <summary>
        /// Used to signal the API has been loaded
        /// </summary>
        private static bool initialized;

        /// <summary>
        /// Used to hold the Parser Error Callback Function Pointer
        /// </summary>
        private static ParserCallbackDelegate pErrorCallBack = new ParserCallbackDelegate(ParserCallback);

        /// <summary>
        /// Prevents a default instance of the Program class from being created
        /// </summary>
        private Program()
        {
        }

        /// <summary>
        /// Called when the Parser Engine has information or an error message
        /// </summary>
        /// <param name="pCallerContext">Called Context given to the Parsing Engine</param>
        /// <param name="ulStatusCode">Message Status Code</param>
        /// <param name="lpDescription">Description Text of Error</param>
        /// <param name="ulType">Type of Message</param>
        public static void ParserCallback(IntPtr pCallerContext, uint ulStatusCode, string lpDescription, NmCallbackMsgType ulType)
        {
            if (ulType == NmCallbackMsgType.Error)
            {
                Console.WriteLine("ERROR: " + lpDescription);
            }
            else
            {
                Console.WriteLine(lpDescription);
            }
        }

        /// <summary>
        /// Simple Expert Example to just spit out command line arguments.
        /// </summary>
        /// <param name="args">arguments from Netmon</param>
        [STAThread]
        public static void Main(string[] args)
        {
            // Load API
            try
            {
                initialized = Program.InitializeNMAPI();
            }
            catch (BadImageFormatException)
            {
                Console.WriteLine("There was an error loading the NMAPI.\n\nPlease ensure you have the correct version installed for your platform.");
            }
            catch (DllNotFoundException)
            {
                Console.WriteLine("There was an error loading the NMAPI DLL.\n\nPlease ensure you have Network Monitor 3.3 installed or try rebooting.");
            }

            CommandLineArguments commandReader = new CommandLineArguments();
            if (commandReader.ParseCommandLineArguments(args))
            {
                if (commandReader.IsNoArguments)
                {
                    Console.WriteLine(CommandLineArguments.GetUsage("ExpertExample"));
                }
                else if (commandReader.IsRequestingHelp)
                {
                    Console.WriteLine(CommandLineArguments.GetUsage("ExpertExample"));
                }
                else if (initialized)
                {
                    Console.WriteLine("Running Test Application with Arguments:");
                    Console.WriteLine("\tCapture File: " + commandReader.CaptureFileName);
                    Console.WriteLine("\tDisplay Filter: " + commandReader.DisplayFilter);
                    Console.WriteLine("\tConversation Filter: " + commandReader.ConversationFilter);
                    Console.WriteLine("\tSelected Frames: " + commandReader.SelectedFramesString);

                    Console.WriteLine();

                    bool loadedparserengine = false;

                    // Configure Parser Engine
                    uint errno;                    
                    IntPtr hNplParser = IntPtr.Zero;
                    IntPtr hFrameParserConfig = IntPtr.Zero;
                    uint conversationFilterId = 0;
                    uint displayFilterId = 0;
                    IntPtr hFrameParser = IntPtr.Zero;

                    // Only load the parsing engine if we have to
                    if (!string.IsNullOrEmpty(commandReader.ConversationFilter) || !string.IsNullOrEmpty(commandReader.DisplayFilter))
                    {
                        Console.WriteLine("Loading Parser Engine...");

                        // Passing in null for the path will use the default configuration as specified in the Netmon UI
                        errno = NetmonAPI.NmLoadNplParser(null, NmNplParserLoadingOption.NmAppendRegisteredNplSets, pErrorCallBack, IntPtr.Zero, out hNplParser);
                        if (errno == ERROR_SUCCESS)
                        {
                            // Configure Frame Parser
                            errno = NetmonAPI.NmCreateFrameParserConfiguration(hNplParser, pErrorCallBack, IntPtr.Zero, out hFrameParserConfig);
                            if (errno == ERROR_SUCCESS)
                            {
                                // Enable Conversations
                                errno = NetmonAPI.NmConfigConversation(hFrameParserConfig, NmConversationConfigOption.None, true);
                                if (errno == ERROR_SUCCESS)
                                {
                                    // Add Filters
                                    if (!string.IsNullOrEmpty(commandReader.ConversationFilter))
                                    {
                                        Console.WriteLine("Adding Conversation Filter...");
                                        errno = NetmonAPI.NmAddFilter(hFrameParserConfig, commandReader.ConversationFilter, out conversationFilterId);
                                    }

                                    if (errno == ERROR_SUCCESS)
                                    {
                                        if (!string.IsNullOrEmpty(commandReader.DisplayFilter))
                                        {
                                            Console.WriteLine("Adding Display Filter...");
                                            errno = NetmonAPI.NmAddFilter(hFrameParserConfig, commandReader.DisplayFilter, out displayFilterId);
                                        }

                                        if (errno == ERROR_SUCCESS)
                                        {
                                            errno = NetmonAPI.NmCreateFrameParser(hFrameParserConfig, out hFrameParser, NmFrameParserOptimizeOption.ParserOptimizeNone);
                                            if (errno == ERROR_SUCCESS)
                                            {
                                                Console.WriteLine("Parser Engine Loaded Successfully!");
                                                Console.WriteLine();

                                                loadedparserengine = true;
                                            }
                                            else
                                            {
                                                Console.WriteLine("Parser Creation Error Number = " + errno);
                                            }
                                        }
                                        else
                                        {
                                            Console.WriteLine("Display Filter Creation Error Number = " + errno);
                                        }
                                    }
                                    else
                                    {
                                        Console.WriteLine("Conversation Filter Creation Error Number = " + errno);
                                    }
                                }
                                else
                                {
                                    Console.WriteLine("Conversation Error Number = " + errno);
                                }

                                if (!loadedparserengine)
                                {
                                    NetmonAPI.NmCloseHandle(hFrameParserConfig);
                                }
                            }
                            else
                            {
                                Console.WriteLine("Parser Configuration Error Number = " + errno);
                            }

                            if (!loadedparserengine)
                            {
                                NetmonAPI.NmCloseHandle(hNplParser);
                            }
                        }
                        else
                        {
                            Console.WriteLine("Error Loading NMAPI Parsing Engine Error Number = " + errno);
                        }
                    }

                    // Wait for confirmation
                    Console.WriteLine("Press any key to continue");
                    Console.ReadKey(true);

                    // Let's open the capture file
                    // Open Capture File
                    IntPtr captureFile = IntPtr.Zero;
                    errno = NetmonAPI.NmOpenCaptureFile(commandReader.CaptureFileName, out captureFile);
                    if (errno == ERROR_SUCCESS)
                    {
                        // Retrieve the number of frames in this capture file
                        uint frameCount;
                        errno = NetmonAPI.NmGetFrameCount(captureFile, out frameCount);
                        if (errno == ERROR_SUCCESS)
                        {
                            // Loop through capture file
                            for (uint ulFrameNumber = 0; ulFrameNumber < frameCount; ulFrameNumber++)
                            {
                                // Get the Raw Frame data
                                IntPtr hRawFrame = IntPtr.Zero;
                                errno = NetmonAPI.NmGetFrame(captureFile, ulFrameNumber, out hRawFrame);
                                if (errno != ERROR_SUCCESS)
                                {
                                    Console.WriteLine("Error Retrieving Frame #" + (ulFrameNumber + 1) + " from file");
                                    continue;
                                }

                                // Need to parse once to get similar results to the UI
                                if (loadedparserengine)
                                {
                                    // Parse Frame
                                    IntPtr phParsedFrame;
                                    IntPtr phInsertedRawFrame;
                                    errno = NetmonAPI.NmParseFrame(hFrameParser, hRawFrame, ulFrameNumber, NmFrameParsingOption.FieldDisplayStringRequired | NmFrameParsingOption.FieldFullNameRequired | NmFrameParsingOption.DataTypeNameRequired, out phParsedFrame, out phInsertedRawFrame);
                                    if (errno == ERROR_SUCCESS)
                                    {
                                        // Check against Filters
                                        if (!string.IsNullOrEmpty(commandReader.ConversationFilter))
                                        {
                                            bool passed;
                                            errno = NetmonAPI.NmEvaluateFilter(phParsedFrame, conversationFilterId, out passed);
                                            if (errno == ERROR_SUCCESS)
                                            {
                                                if (passed)
                                                {
                                                    if (!string.IsNullOrEmpty(commandReader.DisplayFilter))
                                                    {
                                                        bool passed2;
                                                        errno = NetmonAPI.NmEvaluateFilter(phParsedFrame, displayFilterId, out passed2);
                                                        if (errno == ERROR_SUCCESS)
                                                        {
                                                            if (passed2)
                                                            {
                                                                PrintParsedFrameInformation(phParsedFrame, ulFrameNumber, commandReader);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        PrintParsedFrameInformation(phParsedFrame, ulFrameNumber, commandReader);
                                                    }
                                                }
                                            }
                                        }
                                        else if (!string.IsNullOrEmpty(commandReader.DisplayFilter))
                                        {
                                            bool passed;
                                            errno = NetmonAPI.NmEvaluateFilter(phParsedFrame, displayFilterId, out passed);
                                            if (errno == ERROR_SUCCESS)
                                            {
                                                if (passed)
                                                {
                                                    PrintParsedFrameInformation(phParsedFrame, ulFrameNumber, commandReader);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            PrintParsedFrameInformation(phParsedFrame, ulFrameNumber, commandReader);
                                        }

                                        NetmonAPI.NmCloseHandle(phInsertedRawFrame);
                                        NetmonAPI.NmCloseHandle(phParsedFrame);
                                    }
                                    else
                                    {
                                        Console.WriteLine("Error Parsing Frame #" + (ulFrameNumber + 1) + " from file");
                                    }
                                }
                                else
                                {
                                    // Just print what I just deleted...
                                    uint pulLength;
                                    errno = NetmonAPI.NmGetRawFrameLength(hRawFrame, out pulLength);
                                    if (errno == ERROR_SUCCESS)
                                    {
                                        if (commandReader.IsSelected(ulFrameNumber))
                                        {
                                            Console.WriteLine("Frame #" + (ulFrameNumber + 1) + " (Selected) Frame Length(bytes): " + pulLength);
                                        }
                                        else
                                        {
                                            Console.WriteLine("Frame #" + (ulFrameNumber + 1) + " Frame Length(bytes): " + pulLength);
                                        }
                                    }
                                    else
                                    {
                                        Console.WriteLine("Error Getting Frame Length for Frame #" + (ulFrameNumber + 1));
                                    }
                                }

                                NetmonAPI.NmCloseHandle(hRawFrame);
                            }
                        }
                        else
                        {
                            Console.WriteLine("Error Retrieving Capture File Length");
                        }

                        // Close Capture File to Cleanup
                        NetmonAPI.NmCloseHandle(captureFile);
                    }
                    else
                    {
                        Console.WriteLine("Could not open capture file: " + commandReader.CaptureFileName);
                        Console.WriteLine(CommandLineArguments.GetUsage("ExpertExample"));
                    }

                    if (loadedparserengine)
                    {
                        NetmonAPI.NmCloseHandle(hFrameParser);
                        NetmonAPI.NmCloseHandle(hFrameParserConfig);
                        NetmonAPI.NmCloseHandle(hNplParser);
                    }
                }
            }
            else
            {
                Console.WriteLine(commandReader.LastErrorMessage);
                Console.WriteLine(CommandLineArguments.GetUsage("ExpertExample"));
            }

            // Pause so we can see the results when launched from Network Monitor
            Console.WriteLine();
            Console.WriteLine("Press any key to continue");
            Console.ReadKey();

            if (initialized)
            {
                CloseNMAPI();
            }
        }
        
        /// <summary>
        /// Used to ask and then print out extended information about a specific frame
        /// </summary>
        /// <param name="hParsedFrame">Parsed Frame</param>
        /// <param name="frameNumber">Frame Number to Display</param>
        /// <param name="command">Command Line Parameters</param>
        private static void PrintParsedFrameInformation(IntPtr hParsedFrame, uint frameNumber, CommandLineArguments command)
        {
            uint errno;
            uint ulFieldCount;
            string ds = "Frame #" + (frameNumber + 1);

            // Is Selected
            if (command.IsSelected(frameNumber))
            {
                ds += " (Selected)";
            }

            // Get Frame Timestamp
            ulong timestamp;
            errno = NetmonAPI.NmGetFrameTimeStamp(hParsedFrame, out timestamp);
            if (errno == ERROR_SUCCESS)
            {
                ds += " " + DateTime.FromFileTimeUtc((long)timestamp).ToString();
            }
            else
            {
                ds += " Timestamp Couldn't be Retrieved.";
            }

            Console.WriteLine(ds);
            Console.Write("Print Frame Info? (y/n) ");

            char key = Console.ReadKey().KeyChar;
            Console.WriteLine();

            if (key == 'y' || key == 'Y')
            {
                errno = NetmonAPI.NmGetFieldCount(hParsedFrame, out ulFieldCount);

                for (uint fid = 0; fid < ulFieldCount; fid++)
                {
                    // Get Field Name
                    char[] name = new char[BUFFER_SIZE * 2];
                    unsafe
                    {
                        fixed (char* pstr = name)
                        {
                            errno = NetmonAPI.NmGetFieldName(hParsedFrame, fid, NmParsedFieldNames.NamePath, BUFFER_SIZE * 2, pstr);
                        }
                    }

                    if (errno == ERROR_SUCCESS)
                    {
                        Console.Write(new string(name).Replace("\0", string.Empty) + ": ");
                    }
                    else
                    {
                        Console.WriteLine("Error Retrieving Field, NmGetFieldName Returned: " + errno);
                        continue;
                    }

                    // Get Field Value as displayed in Netmon UI
                    name = new char[BUFFER_SIZE];
                    unsafe
                    {
                        fixed (char* pstr = name)
                        {
                            errno = NetmonAPI.NmGetFieldName(hParsedFrame, fid, NmParsedFieldNames.FieldDisplayString, BUFFER_SIZE, pstr);
                        }
                    }

                    if (errno == ERROR_SUCCESS)
                    {
                        Console.WriteLine(new string(name).Replace("\0", string.Empty));
                    }
                    else if (errno == ERROR_NOT_FOUND)
                    {
                        Program.PrintParsedFrameFieldValue(hParsedFrame, fid);
                    }
                    else
                    {
                        Console.WriteLine("Error Retrieving Value, NmGetFieldName Returned: " + errno);
                        continue;
                    }
                }

                Console.WriteLine();
            }
        }

        /// <summary>
        /// Prints out a field's value if the display string couldn't be found.
        /// </summary>
        /// <param name="hParsedFrame">Parsed Frame</param>
        /// <param name="fieldId">Field Number to Display</param>
        private static void PrintParsedFrameFieldValue(IntPtr hParsedFrame, uint fieldId)
        {
            NmParsedFieldInfo parsedField = new NmParsedFieldInfo();
            parsedField.Size = (ushort)System.Runtime.InteropServices.Marshal.SizeOf(parsedField);

            uint errno = NetmonAPI.NmGetParsedFieldInfo(hParsedFrame, fieldId, parsedField.Size, ref parsedField);
            if (errno == ERROR_SUCCESS)
            {
                if (parsedField.NplDataTypeNameLength != 0)
                {
                    char[] name = new char[BUFFER_SIZE];
                    unsafe
                    {
                        fixed (char* pstr = name)
                        {
                            errno = NetmonAPI.NmGetFieldName(hParsedFrame, fieldId, NmParsedFieldNames.DataTypeName, BUFFER_SIZE, pstr);
                        }
                    }

                    Console.Write("(" + new string(name).Replace("\0", string.Empty) + ") ");
                }

                if (parsedField.FieldBitLength > 0)
                {
                    byte number8Bit = 0;
                    ushort number16Bit = 0;
                    uint number32Bit = 0;
                    ulong number64Bit = 0;
                    ulong rl = parsedField.ValueBufferLength;

                    switch (parsedField.ValueType)
                    {
                        case FieldType.VT_UI1:
                            errno = NetmonAPI.NmGetFieldValueNumber8Bit(hParsedFrame, fieldId, out number8Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine(number8Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_I1:
                            errno = NetmonAPI.NmGetFieldValueNumber8Bit(hParsedFrame, fieldId, out number8Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine((sbyte)number8Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_UI2:
                            errno = NetmonAPI.NmGetFieldValueNumber16Bit(hParsedFrame, fieldId, out number16Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine(number16Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_I2:
                            errno = NetmonAPI.NmGetFieldValueNumber16Bit(hParsedFrame, fieldId, out number16Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine((short)number16Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_UI4:
                            errno = NetmonAPI.NmGetFieldValueNumber32Bit(hParsedFrame, fieldId, out number32Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine(number32Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_I4:
                            errno = NetmonAPI.NmGetFieldValueNumber32Bit(hParsedFrame, fieldId, out number32Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine((int)number32Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_UI8:
                            errno = NetmonAPI.NmGetFieldValueNumber64Bit(hParsedFrame, fieldId, out number64Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine(number64Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_I8:
                            errno = NetmonAPI.NmGetFieldValueNumber64Bit(hParsedFrame, fieldId, out number64Bit);
                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine((long)number64Bit);
                            }
                            else
                            {
                                Console.WriteLine("Error " + errno);
                            }

                            break;
                        case FieldType.VT_ARRAY | FieldType.VT_UI1:
                            byte[] byteArray = new byte[BUFFER_SIZE];
                            unsafe
                            {
                                fixed (byte* barr = byteArray)
                                {
                                    errno = NetmonAPI.NmGetFieldValueByteArray(hParsedFrame, fieldId, BUFFER_SIZE, barr, out number32Bit);
                                }
                            }

                            if (errno == ERROR_SUCCESS)
                            {
                                for (uint i = 0; i < number32Bit; i++)
                                {
                                    Console.Write(byteArray[i].ToString("X2") + " ");
                                }

                                if ((parsedField.FieldBitLength >> 3) > number32Bit)
                                {
                                    Console.Write(" ... " + ((parsedField.FieldBitLength >> 3) - number32Bit) + " more bytes not displayed");
                                }

                                Console.WriteLine();
                            }
                            else if (errno == ERROR_RESOURCE_NOT_AVAILABLE)
                            {
                                Console.WriteLine("The field is a container");
                            }

                            break;
                        case FieldType.VT_LPWSTR:
                            char[] name = new char[BUFFER_SIZE];
                            unsafe
                            {
                                fixed (char* pstr = name)
                                {
                                    errno = NetmonAPI.NmGetFieldValueString(hParsedFrame, fieldId, BUFFER_SIZE, pstr);
                                }
                            }

                            if (errno == ERROR_SUCCESS)
                            {
                                Console.WriteLine(new string(name).Replace("\0", string.Empty));
                            }
                            else
                            {
                                Console.WriteLine("String is too long to display");
                            }

                            break;
                        case FieldType.VT_LPSTR:
                            Console.WriteLine("Should not occur");
                            break;
                        case FieldType.VT_EMPTY:
                            Console.WriteLine("Struct or Array types expect description");
                            break;
                        default:
                            Console.WriteLine("Unknown Type " + parsedField.ValueType);
                            break;
                    }
                }
                else
                {
                    Console.WriteLine("Empty");
                }
            }
            else
            {
                Console.WriteLine("Could Not Retrieve Parsed Field Info " + errno);
            }
        }

        #region API Initialization and Cleanup
        /// <summary>
        /// Called to close the Network Monitor API when we're done
        /// </summary>
        private static void CloseNMAPI()
        {
            ulong errno = NetmonAPI.NmApiClose();
            if (errno != ERROR_SUCCESS)
            {
                Console.WriteLine("Error unloading NMAPI Error Number = " + errno);
            }
        }

        /// <summary>
        /// Takes care of initializing the Network Monitor API
        /// </summary>
        /// <returns>true on success</returns>
        private static bool InitializeNMAPI()
        {
            // Initialize the NMAPI          
            NM_API_CONFIGURATION apiConfig = new NM_API_CONFIGURATION();
            apiConfig.Size = (ushort)System.Runtime.InteropServices.Marshal.SizeOf(apiConfig);
            ulong errno = NetmonAPI.NmGetApiConfiguration(ref apiConfig);
            if (errno != ERROR_SUCCESS)
            {
                Console.WriteLine("Failed to Get NMAPI Configuration Error Number = " + errno);
                return false;
            }

            // Set possible configuration values for API Initialization Here
            ////apiConfig.CaptureEngineCountLimit = 4;

            errno = NetmonAPI.NmApiInitialize(ref apiConfig);
            if (errno != ERROR_SUCCESS)
            {
                Console.WriteLine("Failed to Initialize the NMAPI Error Number = " + errno);
                return false;
            }

            return true;
        }
        #endregion
    }
}
