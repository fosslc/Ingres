$! -----------------------------------------------------------------------
$!
$!  Copyright (c) 1999, 2006 Ingres Corporation
$!
$!  Name: iisurms.com -- IVP (setup) script for the Ingres RMS Gateway
$!
$!  Usage:
$!	@iisurms
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up the Ingres RMS Gateway.
$!	It assumes that II_SYSTEM has been set in the JOB, GROUP or 
$!	SYSTEM logical name table.
$!
$!!  History:
$!!       04-aug-99 (kinte01)
$!!		created from iisudbms.com
$!!	 17-jan-00 (kinte01)
$!!		remove switch of release.dat file as RMS GW is now 
$!!		distributed with Ingres
$!!      24-mar-2000 (devjo01)
$!!          Make gen of scratch file names safe for Clustered Ingres.
$!!      18-Jun-2001 (horda03)
$!!          Only allow Ingres System Administrator of the VMSINSTAL script
$!!          to invoke the IVP scripts.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!      02-sep-2003 (chash01)
$!!          RMS Gateway configuration logicals are becoming configurable
$!!          parameters in config.dat 
$!!      04-dec-2003 (chash01)
$!!          Correct errors accidently introduced in previous modification. 
$!!          An ENDIF is misplaced.
$!!      12-dec-2003 (chash01)
$!!          remove the parts which copy and purge shared images.  Redo
$!!          prompt for express setup.
$!!      090feb-04 (chash01)
$!!          Enlarge stack_size to 128K, but specifying it in rms.crs
$!!          doesn't work for upgrade.  Use iisetres to set stack_size.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!             statement(s) for CONFIG.DAT.
$!!     15-Aug-2007 (Ralph Loen) SIR 118898
$!!         Added symbol definition of iinethost.
$!------------------------------------------------------------------------	
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
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
$ iishowres	:= $ii_system:[ingres.bin]iishowres.exe
$ iisetres	:= $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:= $ii_system:[ingres.utility]iisulock.exe
$ iisyschk	:= @ii_system:[ingres.utility]iisyschk.com
$ iivalres	:= $ii_system:[ingres.utility]iivalres.exe
$!
$! Other variables, macros and temporary files
$!
$ iisulocked         = 0                               !Do we hold the setup lock?
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") ! Current values
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   !  for shared
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  !   library definitions
$ node_name	     = f$getsyi("NODENAME")
$ tmpfile            = "ii_system:[ingres]iisurms.''node_name'_tmp" !Temp file
$ saved_message      = f$environment( "MESSAGE" )
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL	     = 1
$ SS$_ABORT	     = 44
$ status	     = SS$_NORMAL
$!
$ say		:= type sys$input
$ echo		:= write sys$output 
$!
$ set noverify
$ user_uic	= f$user() !For RUN command (detached processes)
$ user_name	= f$trnlnm( "II_SUINGRES_USER","LNM$JOB" ) !Installation owner
$!
$ IF "''user_name'".EQS.""
$ THEN
$       user_name= f$getjpi("","USERNAME") !User running this procedure
$!
$! Verify not SYSTEM account.
$!
$      if ( (f$process() .eqs. "SYSTEM") .or. (f$user() .eqs. "[SYSTEM]"))
$      then
$         type sys$input

  --------------------------------------------------------------------
 | This setup procedure must not be run from the SYSTEM account.      |
 | Please execute this procedure from the Ingres System Administrator |
 | account.                                                           |
  --------------------------------------------------------------------
$         goto EXIT_FAIL
$      endif
$
$      isa_uic = f$file_attributes( "II_SYSTEM:[000000]INGRES.DIR", "UIC")
$
$      if isa_uic .nes. user_uic
$      then
$         type sys$input

     ---------------------------------------------------------
    | This setup procedure must be run from the Ingres System |
    | Administrator account.                                  |
     ---------------------------------------------------------
$
$        goto EXIT_FAIL
$      endif
$ endif
$! 
$! VMSINSTAL is not necessarily run by the Ingres system owner/administrator.
$! For that reason, save the "IVP" userid for later.
$!
$ ivp_name= f$getjpi("","USERNAME") 
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" ) 
$!
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
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
$ echo ""
$ echo "Setting up the Ingres RMS Gateway..."
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
$ iisulock "Ingres RMS Gateway setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=rms -output='tmpfile
$ set noon
$!
$ set_message_on
$ open tmp 'tmpfile
$ read tmp version_string
$ close tmp
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
$! Check whether RMS Gateway is already set up.
$!
$ iigetres ii.'node_name.config.rms.'release_id rms_setup_status
$ set_message_off
$ rms_setup_status = f$trnlnm( "rms_setup_status" )
$ deassign "rms_setup_status"
$ set_message_on
$ if( ( rms_setup_status .nes. "" ) .and. ( rms_setup_status .eqs. "COMPLETE" ) )
$ then
$    echo ""
$    echo "The ''version_string' version of the Ingres RMS Gateway has "
$    echo "already been set up on local node ''node_name'."
$    echo ""
$    goto EXIT_OK 
$ endif
$!
$! Make sure that INGRES/DBMS is set up.
$!
$ set_message_off
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ deassign "dbms_setup_status"
$ set_message_on
$ if( ( dbms_setup_status .nes. "COMPLETE" ) )
$ then
$    say

The setup procedures for the following version of Ingres DBMS :
$    echo "     ''version_string'"
$    say

must be completed up before the Ingres RMS Gateway can be set up on
this host:

$    echo "     ''node_name'"
$    echo ""
$    goto EXIT_FAIL
$ endif
$ iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$ set_message_off
$ ii_installation = f$trnlnm( "ii_installation_value" )
$ deassign "ii_installation_value"
$!
$! unconditionally deassign all rms logicals
$!
$ if( f$trnlnm("ii_rms_fab_size", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_fab_size
$ if( f$trnlnm("ii_rms_hash_size", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_hash_size
$ if( f$trnlnm("ii_rms_rrl", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_rrl
$ if( f$trnlnm("ii_rms_readonly", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_readonly
$ if( f$trnlnm("ii_rms_buffer_rfa", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_buffer_rfa
$ if( f$trnlnm("ii_rms_close_file", "LNM$PROCESS") .nes. "") then deassign/process ii_rms_close_file
$!
$ if( f$trnlnm("ii_rms_fab_size", "LNM$GROUP") .nes. "") then deassign/group ii_rms_fab_size
$ if( f$trnlnm("ii_rms_hash_size", "LNM$GROUP") .nes. "") then deassign/group ii_rms_hash_size
$ if( f$trnlnm("ii_rms_rrl", "LNM$GROUP") .nes. "") then deassign/group ii_rms_rrl
$ if( f$trnlnm("ii_rms_readonly", "LNM$GROUP") .nes. "") then deassign/group ii_rms_readonly
$ if( f$trnlnm("ii_rms_buffer_rfa", "LNM$GROUP") .nes. "") then deassign/group ii_rms_buffer_rfa
$ if( f$trnlnm("ii_rms_close_file", "LNM$GROUP") .nes. "") then deassign/group ii_rms_close_file
$!
$ if( f$trnlnm("ii_rms_fab_size", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_fab_size
$ if( f$trnlnm("ii_rms_hash_size", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_hash_size
$ if( f$trnlnm("ii_rms_rrl", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_rrl
$ if( f$trnlnm("ii_rms_readonly", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_readonly
$ if( f$trnlnm("ii_rms_buffer_rfa", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_buffer_rfa
$ if( f$trnlnm("ii_rms_close_file", "LNM$SYSTEM") .nes. "") then deassign/system ii_rms_close_file
$!
$! Look up II_RMS_FAB_SIZE, keep its value and remove
$! this ingres logical from config.dat. This process
$! is repeated for a total of 6 rms logicals
$!
$ iigetres ii.*.lnm.ii_rms_fab_size ii_fab_size_value
$ set_message_off
$ rms_fab_size = f$trnlnm( "ii_fab_size_value" )
$ deassign "ii_fab_size_value"
$ iiremres ii.*.lnm.ii_rms_fab_size
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_fab_size .eqs. "")
$ then
$   iigetres ii.'node_name.rms.*.ii_rms_fab_size ii_fab_size_value
$   set_message_off
$   rms_fab_size = f$trnlnm( "ii_fab_size_value" )
$   deassign "ii_fab_size_value"
$   set_message_on
$ endif
$!
$! Look up II_RMS_HASH_SIZE.
$!
$ iigetres ii.*.lnm.ii_rms_hash_size ii_hash_size_value
$ set_message_off
$ rms_hash_size = f$trnlnm( "ii_hash_size_value" )
$ deassign "ii_hash_size_value"
$ iiremres ii.*.lnm.ii_rms_hash_size
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_hash_size .eqs. "")
$ then
$   iigetres ii.'node_name.rms.*.ii_rms_hash_size ii_hash_size_value
$   set_message_off
$   rms_hash_size = f$trnlnm( "ii_hash_size_value" )
$   deassign "ii_hash_size_value"
$   set_message_on
$ endif
$!
$! Look up II_RMS_RRL.
$!
$ iigetres ii.*.lnm.ii_rms_rrl ii_rrl_value
$ set_message_off
$ rms_rrl = f$trnlnm( "ii_rrl_value" )
$ deassign "ii_rrl_value"
$ iiremres ii.*.lnm.ii_rms_rrl
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_rrl .eqs. "")
$ then
$   iigetres ii.'node_name.rms.*.ii_rms_rrl ii_rrl_value
$   set_message_off
$   rms_rrl = f$trnlnm( "ii_rrl_value" )
$   set_message_on
$ endif
$!
$! Look up II_RMS_READONLY.
$!
$ iigetres ii.*.lnm.ii_rms_readonly ii_readonly_value
$ set_message_off
$ rms_readonly = f$trnlnm( "ii_readonly_value" )
$ deassign "ii_readonly_value"
$ iiremres ii.*.lnm.ii_rms_readonly
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_readonly .eqs. "")
$ then
$   iigetres ii.'node_name.rms.*.ii_rms_readonly ii_readonly_value
$   set_message_off
$   rms_readonly = f$trnlnm( "ii_readonly_value" )
$   deassign "ii_readonly_value"
$   set_message_on
$ endif
$!
$! Look up II_RMS_BUFFER_RFA.
$!
$ iigetres ii.*.lnm.ii_rms_buffer_rfa ii_buffer_rfa_value
$ set_message_off
$ rms_buffer_rfa = f$trnlnm( "ii_buffer_rfa_value" )
$ deassign "ii_buffer_rfa_value"
$ iiremres ii.*.lnm.ii_rms_buffer_rfa
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_buffer_rfa .eqs. "")
$ then
$  iigetres ii.'node_name.rms.*.ii_rms_buffer_rfa ii_buffer_rfa_value
$  set_message_off
$  rms_buffer_rfa = f$trnlnm( "ii_buffer_rfa_value" )
$  deassign "ii_buffer_rfa_value"
$  set_message_on
$ endif
$!
$! Look up II_RMS_CLOSE_FILE.
$!
$ iigetres ii.*.lnm.ii_rms_close_file ii_close_file_value
$ set_message_off
$ rms_close_file = f$trnlnm( "ii_close_file_value" )
$ deassign "ii_close_file_value"
$ iiremres ii.*.lnm.ii_rms_close_file
$ set_message_on
$!
$! if value is empty, we need to check config param
$!
$ if (rms_close_file .eqs. "")
$ then
$  iigetres ii.'node_name.rms.*.ii_rms_close_file ii_close_file_value
$  set_message_off
$  rms_close_file = f$trnlnm( "ii_close_file_value" )
$  deassign "ii_close_file_value"
$  set_message_on
$ endif
$!
$! Confirm setup. 
$!
$ say 

This procedure will set up the following release of the Ingres RMS Gateway:

$ echo "	''version_string'"
$ echo ""
$ echo "to run on local host: 
$ echo ""
$ echo "	''node_name'"
$ echo ""
$ echo "This installation will be owned by:"
$ echo ""
$ echo "	''user_name' ''user_uic'" 
$!

$! Generate default RMS Gateway configuration.
$!
$ if( rms_setup_status .nes. "DEFAULTS" )
$ then
$    say

Generating default configuration...

$    on error then goto IIGENRES_FAILED
$    iigenres 'node_name ii_system:[ingres.files]rms.rfm
$    set noon
$    iisetres ii.'node_name.config.rms.'release_id "DEFAULTS"
$!   an upgrade installation needs a larger  
$!   stack size for rms
$    iisetres ii.'node_name.rms.*.stack_size 131072
$    goto IIGENRES_DONE
$!
$ IIGENRES_FAILED:
$!
$    set noon
$    say
An error occured while generating the default configuration.

$    goto EXIT_FAIL 
$ else
$    say

Default configuration generated.

$ endif
$!
$ IIGENRES_DONE:
$!
$ say

 Express setup allows setup to run WITHOUT the user providing values to
 certain parameters of Enterprise Access to RMS.  The user may change 
 certain parameter values in a later time by using CBF.

$!
$ CONFIRM_EXPRESS:
$ inquire/punct exp_setup "Do you wish to choose the EXPRESS option? (y/n) "
$!
$ if exp_setup .EQS. "" then goto CONFIRM_EXPRESS
$ exp_yn = F$EXTRACT(0, 1, exp_setup)
$ exp_yn = F$EDIT(exp_yn, "UPCASE")
$ if (exp_yn .eqs. "N")
$ then
$ RMS_HELP_PROMPT:
$!
$   inquire/nopunc rms_help "Do you want instructions on the RMS gateway configurable parameters that need to be set up? (y/n) "
$   if (rms_help .eqs. "") then goto RMS_HELP_PROMPT
$!
$   if ( rms_fab_size .eqs. "") then rms_fab_size = 16 
$   if (rms_help) 
$   then
$      say 

II_RMS_FAB_SIZE defines the resource to cache RMS file information. Set this 
value to some estimate of the largest number of RMS files that will be open 
concurrently by the gateway.

$   endif
$   inquire ii_rms_fab_size "II_RMS_FAB_SIZE [''rms_fab_size']"
$   if (ii_rms_fab_size .eqs. "") then ii_rms_fab_size = rms_fab_size
$   iisetres ii.'node_name.rms.*.ii_rms_fab_size 'ii_rms_fab_size
$!
$   if ( rms_hash_size .eqs. "") then rms_hash_size = 7
$   if (rms_help) 
$   then
$      say 

II_RMS_HASH_SIZE defines the hash table size used for hashing RMS file 
information blocks.  This value should be about one-half to one-third of the 
II_RMS_FAB_SIZE.

$   endif
$   inquire ii_rms_hash_size "II_RMS_HASH_SIZE [''rms_hash_size']"
$   if (ii_rms_hash_size .eqs. "") then ii_rms_hash_size = rms_hash_size
$   iisetres ii.'node_name.rms.*.ii_rms_hash_size 'ii_rms_hash_size
$!
$   if ( rms_rrl .eqs. "") then rms_rrl = "OFF"
$   rms_rrl = F$EDIT(rms_rrl, "UPCASE")
$   if( rms_rrl .eqs. "YES" ) then rms_rrl = "ON"
$   if( rms_rrl .eqs. "NO"  ) then rms_rrl = "OFF"
$   if (rms_help) 
$   then
$      say 

II_RMS_RRL dictates the RMS gateway server to read a record from a RMS file
regardless if the record is locked by other sessions or applications.

$   endif
$   inquire ii_rms_rrl "II_RMS_RRL [''rms_rrl']"
$   if (ii_rms_rrl .eqs. "") then ii_rms_rrl = rms_rrl
$   iisetres ii.'node_name.rms.*.ii_rms_rrl 'ii_rms_rrl
$!
$   if ( rms_readonly .eqs. "") then rms_readonly = "OFF"
$   rms_readonly = F$EDIT(rms_readonly, "UPCASE")
$   if( rms_readonly .eqs. "YES" ) then rms_readonly = "ON"
$   if( rms_readonly .eqs. "NO"  ) then rms_readonly = "OFF"
$   if (rms_help)
$   then
$     say 

II_RMS_READONLY defines the RMS gateway server to be a read-only server. If 
this is se to ON, all non-read activities are not allowed.

$   endif
$   inquire ii_rms_readonly "II_RMS_READONLY [''rms_readonly']"
$   if (ii_rms_readonly .eqs. "") then ii_rms_readonly = rms_readonly
$   iisetres ii.'node_name.rms.*.ii_rms_readonly 'ii_rms_readonly
$!
$   rms_buffer_rfa = F$EDIT(rms_buffer_rfa, "UPCASE")
$   if( rms_buffer_rfa .eqs. "YES" ) then rms_buffer_rfa = "ON"
$   if( rms_buffer_rfa .eqs. "NO"  ) then rms_buffer_rfa = "OFF"
$!
$   if ( rms_buffer_rfa .eqs. "") 
$   then 
$     if (ii_rms_readonly .eqs. "ON") 
$     then
$       rms_buffer_rfa = "ON"
$     else
$       rms_buffer_rfa = "OFF"
$     endif
$   else
$     rms_buffer_rfa = "OFF"
$   endif
$   if (rms_help) 
$   then
$     say 

II_RMS_BUFFER_RFA informs the server that the Record File Address (RFA) 
associated with an RMS record should be buffered.  This should only be defined 
as "ON" with a positive logical definition of II_RMS_READONLY"

$   endif
$   inquire ii_rms_buffer_rfa "II_RMS_BUFFER_RFA [''rms_buffer_rfa']"
$   if (ii_rms_buffer_rfa .eqs. "") then ii_rms_buffer_rfa = rms_buffer_rfa
$   iisetres ii.'node_name.rms.*.ii_rms_buffer_rfa 'ii_rms_buffer_rfa
$!
$   if ( rms_close_file .eqs. "") then rms_close_file = "OFF"
$   rms_close_file = F$EDIT(rms_close_file, "UPCASE")
$   if( rms_close_file .eqs. "YES" ) then rms_close_file = "ON"
$   if( rms_close_file .eqs. "NO"  ) then rms_close_file = "OFF"
$   if (rms_help)
$   then
$     say 

II_RMS_CLOSE_FILE determines whether a file is closed after the last 
session terminates that was accessing the file.

To support repeating groups and improve performance  all files are opened as 
shared. They are not closed until the server is shutdown, a table registered 
against the open file is removed with no outstanding usage of this file by 
any other registered table, or the server decides internally to close this 
file. 

If this value is set to "ON" then files will be closed after the 
termination of the last session accessing the file.

$   endif
$   inquire ii_rms_close_file "II_RMS_CLOSE_FILE [''rms_close_file']"
$   if (ii_rms_close_file .eqs. "") then ii_rms_close_file = rms_close_file
$   iisetres ii.'node_name.rms.*.ii_rms_close_file 'ii_rms_close_file
$!
$! end of exp_yn .eqs. "N"
$ endif 
$!
$!
$ sub1 = f$environment("ON_SEVERITY")
$ set noon
$ if (sub1.nes."NONE") then set on
$ set_message_on
$!
$! Define shared library logicals and make sure these are
$! not confused with any installed images.
$!
$ define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib'ii_installation.exe
$ define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib'ii_installation.exe
$ define/nolog/job ii_framelib ii_system:[ingres.library]framefelib'ii_installation.exe
$!
$! Prompt for concurrent users. 
$!
$ iigetres ii.'node_name.rms.*.connect_limit default_users 
$ default_users = f$trnlnm( "default_users" )
$ deassign "default_users"
$ echo ""
$!
$ CONCURRENT_USERS_PROMPT:
$ set_message_on
$ inquire/nopunc user_limit "How many concurrent RMS gateway users do you want to support? [''default_users'] "
$ if( user_limit .eqs. "" ) then user_limit = 'default_users
$ on error then goto CONCURRENT_USERS_PROMPT
$ SKIP_INQ_9:
$ iivalres -v ii.'node_name.rms.*.connect_limit 'user_limit 
$ set noon
$ say

Updating configuration...
$ iisetres ii.'node_name.rms.*.connect_limit 'user_limit 
$ default_users = 32
$ on error then goto CONCURRENT_USERS_PROMPT
$ iisyschk
$ set_message_off
$ set noon
$    say
Removing defunct utilities and files...
$    delete/noconfirm ii_system:[ingres]iirmsbuild.com;* 
$    delete/noconfirm ii_system:[ingres]release_gwrms.doc;* 
$    delete/noconfirm ii_system:[ingres]version_gwrms.rel;* 
$    delete/noconfirm ii_system:[ingres.utility]iirmssymdef.com;* 
$    delete/noconfirm ii_system:[ingres.utility]iirunrms.cld;* 
$    delete/noconfirm ii_system:[ingres.utility]iirunrms.exe;* 
$    delete/noconfirm ii_system:[ingres.bin]rms_optimizedb.exe;* 
$    delete/noconfirm ii_system:[ingres.bin]rms_statdump.exe;* 
$    delete/noconfirm ii_system:[ingres.bin]rms_sysmod.exe;* 
$    set_message_on
$!
$    say
Ingres RMS Gateway setup complete.

Refer to the Ingres Installation Guide for information about 
starting and using Ingres.

$    iisetres ii.'node_name.config.rms.'release_id "COMPLETE" 
$ goto EXIT_OK
$ EXIT_OK:
$ status = SS$_NORMAL
$ goto exit_ok1
$ EXIT_FAIL:
$! There is a small window where an abort can leave ii_system undefined
$ if "''ii_system_value'".eqs."''f$trnlnm( "ii_system_value" )'"
$ then
$     if "''f$trnlnm("II_SYSTEM")'".eqs.""
$     then
$         define/job ii_system "''ii_system_value'"
$     endif
$ endif
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
$ if f$search( "''tmpfile';*" .nes."" ) then -
	delete/noconfirm 'tmpfile;* 
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
	delete/noconfirm ii_system:[ingres.files]config.lck;*
$!
$ set on
$ exit 'status'
$!   
