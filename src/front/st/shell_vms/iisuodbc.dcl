$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 2004, 2008 Ingres Corporation
$!
$!  Name:
$!	iisuodbc -- set-up script for ODBC.
$!
$!  Usage:
$!	iisuodbc
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up the Ingres/EDBC
$!      ODBC configuration files. 
$!	It assumes that II_SYSTEM  has been set in the logical name table 
$!	(SYSTEM or GROUP) which is to be used to configure the installation 
$!	or in the JOB logical table.
$!
$!! History:
$!!	29-jan-2004 (loera01)
$!!	    Created.
$!!	30-jun-2004 (loera01)
$!!         Added ODBC trace library.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	18-apr-2005 (abbjo03)
$!!	    Fix reference to "JDBC".
$!!     05-may-2005 (loera01)
$!!         Revised naming of read-only ODBC driver and added iisetres for
$!!         the ODBC CLI.  Added support for "-batch" parameter.
$!!     31-may-2005 (loera01) Bug 114601
$!!         When prompting for read-only, check for both "n" and "N" replies.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!     12-jun-2007 (loera01) Bug 118495
$!!         Added references to II_ODBC_LIB and II_ODBC_CLILIB in config.dat.
$!!	22-jun-2007 (upake01)
$!!	    Answering "N"o to "Do you wish to continue this setup procedure? (y/n"
$!!         results in a "%SYSTEM-F-ABORT, abort" message.  Answering "N"o should
$!!         not be treated as an error as user just wishes to not install the
$!!         component.  Instead, display a message "*** The above component will
$!!         not be installed ***" and exit gracefully.
$!!     24-oct-1007 (Ralph Loen) Bug 119353
$!!         Rename ODBC libraries with the 'ii_installation extension, since
$!!         these libraries are now installed.
$!!     26-oct-1007 (Ralph Loen) Bug 119353
$!!         Add a single quote in front of the ii_installation symbol when
$!!         iisetres writes ii.'node_name.lnm.ii_installation.  Otherwise,
$!!         the symbol will not be expanded and string "ii_installation" will
$!!         be written.
$!!      Fixed grammar in an informational message.
$!! 08-jan-2008 (Ralph Loen) Bug 119667
$!!      For client-only installations, added definitions of shared 
$!!      logicals so that iigetres can function.  Exit with abort status if
$!!      iisunet or iisudbms has not been invoked.  Completed treatment of
$!!      iisulock.
$!!     11-nov-2008 (upake01) Bug 121214
$!!         Support "express" installation.
$!----------------------------------------------------------------------------
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ set noon
$! set noverify
$!
$! Define symbols for programs called locally.
$!
$ iigenres	:= $ii_system:[ingres.utility]iigenres.exe
$ iigetres	:= $ii_system:[ingres.utility]iigetres.exe
$ iigetsyi	:= @ii_system:[ingres.utility]iigetsyi.com
$ iiingloc      := $ii_system:[ingres.utility]iiingloc.exe
$ iijobdef      := $ii_system:[ingres.utility]iijobdef.exe
$ iipmhost      := $ii_system:[ingres.utility]iipmhost.exe
$ iimaninf	:= $ii_system:[ingres.utility]iimaninf.exe
$ iiresutl	:= $ii_system:[ingres.utility]iiresutl.exe
$ iisetres	:= $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:= $ii_system:[ingres.utility]iisulock.exe
$ iivalres	:= $ii_system:[ingres.utility]iivalres.exe
$ iiodbcinst	:= $ii_system:[ingres.utility]iiodbcinst.exe
$!
$! Other variables, macros and temporary files
$!
$! Get the current values for share library logical definitions.
$!
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") 
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  
$ ii_interplib_def   = f$trnlnm("ii_interplib","LNM$JOB") 
$ iisulocked         = 0                             !Do we hold the setup lock?
$ saved_message      = f$environment( "MESSAGE" )    ! save message settings
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL	     = 1
$ SS$_ABORT	     = 44
$ status	     = SS$_NORMAL
$ node_name	     = f$getsyi("NODENAME")
$ tmpfile            = "ii_system:[ingres]iisuodbc.''node_name'_tmp" !Temp file
$!
$! Make temporary definitions for shared library logicals to 
$! be the unconcealed path of the library directory within ii_system
$!
$ if ii_compatlib_def .eqs. "" 
$ then 
$   define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$ endif
$ if ii_libqlib_def .eqs. "" 
$ then
$   define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$ endif
$ if ii_framelib_def .eqs. ""
$ then
$   define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ endif
$ if ii_interplib_def .eqs. ""
$ then
$   define/nolog/job ii_interplib  'f$parse("ii_system:[ingres.library]interpfelib.exe",,,,"NO_CONCEAL")
$ endif
$!
$! Define ii_config for the installation configuration files
$!
$ define/nolog/process II_CONFIG    II_SYSTEM:[ingres.files]
$!
$  say		:= type sys$input
$  echo		:= write sys$output 
$!
$ clustered	= f$getsyi( "CLUSTER_MEMBER" ) !Is this node clustered?
$ user_uic	= f$user() !For RUN command (detached processes)
$ user_name= f$getjpi("","USERNAME") !User running this procedure
$!
$ ivp_name= f$getjpi("","USERNAME")
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" )
$!
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
$!
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=odbc -output='tmpfile
$ set noon
$!
$ open iioutput 'tmpfile
$ read iioutput version_string
$ close iioutput
$ delete/noconfirm 'tmpfile;*
$ release_id = f$edit( version_string, "collapse, trim" )
$!
$ STRIPPER_1:
$!
$ offset = f$locate( ".", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_2
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_1
$!
$ STRIPPER_2:
$!
$ offset = f$locate( "/", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_3
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_2
$!
$ STRIPPER_3:
$!
$ offset = f$locate( "(", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_4
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_3
$!
$ STRIPPER_4:
$!
$ offset = f$locate( ")", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPERS_DONE
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_1
$!
$ STRIPPERS_DONE:
$!
$ set_message_off
$ iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$ ii_installation = f$trnlnm( "ii_installation_value" )
$ if( ii_installation .eqs. "" ) 
$ then
$   ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$   if (ii_installation .nes. "")
$   then
$      iisetres ii.'node_name.lnm.ii_installation 'ii_installation
$   endif
$ else
$   deassign "ii_installation_value"
$ endif
$ !
$ if p1 .eqs. "-BATCH"
$ then
$    gosub batch_install
$ endif
$!
$!
$! Lock the configuration file. 
$!
$ on error then goto EXIT_FAIL
$ iisulock "Ingres ODBC setup"
$ iisulocked = 1
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ if dbms_setup_status .nes. "" then deassign "dbms_setup_status"
$ iigetres ii.'node_name.config.net.'release_id net_setup_status
$ net_setup_status = f$trnlnm( "net_setup_status" )
$ if net_setup_status .nes. "" then deassign "net_setup_status"
$ set_message_on
$ if ( dbms_setup_status .eqs. "" .and. net_setup_status .eqs. "" ) 
$ then 
$       say 
 
The setup procedure for the following version of the Ingres DBMS or IngresNet:
 
$       echo "  ''version_string'"
$       say
 
must be completed up before the Ingres ODBC Driver can be set up on this node:
 
$       echo "  ''node_name'"
$       echo ""
$       goto EXIT_FAIL
$ set_message_off
$ endif
$!
$ set_message_on
$    say 
 
This procedure will set up the following version of Ingres ODBC Server:
 
$    echo "	''version_string'"
$    say
 
to run on local node:
 
$    echo "	''node_name'"
$    say 
 

$!
$ CONFIRM_CONFIG_SETUP:
$!
$    inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_CONFIG_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       status = SS$_NORMAL
$       goto EXIT_OK
$    endif
$!
$ !
$ say

   You have the option of selecting an EXPRESS installation.  The EXPRESS
   option writes the odbcinst.ini file to the default location and defines the
   Ingres drivers as residing in II_SYSTEM:[ingres.library].

$ EXPRESS_INSTALL:
$    inquire/nopunc answer "Do you wish to select the EXPRESS option? (y/n)"
$    if( answer .eqs. "" ) then goto EXPRESS_INSTALL
$    if( answer .eqs. "Y" ) 
$    then
$       gosub batch_install
$       goto EXIT_SUCCESS
$    endif
$!
$!
$! Define system definitions from config.dat.
$!
$ set_message_on
$ say

     Loading existing configuration...

$ iijobdef
$ echo ""
$ echo "II_INSTALLATION configured as ''ii_installation."
$ !
$ if f$search("ii_system:[ingres]iistartup.del") .nes. "" then @ii_system:[ingres]iistartup.del
$ set_message_off
$ newpath = f$trnlnm("ODBCSYSINI")
$ if (newpath .eqs. "") 
$ then 
$    newpath = f$trnlnm("SYS$SHARE")
$ else
$    echo "ODBCSYSINI is defined.  Value is ''newpath'"
$ endif
$ set_message_on
$ ENTER_PATH:
$ inquire/nopunc path -
     "Enter the default ODBC configuration path [ ''newpath' ]: "
$ CONFIRM_PATH:
$ if path .eqs. "" then path = newpath
$ inquire/nopunc answer -
   "Is the path [ ''path' ] correct? (y/n): "
$ if( answer .eqs. "" ) then goto CONFIRM_PATH
$ if( .not. answer ) then goto ENTER_PATH
$!
$ if path .eqs. "" then path = newpath
$ if (newpath .nes. path) then pathFlag = "-p ''path' "
$ if p1 .eqs. "-R"
$ then
$ readOnlyFlag = "-R"
$ else
$   ENTER_READ_ONLY:
$   inquire/nopunc answer -
     "Is your ODBC driver read-only? (y/n) [n]: "
$   if( answer .eqs. "" .or. answer .eqs. "n" .or. answer .eqs. "N") 
$   then 
$      readOnlyFlag = ""
$      echo "Your ODBC driver will allow updates"
$   else
$      readOnlyFlag = "-R"
$      echo "Your ODBC driver will be read-only"
$   endif
$   CONFIRM_READ_ONLY:
$   inquire/nopunc answer -
      "Is this information correct? (y/n): "
$   if( answer .eqs. "" ) then goto CONFIRM_READ_ONLY
$   if( .not. answer ) then goto ENTER_READ_ONLY
$ endif
$ if ii_installation .nes. ""
$ then
$   set_message_on
$   copy II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$   purge II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB'II_INSTALLATION.EXE
$   copy II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$   purge II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB'II_INSTALLATION.EXE
$   copy II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$   purge II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB'II_INSTALLATION.EXE
$   set_message_off
$ endif
$ define/user sys$input sys$command
$ iiodbcinst 'pathFlag' 'readOnlyFlag'
$ if (readOnlyFlag .eqs. "-R")
$ then
$   iisetres ii.'node_name.odbc.module_name "ODBCROFELIB''ii_installation.EXE"
$ else
$   iisetres ii.'node_name.odbc.module_name "ODBCFELIB''ii_installation.EXE"
$ endif
$ @ii_system:[ingres.utility]iishare delete
$ iisetres ii.'node_name.lnm.ii_odbcrolib -
          "II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB''ii_installation.EXE"
$ iisetres ii.'node_name.lnm.ii_odbclib -
          "II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB''ii_installation.EXE"
$ iisetres ii.'node_name.odbc.cli_module_name -
          "ODBCCLIFELIB''ii_installation.EXE"
$ iisetres ii.'node_name.lnm.ii_odbc_clilib -
          "II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB''ii_installation.EXE"
$ iisetres ii.'node_name.odbc.trace_module_name -
          "ODBCTRACEFELIB''ii_installation.EXE"
$ @ii_system:[ingres.utility]iishare add
$!
$! Exit with success or error.
$!
$ EXIT_SUCCESS:
$ say
The Ingres ODBC driver has been successfully set up for this installation.

Refer to the Ingres Installation Guide for more information about 
configuring and using Ingres.

$ status = SS$_NORMAL
$ goto EXIT_OK
$!
$ EXIT_FAIL:
$ echo "ODBC configuration aborted."
$ status = SS$_ABORT
$ EXIT_OK:
$ set_message_on
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ set on
$ if iisulocked .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" 
$ then
$   delete/noconfirm ii_system:[ingres.files]config.lck;*
$ endif
$ exit 'status'
$!
$!
$!
$ BATCH_INSTALL:
$ iisetres ii.'node_name.odbc.module_name ODBCFELIB'ii_installation.EXE
$ iisetres ii.'node_name.odbc.cli_module_name -
        ODBCCLIFELIB'ii_installation.EXE
$ iisetres ii.'node_name.lnm.ii_odbclib -
        II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB'ii_installation.EXE
$ iisetres ii.'node_name.lnm.ii_odbc_clilib -
        II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB'ii_installation.EXE
$ iisetres ii.'node_name.odbc.trace_module_name "ODBCTRACEFELIB.EXE"
$ copy II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$ purge II_SYSTEM:[INGRES.LIBRARY]ODBCCLIFELIB'II_INSTALLATION.EXE
$ copy II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$ purge II_SYSTEM:[INGRES.LIBRARY]ODBCFELIB'II_INSTALLATION.EXE
$ copy II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB.exe -
                 II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB'II_INSTALLATION.EXE-
                 /prot=(s:rwed,o:rwed,g:rwed,w:re)
$ purge II_SYSTEM:[INGRES.LIBRARY]ODBCROFELIB'II_INSTALLATION.EXE
$ iiodbcinst -batch
$ return
$!
