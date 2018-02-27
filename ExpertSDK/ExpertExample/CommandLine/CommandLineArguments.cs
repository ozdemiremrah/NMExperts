//-----------------------------------------------------------------------
// <copyright file="CommandLineArguments.cs" company="Microsoft">
//     Copyright (c) 2009 Microsoft. All rights reserved.
// </copyright>
// <author>Michael A. Hawker</author>
//-----------------------------------------------------------------------
namespace Microsoft.NetworkMonitor.Automation
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Provides an automated way to retrieve the commandline arguments passed to an Expert
    /// from the Network Monitor UI
    /// </summary>
    public class CommandLineArguments
    {
        /// <summary>
        /// Passed Capture File
        /// </summary>
        private string captureFileName;

        /// <summary>
        /// [Optional] The Display Filter currently applied in Network Monitor
        /// </summary>
        private string displayFilter = string.Empty;

        /// <summary>
        /// [Optional] The Filter used by the Conversation Tree in Network Monitor
        /// </summary>
        private string conversationFilter = string.Empty;

        /// <summary>
        /// [Optional] The Frames currently selected by the user in Network Monitor
        /// </summary>
        private string selectedFrames = string.Empty;

        /// <summary>
        /// Any string error message created when calling ParseCommandLineArguments
        /// </summary>
        private string errorMessage;

        /// <summary>
        /// Set true if the user calls the Expert with a /? parameter only
        /// </summary>
        private bool wantHelp;

        /// <summary>
        /// Set true if the user called the Expert with no arguments
        /// </summary>
        private bool noArguments;

        /// <summary>
        /// Initializes a new instance of the CommandLineArguments class
        /// </summary>
        public CommandLineArguments()
        {
        }

        /// <summary>
        /// Gets the Capture File Name
        /// </summary>
        public string CaptureFileName
        {
            get { return this.captureFileName; }
        }

        /// <summary>
        /// Gets the Display Filter
        /// </summary>
        public string DisplayFilter
        {
            get { return this.displayFilter; }
        }

        /// <summary>
        /// Gets the Conversation Filter
        /// </summary>
        public string ConversationFilter
        {
            get { return this.conversationFilter; }
        }

        /// <summary>
        /// <para>Gets both filters combined (the usual case) to see what the user sees
        /// (this usually requires parsing the file twice to completely process conversations correctly).
        /// This property returns the combined filter to emulate the Network Monitor 
        /// Frame Summary View.display.</para>
        /// <para>Network Monitor's actual behavior is to filter against the conversation filter and 
        /// the display filter separately.</para>
        /// </summary>
        public string CombinedFilter
        {
            get
            {
                if (string.IsNullOrEmpty(this.displayFilter) || string.IsNullOrEmpty(this.conversationFilter))
                {
                    return this.conversationFilter + this.displayFilter;
                }
                else
                {
                    return "(" + this.conversationFilter + ") && (" + this.displayFilter + ")";
                }
            }
        }

        /// <summary>
        /// Gets the Last Error Message
        /// </summary>
        public string LastErrorMessage
        {
            get { return this.errorMessage; }
        }

        /// <summary>
        /// Gets a value indicating whether the user has request help on using the Expert from the commandline
        /// </summary>
        public bool IsRequestingHelp
        {
            get { return this.wantHelp; }
        }

        /// <summary>
        /// Gets a value indicating whether if the User has selected frames (usually always the case
        /// as no deselection possible in the UI, but may be called from the commandline or run
        /// on a fresh file)
        /// </summary>
        public bool IsSelectingFrames
        {
            get { return this.selectedFrames.Length != 0; }
        }

        /// <summary>
        /// Gets a value indicating whether the user has called the Expert with no arguments
        /// </summary>
        public bool IsNoArguments
        {
            get { return this.noArguments; }
        }

        /// <summary>
        /// Gets an Iterator for only the frames selected within the Network Monitor UI
        /// If no commandline parameter was specified, will not provide any numbers
        /// Note that Network Monitor returns the visualized indexes of frames which start at 1;
        /// however the actual frame numbers when accessed by the NMAPI start at 0.  This function
        /// returns the indexes to the actual file so no further calculations are necessary.
        /// </summary>
        public IEnumerable<uint> SelectedFrames
        {
            get
            {
                if (this.selectedFrames.Length != 0)
                {
                    // First split our tokens by groups
                    string[] tokens = this.selectedFrames.Split(',');
                    for (uint x = 0; x < tokens.Length; x++)
                    {
                        // Check if we have a group range of frames
                        if (tokens[x].IndexOf("-", StringComparison.Ordinal) != -1)
                        {
                            string[] range = tokens[x].Split('-');
                            for (uint y = uint.Parse(range[0]); y <= uint.Parse(range[1], CultureInfo.InvariantCulture); y++)
                            {
                                yield return y - 1;
                            }
                        }
                        else
                        {
                            // Just return the evaluated single item
                            yield return uint.Parse(tokens[x], CultureInfo.InvariantCulture) - 1;
                        }
                    }
                }
                else
                {
                    yield break;
                }
            }
        }

        /// <summary>
        /// Gets the string representation of the Selected Frames as passed by Network Monitor
        /// </summary>
        public string SelectedFramesString
        {
            get
            {
                return this.selectedFrames;
            }
        }

        /// <summary>
        /// Returns the usage needed for the Expert based on this class
        /// </summary>
        /// <param name="applicationName">Application Name to display</param>
        /// <returns>Returns the usage string for printing or display with the Application Name first</returns>
        public static string GetUsage(string applicationName)
        {
            return applicationName + " {NetmonOpenCaptureFile} [/d {NetmonDisplayFilter}] [/c {NetmonConversationFilter}] [/f {NetmonSelectedFrameRange}]";
        }

        /// <summary>
        /// Checks if the frame number index (from 0) passed in is selected or not,
        /// If no frames are selected, will always return false.
        /// </summary>
        /// <param name="frameNumber">the frame number to check</param>
        /// <returns>true if the frame was selected by the user in Network Monitor</returns>
        public bool IsSelected(uint frameNumber)
        {
            frameNumber++; // Increment to match visualization

            if (this.selectedFrames.Length != 0)
            {
                // First split our tokens by groups
                string[] tokens = this.selectedFrames.Split(',');
                for (uint x = 0; x < tokens.Length; x++)
                {
                    // Check if we have a group range of frames
                    if (tokens[x].IndexOf("-", StringComparison.Ordinal) != -1)
                    {
                        string[] range = tokens[x].Split('-');
                        if (uint.Parse(range[0], CultureInfo.InvariantCulture) <= frameNumber && frameNumber <= uint.Parse(range[1], CultureInfo.InvariantCulture))
                        {
                            return true;
                        }
                    }
                    else
                    {
                        if (uint.Parse(tokens[x], CultureInfo.InvariantCulture) == frameNumber)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Provides a way if being called from the command line to iterate over the entire
        /// capture or just what's selected.  The selected frames parameter is always passed (if requested) from
        /// the UI once a user has selected a frame once.
        /// </summary>
        /// <param name="frameCount">Number of frames obtained from NmGetFrameCount</param>
        /// <returns>An Iterator over the file or selected frames</returns>
        public IEnumerable<uint> SelectedFramesOrCount(uint frameCount)
        {
            if (this.IsSelectingFrames)
            {
                foreach (uint value in this.SelectedFrames)
                {
                    yield return value;
                }
            }
            else
            {
                for (uint x = 0; x < frameCount; x++)
                {
                    yield return x;
                }
            }
        }

        /// <summary>
        /// Called to parse the arguments passed to the Expert's Main() function.
        /// </summary>
        /// <param name="args">Commandline arguments from Main()</param>
        /// <returns>true if successfully parsed, otherwise false, retrieve LastErrorMessage </returns>
        public bool ParseCommandLineArguments(string[] args)
        {
            this.errorMessage = null;

            // Check if we have any arguments or are asking for help
            if (args.Length == 0)
            {
                this.noArguments = true;
                return true;
            }
            else if (args.Length == 1 && args[0].Contains("/?"))
            {
                this.wantHelp = true;
                return true;
            }
            else
            {
                List<string> arguments = new List<string>(args);

                // Get Capture File Name
                this.captureFileName = arguments[0].Trim();

                arguments.RemoveAt(0);

                int skip;

                // Process Arguments until none are left
                while (arguments.Count > 0)
                {
                    skip = 1;
                    string cmd = arguments[0].Trim();

                    if (cmd.StartsWith("/d", StringComparison.CurrentCultureIgnoreCase))
                    {
                        if (arguments.Count > 1)
                        {
                            string arg = arguments[1].Trim();
                            if (!arg.StartsWith("/c", StringComparison.CurrentCultureIgnoreCase) && !arg.StartsWith("/f", StringComparison.CurrentCultureIgnoreCase))
                            {
                                this.displayFilter = arg;
                                skip = 2;
                            }
                        }
                    }
                    else if (cmd.StartsWith("/c", StringComparison.CurrentCultureIgnoreCase))
                    {
                        if (arguments.Count > 1)
                        {
                            string arg = arguments[1].Trim();
                            if (!arg.StartsWith("/d", StringComparison.CurrentCultureIgnoreCase) && !arg.StartsWith("/f", StringComparison.CurrentCultureIgnoreCase))
                            {
                                this.conversationFilter = arg;
                                skip = 2;
                            }
                        }
                    }
                    else if (cmd.StartsWith("/f", StringComparison.CurrentCultureIgnoreCase))
                    {
                        if (arguments.Count > 1)
                        {
                            string arg = arguments[1].Trim();
                            if (!arg.StartsWith("/c", StringComparison.CurrentCultureIgnoreCase) && !arg.StartsWith("/d", StringComparison.CurrentCultureIgnoreCase))
                            {
                                this.selectedFrames = arg;
                                skip = 2;
                            }
                        }
                    }
                    else
                    {
                        this.errorMessage = "Unrecognized parameter: " + cmd;
                        return false;
                    }

                    // Skip to next set of arguments
                    arguments.RemoveRange(0, skip);
                }
            }

            return true;
        }
    }
}
