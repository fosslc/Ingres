$!
$!  Copyright (c) 2002, 2009 Ingres Corporation
$!
$!  Name: iisukerberos -- setup script for Ingres Kerberos Driver
$!  Usage:
$!	iisukerberos
$!
$!  Description:
$!	This script should only be called by the Ingres installation
$!	utility.  It sets up the Ingres Kerberos Driver.
$!
$!  Exit Status:
$!	0	setup procedure completed.
$!	1	setup procedure did not complete.
$!
$!! History:
$!!	08-Jan-2002 (loera01)
$!!		Created.
$!!      28-Mar-2002 (loera01)
$!!              Ported to VMS.
$!!      21-Nov-2002 (loera01)
$!!              Use "search" instead of "grep" for purging Kerberos lines
$!!              from config.dat.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!   17-may-2006 (abbjo03)
$!!       Remove double quotes from II_CONFIG process level definition.
$!!   22-Aug-2006 (Fei Ge) Bug 116528
$!!       Removed reference to ca.com.
$!!   02-Jan-2006 (loera01) Bug 117418 
$!!       Added reference to iisulock.
$!!   22-Jun-2007 (upake01)
$!!	  Answering "N"o to "Do you wish to continue this setup procedure? (y/n"
$!!       results in a "%SYSTEM-F-ABORT, abort" message.  Answering "N"o should
$!!       not be treated as an error as user just wishes to not install the
$!!       component.  Instead, display a message "*** The above component will
$!!       not be installed ***" and exit gracefully.
$!!	16-jul-2007 (upake01)
$!!       SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!           statement(s) for CONFIG.DAT.
$!!   03-Aug-2007 (Ralph Loen) SIR 118898
$!!       Use iinethost to obtain the default domain name.
$!!   10-Aug-2007 (Ralph Loen) SIR 118898
$!!      Remove iigenres items, as they are now added from all.crs.
$!!      Prompting for domain is no longer necessary, since the
$!!      iinethost utility should enter the correct host name into
$!!      config.dat.  Iisukerberos now prompts for three main
$!!      configurations.
$!!  20-Aug-2007 (Ralph Loen) SIR 118898
$!!      Removed infinite loop due to invalid conditional for
$!!      evaluation of the client "remove" option.
$!!  19-nov-2007 (Ralph Loen)  Bug 119497
$!!       Prompt for installation of GSS$RTL32.EXE and KRB$RTL32.EXE.
$!!       Fail iisukerberos if KRB$ROOT, GSS$RTL32.EXE or KRB$RTL32.EXE
$!!       are missing.  Install the Kerberos driver.  Rename libgcskrb.exe 
$!!       as libgcskrb'ii_installation.exe.  Rename 
$!!       ii.hostname.gcf.mech.module as gcskrb'ii_installation.
$!!  29-Nov-2007 (Ralph Loen) Bug 119358
$!!      "Server-level" (gcf.mech.user_authentication) has been deprecated.
$!!      Menu option 3 becomes "standard" Kerberos authentication, i.e., 
$!!      Kerberos listed in the general mechanism list and as a remote 
$!!      mechanism for the Name Server.
$!! 31-Jan-2007 (Ralph Loen) Bug 119682
$!!      Fixed grammar in an informational message.
$!! 14-May-2008 (Ralph Loen) Bug 117737
$!!      Added calls to iiinitres to set DBMS-type mechanism values to "none"
$!!      in case of an upgrade.   
$!!	18-mar-2009 (joea)
$!!	    Test for existence of {dbms|star|rms}.rfm before calling initres
$!!	    on them.
$!
$!  PROGRAM = iisukerberos
$!
$!  DEST = utility
$!----------------------------------------------------------------------------

$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ set noon
$ set noverify
$!
$! Define symbols for programs called locally.
$!
$ iigenres      := $II_system:[ingres.utility]iigenres.exe
$ iigetres      := $II_system:[ingres.utility]iigetres.exe
$ iigetsyi      := @ii_system:[ingres.utility]iigetsyi.com
$ iijobdef      := $II_system:[ingres.utility]iijobdef.exe
$ iiingloc      := $II_system:[ingres.utility]iiingloc.exe
$ iiinitres     := $ii_system:[ingres.utility]iiinitres.exe
$ iipmhost      := $II_system:[ingres.utility]iipmhost.exe
$ iinethost     := $II_system:[ingres.utility]iinethost.exe
$ iimaninf      := $II_system:[ingres.utility]iimaninf.exe
$ iiresutl      := $II_system:[ingres.utility]iiresutl.exe
$ iisetres      := $II_system:[ingres.utility]iisetres.exe
$ iivalres      := $II_system:[ingres.utility]iivalres.exe
$ iisulock      := $II_system:[ingres.utility]iisulock.exe
$ iisulocked         = 0                             !Do we hold the setup lock?
$ saved_message      = f$environment( "MESSAGE" )    ! save message settings
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL	     = 1
$ SS$_ABORT	     = 44
$ status	     = SS$_NORMAL
$ node_name	     = f$getsyi("NODENAME")
$ tmpfile            = "ii_system:[ingres]iisukerberos.''node_name'_tmp" !Temp file
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") 
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  
$ ii_interplib_def   = f$trnlnm("ii_interplib","LNM$JOB") 
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
$ say		:= type sys$input
$ echo		:= write sys$output 
$ user_name     = f$trnlnm( "II_SUINGRES_USER","LNM$JOB" ) !Installation owner
$ user_uic	= f$user() !For RUN command (detached processes)
$ iinethost 'tmpfile
$ open iinethostout 'tmpfile
$ read iinethostout full_host_name
$ close iinethostout
$ delete 'tmpfile;*
$!
$ goto START
$!
$! Exit module.
$!
$!
$ EXIT_FAIL:
$ echo ""
$ echo "Installation of the Kerberos security mechanism has failed."
$ echo ""
$ status = SS$_ABORT
$ EXIT_OK:
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
        delete/noconf/nolog ii_system:[ingres.files]config.lck;*
$ exit 'status'
$ START:
$ IF "''user_name'".EQS.""
$ THEN
$    isa_uic = f$file_attributes( "II_SYSTEM:[000000]INGRES.DIR", "UIC")
$    if isa_uic .nes. user_uic
$    then
$       say

     ---------------------------------------------------------
    | This setup procedure must be run from the Ingres System |
    | Administrator account.                                  |
     ---------------------------------------------------------
$
$ goto EXIT_FAIL
$ endif
$ user_name= f$getjpi("","USERNAME") !User running this procedure
$ ENDIF
$ ivp_name= f$getjpi("","USERNAME")
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" )
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
$ if ( "''p1'" .eqs. "-RMPKG" )
$ then
$ copy ii_system:[ingres.files]config.dat ii_system:[ingres.files]config.tmp
$ on error then copy ii_system:[ingres.files]config.tmp -
      ii_system:[ingres.files]config.dat
$ search ii_system:[ingres.files]config.dat "gcf.mech.kerberos"/match=nor, -
       "kerberos_driver"/match=nor, "gss$rtl32.exe"/match=nor, -
       "krb$rtl32.exe"/match=nor, "ii_kerberoslib"/match=nor -
       /output=ii_system:[ingres.files]config.new
$ if (f$search("ii_system:[ingres.files]config.new") .nes. "") 
$ then 
$     copy ii_system:[ingres.files]config.new -
            ii_system:[ingres.files]config.dat
$ iisetres ii.'node_name.gcf.mechanisms ""
$ iisetres ii.'node_name.gcf.remote_mechanism "none"
$ iisetres ii.'node_name.gcn.mechanisms ""
$ iisetres ii.'node_name.gcn.remote_mechanism "none"
$ iisetres ii.'node_name.gcc.*.mechanisms ""
$ else
$     say
    There was a problem rewriting the configuration file 
    II_SYSTEM:[INGRES.FILES]CONFIG.DAT.  Please check that the
    file exists, is readable, and that the logical II_SYSTEM is
    defined.
$ goto EXIT_FAIL
$ endif
$ set noon
$ del/nol ii_system:[ingres.files]config.tmp;*,config.new;*
$ say

  Ingres Kerberos Driver has been removed.
  
$ CONFIRM_REM:
$ if f$file_attributes( "SYS$LIBRARY:GSS$RTL32.EXE", "KNOWN") .eqs. "TRUE" -
  .and. f$file_attributes( "SYS$LIBRARY:KRB$RTL32.EXE", "KNOWN") .eqs. "TRUE"
$ then
$ echo "Do you wish to de-install the libraries SYS$LIBRARY:GSS$RTL32.EXE"
$ inquire/nopunc answer  "and SYS$LIBRARY:KRB$RTL32.EXE at this time? "
$ if( answer .eqs. "" ) then goto CONFIRM_REM
$   if ( answer .eqs. "Y")
$   then
$     set_message_off
$     install remove SYS$LIBRARY:GSS$RTL32.EXE
$     install remove SYS$LIBRARY:KRB$RTL32.EXE
$     echo "SYS$LIBRARY:GSS$RTL32.EXE and SYS$LIBRARY:KRB$RTL32.EXE were "
$     echo "successfully removed."
$     set_message_on
$   endif
$ endif
$ goto EXIT_OK
$ else
$ echo "Setting up the Ingres Kerberos Driver..."
$!
$! Lock the configuration file. 
$!
$ on error then goto EXIT_FAIL
$ iisulock "Ingres Kerberos setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=basic -output='tmpfile
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

$ krb_root = f$trnlnm( "KRB$ROOT" )
$ if (krb_root .eqs. "")
$ then
$ echo ""
$ echo "The Kerberos logical KRB$ROOT is not defined.  Please execute
$ echo "SYS$MANAGER:KRB$LOGICALS.COM and re-run iisukerberos."
$ echo ""
$ goto EXIT_FAIL 
$ endif
$ if( f$getdvi("SYS$LIBRARY:GSS$RTL32.EXE","EXISTS") .eqs. "FALSE")
$ then
$ echo ""
$ echo "The Kerberos library SYS$LIBRARY:GSS$RTL32.EXE does not exist.
$ echo "Please install Kerberos on your local host before running iisukerberos."
$ echo ""
$ goto EXIT_FAIL 
$ endif
$ if( f$getdvi("SYS$LIBRARY:KRB$RTL32.EXE","EXISTS") .eqs. "FALSE")
$ then
$ echo ""
$ echo "The Kerberos library SYS$LIBRARY:KRB$RTL32.EXE does not exist.
$ Please install Kerberos on your local host before running iisukerberos."
$ echo ""
$ goto EXIT_FAIL 
$ endif
$ set_message_off
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ deassign "dbms_setup_status"
$ iigetres ii.'node_name.config.net.'release_id net_setup_status
$ net_setup_status = f$trnlnm( "net_setup_status" )
$ deassign "net_setup_status"
$ set_message_on
$ if ( dbms_setup_status .eqs. "" .and. net_setup_status .eqs. "" ) 
$ then 
$       say 
 
The setup procedure for the following version of the Ingres DBMS or IngresNet:
 
$       echo "  ''version_string'"
$       say
 
must be completed up before the Ingres Kerberos Driver can be set up on this node:
 
$       echo "  ''node_name'"
$       echo ""
$       goto EXIT_FAIL
$ endif
$!
$! The following will determine what type of installation (Server or
$! client) we are dealing with. 
$!
$ iijobdef
$ set_message_off
$ iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$ ii_installation = f$trnlnm( "ii_installation_value" )
$ deassign "ii_installation_value"
$ if( ii_installation .eqs. "" ) then -
      ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$ set_message_on
$ CONFIRM_SERVER_SETUP:
$    say 
 
This procedure will set up the following version of Ingres Kerberos Driver:
 
$    echo "	''version_string'"
$    say
 
to run on local node:
 
$    echo "	''full_host_name'"
$    say 
 
$ inquire/nopunc answer "Do you wish to continue this setup procedure? [y] "
$ if answer .eqs. "Y" .or. answer .eqs. "" then goto CONFIRM_INST
$ if answer .eqs. "N" then goto EXIT_OK
$ if answer .nes. "Y" .and. answer .nes. "N" 
$ then
$ echo "Entry must be 'y' or ' n' "
$ inquire junk "Hit RETURN continue"
$ goto CONFIRM_SERVER_SETUP
$ endif
$ CONFIRM_INST:
$ if f$file_attributes( "SYS$LIBRARY:GSS$RTL32.EXE", "KNOWN") .nes. "TRUE" -
  .and. f$file_attributes( "SYS$LIBRARY:KRB$RTL32.EXE", "KNOWN") .nes. "TRUE"
$ then
$ echo ""
$ echo "Installation of the Kerberos libraries SYS$LIBRARY:GSS$RTL32.EXE and"
$ echo "SYS$LIBRARY:KRB$RTL32.EXE is strongly recommended."
$ echo ""
$ inquire/nopunc answer -
 "Install SYS$LIBRARY:GSS$RTL32.EXE and SYS$LIBRARY:KRB$RTL32.EXE? (y/n) "
$ if answer .nes. "Y" .and. answer .nes. "N" 
$ then
$ echo "Entry must be 'y' or ' n' "
$ inquire junk "Hit RETURN continue"
$ goto CONFIRM_INST
$ endif
$ if ( answer .eqs. "Y")
$ then
$ install add SYS$LIBRARY:GSS$RTL32.EXE/open/header/shared
$ install add SYS$LIBRARY:KRB$RTL32.EXE/open/header/shared
$ echo "SYS$LIBRARY:GSS$RTL32.EXE and SYS$LIBRARY:KRB$RTL32.EXE were successfully installed."
$ else
$ echo ""
$ echo "SYS$LIBRARY:GSS32$RTL32.EXE and SYS$LIBRARY:KRB$RTL32.EXE were not installed."
$ echo "Utilities such as INGSTOP, IIMONITOR or CREATEDB may not work."
$ echo ""
$ endif
$ endif
$ GENRES:
$! 
$! Generate default configuration resources in case iisukerberos was
$! previously called with the "-rmpkg" qualifier.
$!
echo ""
echo "Generating default configuration..."
$ on error then goto GENRES_FAILED	
$ iigenres 'node_name ii_system:[ingres.files]net.rfm 
$ iisetres ii.'node_name.gcn.mechanisms ""
$ iisetres ii.'node_name.gcc.*.mechanisms ""
$ iisetres ii.'node_name.lnm.ii_kerberoslib -
"ii_system:[ingres.library]libgcskrb''ii_installation'.exe"
$ iisetres ii.'node_name.gcf.mech.kerberos.module "gcskrb''ii_installation'"
$! Initialize DBMS server mechanisms to "none" if not already initialized
$! in iigenres.
$ if f$search("II_SYSTEM:[ingres.files]dbms.rfm") .nes. ""
$ then
$	define/user sys$output nl:
$	iiinitres -keep mechanisms ii_system:[ingres.files]dbms.rfm
$ endif
$ if f$search("II_SYSTEM:[ingres.files]star.rfm") .nes. ""
$ then
$	define/user sys$output nl:
$	iiinitres -keep mechanisms ii_system:[ingres.files]star.rfm
$ endif
$ if f$search("II_SYSTEM:[ingres.files]rms.rfm") .nes. ""
$ then
$	define/user sys$output nl:
$	iiinitres -keep mechanisms ii_system:[ingres.files]rms.rfm
$ endif
$ @ii_system:[ingres.utility]iishare delete
$ if ii_installation .nes. ""
$ then
$   copy II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB.EXE -
      II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB'ii_installation.exe-   
       /prot=(s:rwed,o:rwed,g:rwed,w:re)
$ endif
$ purge/nol II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB'ii_installation.exe
$ @ii_system:[ingres.utility]iishare add
$ goto GENRES_OK
$ GENRES_FAILED:
$ say
An error occurred while generating the default configuration.


$ goto EXIT_FAIL
$ GENRES_OK:
$ set noon
$ set_message_off
$ if f$file_attributes(-
 "II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB''ii_installation'.exe",-
 "KNOWN") .eqs. "TRUE"
$ then
$ install remove II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB'ii_installation.exe
$ endif
$ install add II_SYSTEM:[INGRES.LIBRARY]LIBGCSKRB'ii_installation.exe -
     /open/header/shared
$ set_message_on
$ say

Default configuration generated.

$ MAINDISP:
$ say
Basic Kerberos Configuration Options

1.  Client Kerberos configuration
2.  Name Server Kerberos configuration
3.  Standard Kerberos authentication (both 1 and 2)
0.  Exit

$ status = SS$_NORMAL
$ inquire/nopunc answer "Select from [0 - 3] above [0]: "
$ if answer .eqs. "" .or. answer .eqs. "0" 
$ then 
$ say 

Kerberos configuration complete.  You may use the cbf utility to fine-tune 
Kerberos security options.

$ goto EXIT_OK
$ endif
$ if answer .eqs. "1" then gosub CLIENT_CONFIG
$ if answer .eqs. "2" then gosub NS_CONFIG
$ if answer .eqs. "3" then gosub SERVER_CONFIG
$ goto MAINDISP
$ CLIENT_CONFIG:
$!
$ say
Client configuration enables this installation to use Kerberos to authenticate
against a remote installation that has been configured to use Kerberos for
authentication.  This is the mimimum level of Kerberos authentication.

$ PROMPT1:
$ echo "Select 'a' to add client-level Kerberos authentication,"
$ inquire/nopunc resp "Select 'r' to remove client-level Kerberos authentication: "
$ if resp .eqs. "A" 
$ then 
$   iisetres ii.'node_name.gcf.mechanisms "kerberos"
$ say

Client Kerberos configuration has been added.

You must add the "authentication_mechanism" attribute in netutil for
each remote node you wish to authenticate using Kerberos.  The
"authentication_mechanism" attribute must be set to "kerberos".  There
is no need to define users or passwords for vnodes using Kerberos
authentication.

$ endif
$ if resp .eqs. "R" 
$ then 
$   iisetres ii.'node_name.gcf.mechanisms ""
$   echo ""
$   echo "Client Kerberos authentication has been removed."
$   echo ""
$ endif
$ if resp .nes. "A" .and. resp .nes. "R" 
$ then
$ echo "Entry must be 'a' or 'r' "
$ inquire junk "Hit RETURN continue"
$ goto PROMPT1
$ endif
$ inquire junk "Hit RETURN continue"
$ return
$ NS_CONFIG:
$ say

Name Server Kerberos authentication allows the local Name Server to
authenticate using Kerberos.

$ PROMPT2:
$ echo "Select 'a' to add Name Server Kerberos authentication,"
$ inquire/nopunc resp "Select 'r' to remove Name Server Kerberos authentication: "
$ if resp .eqs. "A" 
$ then 
$   iisetres ii.'node_name.gcn.mechanisms "kerberos"
$   iisetres ii.'node_name.gcn.remote_mechanism "kerberos"
$   echo ""
$   echo "Kerberos authentication has been added to the Name Server on ''full_host_name'"
$   echo ""
$ endif
$ if resp .eqs. "R" 
$ then 
$   iisetres ii.'node_name.gcn.mechanisms ""
$   iisetres ii.'node_name.gcn.remote_mechanism "none"
$   echo ""
$   echo "Kerberos authentication has been removed from the Name Server on ''full_host_name'"
$   echo ""
$ endif
$ if resp .nes. "A" .and. resp .nes. "R" 
$ then
$ echo "Entry must be 'a' or 'r' "
$ inquire junk "Hit RETURN continue"
$ goto PROMPT2
$ endif
$ inquire junk "Hit RETURN continue"
$ return
$ SERVER_CONFIG:
$ say

Standard Kerberos authentication specifies Kerberos as the remote
authentication mechanism for the Name Server, and allows clients to specify
Kerberos for remote servers that authenticate with Kerberos.

$ PROMPT3:
$ echo "Select 'a' to add standard Kerberos authentication,"
$ inquire/nopunc resp "Select 'r' to remove standard Kerberos authentication: "
$ if resp .eqs. "A" 
$ then 
$   iisetres ii.'node_name.gcf.mechanisms "kerberos"
$   iisetres ii.'node_name.gcn.mechanisms "kerberos"
$   iisetres ii.'node_name.gcn.remote_mechanism "kerberos"
$ say

Standard Kerberos authentication has been added.

You must add the "authentication_mechanism" attribute in netutil for
each remote node you wish to authenticate using Kerberos.  The
"authentication_mechanism" attribute must be set to "kerberos".  There
is no need to define users or passwords for vnodes using Kerberos
authentication.

$ endif
$ if resp .eqs. "R" 
$ then 
$   iisetres ii.'node_name.gcf.mechanisms ""
$   iisetres ii.'node_name.gcn.mechanisms ""
$   iisetres ii.'node_name.gcn.remote_mechanism "none"
$   echo ""
$   echo "Standard Kerberos authentication has been removed."
$   echo ""
$ endif
$ if resp .nes. "A" .and. resp .nes. "R" 
$ then
$ echo "Entry must be 'a' or 'r' "
$ inquire junk "Hit RETURN continue"
$ goto PROMPT3
$ endif
$ inquire junk "Hit RETURN continue"
$ return
