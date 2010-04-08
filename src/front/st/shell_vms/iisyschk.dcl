$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iisyschk.com -- system resource checker 
$!
$!  Usage:
$!	@iisyschk [ -silent ]
$!
$!  Description:
$!	This script checks requirements for various system resources
$!	(stored in config.dat) against available resources to determine
$!	whether Ingres can start up as configured.
$!
$!!     24-mar-2000 (devjo01)
$!!         Make gen of scratch file names safe for Clustered Ingres.
$!!     12-Jan-2000 (horda03) Bug 102803/INGSRV 1281
$!!         Check that the memory requirements for the DBMS servers
$!!         do not exceed the Virtual Address area (for P0 and P1 space).
$!!     14-Jun-2001 (horda03) Bug 104977 
$!!         Do not check the P0/P1 memory requirements on a CLIENT
$!!         only installation.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!------------------------------------------------------------------------	
$!
$! Define symbols for programs called locally.
$!
$ iigetres    := $ii_system:[ingres.utility]iigetres.exe
$ iipmhost    := $ii_system:[ingres.utility]iipmhost.exe
$ iilgkmem    := $ii_system:[ingres.utility]iilgkmem.exe
$ iishowres   := $ii_system:[ingres.bin]iishowres.exe
$!
$! Other variables, macros and temporary files
$!
$ resources_ok = "TRUE"
$ node_name    = f$getsyi("NODENAME")
$ tmpfile      = "ii_system:[ingres]syscheck." + node_name + -
	"_" + F$GetJPI(0,"PID") + "_tmp"
$ warnings     = "FALSE"
$!
$ say         := type sys$input
$ echo        := write sys$output 
$!
$ if( "''p1'" .eqs. "-SILENT" )
$ then
$    verbose = "FALSE"
$ else
$    verbose = "TRUE"
$ endif
$ if( verbose )
$ then
$    echo ""
$    echo "Checking node ''node_name' for system resource required to run Ingres..."
$ endif
$!
$! Check to make sure II_SYSTEM is defined properly
$!
$ iigetres ii.'node_name.lnm.table.id tableid
$ tableid = f$trnlnm( "tableid" )
$ if( tableid .eqs. "")
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "ii.''node_name'.lnm.table.id not found in CONFIG.DAT."
$       echo ""
$    endif 
$    goto EXIT_FAIL
$ endif
$ deassign "tableid"
$!
$ if( f$trnlnm("II_SYSTEM","''tableid'",,"EXECUTIVE",,"ACCESS_MODE") .nes. "EXECUTIVE" )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "II_SYSTEM must be defined in EXECUTIVE_MODE in the ''tableid' table."
$       echo ""
$    endif 
$    goto EXIT_FAIL     
$ endif
$
$!
$! Determine Installation type
$!
$! If IISHOWRES.EXE doesn't exist, then this is a client only
$! installation.
$ if f$search( f$element(1, "$", iishowres)) .eqs. ""
$ then
$    client_installation = 1
$ else
$    client_installation = 0
$ endif
$
$!
$! Check the global memory requirements.
$!
$ iigetres ii.'node_name.syscheck.gblpages reqgpgs 
$ reqgpgs = f$trnlnm( "reqgpgs" )
$ if( reqgpgs .eqs. "" )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "ii.''node_name'.syscheck.gblpages not found in CONFIG.DAT."
$       echo "" 
$    endif
$    goto EXIT_FAIL 
$ endif
$ deassign "reqgpgs"
$!
$! Check for global memory.
$!
$                           ! Convert the pages to bytes.  For AXP pages
$ reqbytes = reqgpgs * 512  ! aren't 512 bytes, but the SYSGEN parm GBLPAGES
$                           ! is kept in pagelets which are 512 bytes.
$
$! Check for sufficient VIRTUALPAGECNT.
$!
$ vpgs = f$getsyi("VIRTUALPAGECNT")
$ iigetres ii.'node_name.syscheck.virtualpagecnt reqvpgs 
$ reqvpgs = f$trnlnm( "reqvpgs" )
$ if( reqvpgs .eqs. "" )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "ii.''node_name'.syscheck.virtualpagecnt not found in CONFIG.DAT."
$       echo "" 
$    endif
$    goto EXIT_FAIL
$ endif
$ deassign "reqvpgs"
$ if( vpgs .lt. reqvpgs )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "SYSGEN parameter VIRTUALPAGECNT should be increased:"
$       echo "''reqvpgs' pages of virtual memory are required for optimal performance."
$       echo "''vpgs' virtual pages are currently available."
$    endif
$    warnings = "TRUE"
$ endif
$!
$! Check for sufficient CHANNELCNT.
$!
$ iigetres ii.'node_name.syscheck.channelcnt reqchans
$ channels = f$getsyi("CHANNELCNT")
$ reqchans = f$trnlnm( "reqchans" )
$ if f$getsyi("hw_model") .ge. 1024
$ then
$    maxchans = 65535
$ else
$    maxchans = 2047
$ endif
$ if( reqchans .eqs. "" )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "ii.''node_name'.syscheck.channelcnt not found in CONFIG.DAT."
$       echo "" 
$    endif
$    goto EXIT_FAIL
$ endif
$ deassign "reqchans"
$ if (reqchans .gt. maxchans) then reqchans = maxchans
$ if( channels .lt. reqchans )
$ then
$    if( verbose )
$    then
$       echo "" 
$       echo "SYSGEN parameter CHANNELCNT should be increased:"
$       echo "''reqchans' I/O channels are required for optimal performance."
$       echo "''channels' are currently available."
$    endif
$    warnings = "TRUE"
$ endif
$
$ if (client_installation .eq. 0)
$ then
$!
$!   Now check that there is sufficient space available to allocate
$!   required memory (i.e. the amount of memory required for P0 and
$!   P1 space is OK.
$!
$    iiresutl -max_dbms_mem >'tmpfile'
$    open/read infile 'tmpfile
$    read infile max_dbms_mem
$    close infile
$    delete/noconfirm/nolog 'tmpfile;*
$
$    iiresutl -max_dbms_stack >'tmpfile'
$    open/read infile 'tmpfile
$    read infile max_dbms_stack
$    close infile
$    delete/noconfirm/nolog 'tmpfile;*
$
$    iishowres >'tmpfile'
$    open/read infile 'tmpfile
$    read infile lgkmemory
$    close infile
$    delete/noconfirm/nolog 'tmpfile;*
$
$    P0_mem = 'max_dbms_mem' + 'lgkmemory'
$    P1_mem = 'max_dbms_stack'
$
$    if P0_mem .LT. 0
$    then
$       resources_ok = "FALSE"
$    else
$       check_mem = P0_mem
$
$       gosub CHECK_MEMORY
$    endif
$
$    if ( .not. resources_ok)
$    then
$       if( verbose )
$       then
$          say

You have configured the Ingres Installation to use more (P0) memory space
than is available. Please reduce the memory requirements for any/all

   OPF_MEMORY, PSF_MEMORY, QSF_MEMORY, RDF_MEMORY, DMF Caches and the
   Logging/Locking system.

$       endif
$    endif
$
$    if (resources_ok)
$    then
$       if P1_mem .LT. 0
$       then
$          resources_ok = "FALSE"
$       else
$          check_mem = P1_mem + 60000
$
$          gosub CHECK_MEMORY
$       endif
$
$       if (.not. resources_ok )
$       then
$          if( verbose )
$          then
$             say

You have configured the Ingres Installation to use more (P1) memory space
than is available. Please reduce the memory requirement for DBMS servers
by either/all

     Reducing the connect_limit
     Reducing the number of Ingres threads (log writer, write behind, etc)
     Reducing the stack_size

$          endif
$       endif
$    endif
$ endif
$ 
$!
$! Check whether any resource problems were detected.
$!
$ if( resources_ok )
$ then
$    if (warnings)
$    then
$       reply/bell/user=('f$getjpi("","USERNAME")) -
            "Warnings occurred checking system resources required to run Ingres as configured."
$    endif
$    !
$    if( verbose )
$    then
$       say

Your system has sufficient resources to run Ingres.

$    endif
$    goto EXIT_OK
$ else
$    if( verbose )
$    then
$       say

Your system does not have sufficient resources to run Ingres as configured.

$    endif
$    goto EXIT_FAIL
$ endif
$!
$ EXIT_OK:
$ status = 0
$ goto exit_ok1
$!
$ EXIT_FAIL:
$ status = -1
$exit_ok1:
$!
$! Before exiting shut off message displays.  This is necessary because
$! an 'exit 0' displays:  %NONAME-W-NOMSG, Message number 00000000
$ set message/nofacility/noidentification/noseverity/notext
$ exit 'status'
$
$GET_MAX_CBF: subroutine
$   on warning then exit
$
$   'P2' == 0
$
$   search/out='tmpfile'/mat=and ii_config:config.dat 'p1','p2':
$
$   open/read max_cbf 'tmpfile'
$
$MAX_CBF_LOOP:
$   read/end_of_file=max_cbf_fin max_cbf record
$
$   val = f$element(1, ":", record)
$
$   if f$type(val) .eqs. "integer"
$   then
$      if 'p2' .lt. 'val' then 'p2' == 'val'
$   endif
$
$   goto MAX_CBF_LOOP
$MAX_CBF_FIN:
$   close max_cbf
$
$   delete/noconf/nolog 'tmpfile;*
$
$   exit
$endsubroutine

$CHECK_MEMORY:
$  if check_mem .eq. 0 then return
$
$  if 'check_mem' .lt. 0
$  then
$     resources_ok = "false"
$     return
$  endif
$  iilgkmem 'check_mem' >'tmpfile'
$  open/read infile 'tmpfile
$  read infile lgkstatus
$  close infile
$  delete/noconfirm/nolog 'tmpfile;*
$!
$! Parse the output from iilgkmem.  The first word of the output will 
$! contain the results of the memory allocation.
$!
$ lgkstatus = F$ELEMENT(0, " ", lgkstatus)
$ if ("''lgkstatus'" .nes. "SUCCESS")
$ then
$!   Check to see if there is a lack of GBLPAGFIL
$    if ("''lgkstatus'" .eqs. "GBLPAGFIL")
$    then
$       if( verbose )
$       then
$          echo ""
$          echo "SYSGEN parameter GBLPAGFIL must be increased to accommodate backing"
$          echo "storage for a maximum value of ''reqgpgs' GBLPAGES.  You can either" 
$          echo "increase GBLPAGFIL to accommodate this entire value, or refer to"
$          echo "the release notes to determine how to calculate a minimal value."
$       endif
$       resources_ok = "FALSE"
$    endif
$!
$!   Check to see if there is a lack of GBLPAGES
$    freepgs = f$getsyi( "FREE_GBLPAGES" )
$    if ( (reqgpgs.ge.freepgs) )
$    then
$       if( verbose )
$       then
$          echo ""
$          echo "SYSGEN parameter GBLPAGES must be increased to accommodate"
$          echo "''reqgpgs' GBLPAGES, there are currently ''freepgs' free pages."
$       endif
$       resources_ok = "FALSE"
$    endif
$!
$    if (("''lgkstatus'" .eqs. "GBLPAGES"))
$    then
$       if( verbose )
$       then
$          echo ""
$          echo "''reqgpgs' contiguous free GBLPAGES required but not available."
$          echo "There are ''reqgpgs' GBLPAGES, but they may not be contiguous. A"
$          echo "reboot of the system may clean up the memory, or you may want to"
$          echo "increase the value of the SYSGEN parameter GBLPAGES."
$       endif
$       resources_ok = "FALSE"
$    endif
$!
$!   Check to see if there is a lack of GBLSECTIONS
$    if ("''lgkstatus'" .eqs. "GBLSECTIONS")
$    then
$       if( verbose )
$       then
$          freescts = f$getsyi( "FREE_GBLSECTS" )
$          echo ""
$          echo "SYSGEN parameter GBLSECTIONS must be increased:"
$          echo "There are currently ''freescts' free global sections."
$       endif
$       resources_ok = "FALSE"
$    endif
$!
$!   If something else failed, bail out displaying the VMS message.
$    if ("''resources_ok'" .nes. "FALSE")
$    then
$       if( verbose )
$       then
$          echo "" 
$          echo "Error allocating global memory. Error returned is:"
$          echo F$MESSAGE( 'lgkstatus' )
$       endif
$       resources_ok = "FALSE"
$    endif
$ endif
$!
$ return
