$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 2001, 2006 Ingres Corporation
$!
$!  Name:
$!	iisujdbc -- set-up script for Ingres/EDBC JDBC Server
$!
$!  Usage:
$!	iisujdbc
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up Ingres/EDBC JDBC Server
$!	It assumes that II_SYSTEM  has been set in the logical name table 
$!	(SYSTEM or GROUP) which is to be used to configure the installation 
$!	or in the JOB logical table.
$!
$!!  History:
$!!	06-mar-2001 (kinte01)
$!!		Created
$!!	05-jul-2001 (kinte01)
$!!		Only output the JDBC listen protocol & port for a first
$!!		time installation.  Don't output this message if this
$!!		version has already been set up on this node.
$!!	27-jul-2001 (kinte01)
$!!		Only allow Ingres System Administrator of the VMSINSTAL script
$!!		to invoke the IVP scripts.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!	07-aug-2003 (abbjo03)
$!!	    Make good on the comment that we're setting up temporary definitions
$!!	    of the shared library logicals.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	08-mar-2005 (abbjo03)
$!!	    iisdall.com has been renamed as ingstop.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!	22-jun-2007 (upake01)
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
$ iiingloc      := $ii_system:[ingres.utility]iiingloc.exe
$ iipmhost      := $ii_system:[ingres.utility]iipmhost.exe
$ iinethost     := $ii_system:[ingres.utility]iinethost.exe
$ iimaninf	:= $ii_system:[ingres.utility]iimaninf.exe
$ iiresutl	:= $ii_system:[ingres.utility]iiresutl.exe
$ iisetres	:= $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:= $ii_system:[ingres.utility]iisulock.exe
$ iivalres	:= $ii_system:[ingres.utility]iivalres.exe
$ ingstart	:= $ii_system:[ingres.utility]ingstart.exe
$ ingstop	:= @ii_system:[ingres.utility]ingstop.com
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
$ node_name	     = f$getsyi("NODENAME")
$ tmpfile            = "ii_system:[ingres]iisunet.''node_name'_tmp" !Temp file
$!
$! Make temporary definitions for shared library logicals to 
$! be the unconcealed path of the library directory within ii_system
$! 
$ define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ if f$search("ii_system:[ingres]iistartup.del") .nes. "" then @ii_system:[ingres]iistartup.del
$ set_message_off
$ if( f$search( "ii_system:[ingres.files]config.dat" ) .nes. "" )
$ then
$    set_message_on
$    @ii_system:[ingres.utility]iishare delete
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
$ ivp_name= f$getjpi("","USERNAME")
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" )
$!
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
$!
$ echo "Setting up Ingres JDBC Server..."
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
$ iisulock "Ingres JDBC setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=jdbc -output='tmpfile
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
$! Run IIJOBDEF.COM if it exists. 
$!
$!
$! Check if Ingres JDBC server is already set up.
$!
$ set_message_off
$ iigetres ii.'node_name.config.jdbc.'release_id jdbc_setup_status
$ jdbc_setup_status = f$trnlnm( "jdbc_setup_status" )
$ deassign "jdbc_setup_status"
$ set_message_on
$ if( ( jdbc_setup_status .nes. "" ) .and. -
     ( jdbc_setup_status .eqs. "COMPLETE" ) )
$ then
$    say 

The latest version of Ingres JDBC Server has already been set up to run on
local
$    echo "node ''node_name'."
$    echo ""
$    status = SS$_NORMAL
$    goto exit_ok1
$ endif
$!
$ set_message_off
$    iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$    ii_installation = f$trnlnm( "ii_installation_value" )
$    deassign "ii_installation_value"
$    if( ii_installation .eqs. "" ) then -
        ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$ set_message_on
$    say 
 
This procedure will set up the following version of Ingres JDBC Server:
 
$    echo "	''version_string'"
$    say
 
to run on local node:
 
$    echo "	''node_name'"
$    say 
 

$!
$ CONFIRM_SERVER_SETUP:
$!
$    inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_SERVER_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       status = SS$_NORMAL
$       goto exit_ok1
$    endif
$    if( jdbc_setup_status .nes. "DEFAULTS" )
$    then
$       ! generate default configuration resources
$       say 
 
Generating default configuration...
$       on error then goto IIGENRES_SERVER_FAILED
$       iigenres 'node_name ii_system:[ingres.files]jdbc.rfm 
$       set noon
$       iisetres ii.'node_name.config.jdbc.'release_id "DEFAULTS"
$       goto IIGENRES_SERVER_DONE
$!
$       IIGENRES_SERVER_FAILED:
$!
$       say 
An error occurred while generating the default configuration.

$       goto EXIT_FAIL
$!
$       IIGENRES_SERVER_DONE:
$!
$    else
$       say 
 
Default configuration generated.
$    endif
$!
$ if ( ( "''ii_installation'" .eqs. "AA" ) .or. -
       ( "''ii_installation'" .eqs. "aa" ) .or. -
       ( "''ii_installation'" .eqs. ""   ) )
$ then
$    iisetres ii.'node_name.jdbc.*.port "II7"
$ else
$    iisetres ii.'node_name.jdbc.*.port "''ii_installation'7"
$ endif
$    iigetres ii.'node_name.jdbc.*.port ii_port_value
$    ii_port = f$trnlnm( "ii_port_value" )
$    deassign "ii_port_value"
$    iigetres ii.'node_name.jdbc.*.protocol ii_protocol_value
$    ii_protocol = f$trnlnm( "ii_protocol_value" )
$    deassign "ii_protocol_value"
$ set_message_on
$ iisetres ii.'node_name.config.jdbc.'release_id "COMPLETE"
$ EXIT_OK:
$ say

JDBC Server Listen Protocol is 
$ echo 	"	''ii_protocol'"
$ say

JDBC Server Listen Port is  
$ echo	"	''ii_port'"
$ say
JDBC Server has been successfully set up in this installation.
$ status = SS$_NORMAL
$ goto exit_ok1
$!
$ EXIT_FAIL:
$ say
JDBC Server setup failed.
$ status = SS$_ABORT
$exit_ok1:
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ if iisulocked .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then - 
        delete/noconfirm ii_system:[ingres.files]config.lck;*
$ set on
$ exit 'status'
