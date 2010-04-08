.NET Data Provider Strong Name Keys

The Ingres .NET Data provider consists of four strong-named assemblies:

Ingres.Client
Ingres.VSDesign
Ingres.Support
Ingres.Install

Strong-named assemblies must be signed with a Strong Name Key (SNK).  In order to preserve security, Ingres Corporation does not distribute a keypair SNK file to fully sign each assembly.  Only the keypublic SNK file is distributed and each data provider assembly is delay-signed (i.e partially-signed).  If you wish to build the Ingres .NET from source and fully sign each assembly then you must create your own keypair and keypublic SNK files before the build.  To create an SNK, locate the Microsoft .NET Strong Name Tool "sn.exe". The "sn.exe" tool is usually in the "bin" directory of Windows SDK or Visual Studio.
For example:
	C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin
	C:\Program Files\Microsoft Visual Studio 8\SDK\v2.0\Bin.

Run the Strong Name Tool to create the .snk files:

	sn.exe -k  keypair.snk
	sn.exe -p  keypair.snk  keypublic.snk
	sn.exe -tp keypublic.snk > keypublic.txt

Keep a copy of the keypair.snk and keypublic.snk files in a safe, secure location as if safeguarding the keys to the data provider assemblies.  This location of the copies should be kept separate from the Ingres source tree directories to prevent accidental loss of the files in the event that the Ingres source tree is deleted and repopulated with a new source distribution.  If this file is lost and the data provider assemblies need to be rebuilt and signed with a new SNK files, all applications compiled against the old data provider that was signed with the old key signature will need to be recompiled.

Copy the keypair.snk and keypublic.snk files to this directory where the Ingres build can use it:

%ING_SRC%\common\dotnet2\specials

The assemblyinfo file in each of the projects for the four assemblies will reference the keypair.snk and keypublic.snk files in this directory to sign the assembly.  All four assemblies must be signed with the same keypair.snk and keypublic.snk files. 

If the Ingres source tree is deleted and repopulated with a new source distribution, simply recopy the keypair.snk and keypublic.snk files from its secure location to the common\dotnet2\specials directory.  Because the assemblies have the same Strong Name Key signature and version, applications do not have to be recompiled.


By default, the build of the Ingres .NET Data Provider assemblies will only delay-sign (partially-sign) the assemblies if the keypair.snk file is not present.  Applications will be able to reference the delayed-signed Ingres.Client assembly but will not be able to load the assembly at application execution run-time.  A "System.IO.FileLoadException: Could not load file or assembly 'Ingres.Client...' Strong name validation failed." exception will be raised.  For development purposes, either rebuild the data provider with a keypair.snk and keypublic.snk files of your own creation as described above, or temporarily turn off strong-name verification using the following command:
	sn.exe -Vr Ingres.Client.dll
Use this command carefully and only during development.  Skipping verification of this assembly name opens a security vulnerability whereby a malicious assembly under this name could be inserted into the computer.

If you used your own keypair.snk file to compile and delay-sign the assembly, issue the following command to fully-sign the assembly:
	sn.exe -Ra Ingres.Client.dll
If you wish to validate that the assembly is fully-signed, issue the following command:
	sn.exe -v  Ingres.Client.dll

In addition to the Ingres.Client assembly, repeat the process for Ingres.VSDesign, Ingres.Support, and Ingres.Install assemblies.
