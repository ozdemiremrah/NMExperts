Network Monitor 3.3 Expert SDK
Copyright 2009 Microsoft
------------------------------
This SDK is licensed under MS-PL, see http://nmexperts.codeplex.com for more details
------------------------------

Overview
========
This SDK provides an example Expert environment.  It shows how to interpret
parameters from Network Monitor and use the Network Monitor API to filter
and display frames the user is viewing in their trace.

The ExpertExample.sln project file was built for Visual Studio 2008 and C#.NET 2.0.

It assumes you have installed Network Monitor 3.3 to the default path.  If not,
you may need to update the location to NetmonAPI.cs.


MSI Creation
============
When the project is built in a Release Configuration for a specific platform (X86, x64)
an MSI will automatically be built and appear under the "msi" directory.

The Expert MSI directory contains the dependencies for the MSI creation.  It requires 
the WiX Toolkit binaries (assumed to be installed to C:\Program Files\WiX\) and 
Perl to function (assumed to be in the default path).

More information is available in the detailed help file in the included project:
Writing Experts for Network Monitor.docx


Resources
=========
Expert Portal: http://nmexperts.codeplex.com/

Network Monitor Download: http://go.microsoft.com/fwlink/?LinkID=103158

The WiX Toolkit binary zip is available here: http://sourceforge.net/project/showfiles.php?group_id=105970&package_id=114109

Strawberry Perl for Windows is available here: http://strawberryperl.com/releases.html