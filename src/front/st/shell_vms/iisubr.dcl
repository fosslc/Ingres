$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 1997, 2006 Ingres Corporation
$!
$!  Name:
$!	iisubr -- set-up script for Ingres Protocol Bridge
$!
$!  Usage:
$!	iisubr
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up Ingres Protocol Bridge.
$!	It assumes that II_SYSTEM has been set in the logical name table 
$!	(SYSTEM or GROUP) which is to be used to configure the installation 
$!	or in the JOB logical table.
$!
$!!  History:
$!!      30-sep-1997 (kinte01)
$!!              Created.
$!!	15-oct-1997 (kinte01)
$!!		Added express install option and removed some extraneous code.
$!!	04-dec-1997 (kinte01)
$!!		Remove requirement that NET must be set up first
$!!	20-apr-98 (mcgem01)
$!!		Product name change to Ingres.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!      10-aug-1998 (kinte01)
$!!	    Remove references to II_AUTH_STRING
$!!      16-sep-1998 (kinte01)
$!!          For spx.port the value of ii_installation was not being
$!!          interpreted causing an iisetres error with insufficient parameters.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!	12-dec-2002 (abbjo03)
$!!	    Fix minor error prior to generating default configuration.
$!!	19-dec-2002 (devjo01)
$!!	    Remove all tcp_ip references.  Make sure tcp_dec in a test
$!!	    installation gets a distinct port.
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
$ iigetsyi	:= @ii_system:[ingres.utility]iigetsyi.com
$ iiingloc	:= $ii_system:[ingres.utility]iiingloc.exe
$ iipmhost      := $ii_system:[ingres.utility]iipmhost.exe
$ iinethost     := $ii_system:[ingres.utility]iinethost.exe
$ iimaninf	:= $ii_system:[ingres.utility]iimaninf.exe
$ iiresutl	:= $ii_system:[ingres.utility]iiresutl.exe
$ iisetres	:= $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:= $ii_system:[ingres.utility]iisulock.exe
$ iivalres	:= $ii_system:[ingres.utility]iivalres.exe
$ ingstart	:= $ii_system:[ingres.utility]ingstart.exe
$ ingstop	:= @ii_system:[ingres.utility]iisdall.com
$!
$! Other variables, macros and temporary files
$!
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") ! Current values
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   !  for shared
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  !   library definitions
$ iisulocked         = 0                             !Do we hold the setup lock?
$ saved_message      = f$environment( "MESSAGE" )    ! save message settings
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL	     = 1
$ SS$_ABORT	     = 44
$ status	     = SS$_NORMAL
$ tmpfile           := ii_system:[ingres]iisubr.tmp      !Temporary file
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
$  say		:= type sys$input
$  echo		:= write sys$output 
$!
$ clustered	= f$getsyi( "CLUSTER_MEMBER" ) !Is this node clustered?
$ user_uic	= f$user() !For RUN command (detached processes)
$ user_name     = f$trnlnm( "II_SUINGRES_USER","LNM$JOB" ) !Installation owner
$ IF "''user_name'".EQS."" -
  THEN user_name= f$getjpi("","USERNAME") !User running this procedure
$!
$! See if user wants express setup
$!
$ echo ""
$ echo "This procedure will set up the Ingres Protocol Bridge."
$ echo "In the course of this process you will be asked many questions"
$ echo "that help configure the installation."
$ echo ""
$ echo "Alternatively, you may choose the EXPRESS option which will result"
$ echo "in as few questions as possible; where feasible, defaults will be chosen."
$ echo ""
$ CONFIRM_EXPRESS:
$ inquire/nopunc answer "Do you wish to choose the EXPRESS option? (y/n) "
$ if answer .EQS. "" then goto CONFIRM_EXPRESS
$ if answer
$ then 
$ 	EXPRESS = 1
$ else
$	EXPRESS = 0
$ endif
$
$!
$ echo "Setting up Ingres Protocol Bridge..."
$!
$! Create ii_system:[ingres.files]config.dat if non-existent. 
$!
$ if( f$search( "ii_system:[ingres.files]config.dat" ) .eqs. "" )
$ then
$    copy nl: ii_system:[ingres.files]config.dat -
	/prot=(s:rwed,o:rwed,g:rwed,w:re)
$ endif
$!
$! Lock the configuration file. 
$!
$ on error then goto EXIT_FAIL
$ iisulock "Ingres Protocol Bridge setup"
$ iisulocked = 1
$ set noon
$!
$! Get the node name
$!
$ define/user sys$output 'tmpfile
$ iipmhost
$ open iipmhostout 'tmpfile
$ read iipmhostout node_name
$ close iipmhostout
$ delete 'tmpfile;*
$ actual_node = node_name
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=bridge -output=sys$scratch:iioutput.tmp
$ set noon
$!
$ open iioutput sys$scratch:iioutput.tmp
$ read iioutput version_string
$ close iioutput
$ delete/noconfirm sys$scratch:iioutput.tmp;*
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
$! Run IIJOBDEF.COM if it exists. 
$!
$ set_message_off
$ if( f$search( "ii_system:[ingres]iijobdef.com" ) .nes. "" )
$ then
$    set_message_on
$    say

Loading existing configuration...
$    ! Save settings thay may be lost
$    !
$    @ii_system:[ingres]iijobdef
$    !
$    ! Restore modified settings used by setup scripts
$    define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$    define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$    define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ endif
$ set_message_on
$!
$! Check if Ingres Protocol Bridge is already set up.
$!
$ set_message_off
$ iigetres ii.'node_name.config.bridge.'release_id bridge_setup_status
$ bridge_setup_status = f$trnlnm( "bridge_setup_status" )
$ deassign "bridge_setup_status"
$ set_message_on
$ if( ( bridge_setup_status .nes. "" ) .and. -
     ( bridge_setup_status .eqs. "COMPLETE" ) )
$ then
$    say 

The latest version of Ingres Protocol Bridge has already been set up to run on
local
$    echo "node ''node_name'."
$    echo ""
$    goto EXIT_OK
$ endif
$ goto CONTINUE
$ EXIT_OK:
$ if f$search("ii_system:[ingres]iijobdef.com;*").nes."" then -
  delete/noconfirm ii_system:[ingres]iijobdef.com;*              
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
$ if f$search( "''tmpfile'" ) then -
	delete/noconfirm 'tmpfile;* 
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
        delete/noconfirm ii_system:[ingres.files]config.lck;*
$!
$ set on
$ exit 'status'
$!
$ CONTINUE:
$   set_message_off
$   ! configuring Ingres Protocol Bridge on a server
$   iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$   ii_installation = f$trnlnm( "ii_installation_value" )
$   deassign "ii_installation_value"
$   if( ii_installation .eqs. "" ) then -
      ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$   set_message_on
$   say 
 
This procedure will set up the following version of Ingres Protocol Bridge:
 
$    echo "	''version_string'"
$    say
 
to run on local node:
 
$    echo "	''node_name'"
$    say 
 
$!
$ CONFIRM_BRIDGE_SETUP:
$!
$    if EXPRESS
$    then
$	answer = "TRUE"
$    else
$	inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CONFIRM_BRIDGE_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$!
$    set_message_on
$    if( bridge_setup_status .nes. "DEFAULTS" )
$    then
$       ! generate default configuration resources
$       say 
 
Generating default configuration...
$       on error then goto IIGENRES_BRIDGE_FAILED
$       iigenres 'node_name ii_system:[ingres.files]bridge.rfm 
$       set noon
$       iisetres ii.'node_name.config.bridge.'release_id "DEFAULTS"
$       goto IIGENRES_BRIDGE_DONE
$!
$       IIGENRES_BRIDGE_FAILED:
$!
$       say 
An error occurred while generating the default configuration.

$       goto EXIT_FAIL
$!
$       IIGENRES_BRIDGE_DONE:
$!
$    else
$       say 
 
Default configuration generated.
$    endif
$!
$!   After determing II_INSTALLATION, establish names of
$!   share libraries and thir logicals.
$!
$    if( ii_installation .nes. "" )
$    then
$       echo ""
$       echo "II_INSTALLATION configured as ''ii_installation'."
$    endif
$!
$!
$! Configure bridge server listen addresses.
$!
$ say 
Configuring bridge server listen addresses...
$ iisetres ii.'node_name.gcb.*.async.port ""
$ iisetres ii.'node_name.gcb.*.iso_oslan.port "OSLAN_''ii_installation'"
$ iisetres ii.'node_name.gcb.*.iso_x25.port "X25_''ii_installation'"
$ iisetres ii.'node_name.gcb.*.sna_lu0.port "(none)"
$ iisetres ii.'node_name.gcb.*.sna_lu62.port "(none)"
$ iisetres ii.'node_name.gcb.*.spx.port "''ii_installation'"
$ !FIXME - This hack fixes bug #65312 by forcing the correct listen
$ !listen addresses into config.dat for system-level installations.
$ if ( ( "''ii_installation'" .eqs. "AA" ) .or. -
       ( "''ii_installation'" .eqs. "aa" ) .or. -
       ( "''ii_installation'" .eqs. ""   ) )
$ then
$       iisetres ii.'node_name.gcb.*.tcp_dec.port "II0"
$       iisetres ii.'node_name.gcb.*.tcp_wol.port "II0"
$       iisetres ii.'node_name.gcb.*.decnet.port "II_GCB_0"
$ else
$       iisetres ii.'node_name.gcb.*.tcp_dec.port "''ii_installation'0"
$       iisetres ii.'node_name.gcb.*.tcp_wol.port "''ii_installation'0"
$       iisetres ii.'node_name.gcb.*.decnet.port "II_GCB''ii_installation'_0"
$ endif
$!
$! Make sure ingstart invokes image installation
$!
$ iisetres ii.'node_name.ingstart.*.begin "@ii_system:[ingres.utility]iishare"
$!
$! Say goodbye
$ say

Ingres Protocol Bridge has been successfully set up for this installation.

Note that you must use the "netutil" program to authorize users for access to
remote INGRES installations.  If you need more information about Ingres
Networking authorization, please refer to the Ingres Net User Guide.

You can now use the "ingstart" command to start your Ingres server.
Refer to the Ingres Installation Guide for more information about
starting and using Ingres.

$ iisetres ii.'node_name.config.bridge.'release_id "COMPLETE"
$!
$ goto EXIT_OK
