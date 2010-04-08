$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 1995, 2008 Ingres Corporation
$!
$!
$!  Name:
$!	iisunet -- set-up script for Ingres Networking
$!
$!  Usage:
$!	iisunet
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up Ingres Networking.
$!	It assumes that II_SYSTEM has been set in the logical name table 
$!	(SYSTEM or GROUP) which is to be used to configure the 
$!	installation or in the JOB logical table.
$!
$!!  History:
$!!	03-feb-1995 (rudtr01)
$!!		Fixed bug 65312 by forcing the correct listen addresses
$!!		into config.dat for system-level installations.  The  
$!!		real fix should be made to GCC so it will use AA for 
$!!		system-level installations instead of II.
$!!	14-apr-1995 (wolf) 
$!!		Clean up some prompts and typos.
$!!	06-jun-1995 (wolf) 
$!!		Use consistent references to manuals.
$!!	16-jun-1995 (wolf) 
$!!		Change "dec_net" to "decnet" to match net.crs entries and
$!!		get CBF to display information about this protocol. (62842)
$!!		Create config.dat with w:re file protections.
$!!	22-jun-95 (albany, for wolf)
$!!		Fix typo.
$!!	22-mar-96 (duursma)
$!!		Added Express Installation features.
$!!	18-dec-1996 (angusm)
$!!		Remove irrelevant check on 'license_readable' (bug 76224)
$!!	25-feb-1997 (boama01)
$!!		B80847: reinstated "irrelevant" check for 'license_readable',
$!!		now that cichkcap has been enhanced for "-d logical_name".
$!!      15-dec-1997 (loera01) Bugs 78907 and 78908:
$!!              For client-only installations, set ingres and installation-
$!!              owner privileges.
$!!      02-feb-1998 (loera01)
$!!              Fixed problem with previous version, and added definition
$!!              for II_GCNxx_LCL_VNODE.  Also, changed bogus definition of
$!!              tcp_ip to tcp_dec, and, for system-level installations,
$!!              set port to II instead of II0 on tcp_dec and tcp_wol.
$!!              Also, added definition of ii_charset.
$!!	23-feb-1998 (kinte01)
$!!		Reapply change 422591 that was lost
$!!    11-jan-96 (loera01)
$!!            Add API shared library.
$!!	20-apr-98 (mcgem01)
$!!		Product name change to Ingres.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!	07-jun-1998 (kinte01)
$!!	    Deinstall shared libraries if this is an upgrade. Ensure that
$!!	    the new shared libraries are renamed to include the 2 letter
$!!	    installation code to ensure there will not be any problems with
$!!	    version mismatch errors if this is an upgrade. (91921)
$!!      10-aug-1998 (kinte01)
$!!	    Remove references to II_AUTH_STRING
$!!      24-mar-2000 (devjo01)
$!!          Make gen of scratch file names safe for Clustered Ingres.
$!!	14-jun-2001 (kinte01)
$!!	    For client only installations, the prompt for the character
$!!	    set had been spread across 2 lines with no continuation 
$!!	    character specified resulting in %DCL-W-IVVERB errors (104998)
$!!      18-Jun-2001 (horda03)
$!!          Only allow Ingres System Administrator of the VMSINSTAL script
$!!          to invoke the IVP scripts.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!	05-Nov-2001 (kinte01)
$!!	    If ii_installation is not defined in config.dat but is set
$!!	    in the local environment, it should then be added to config.dat
$!!	09-dec-2002 (abbjo03)
$!!	    Correct 5-nov-01 change to not pick up II_INSTALLATION from logical.
$!!	10-jan-2003 (abbjo03)
$!!	    Remove incomplete message about EA products.
$!!	06-aug-2003 (abbjo03)
$!!	    Correct iisetres from setup.net to config.net.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	08-mar-2005 (abbjo03)
$!!	    iisdall.com has been renamed as ingstop.
$!!	18-jul-2005 (abbjo03)
$!!	    Change default DECnet port id's without the 0 or _0.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!	05-Jan-2007 (bonro01)
$!!	    Change NET Users Guide to Connectivity Guide.
$!!	22-jun-2007 (upake01)
$!!	    Answering "N"o to "Do you wish to continue this setup procedure? (y/n"
$!!         results in a "%SYSTEM-F-ABORT, abort" message.  Answering "N"o should
$!!         not be treated as an error as user just wishes to not install the
$!!         component.  Instead, display a message "*** The above component will
$!!         not be installed ***" and exit gracefully.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!             statement(s) for CONFIG.DAT.
$!!     26-oct-1007 (Ralph Loen) Bug 119353
$!!         Add a single quote in front of the ii_installation symbol when
$!!         iisetres writes ii.'node_name.lnm.ii_installation.  Otherwise,
$!!         the symbol will not be expanded and string "ii_installation" will
$!!         be written.
$!!     15-Aug-2007 (Ralph Loen) SIR 118898
$!!         Added symbol definition of iinethost.
$!!     08-jan-2008 (Ralph Loen) Bug 119667
$!!         Added missing definitions of II_APILIB and II_INTERPLIB.  Corrected
$!!         another incorrect reference to ii.'node_name.lnm.ii_installation.
$!!         Added default entry for ii.'node_name.gcn.remote_vnode; otherwise,
$!!         entry of "" displays an error from iisetres.  Remove defunct
$!!         definition of ii.'node_name.gcn.rmt_vnode.
$!!    11-feb-2008 (Ralph Loen) Bug 116097 
$!!         Added iicvtgcn utility.  Iicvtgcn converts the node and login
$!!         files from standard RACC to optimized RACC format, if the
$!!         iisunet is done as an upgrade from Ingres 2.0.  Otherwise,
$!!         iicvtgcn exits without converting the files.
$!!	15-apr-2008 (upake01) Bug 120275
$!!         Move the block that defines "vnode" logical few lines down
$!!         after the installation code is prompted for.  
$!!         For charset, if the installation id is defined, set it accordingly.
$!!     16-Jul-2009 (horda03) Bug 122285
$!!         gcn_lcl_vnode is defined on SYSTEM level installations.
$!!         ii_gcn_lcl_vnode should be defined.
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
$ iijobdef	:= $ii_system:[ingres.utility]iijobdef.exe
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
$ iicvtgcn      := $ii_system:[ingres.utility]iicvtgcn.exe
$!
$! Other variables, macros and temporary files
$!
$! Get the current values for shared library definitions.
$!
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB")
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")  
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB") 
$ ii_interplib_def    = f$trnlnm("ii_interplib","LNM$JOB") 
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
$ ivp_name= f$getjpi("","USERNAME")
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" )
$!
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
$!
$! See if user wants express setup
$!
$ echo ""
$ echo "This procedure will set up the Ingres DBMS for networking."
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
$ echo "Setting up Ingres Networking..."
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
$ iisulock "Ingres Networking setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=net -output='tmpfile
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
$ !
$ ! Restore modified settings used by setup scripts
$ define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ set_message_on
$!
$! Check if Ingres Networking is already set up.
$!
$ set_message_off
$ iigetres ii.'node_name.config.net.'release_id net_setup_status
$ net_setup_status = f$trnlnm( "net_setup_status" )
$ deassign "net_setup_status"
$ set_message_on
$ if( ( net_setup_status .nes. "" ) .and. -
     ( net_setup_status .eqs. "COMPLETE" ) )
$ then
$    say 

The latest version of Ingres Networking has already been set up to run on
local
$    echo "node ''node_name'."
$    echo ""
$    goto EXIT_OK
$ endif
$!
$! The following will determine what type of installation (Server or
$! client) we are dealing with. 
$!
$ set_message_off
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ deassign "dbms_setup_status"
$ set_message_on
$ if( dbms_setup_status .nes. "" ) 
$ then 
$    ! configuring Ingres Networking on a server
$    if( dbms_setup_status .nes. "COMPLETE" )
$    then
$       say 
 
The setup procedure for the following version of the Ingres DBMS:
 
$       echo "	''version_string'"
$       say
 
must be completed up before Ingres Networking can be set up on this node:
 
$       echo "	''node_name'"
$       echo ""
$       goto EXIT_FAIL
$    endif
$    set_message_off
$    iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$    ii_installation = f$trnlnm( "ii_installation_value" )
$    deassign "ii_installation_value"
$    if( ii_installation .eqs. "" ) 
$    then
$        ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$	 if (ii_installation .nes. "")
$	 then
$          iisetres ii.'node_name.lnm.ii_installation 'ii_installation
$	 endif
$    endif
$    set_message_on
$    say 
 
This procedure will set up the following version of Ingres Networking:
 
$    echo "	''version_string'"
$    say
 
to run on local node:
 
$    echo "	''node_name'"
$    say 
 
The Ingres DBMS has been set up on this node; therefore, this installation
has the capacity to act as an Ingres "server", which means it can service
queries on local Ingres databases.  An Ingres server can also service
remote queries if Ingres Networking is installed. 

If you do not need to access to this Ingres server from other nodes,
then you do not need to set up Ingres Networking. 

$!
$ CONFIRM_SERVER_SETUP:
$!
$    if EXPRESS
$    then
$	answer = "TRUE"
$    else
$	inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CONFIRM_SERVER_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$!
$    if( net_setup_status .nes. "DEFAULTS" )
$    then
$       ! generate default configuration resources
$       say 
 
Generating default configuration...
$       on error then goto IIGENRES_SERVER_FAILED
$       iigenres 'node_name ii_system:[ingres.files]net.rfm 
$       iicvtgcn
$       set noon
$       iisetres ii.'node_name.config.net.'release_id "DEFAULTS"
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
$!   After determining II_INSTALLATION, establish names of
$!   shared libraries and their logicals.
$!
$    if( ii_installation .nes. "" )
$    then
$       echo ""
$       echo "II_INSTALLATION configured as ''ii_installation'."
$!
$       copy ii_system:[ingres.library]clfelib.exe -
                 ii_system:[ingres.library]clfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_compatlib "ii_system:[ingres.library]clfelib''ii_installation'.exe"
$       copy ii_system:[ingres.library]framefelib.exe -
                 ii_system:[ingres.library]framefelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_framelib "ii_system:[ingres.library]framefelib''ii_installation'.exe" 
$       copy ii_system:[ingres.library]interpfelib.exe -
                 ii_system:[ingres.library]interpfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_interplib "ii_system:[ingres.library]interpfelib''ii_installation'.exe" 
$       copy ii_system:[ingres.library]libqfelib.exe -
                 ii_system:[ingres.library]libqfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_libqlib "ii_system:[ingres.library]libqfelib''ii_installation'.exe" 
$       copy ii_system:[ingres.library]apifelib.exe -
	         ii_system:[ingres.library]apifelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_apilib "ii_system:[ingres.library]apifelib''ii_installation'.exe"
$       define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib'ii_installation.exe
$       define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib'ii_installation.exe
$       define/nolog/job ii_framelib ii_system:[ingres.library]framefelib'ii_installation.exe
$       define/nolog/job ii_interplib ii_system:[ingres.library]interpfelib'ii_installation.exe
$    else
$       ii_installation = "AA"
$!
$       define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib.exe
$       define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib.exe
$       define/nolog/job ii_framelib ii_system:[ingres.library]framefelib.exe
$       define/nolog/job ii_interplib ii_system:[ingres.library]interpfelib.exe
$    endif
$    @ii_system:[ingres.utility]iishare add
$ else
$    ! client setup 
$    say 

This procedure will set up the following version of Ingres Networking:
 
$    echo "	''version_string'"
$    say

to run on local node:
 
$    echo "	''node_name'"
$    say 

This procedure sets up an Ingres "client" installation which lets you
access remote Ingres "servers" from this node.

$!
$    CONFIRM_CLIENT_SETUP:
$!
$    if EXPRESS
$    then
$	answer = "TRUE"
$    else
$	inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CONFIRM_CLIENT_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$!
$    say 

This procedure assumes that the INGRES server you wish to access
from this installation has already been set up on a remote node.  To
complete this procedure, you must know the name of the remote node
on which the server you wish to access is installed. 

After you have set up Ingres Networking for this installation, you can 
establish connections to remote INGRES DBMS servers of compatible releases.
To do so, you must set up Ingres Networking on the server and create
the necessary authorizations.

$    if EXPRESS
$    then
$	answer = "TRUE"
$    else
$	inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CONFIRM_CLIENT_SETUP
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$!
$    set_message_on
$    !
$    !
$    !
$    if( net_setup_status .nes. "DEFAULTS" ) 
$    then
$       ! generate default configuration resources
$       say 
 
Generating default configuration...
$       on error then goto IIGENRES_CLIENT_FAILED
$       iigenres 'node_name ii_system:[ingres.files]net.rfm 
$       iicvtgcn
$ set_message_on
$!
$! Give SYSTEM and installation owner all Ingres privileges 
$!
$ iisetres ii.'node_name.privileges.user.'ing_user -
     "SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$ iisetres ii.'node_name.privileges.user.system -
     "SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$ if ("''ing_user'" .nes. "''ivp_user'") then -
        iisetres ii.'node_name.privileges.user.'ivp_user - 
        "SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$!
$       set noon
$       iisetres ii.'node_name.config.net.'release_id "DEFAULTS"
$       goto IIGENRES_CLIENT_DONE
$!
$       IIGENRES_CLIENT_FAILED:
$!
$       say 
An error occurred while generating the default configuration.

$       goto EXIT_FAIL
$!
$       IIGENRES_CLIENT_DONE:
$!
$    else
$       say 
 
Default configuration generated.
$    endif
$!
$    CONFIRM_LNM_TABLE:
$!
$    set_message_off
$    if( f$trnlnm( "II_SYSTEM", "LNM$JOB" ) .nes. "" )
$    then
$       job_ii_system = "TRUE" 
$    else
$       job_ii_system = "FALSE" 
$    endif
$    if( job_ii_system .or. f$trnlnm( "II_SYSTEM", "LNM$GROUP" ) .eqs. "" )
$    then
$       if( job_ii_system .or. f$trnlnm( "II_SYSTEM", "LNM$SYSTEM" ) .eqs. "" )
$       then
$          say 

You have the option of setting up this installation using GROUP
or SYSTEM logical names to store configuration data.

You may only use SYSTEM logicals if there is no existing INGRES
installation on this machine uses SYSTEM logicals.  If there
is an existing installation on this machine which uses SYSTEM
logicals, then this installation must be set up using GROUP
logicals.

Unlike SYSTEM logicals, GROUP logicals can be used to set up multiple
INGRES installations on the same machine.  Before you set up this
installation using GROUP logicals however, be sure that no existing
INGRES installation on this machine is set up to use SYSTEM logicals
under an account which has the same GROUP UIC as the account being
used for this installation.  Therefore, it is permissible to set up
this installation using GROUP logicals if there is a SYSTEM level
installation set up under an account with a different GROUP UIC.

$          !
$           CONFIRM_SETUP_3:
$          !
$          if EXPRESS
$          then
$	      answer = "TRUE"
$          else
$	      inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$          endif
$          if( answer .eqs. "" ) then goto CONFIRM_SETUP_3
$          if( .not. answer )
$          then 
$             say
*** The above component will not be installed ***
$!
$             goto EXIT_OK
$          endif
$!
$          !
$           CONFIRM_LNM_TABLE:
$          !
$          echo ""
$          inquire/nopunc answer "Do you want to set up this installation using GROUP logicals? (y/n) "
$          if( answer .eqs. "" ) then goto CONFIRM_LNM_TABLE
$          if( answer )
$          then
$             say

You have chosen to set up this installation using GROUP logicals.

If there is another INGRES installation on this machine which is set up
under an account with the same GROUP UIC as this account, do not continue
this setup procedure.

$             lnm_table = "LNM$GROUP"
$          else
$             say

You have chosen to set up this installation using SYSTEM logicals.

If there is another INGRES installation on this system which uses SYSTEM
level logical names, you must not continue this setup procedure. 

$             lnm_table = "LNM$SYSTEM"
$          endif
$          !
$           CONFIRM_SETUP_4:
$          !
$          inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$          if( answer .eqs. "" ) then goto CONFIRM_SETUP_4
$          if( answer )
$          then
$             iisetres ii.'node_name.lnm.table.id "''lnm_table'"
$          else
$             say
*** The above component will not be installed ***
$             goto EXIT_OK
$          endif
$       else
$          set_message_on
$          say 

Since II_SYSTEM is defined in the SYSTEM logical name table, this setup
procedure will configure Ingres using SYSTEM level logicals.

If there is another INGRES installation on this system which uses SYSTEM
level logical names, do not continue this setup procedure. 

$          !
$           CONFIRM_SYSTEM_SETUP:
$          !
$          inquire/nopunc answer "Do you wish to continue this setup procedure? (y/n) "
$          if( answer .eqs. "" ) then goto CONFIRM_SYSTEM_SETUP
$          if( answer )
$          then
$             iisetres ii.'node_name.lnm.table.id "LNM$SYSTEM"
$          else
$             say
*** The above component will not be installed ***
$             goto EXIT_OK
$          endif
$          lnm_table = "LNM$SYSTEM" 
$       endif
$    else
$       say 

Since II_SYSTEM is defined in the GROUP logical name table, this setup
procedure will configure Ingres using GROUP level logicals.

If there is an INGRES installation on this machine which uses SYSTEM
level logicals, you must set up this installation using an account
with a different GROUP UIC than the account used to install the
SYSTEM level installation.

$       !
$        CONFIRM_GROUP_SETUP:
$       !
$       inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$       if( answer .eqs. "" ) then goto CONFIRM_GROUP_SETUP
$       if( answer )
$       then
$          iisetres ii.'node_name.lnm.table.id "LNM$GROUP"
$       else
$          say
*** The above component will not be installed ***
$          goto EXIT_OK
$       endif 
$       lnm_table = "LNM$GROUP"
$    endif
$    !
$     LNM_TABLE_CONFIRMED:
$    !
$    ! Define II_SYSTEM in the correct logical table.
$    !
$    set_message_off
$    ii_system_value = f$trnlnm( "ii_system" )
$    deassign /proc ii_system
$    deassign /proc/exec ii_system
$    deassign /job ii_system
$    deassign /job/exec ii_system
$    define/exec/table='lnm_table/trans=concealed ii_system "''ii_system_value'"
$    deassign "ii_system_value"
$    set_message_on
$    !
$    ! Configure II_INSTALLATION if required.
$    !
$    set_message_off
$    iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$    ii_installation = f$trnlnm( "ii_installation_value" )
$    deassign "ii_installation_value"
$    if( ii_installation .eqs. "" ) 
$    then 
$       ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$	if (ii_installation .nes. "")
$	then
$          iisetres ii.'node_name.lnm.ii_installation 'ii_installation
$	endif
$    endif
$    set_message_on
$    if( ( lnm_table .eqs. "LNM$GROUP" ) .and. ( ii_installation .eqs. "" ) )
$    then
$       say 

To differentiate between multiple INGRES installations set up to run on
the same machine using GROUP level logical names, you must assign each
installation a two-letter code that is unique across the machine.

The first character of the installation code must be a letter; the
second character may be a letter or a number.  You may use any code which
satisfies these conditions except for the following:

"AA" may not be used since it is the installation code used for SYSTEM
level installations. 

"TT" may not be used since it translates to the VMS terminal identifier.

The installation code you select will be assigned to the GROUP level
logical II_INSTALLATION when you start INGRES. Before you choose an
installation code, make certain that it is not already in use by another
(GROUP level) installation on this machine.

$!
$       II_INSTALLATION_PROMPT:
$!
$       inquire ii_installation "Please enter a valid installation code" 
$       if( f$length( ii_installation ) .ne. 2 ) then -
           goto II_INSTALLATION_PROMPT
$       char1 = f$extract( 0, 1, ii_installation )
$       char2 = f$extract( 1, 1, ii_installation )
$       if( ( "''char1'" .lts. "A" ) .or. ( "''char1'" .gts. "Z" ) -
           .or. ( ( ( "''char2'" .lts. "A" ) .or. ( "''char2'" .gts. "Z" ) ) -
           .and. ( ( "''char2'" .lts. "0" ) .or. ( "''char2'" .gts. "9" ) ) ) -
           .or. ( ii_installation .eqs "AA" ) -
           .or. ( ii_installation .eqs. "TT" ) )
$       then
$          say

You have entered an invalid installation code.

$          goto II_INSTALLATION_PROMPT
$       endif
$       iisetres ii.'node_name.lnm.ii_installation "''ii_installation'"
$	@ii_system:[ingres.utility]iishare delete
$!
$!  Create unique shared libraries for this installation
$!
$       copy ii_system:[ingres.library]clfelib.exe -
           ii_system:[ingres.library]clfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_compatlib "ii_system:[ingres.library]clfelib''ii_installation'.exe" 
$       purge/nolog ii_system:[ingres.library]clfelib'ii_installation.exe
$!
$       copy ii_system:[ingres.library]framefelib.exe -
             ii_system:[ingres.library]framefelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_framelib "ii_system:[ingres.library]framefelib''ii_installation'.exe" 
$       purge/nolog ii_system:[ingres.library]framefelib'ii_installation.exe
$!
$       copy ii_system:[ingres.library]interpfelib.exe -
             ii_system:[ingres.library]interpfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_interplib "ii_system:[ingres.library]interpfelib''ii_installation'.exe" 
$       purge/nolog ii_system:[ingres.library]interpfelib'ii_installation.exe
$!
$       copy ii_system:[ingres.library]libqfelib.exe -
             ii_system:[ingres.library]libqfelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_libqlib "ii_system:[ingres.library]libqfelib''ii_installation'.exe" 
$       purge/nolog ii_system:[ingres.library]libqfelib'ii_installation.exe
$       copy ii_system:[ingres.library]apifelib.exe -
              ii_system:[ingres.library]apifelib'ii_installation.exe
$       iisetres ii.'node_name.lnm.ii_apilib "ii_system:[ingres.library]apifelib''ii_installation'.exe"
$       purge/nolog ii_system:[ingres.library]apifelib'ii_installation.exe
$       !
$       ! Define unique shared library logicals for this installation.
$       !
$       define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib'ii_installation.exe
$       define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib'ii_installation.exe
$       define/nolog/job ii_framelib ii_system:[ingres.library]framefelib'ii_installation.exe
$       define/nolog/job ii_interplib ii_system:[ingres.library]interpfelib'ii_installation.exe
$       define/nolog/job ii_apilib ii_system:[ingres.library]apifelib'ii_installation.exe
$       !
$    else
$       if (ii_installation.eqs."") 
$	then 
$		ii_installation = "AA"
$       	echo "" 
$       	echo "II_INSTALLATION configured as ""''ii_installation'"""
$       	!
$       	! Define shared library logicals for a system installation.
$       	!
$       	define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib.exe
$       	define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib.exe
$       	define/nolog/job ii_framelib ii_system:[ingres.library]framefelib.exe
$       	define/nolog/job ii_interplib ii_system:[ingres.library]interpfelib.exe
$       	define/nolog/job ii_apilib ii_system:[ingres.library]apifelib.exe
$	else
$       	echo "" 
$       	echo "II_INSTALLATION configured as ""''ii_installation'"""
$       	copy ii_system:[ingres.library]clfelib.exe -
        	   ii_system:[ingres.library]clfelib'ii_installation.exe
$       	iisetres ii.'node_name.lnm.ii_compatlib "ii_system:[ingres.library]clfelib''ii_installation'.exe" 
$       	purge/nolog ii_system:[ingres.library]clfelib'ii_installation.exe
$!	
$       	copy ii_system:[ingres.library]framefelib.exe -
        	     ii_system:[ingres.library]framefelib'ii_installation.exe
$       	iisetres ii.'node_name.lnm.ii_framelib "ii_system:[ingres.library]framefelib''ii_installation'.exe" 
$       	purge/nolog ii_system:[ingres.library]framefelib'ii_installation.exe
$!
$		copy ii_system:[ingres.library]interpfelib.exe -
 	            ii_system:[ingres.library]interpfelib'ii_installation.exe
$		iisetres ii.'node_name.lnm.ii_interplib "ii_system:[ingres.library]interpfelib''ii_installation'.exe" 
$		purge/nolog ii_system:[ingres.library]interpfelib'ii_installation.exe
$!
$		copy ii_system:[ingres.library]libqfelib.exe -
 	            ii_system:[ingres.library]libqfelib'ii_installation.exe
$		iisetres ii.'node_name.lnm.ii_libqlib "ii_system:[ingres.library]libqfelib''ii_installation'.exe" 
$		purge/nolog ii_system:[ingres.library]libqfelib'ii_installation.exe
$		copy ii_system:[ingres.library]apifelib.exe -
        	      ii_system:[ingres.library]apifelib'ii_installation.exe
$		iisetres ii.'node_name.lnm.ii_apilib "ii_system:[ingres.library]apifelib''ii_installation'.exe"
$		purge/nolog ii_system:[ingres.library]apifelib'ii_installation.exe
$	       !
$	       ! Define unique shared library logicals for this installation.
$	       !
$	       define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib'ii_installation.exe
$	       define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib'ii_installation.exe
$	       define/nolog/job ii_framelib ii_system:[ingres.library]framefelib'ii_installation.exe
$	       define/nolog/job ii_interplib ii_system:[ingres.library]interpfelib'ii_installation.exe
$	       define/nolog/job ii_apilib ii_system:[ingres.library]apifelib'ii_installation.exe
$              !
$       endif
$	@ii_system:[ingres.utility]iishare add
$    endif 
$!
$!
$! II_GCNxx_LCL_VNODE must be defined at startup time, insert appropriate
$! entry.
$ if ("''ii_installation'".nes."")
$ then
$     temp_str = "ii_gcn" + ii_installation + "_lcl_vnode"
$     iisetres ii.'node_name.lnm.'temp_str 'node_name
$ else
$     iisetres ii.'node_name.lnm.ii_gcn_lcl_vnode 'node_name
$ endif
$!
$! Configure II_CHARSETxx
$!
$ iigetres ii.'node_name.lnm.ii_charset'ii_installation ii_charset_value
$ set_message_off
$ ii_charset = f$trnlnm( "ii_charset_value" )
$ deassign "ii_charset_value"
$ if( ii_charset .eqs. "" ) then -
     ii_charset = f$trnlnm( "II_CHARSET''ii_installation'", "LNM$JOB" )
$ set_message_on
$ if( ii_charset .eqs. "" )
$ then
$    if EXPRESS then goto CHARSET_PROMPT
$    say

Ingres supports different character sets.  You must now enter the
character set you want to use with this Ingres installation.

IMPORTANT NOTE: You will be unable to change character sets once you
make your selection.  If you are unsure of which character set to use,
exit this program and refer to the Ingres Installation Guide.

$    iivalres -v ii.*.setup.ii_charset BOGUS_CHARSET
$!
$ CHARSET_PROMPT:
$!
$    set noon
$    iigetres ii.'node_name.setup.ii_charset default_charset
$    default_charset = f$trnlnm( "default_charset" )
$    deassign "default_charset"
$    if EXPRESS
$    then
$       ii_charset = default_charset
$       goto SKIP_INQ_D
$    endif
$    inquire/nopunc ii_charset "Please enter a valid character set [''default_charset'] "
$    if( ii_charset .eqs. "" ) then -
        ii_charset = default_charset
$    on error then goto CHARSET_PROMPT
$ SKIP_INQ_D:
$    iivalres -v ii.*.setup.ii_charset 'ii_charset
$    set noon
$    iigetres ii.*.setup.charset.'ii_charset charset_text
$    charset_text = f$trnlnm( "charset_text" )
$    deassign "charset_text"
$    if EXPRESS then goto II_CHARSET_OK
$    say

The character set you have selected is:

$    echo "     ''ii_charset' (''charset_text')"
$    echo ""
$    !
$     CONFIRM_CHARSET:
$    !
$    if EXPRESS then goto II_CHARSET_OK
$    inquire/nopunc answer "Is this the character set you want to use? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_CHARSET
$    if( .not. answer )
$    then
$       iivalres -v ii.*.setup.ii_charset BOGUS_CHARSET
$       goto CHARSET_PROMPT
$    endif
$ else
$    echo ""
$    echo "II_CHARSET''ii_installation' configured as ''ii_charset'."
$ endif
$ II_CHARSET_OK:
$ if ("''ii_installation'".nes."")
$ then
$    iisetres ii.'node_name.lnm.ii_charset'ii_installation "''ii_charset'"
$ else
$    iisetres ii.'node_name.lnm.ii_charset "''ii_charset'"
$ endif
$    !
$    ! Configure II_TIMEZONE_NAME
$    !
$    iigetres ii.'node_name.lnm.ii_timezone_name ii_timezone_name_value
$    set_message_off
$    ii_timezone_name = f$trnlnm( "ii_timezone_name_value" )
$    deassign "ii_timezone_name_value"
$    if( ii_timezone_name .eqs. "" ) then -
        ii_timezone_name = f$trnlnm( "ii_timezone_name", "''lnm_table'" )
$    set_message_on
$    if( ii_timezone_name .eqs. "" )
$    then
$       say

You must now specify the time zone this installation is located in.
To specify a time zone, you must first select the region of the world
this installation is located in.  You will then be prompted with a
list of the time zones in that region.
$       !
$        REGION_PROMPT:
$       !
$       set noon
$       iivalres -v ii.*.setup.region BOGUS_REGION -
           ii_system:[ingres.files]net.rfm
$       !
$        REGION_PROMPT_2:
$       !
$       set noon
$       inquire/nopunc region "Please enter one of the named regions: "
$       if( region .eqs. "" ) then goto REGION_PROMPT_2
$       on error then goto REGION_PROMPT_2
$       iivalres -v ii.*.setup.region 'region ii_system:[ingres.files]net.rfm
$       set noon
$       say

If you have selected the wrong region, press RETURN.
$       set noon
$       iivalres -v ii.*.setup.'region.tz BOGUS_TIMEZONE -
           ii_system:[ingres.files]net.rfm
$       !
$        TIMEZONE_PROMPT:
$       !
$       set noon
$       inquire/nopunc ii_timezone_name "Please enter a valid time zone: " 
$       if( ii_timezone_name .eqs. "" )
$       then 
$          say

Returning to region prompt...
$          goto REGION_PROMPT
$       endif
$       on error then goto TIMEZONE_PROMPT
$       iivalres -v ii.*.setup.'region.tz 'ii_timezone_name -
           ii_system:[ingres.files]net.rfm
$       set noon
$       iigetres ii.*.setup.'region.'ii_timezone_name tz_text
$       tz_text = f$trnlnm( "tz_text" )
$       deassign "tz_text"
$       say

The time zone you have selected is:

$       echo "	''ii_timezone_name' (''tz_text')"
$       say

If this is not the correct time zone, you will be given the opportunity to
select another region.

$       !
$        CONFIRM_TIMEZONE:
$       !
$	if EXPRESS 
$	then
$	   answer = "TRUE"
$	else
$          inquire/nopunc answer "Is this time zone correct? (y/n) "
$	endif
$       if( answer .eqs. "" ) then goto CONFIRM_TIMEZONE
$       if( .not. answer )
$       then
$          say

Please select another time zone region.
$          goto REGION_PROMPT
$       endif
$    else
$       echo "" 
$       echo "II_TIMEZONE_NAME configured as ''ii_timezone_name'."
$    endif
$    iisetres ii.'node_name.lnm.ii_timezone_name "''ii_timezone_name'"
$    !
$    REMOTE_SERVER_PROMPT:
$    !
$    echo ""
$    echo "Enter the vnode name of the INGRES server node"
$    inquire server_node "you wish to access [''node_name'_SERVER]"
$    !
$    ! ### PING REMOTE HOST IF POSSIBLE
$    !
$    CONFIRM_REMOTE_SERVER:
$    !
$    inquire/nopunc answer "Have you entered the correct node name? (y/n) " 
$    if( answer .eqs. "" ) then goto CONFIRM_REMOTE_SERVER
$    if answer
$    then
$       goto REMOTE_SERVER_PROMPT_DONE
$    endif
$    goto REMOTE_SERVER_PROMPT
$    !
$    REMOTE_SERVER_PROMPT_DONE:
$    !
$    if server_node .eqs."" then server_node = "''node_name'_server"
$    iisetres ii.'node_name.gcn.remote_vnode 'server_node 
$ endif
$!
$! Configure Net server listen addresses.
$!
$ say 

Configuring Net server listen addresses...
$ iisetres ii.'node_name.gcc.*.async.port "" 
$ iisetres ii.'node_name.gcc.*.iso_oslan.port "OSLAN_''ii_installation'"
$ iisetres ii.'node_name.gcc.*.iso_x25.port "X25_''ii_installation'"
$ iisetres ii.'node_name.gcc.*.sna_lu0.port "(none)" 
$ iisetres ii.'node_name.gcc.*.sna_lu62.port "(none)" 
$ iisetres ii.'node_name.gcc.*.spx.port 'ii_installation
$ !FIXME - This hack fixes bug #65312 by forcing the correct listen
$ !listen addresses into config.dat for system-level installations.
$ if ( ( "''ii_installation'" .eqs. "AA" ) .or. -
       ( "''ii_installation'" .eqs. "aa" ) .or. -
       ( "''ii_installation'" .eqs. ""   ) )
$ then
$	iisetres ii.'node_name.gcc.*.tcp_dec.port "II"
$	iisetres ii.'node_name.gcc.*.tcp_wol.port "II"
$	iisetres ii.'node_name.gcc.*.decnet.port "II_GCC"
$ else
$	iisetres ii.'node_name.gcc.*.tcp_dec.port 'ii_installation
$	iisetres ii.'node_name.gcc.*.tcp_wol.port 'ii_installation
$	iisetres ii.'node_name.gcc.*.decnet.port "II_GCC''ii_installation'"
$ endif
$!
$! Make sure ingstart invokes image installation
$!
$ iisetres ii.'node_name.ingstart.*.begin "@ii_system:[ingres.utility]iishare"
$!
$! ### 
$! ### FINISH ME 
$! ###
$ goto SKIP_UNFINISHED_CODE
$!
$! Do installation password setup.
$!
$ if( dbms_setup_status .nes. "" ) 
$ then 
$    ! allow user to create installation password on the server 
$    say

Users of other Ingres installations must be authorized to connect to
this Ingres DBMS installation.  For a user to be authorized, correct
authorization information must be set up locally and on remote
clients.  You can authorize remote users for access to this server
using installation passwords (in addition to Ingres Networking user
passwords supported in past releases).

Installation passwords offer the following advantages over user passwords:

+ Remote users do not need login accounts on the server host.

+ Installation passwords are independent of host login passwords.

+ Installation passwords are not transmitted over the network in any
  form, thus providing greater security than user passwords.

+ User identity is always preserved.

If you need more information about Ingres Networking authorization,
please refer to the Ingres Connectivity Guide.

$    inquire/nopunc junk "Press RETURN to continue:" 
$    say

You now have the option of creating an installation password for this
server.  This password will only be valid for the TCP/IP protocol.  If
you require remote authorization for other protocols, or if you choose
not to set up a TCP/IP installation password on this server, then you
must use the "netutil" program (as described in the Ingres Connectivity
Guide) to authorize remote users for access to this server.

Be aware that simply creating an installation password on this server does
not automatically authorize remote users for access.  To authorize a remote
user, a correct pasword must be entered into the Ingres Networking
authorization profile for that user on the client.  You can authorize
remote users in either of the following ways:

o Use "netutil" as described in the Ingres Connectivity Guide.

o Select the installation password option during the Ingres Networking
  setup procedure on the client.

$!
$ CREATE_PASSWORD_PROMPT:
$!
$    if EXPRESS
$    then
$	answer = "FALSE"
$    else
$       inquire/nopunc answer " Do you wish to create an installation password for this server? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CREATE_PASSWORD_PROMPT
$    if( answer )
$    then
$       say

Starting the Ingres name server for password creation...
$       on error then goto EXIT_FAIL
$       ingstart /iigcn
$       set noon
$!      ####
$!      #### FINISH ME
$!      ####
$    endif
$ else
$    ! allow user to enter authorization on client 
$!   ####
$!   #### FINISH ME
$!   ####
$ endif
$! ### 
$! ### FINISH ME 
$! ###
$ SKIP_UNFINISHED_CODE:
$!
$! Say goodbye
$!
$ say

Ingres Networking has been successfully set up for this installation.

Note that you must use the "netutil" program to authorize users for access to
remote INGRES installations.  If you need more information about Ingres
Networking authorization, please refer to the Ingres Connectivity Guide.

You can now use the "ingstart" command to start your Ingres server.  If
you have not already done so, please execute the command procedure
"@II_SYSTEM:[INGRES]INGSYSDEF.COM" before you execute ingstart.

Refer to the Ingres Installation Guide for more information about 
starting and using Ingres.

$ iisetres ii.'node_name.config.net.'release_id "COMPLETE"
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
$ set 
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
$ if "''ii_interplib_def'".eqs."" 
$ then
$     deassign/job ii_interplib
$ else
$     define/job/nolog ii_interplib "''ii_interplib_def'"
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
