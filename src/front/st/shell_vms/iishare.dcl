$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iishare.com -- Install shared images script for Ingres
$!
$!  Usage:
$!	@iishare  [add | delete]
$!
$!      The default action is to "add" installed images.
$!
$!  Description:
$!	This script is called to install or remove installed images.
$!
$!!  History:
$!!	6-Jul-95 (duursma)
$!!	    Added priviliges prmmbx, syslck, exquota to dmfjsp; these are
$!!	    required to run ckpdb on AXP OpenVMS 6.1.   (Bug 69780)
$!!	17-Jul-95 (wagner)
$!!	    Moved dmfjsp fromt the production_list to the dbms_list and added 
$!!	    the installation code
$!!	23-feb-98 (kinte01)
$!!	    Reapply change 422591 that was lost
$!!          11-Jan-96 (loera01)
$!!             Add API shared library.
$!!	24-feb-98 (kinte01)
$!!	    Install shared libraries on Alpha as resident to gain a
$!!	    performance improvement.
$!!	20-apr-98 (mcgem01)
$!!	    Product name change to Ingres.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!	06-Oct-1998 (kinte01)
$!!	    Add CA license startup
$!!	03-feb-1999 (kinte01)
$!!	    Don't install ii_useradt as resident as it is not always 
$!!	    linked so that in can be installed in this manner (95232).
$!!      13-may-1999 (chash01)
$!!          add oracle gateway installation
$!!      19-may-1999 (chash01)
$!!          make iigwora group share image without '-'
$!!	21-may-1999 (kinte01)
$!!	    change the package used to check the version to basic instead
$!!	    of dbms. All package versions will be the same. For packaging
$!!	    the gateways by themselves, the modified release.dat will not
$!!	    contain the dbms package causing iishare to fail on the version
$!!	    check. Chose basic as a replacement as it must be included in
$!!	    both the standard release.dat file and the modified gateway
$!!	    version. This change is only necessary for 2.0 as with 2.5 
$!!	    the gateway will be provided as part of the standard release
$!!	24-may-1999 (kinte01)
$!!	    correct dcl -if-then-else syntax added for oracle gw from 
$!!	    change 441784
$!!	13-sep-1999 (kinte01)
$!!	    only install the CA licensing shared image once and not
$!!	    twice (100173).
$!!      24-mar-2000 (devjo01)
$!!	    Make gen of scratch file names safe for Clustered Ingres.
$!!      19-feb-2002 (chash01)
$!!          Add Rdb gateway image installation at startup time.
$!!      13-jun-2002 (chash01)
$!!          Issue 11941795, bug 108028.  VDBA gets SQL-F-NOMSG  when an
$!!          error message is expected.  Change install so that no privilege
$!!          is listed.
$!!      16-oct-2003 (horda03) Bugs 105390 and 109531
$!!          GBLSECTS are being lost when the shared libraries are installed
$!!          with the "/resident" flag. @IISHARE DELETE cannot release the
$!!          the GBLSECTS created due to the /resident flag. (105390)
$!!          Also, using /resident, means that the II_SYSTEM disk cannot be
$!!          dismounted, following an INGSTART. (109531)
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	08-jul-2005 (abbjo03)
$!!	    Remove invocation of calic_startup.com.
$!!     24-oct-2007 (Ralph Loen) Bug 119353
$!!         Added installation of II_ODBCLIB, II_ODBCROLIB, and
$!!         II_ODBC_CLILIB.
$!!     19-nov-2007 (Ralph Loen) Bug 119497
$!!         Added installation of II_KERBEROSLIB.
$!------------------------------------------------------------------------	
$!
$ ii_installation = f$trnlnm("II_INSTALLATION")
$!
$! Be careful when modifying these lists of installed images.
$! The lists consist of a series of space-separated images and install
$! parameters.  Watch out for the space at the end of each line!
$!
$! Images installed for a client installation
$!
$ if f$getsyi("hw_model") .lt. 1024 
$ then  client_list = -
    "ii_system:[ingres.library]iiuseradt''ii_installation'.exe    /open/header/share " + -
    "ii_system:[ingres.library]framefelib''ii_installation'.exe   /open/header/share " + -
    "ii_system:[ingres.library]libqfelib''ii_installation'.exe    /open/header/share " + -
    "ii_system:[ingres.library]clfelib''ii_installation'.exe      /open/header/share " + -
    "ii_system:[ingres.library]interpfelib''ii_installation'.exe  /open/header/share " + -
    "ii_system:[ingres.library]apifelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcfelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcrofelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcclifelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]libgcskrb''ii_installation'.exe     /open/header/share " + -
    " "
$ else 
$   client_list = -
    "ii_system:[ingres.library]iiuseradt''ii_installation'.exe    /open/header/share " + -
    "ii_system:[ingres.library]framefelib''ii_installation'.exe   /open/header/share " + -
    "ii_system:[ingres.library]libqfelib''ii_installation'.exe    /open/header/share " + -
    "ii_system:[ingres.library]clfelib''ii_installation'.exe      /open/header/share " + -
    "ii_system:[ingres.library]interpfelib''ii_installation'.exe  /open/header/share " + -
    "ii_system:[ingres.library]apifelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcfelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcrofelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]odbcclifelib''ii_installation'.exe     /open/header/share " + -
    "ii_system:[ingres.library]libgcskrb''ii_installation'.exe     /open/header/share " + -
    " "
$ endif
$!
$! Images installed for a DBMS installation 
$!
$ dbms_list =  -
  "ii_system:[ingres.bin]upgradedb.exe  /priv=(sysprv,cmkrnl) " + -
  "ii_system:[ingres.bin]createdb.exe   /priv=(sysprv,cmkrnl) " 
$ dbms_list =  dbms_list + -
  "ii_system:[ingres.bin]iimonitor.exe  /priv=(prmmbx,sysprv,cmkrnl,world,readall,syslck,share) " + -
  "ii_system:[ingres.bin]ingcntrl.exe   /priv=(sysprv) " + -
  "ii_system:[ingres.bin]destroydb.exe  /priv=(sysprv,cmkrnl) " + -
  "ii_system:[ingres.bin]upgradefe.exe  /priv=(sysprv) " + -
  "ii_system:[ingres.bin]dmfjsp''ii_installation'.exe      /priv=(sysprv,cmkrnl,phy_io,prmmbx,syslck,exquota) " + -
  " "
$!
$! Images installed for production only installations
$!
$ production_list = " "
$!
$! Images installed for STAR
$!
$ star_list = -
 "ii_system:[ingres.bin]starview.exe    /priv=(sysprv) " + -
 " "
$!
$! Images installed for Oracle gateway
$!
$ if ( ii_installation .eqs. "" ) 
$ then
$   gwora_list = -
      "ii_system:[ingres.bin]iigwora.exe    /priv=(sysprv) " + " "
$ else
$   gwora_list = -
      "ii_system:[ingres.bin]iigwora''ii_installation'.exe  /priv=(sysprv) " -
      + " "
$ endif
 
$!
$! Images installed for Rdb gateway
$!
$ if ( ii_installation .eqs. "" ) 
$ then
$   gwrdb_list = -
      "ii_system:[ingres.bin]iigwrdb.exe /open/header/shared " + " "
$ else
$   gwrdb_list = -
      "ii_system:[ingres.bin]iigwrdb''ii_installation'.exe /open/header/shared" -
      + " "
$ endif
 
$!----------------------------------------------------------------------
$ operation = "ADD"
$ if f$edit(p1,"TRIM,UPCASE") .eqs. "DELETE" then operation = "DELETE"
$!  
$!Get the currenv values for share library defs.
$!
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB")
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")  
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB") 
$ ii_interplib_def    = f$trnlnm("ii_framelib","LNM$JOB") 
$ ii_apilib_def    = f$trnlnm("ii_apilib","LNM$JOB") 
$ ii_odbclib_def    = f$trnlnm("ii_odbclib","LNM$JOB") 
$ ii_odbcrolib_def    = f$trnlnm("ii_odbcrolib","LNM$JOB") 
$ ii_odbc_clilib_def    = f$trnlnm("ii_odbc_clilib","LNM$JOB") 
$ ii_kerberoslib_def    = f$trnlnm("ii_kerberoslib","LNM$JOB") 
$!
$! Make temporary definitions for shared library logicals if necessary.
$!
$ if ( ii_compatlib_def .eqs. "" ) then -
   define/nolog/job ii_compatlib -
   "ii_system:[ingres.library]clfelib''ii_installation'.exe"
$ if ( ii_libqlib_def .eqs. "" ) then -
  define/nolog/job ii_libqlib -
  "ii_system:[ingres.library]libqfelib''ii_installation'.exe"
$ if ( ii_framelib_def .eqs. "") then -
  define/nolog/job ii_framelib -
  "ii_system:[ingres.library]framefelib''ii_installation.exe"
$ if ( ii_interplib_def .eqs. "") then -
  define/nolog/job ii_interplib -
  "ii_system:[ingres.library]interpfelib''ii_installation.exe"
$ if ( ii_apilib_def .eqs. "") then -
  define/nolog/job ii_apilib -
  "ii_system:[ingres.library]apifelib''ii_installation.exe"
$ if ( ii_odbclib_def .eqs. "") then -
  define/nolog/job ii_odbclib -
  "ii_system:[ingres.library]odbcfelib''ii_installation.exe"
$ if ( ii_odbcrolib_def .eqs. "") then -
  define/nolog/job ii_odbcrolib -
  "ii_system:[ingres.library]odbcrofelib''ii_installation.exe"
$ if ( ii_odbc_clilib_def .eqs. "") then -
  define/nolog/job ii_odbc_clilib -
  "ii_system:[ingres.library]odbcclifelib''ii_installation.exe"
$ if ( ii_kerberoslib_def .eqs. "") then -
  define/nolog/job ii_kerberoslib -
  "ii_system:[ingres.library]libgcskrb''ii_installation.exe"
$!
$! Define symbols for programs called locally.
$!
$ iipmhost      := $ii_system:[ingres.utility]iipmhost.exe
$ iigetres      := $ii_system:[ingres.utility]iigetres.exe
$ iimaninf      := $ii_system:[ingres.utility]iimaninf.exe
$ iisetres      := $ii_system:[ingres.utility]iisetres.exe
$ install       := $install/command_mode
$!
$! Other variables, macros and temporary files
$!
$ node_name = f$getsyi("NODENAME")
$ tmpfile = "ii_system:[ingres]iishare.''node_name'_tmp"  !Temporary file
$ saved_message   = f$environment( "MESSAGE" )
$ set_message_off = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on  = "set message''saved_message'"
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=basic -output='tmpfile
$ set noon
$!
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
$! Check if DBMS is set up.
$!
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ if (dbms_setup_status .nes. "") then deassign "dbms_setup_status"
$!
$! Check if Net is set up.
$!
$ iigetres ii.'node_name.config.net.'release_id net_setup_status
$ net_setup_status = f$trnlnm( "net_setup_status" )
$ if (net_setup_status .nes. "") then deassign "net_setup_status"
$!
$! Check if Star is set up.
$!
$ iigetres ii.'node_name.config.star.'release_id star_setup_status
$ star_setup_status = f$trnlnm( "star_setup_status" )
$ if (star_setup_status .nes. "") then deassign "star_setup_status"
$!
$! Check if Oracle gateway is set up.
$!
$ iigetres ii.'node_name.config.oracle.'release_id gwora_setup_status
$ gwora_setup_status = f$trnlnm( "gwora_setup_status" )
$ if (gwora_setup_status .nes. "") then deassign "gwora_setup_status"
$!
$! Check if Rdb gateway is set up.
$!
$ iigetres ii.'node_name.config.rdb.'release_id gwrdb_setup_status
$ gwrdb_setup_status = f$trnlnm( "gwrdb_setup_status" )
$ if (gwrdb_setup_status .nes. "") then deassign "gwrdb_setup_status"
$!
$!
$! If we are installing images, go ahead and try to delete them
$! first as a precaution.
$ if ( operation .eqs. "ADD" ) 
$ then 
$   @ii_system:[ingres.utility]iishare DELETE
$ endif
$!
$! Perform the operation on the set of images
$!
$ set noon
$!
$ if (dbms_setup_status .nes. "")
$ then
$    work_list = dbms_list
$    gosub process_list
$    net_setup_status = "YES"
$ endif
$!
$ if (net_setup_status .nes. "")
$ then
$    work_list = client_list
$    gosub process_list
$ endif
$!
$ if (star_setup_status .nes. "")
$ then
$    work_list = star_list
$    gosub process_list
$ endif
$!
$ if (ii_installation .eqs. "")
$ then
$    work_list = production_list
$    gosub process_list
$ endif
$!
$ if (gwora_setup_status .nes. "")
$ then
$    work_list = gwora_list
$    gosub process_list
$ endif
$!
$ if (gwrdb_setup_status .nes. "")
$ then
$    work_list = gwrdb_list
$    gosub process_list
$ endif
$!
$ if ( ii_compatlib_def .eqs. "" ) then deassign/job ii_compatlib
$ if ( ii_libqlib_def .eqs. "" ) then deassign/job ii_libqlib
$ if ( ii_framelib_def .eqs. "") then deassign/job ii_framelib
$!
$ EXIT_FAIL:
$!
$ exit
$!
$!
$! Subroutine to perform the install operation on the images
$!
$ process_list:
$   cmkrnl = f$privilege("CMKRNL")
$   set_message_off
$   set process/priv=CMKRNL
$   set_message_on
$   i = 0
$   work_list = f$edit(work_list,"COMPRESS")
$   next_image:
$       image = f$element(i, " ", work_list)
$       if ( "''image'" .eqs. " " .or. image .eqs. "" ) then goto end_image
$       i = i + 1
$       params = f$element(i, " ", work_list)
$       i = i + 1
$       set_message_off
$       if (operation .eqs. "ADD")
$       then
$           image = f$parse(image,,,,"NO_CONCEAL")
$           install add 'image 'params
$       else
$           install delete 'image
$       endif
$       set_message_on
$       goto next_image
$  end_image:
$  if ( cmkrnl .eqs. "FALSE") then set process/priv=NOCMKRNL
$  return
