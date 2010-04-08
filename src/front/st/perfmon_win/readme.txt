Using the Ingres Server Performance DLL

Overview
--------

The Ingres Server Performance DLL provides server measurements for use 
with the Windows NT Performance Monitor. 


Building and Installing
-----------------------

The DLL must be built from the source code using the Windows NT build
Utilities and then installed on the target system using the following
steps:

    1. Load the driver entries into the registry using the following
    command line:

        REGEDIT oipfctrs.reg

    2. Load the performance names into the registry using the command
    line:

        LODCTR oipfctrs.ini

    "The following example shows the registry location where performance
    counter names and descriptions are stored.

        HKEY_LOCAL_MACHINE
            \SOFTWARE
	        \Microsoft
	            \Windows NT
	    	        \CurrentVersion
	    	            \Perflib
	    		        Last Counter = [highest counter index]
	    		        Last Help = [highest help index]
	    		        \009
	    		       	    Counter = 2 System 4 Memory ...
	    		    	    Help = 3 The System object type ...
	    		        \[supported language, other than U.S. English]
	    		            Counter = ...
	    		    	    Help = ...

    "The lodctr utility takes strings from an .INI file and adds them to the
    Counter and Help values under the appropriate language subkeys. It also
    updates the Last Counter and Last Help values. In addition to adding values 
    under the PerfLib key, the lodctr utility also adds the following value
    entries to the Services node for the application.

    	HKEY_LOCAL_MACHINE
	    \SYSTEM
	        \CurrentControlSet
		    \Services
		        \oiPfCtrs
			    \Performance
			        First Counter = [lowest counter index]
				First Help = [lowest help index]
				Last Counter = [highest counter index]
				Last Help = [highest help index]"
    
    [From the Microsoft Platform SDK Help*, "Adding Performance Counters", 
    Built on: Monday, November 16, 1998]

    3. Start the Ingres DBMS and use the terminal monitor to create the 
    appropriate tables and objects in the IMADB database, using the file 
    makiman.sql. For instance,

    	sql imadb <makiman.sql

    4. If you are not running the performance monitor as user "ingres", 
    you need to grant authorization to your userid for the appropriate 
    IMA tables. You can use the SQL Terminal Monitor to do this, as user
    ingres:

    	sql imadb
	>grant select on ima_dmf_cache_stats to public;
	>grant select on ima_locks to public;
	>grant select on ima_cssampler_threads to public;
	>grant select on ima_cssampler_stats to public;

    5. In order to view values from the ima_cssampler_threads and 
    ima_cssampler_stats tables, you must first start sampling from 
    within iimonitor. For example,

    	iimonitor ii\ingres\f6
	IIMONITOR> start sampling
	IIMONITOR> quit
    
    6. The following environment variable must be set before starting the
    Windows NT Performance Monitor.

    	set II_SYSTEM=[the path to the Ingres installation]

    This is necessary for the performance DLL to use the config.dat file
    for GCN, GCA, and the like. 
    
    7. Ensure that %II_SYSTEM%\ingres\bin and ...\utility are in your path when 
    you start the Performance Monitor, in order to to access the Ingres 
    Extensible Performance Counters DLL, iipfctrs.dll.
    
    For example, at the command line, type: 
	set II_SYSTEM=c:\oping
	set PATH=%PATH%;%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility
	perfmon
    where the config file is c:\oping\ingres\files\config.dat.


To Verify the Installation of the Ingres Extensible Counter DLL
---------------------------------------------------------------

Use the Windows NT utility, EXCTRLST, to verify the installation of the Ingres 
Extensible Counter DLL counters and help text.

    "The extensible counter list utility (EXCTRLST.EXE) is used to obtain 
    information about the extensible performance counter dynamic-link libraries 
    (DLLs) on a computer running Windows NT. This utility lists the 
    applications, drivers, and services that are registered to provide 
    performance data through the Windows NT performance registry. You can use 
    this utility for finding information and for troubleshooting."

    [From the Microsoft Platform SDK Help*, "ExCtrLst", Built on: Monday, 
    November 16, 1998]

    "When the EXCTRLST utility is started, it scans the registry to determine 
    which applications, devices, and services have registered extensible 
    performance counter DLLs. These DLLs are listed in the list box of the 
    display along with the corresponding application, driver, or service. The 
    EXCTRLST utility does not test for the existence of the DLLs.

    "Note  The values for the Counter ID Range and Help ID Range fields must 
    correspond to the entries found in the following registry key: 

    HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Perflib\<LangID>\Counter 

    where <LangID> corresponds to the specific entry for the default language 
    on the machine (for example, 009 indicates that the language is English). 
    If these values do not correspond, the counters and objects described for 
    the service are not displayed correctly."

    [From the Microsoft Platform SDK Help*, "Using EXCTRLST", Built on: Monday, 
    November 16, 1998]

At this point the software is installed and ready to use. 

	NOTE: The system may need to be restarted after these instructions 
	are completed for these objects to be seen by remote computers.


Monitoring via the Ingres Server Performance DLL
------------------------------------------------

You may start the performance monitor from the command line, via Run... on the 
Start menu, or by using the NT explorer. Be sure that the II_SYSTEM variable 
and PATH are set.

Start the Windows NT performance monitor (Perfmon) and, on the Edit menu, 
select Add To Chart... to display the Add to Chart dialog. Note that it may 
take a few seconds to display the dialog, because the Ingres Extensible Counter 
DLL uses API to start a user session with the Ingres DBMS server. 

From the Object drop-down list, select one of the Ingres Server objects:
	Ingres Server Cache, 
	Ingres Server Locks, 
	Ingres Server Sampler, or 
	Ingres Server Threads.

The Counter list will display a scrollable list of the counters within the 
chosen Ingres Server Object. Choose one or more counters to monitor and 
display, pressing the Add button for each counter selected.

The Ingres Server Cache object displays an Instance of each counter for each 
cache size that is defined for the DBMS server (2K pages, 4K pages, ...). 
Select the desired Instance before pressing the Add button.

The Ingres Server Threads object displays an instance of each counter for every 
thread type in the server: 
	2_Phase_Commit   - Two-phase commit
	Audit            - Audit
	Check_Dead       - Checks for abnormal process termination
	Check_Term       - Used to check for termination
	Cp_Timer         - Previously called the Fast Commit thread, now all
			   servers use this thread to perform consistency points
	Cs_Intrnl_Thread - The main thread of the Ingres server process
	Event            - Handles event processing
	Factotum         - A "light-weight" thread used for parallel operations
	Fast_Commit      - Fast commit (replaced by consistency point thread)
	Force_Abort      - Performs force abort processing
	Group_Commit     - Performs group commit processing
	License          - License
	Lkcallback       - Locking system callback
	Lkdeadlock       - Locking system deadlock detection
	Logwriter        - Performs transaction log file writes
	Monitor          - A thread used for an iimonitor session
	Normal           - Normal, user session
	Read_Ahead       - Read-ahead
	Recovery         - In recovery process, performs on-line recovery
	Rep_Qman         - Replicator queue manager
	Sampler          - The monitor sampler (if sampling is started)
	Secaud_Writer    - In C2 enabled servers, performs security auditing
	Server_Init      - Server initialization
	Sort             - Query sort
	Write_Along      - Write-along
	Write_Behind     - Performs write-behind processing

Note that some thread types are used only on specific platforms and operating 
systems; not all thread types will contain meaningful counter values. 

If the Ingres monitor sampler is not running, no instances will be displayed 
for the Ingres Server Threads Object counters. If that is the case, press the 
Cancel button on the dialog, start the sampler from within iimonitor, and 
redisplay the Add to Chart dialog--the Ingres Server Threads object instances 
should now be visible.


Troubleshooting
---------------

If the Ingres Server objects are not visible in the Add to Chart dialog 
Object drop-down list, start the Event Viewer by clicking on the Windows NT 
Start menu: Programs, Administrative Tools (Common), Event Viewer. Display the 
Application Log by clicking Application on the Log menu. 

The Ingres Extensible Performance Counter DLL writes any error messages to the 
NT error log: the "Source" column is "oipfctrs". Double-click on the message to 
display the Event Detail dialog. The Description contains a textual description 
of the error and the Data contains relevant numeric (hexadecimal) status codes.


Monitoring a Remote Ingres Server
---------------------------------

To use the NT performance monitor for a remote Ingres server (on any platform 
type), you must set the Windows NT environment variable "II_PF_VNODE" to be the 
GCF address (vnode name) of the desired server. For instance, 
	set II_PF_VNODE=usilsu98
You do not need to set the II_PF_VNODE variable to monitor the local, Windows 
NT server.

The Ingres Name Server (iigcn) and Communications Server (iigcc) must be 
active on the Windows NT system in order to monitor performance via the Ingres 
Extensible Counters DLL (iipfctrs.dll). For instance:

	C:\>set II_SYSTEM=c:\oping

	C:\>set PATH=%PATH%;%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility

	C:\>set II_PF_VNODE=usilsu98

	C:\>ingstart -iigcn
	 
	Starting the name server...
	 
	 
	C:\>ingstart -iigcc
	 
	Starting Net server (default).............
	 
	 
	C:\>perfmon


Uninstalling the Ingres Server Performance DLL
----------------------------------------------

To uninstall the Ingres Server performance DLL:

    1.	Remove the counter names and descriptions from the registry using
    the command line:

    	UNLODCTR oipfctrs

    "The unlodctr utility looks up the First Counter and Last Counter values
    in the application's Performance key to determine the indexes of the
    counter objects to remove. Using these indexes, it makes the following
    changes to the Perflib key.

        HKEY_LOCAL_MACHINE
            \SOFTWARE
	        \Microsoft
	            \Windows NT
	    	        \CurrentVersion
	    	            \Perflib
	    		        Last Counter = [updated if changed]
	    		        Last Help = [updated if changed]
	    		        \009
	    		       	    Counter = [application text removed]
	    		    	    Help    = [application text removed] 
	    		        \[supported language, other than U.S. English]
	    		            Counter = [application text removed]
	    		    	    Help    = [application text removed] 

    "The unlodctr utility also removes the First Counter, First Help, Last 
    Counter, and Last Help values from the application's Performance key."

    [From the Microsoft Platform SDK Help*, "Adding Performance Counters", 
    Built on: Monday, November 16, 1998]


*The Microsoft Platform SDK Help is Copyright © 1998 Microsoft Corporation. 
