$!----------------------------------------------------------------------------
$!
$!  Copyright (c) 1994, 2006 Ingres Corporation
$!
$!  Name:
$!	iisuabf -- set-up script for Ingres Application-By-Forms
$!
$!  Usage:
$!	iisuabf
$!
$!  Description:
$!      This script is called by VMSINSTAL to set up Ingres 
$!	Application-By-Forms.  It assumes that II_SYSTEM has been set 
$!      in the logical name table (SYSTEM or GROUP) which is to be used 
$!	to configure the installation or in the JOB logical table.
$!
$!!  History:
$!!	17-feb-94 (joplin)
$!!		Created
$!!	14-apr-95 (dougb)
$!!		Add II_CONFIG definition to place config.lck correctly.
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
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!             statement(s) for CONFIG.DAT.
$!!     14-Jan-2010 (horda03) Bug 123153
$!!         Upgrade all nodes in a clustered environment.
$!----------------------------------------------------------------------------
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$ set noon
$ set noverify
$!
$! Define symbols for programs called locally.
$!
$ iiingloc      :== $ii_system:[ingres.utility]iiingloc.exe
$ iipmhost      :== $ii_system:[ingres.utility]iipmhost.exe
$ iigetres      :== $ii_system:[ingres.utility]iigetres.exe
$ iimaninf	:== $ii_system:[ingres.utility]iimaninf.exe
$ iisetres      :== $ii_system:[ingres.utility]iisetres.exe
$ iisulock      :== $ii_system:[ingres.utility]iisulock.exe
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
$ node_name          = f$edit(f$getsyi("NODENAME"), "lowercase")
$ tmpfile            = "ii_system:[ingres]iisuabf.''node_name'_tmp"
$
$ script_id = "IISUABF"
$ @II_SYSTEM:[ingres.utility]iishlib _iishlib_init "''script_id'"
$!
$  say          := type sys$input
$  echo         := write sys$output 
$!
$ user_uic      = f$user() !For RUN command (detached processes)
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
$
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
$ echo "Setting up Ingres Application-By_Forms..."
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
$ iisulock "Ingres Application-By-Forms setup"
$ iisulocked = 1
$ set noon
$ on error then goto EXIT_FAIL
$
$! Check if this is a cluster Ingres member.
$!
$ get_nodes 'node_name
$ if iishlib_err .ne. ii_status_ok then goto EXIT_FAIL
$ nodes = iishlib_rv1
$ cluster_mode = iishlib_rv2
$!
$! Get ABF version string
$!
$ iimaninf -version=abf -output='tmpfile
$ set noon
$!
$ open iioutput 'tmpfile
$ read iioutput version_string
$ close iioutput
$ delete 'tmpfile;*
$!
$ say 
 
This procedure will set up the following version of Ingres
Application-By-Forms:
 
$ echo "	''version_string'"
$
$ if nodes .nes. node_name
$ then
$    say
 
to run on the following cluster hosts:
 
$    display_nodes "''nodes'"
$ else
$    say
 
to run on local host:
 
$    echo f$fao("	!AS.", f$edit(node_name, "upcase"))
$ endif
$ say 
 
Generating default configuration...

$!
$! Create the ABF directory if necessary
$! and set the directories appropriate protection.
$!
$ if f$search("ii_system:[ingres]abf.dir").eqs."" -
  then create/directory ii_system:[ingres.abf]
$ set file/protection=(s:rwed,o:rwed,g:rwe,w:rwe)/owner='user_uic -
  ii_system:[ingres]abf.dir
$
$!
$! Create the ING_ABFDIR logical name
$!
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ing_abfdir ""II_SYSTEM:[INGRES.ABF]"""
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
$
$ cleanup "''script_id'"
$
$ set_message_on
$!
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
	delete/noconfirm ii_system:[ingres.files]config.lck;*
$!
$ set on
$ exit 'status'
