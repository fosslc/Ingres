$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 1995, 2006 Ingres Corporation
$!
$!  Name:
$!	iisuc2 -- set-up script for C2 security auditing package 
$!
$!  Usage:
$!	iisuc2
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up the INGRES C2
$!	security auditing package.  It assumes that II_SYSTEM has been 
$!	set in the logical name table (SYSTEM or GROUP) which is to be used
$!	to configure the installation or in the JOB logical table.
$!
$!!  History:
$!!	22-feb-1995 (wolf) 
$!!		Created placeholder version.
$!!	10-apr-1995 (wolf) 
$!!		Wrote the real thing.
$!!	08-jun-1995 (wolf) 
$!!		Give user the choice of continuing or not.
$!!	19-jun-1995 (dougb)
$!!		Correct protections on CONFIG.DAT and don't use CREATE.
$!!	20-apr-98 (mcgem01)
$!!		Product name change to Ingres.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!      24-mar-2000 (devjo01)
$!!          Make gen of scratch file names safe for Clustered Ingres.
$!!      18-Jun-2001 (horda03)
$!!          Only allow Ingres System Administrator of the VMSINSTAL script
$!!          to invoke the IVP scripts.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!	21-jun-2007 (upake01)
$!!	    Answering "N"o to "Do you wish to continue this setup procedure? (y/n"
$!!         results in a "%SYSTEM-F-ABORT, abort" message.  Answering "N"o should
$!!         not be treated as an error as user just wishes to not install the
$!!         component.  Instead, display a message "*** The above component will
$!!         not be installed ***" and exit gracefully.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!             statement(s) for CONFIG.DAT.
$!!     15-Aug-2007 (Ralph Loen) SIR 118898
$!!         Added symbol definition of iinethost.
$!----------------------------------------------------------------------------
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ set noon
$ set noverify
$!
$! Define symbols for programs called locally.
$!
$ iigenres	:= $ii_system:[ingres.utility]iigenres.exe
$ iigetres	:= $ii_system:[ingres.utility]iigetres.exe
$ iigetsyi      := @ii_system:[ingres.utility]iigetsyi.com
$ iiingloc      := $ii_system:[ingres.utility]iiingloc.exe
$ iipmhost      := $ii_system:[ingres.utility]iipmhost.exe
$ iinethost     := $ii_system:[ingres.utility]iinethost.exe
$ iimaninf	:= $ii_system:[ingres.utility]iimaninf.exe
$ iircpseg	:= @ii_system:[ingres.utility]iircpseg.com
$ iiresutl	:= $ii_system:[ingres.utility]iiresutl.exe
$ iisetres	:= $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:= $ii_system:[ingres.utility]iisulock.exe
$ iivalres	:= $ii_system:[ingres.utility]iivalres.exe
$ ingstart	:= $ii_system:[ingres.utility]ingstart.exe
$ ingstop	:= @ii_system:[ingres.utility]iisdall.com
$!
$! Other variables, macros and temporary files
$!
$ iisulocked         = 0                               !Do we hold the setup lock?
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") ! Current values
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   !  for shared
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  !   library definitions
$ saved_message      = f$environment( "MESSAGE" )
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL         = 1
$ SS$_ABORT          = 44
$ status             = SS$_NORMAL
$ node_name	     = f$getsyi("NODENAME")
$ tmpfile            = "ii_system:[ingres]iisuc2.''node_name'_tmp" !Temp file
$!
$  say		:= type sys$input
$  echo		:= write sys$output 
$!
$ clustered	= f$getsyi( "CLUSTER_MEMBER" ) !Is this node clustered?
$ user_uic	= f$user() !For RUN command (detached processes)
$ user_name     = f$trnlnm( "II_SUINGRES_USER","LNM$JOB" ) !Installation owner
$ IF "''user_name'".EQS.""
$ THEN
$    isa_uic = f$file_attributes( "II_SYSTEM:[000000]INGRES.DIR", "UIC")
$
$    if isa_uic .nes. user_uic
$    then
$       type sys$input

     ---------------------------------------------------------
    | This setup procedure must be run from the Ingres System |
    | Administrator account.                                  |
     ---------------------------------------------------------
$
$      goto EXIT_FAIL
$    endif
$
$    user_name= f$getjpi("","USERNAME") !User running this procedure
$ ENDIF
$!
$! Make temporary definitions for shared library logicals to 
$! be the unconcealed path of the library directory within ii_system
$! 
$ define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ if f$search("ii_system:[ingres]iistartup.del") .nes. "" then @ii_system:[ingres]iistartup.del
$!
$! Define ii_config for the installation configuration files
$!
$ define/nolog/process II_CONFIG    II_SYSTEM:[ingres.files]
$!
$ echo "Setting up Ingres C2 Security Auditing ..."
$!
$! Create ii_system:[ingres.files]config.dat if non-existent. 
$!
$ if( f$search( "ii_system:[ingres.files]config.dat" ) .eqs. "" )
$ then
$    copy nl: ii_system:[ingres.files]config.dat/prot=(s:rwed,o:rwed,g:rwed,w:re)
$ endif
$!
$! Lock the configuration file. 
$!
$ on error then goto EXIT_FAIL 
$ iisulock "Ingres C2 Security Auditing setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=dbms -output='tmpfile
$ set noon
$!
$ open iioutput 'tmpfile
$ read iioutput version_string
$ close iioutput
$ delete 'tmpfile;*
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
$! Check if Ingres C2 Security Auditing is already set up.
$!
$ set_message_off
$ iigetres ii.'node_name.config.c2.'release_id c2_setup_status
$ c2_setup_status = f$trnlnm( "c2_setup_status" )
$ deassign "c2_setup_status"
$ set_message_on
$ if( ( c2_setup_status .nes. "" ) .and. -
     ( c2_setup_status .eqs. "COMPLETE" ) )
$ then
$    say 

The latest version of Ingres C2 Security Auditing has already been set up
to run on local
$    echo "node ''node_name'."
$    echo ""
$    goto EXIT_OK
$ endif
$!
$! Make sure that the Ingres DBMS is set up. 
$!
$ set_message_off
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ deassign "dbms_setup_status"
$ set_message_on
$ if( dbms_setup_status .nes. "COMPLETE" ) 
$ then
$    say 
 
The setup procedure for the following version of Ingres DBMS:
 
$    echo "	''version_string'"
$    say
 
must be completed up before security auditing can be set up on this host:
 
$    echo "	''node_name'"
$    echo ""
$    goto EXIT_FAIL
$ endif
$!
$ set_message_off
$ iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$ ii_installation = f$trnlnm( "ii_installation_value" )
$ deassign "ii_installation_value"
$ set_message_on
$ say 
 
This procedure will set up Ingres Security Auditing for local host:
 
$ echo "	''node_name'"
$ echo ""
$!
$    CONFIRM_SETUP:
$!
$ inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$ if( answer .eqs. "" ) then goto CONFIRM_SETUP
$ if( .not. answer )
$ then 
$    say
*** The above component will not be installed ***
$!
$    goto EXIT_OK
$ endif
$!
$ say 
 
Generating default security auditing configuration...
$ on error then goto IIGENRES_FAILED
$ iigenres 'node_name ii_system:[ingres.files]secure.rfm
$ set noon
$ iisetres ii.'node_name.config.c2.'release_id "COMPLETE" 
$ goto IIGENRES_DONE
$!
$ IIGENRES_FAILED:
$!
$ say
An error occured while generating the default configuration.

$ goto EXIT_FAIL 
$!
$ IIGENRES_DONE:
$!
$ say

Ingres C2 Security Auditing has been set up for this installation.
$!
$ EXIT_OK:
$ status = SS$_NORMAL
$ goto exit_ok1
$!
$ EXIT_FAIL:
$ status = SS$_ABORT
$exit_ok1:
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$!
$! Restore previous values of shared library definitions
$!
$ set_message_off
$ if "''ii_compatlib_def'".eqs."" 
$ then
$     deassign/job ii_compatlib
$ else
$     define/job/nolog ii_compatlib "''ii_compatlib_def'"
$ endif
$ if "''ii_libqlib_def'".eqs."" 
$ then
$     deassign/job ii_libqlib
$ else
$     define/job/nolog ii_libqlib "''ii_libqlib_def'"
$ endif
$ if "''ii_framelib_def'".eqs."" 
$ then
$     deassign/job ii_framelib
$ else
$     define/job/nolog ii_framelib "''ii_framelib_def'"
$ endif
$ set_message_on
$!
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
	delete/noconfirm ii_system:[ingres.files]config.lck;*
$!
$ set on
$ exit 'status'
