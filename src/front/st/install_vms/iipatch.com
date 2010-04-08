$GOTO beginning

$help:
$type SYS$INPUT

                         INGRES II PATCH INSTALLATION
                         ============================

   This utility will install the specified patch. The utility will load the
   files contained in the patch for the Ingres packages installed.

   The patch files will be unloaded into a temporary directory. Only the
   required files will be removed from the temporary directory to the Ingres
   installation.

   By default, this utility will create a saveset of all the files being
   replaced by the patch. This saveset can subsequently be used to uninstall
   the patch.

   Should the patch installer encounter a problem from which it cannot
   continue, any files copied from the temporary directory to the Ingres
   Installation will be deleted.
  
   There is no need to shutdown the Ingres Installation before commencing the
   patch install/uninstall. However, once the files to be replaced have been
   determined if the Ingres installation is running you will be asked to Stop
   it. At this point the only way to continue is to allow the utility to
   shutdown Ingres. If you indicate that Ingres should not be stopped then
   the patch install/uninstall will abort.

   Once the patch has been installed, the utility will prompt to see if it
   should start the Ingres installation.
   
   Syntax:

      IIPATCH <OP> <saveset> [-T<temp dir>] [-U<backup dir>] [-N] [-P]

   <OP>      :== INSTALL | UNINSTALL | HELP

   <saveset> :== Specifies the saveset to be used by the IIPATCH utility.
                 For INSTALL   this is the patch being installed.
                 FOR UNINSTALL this is the saveset created by IIPATCH when
                               the current patch was installed

   -T        :== Location where IIPATCH creates temporary files whilst
                 installing patch. If no <temp dir> is specified
                 SYS$SCRATCH is used

   -U        :== Area where Uninstall saveset will be stored
                 IF no <backup dir> specified, backup file will be created
                 in the II_SYSTEM:[INGRES] directory

   -N        :== No Uninstall Saveset created.

   -P        :== PURGE the replaced files.

$ EXIT

## History:
##     13-Feb-2001 (horda03)
##          Created.
##     15-Aug-2002 (horda03) Bug 108517
##          Added Oracle as a package.
##     16-Aug-2002 (bolke01) Bug 108524
##          Updated load process to allow for there having been a 
##          failure of the previous install
##     24-oct-2002 (horda03)
##          Miscellaneous spelling mistakes.
##     25-oct-2002 (horda03)
##          Added -P to signal PURGE required of replaced files.
##     04-apr-2003 (horda03) Bug 110048
##          Group installation files which require the Installation
##          ID in their names, are now COPY'ed from the original
##          and not hard linked, to prevent problems when running
##          any iisu*.com scripts.
##     11-apr-2003 (horda03) Bug 110043
##          If sys$system:ingres_installations.dat files doesn't
##          exist, don't try to append to it.
##     22-jul-2004 (horda03) Bug 112727
##          If the Gen Level of the patch is different from the
##          Gen Level installed, add the new Gen Level to CONFIG.DAT
##     14-dec-2004 (horda03) Bug 113635/INGINS283
##          On SYSTEM level installation, processes with the name
##          II_PC_<pid> should be ignored when checking to see if
##          the installation being patched is executing.
##     17-feb-2005 (horda03) Bug 112727
##          Extended earlier change to handle Ingres Cluster patch.
##     08-jun-2005 (horda03) Bug 114647/INGINS323
##          Add JDBC package
##     20-jun-2007 (horda03)
##          patch.html is now supplied on VMS, not patch.doc
##     10-Jam-2008 (bolke01)
##          Updated licensing checks. Not used currently in 2006
##          Added SHELL_DEBUG statements for a verbose useage
##          Modified the VSIPRO package from VISIONTUTOR to IIABF
##          Added the DAS and ODBC packages which are now visible 
##          on VMS.
##          Corrected X-int change 478024 which introduced a failure.
##     03-mar-2008 (bolke01) Bug 120020
##          Failure to uninstall if uninstalling the first patch installed 
##          on an installation.
##     13-Jun-2008 (horda03) Bug 120507
##          Added DOCUMENTATION package details.
##    06-Apr-2010 (horda03)
##          RELEASE.DAT is located in the [INGRES.MANIFEST] directory.
##
$!If request backup the files that will be replaced
$!
$!Create links with version 'backup_version' to the
$!files in II_SYSTEM:[INGRES...] which will be replaced
$!by the patch. Any files installed in the patch which
$!are not currently loaded will have an entry setup
$!in the UNINSTALL script to delete it.
$!
$backup_inst:
$
$  IF nobackup .EQ. 1 THEN RETURN
$
$  say "Preparing Installation files to backup..."
$
$  ON ERROR     THEN GOTO backup_inst_err
$  ON CONTROL_Y THEN GOTO backup_inst_ctrly
$
$  OPEN/WRITE open_file II_SYSTEM:[INGRES]UNINSTALL_INFO.COM;'backup_version'
$  file_open = 1
$  WRITE open_file "$! Uninstall [''patch_level']"
$
$  IF F$SEARCH( "II_SYSTEM:[INGRES]VERSION.REL") .EQS. ""
$  THEN
$     ! Patching a GA installation, so remove the VERSION.REL and PATCH.HTML
$
$     WRITE open_file "$ DELETE II_SYSTEM:[INGRES]VERSION.REL;*"
$     WRITE open_file "$ DELETE II_SYSTEM:[INGRES]PATCH.HTML;*"
$  ENDIF
$
$  del_dir = ""
$
$backup_loop:
$  backup_file = F$SEARCH( "iipatch:[INGRES...]*.*;''iipatch_install_version'", 0)
$
$  IF backup_file .EQS. "" THEN GOTO backup_files
$
$  backup_name = F$ELEMENT(0, ";", F$ELEMENT(1, ":", backup_file))
$
$  IF F$PARSE( backup_name,,"TYPE") .NES. ".DIR"
$  THEN
$     IF F$SEARCH( "II_SYSTEM:''backup_name'" ) .NES. ""
$     THEN
$        SET FILE/ENTER=II_SYSTEM:'backup_name';'backup_version' II_SYSTEM:'backup_name'
$     ELSE
$        WRITE open_file "$ DELETE II_SYSTEM:''backup_name';*"
$     ENDIF
$  ELSE
$     del_dir = F$FAO( "!AS,!AS", backup_name, del_dir)  !Stored in reverse order (for deletion)
$  ENDIF
$
$  GOTO backup_loop
$
$backup_files:
$  ! Check if we need to delete any directories.
$  IF del_dir .NES. ""
$  THEN
$     elem = 0
$
$del_dir_loop:
$     dir_name = F$ELEMENT( elem, ",", del_dir)
$
$     IF dir_name .EQS. "" THEN GOTO del_dir_fin
$
$     WRITE open_file "$ CALL delete_dir ''dir_name'"
$
$     elem = elem + 1
$
$     GOTO del_dir_loop
$del_dir_fin:
$     WRITE open_file "$ EXIT"
$     WRITE open_file "$delete_dir: SUBROUTINE"
$     WRITE open_file "$ check_dir = F$FAO( ""!AS.!AS]"", F$PARSE(P1,,,""DIRECTORY"") - ""]"", F$PARSE(P1,,,""NAME""))"
$     WRITE open_file F$FAO( "$ IF F$SEARCH( ""II_SYSTEM:!2*'check_dir'*.*"") .NES. """"")
$     WRITE open_file "$ THEN"
$     WRITE open_file F$FAO( "$    WRITE SYS$OUTPUT ""WARNING: Dir """"II_SYSTEM:!2*'P1'"""" cannot be deleted.""")
$     WRITE open_file "$ ELSE"
$     WRITE open_file F$FAO( "$    DELETE II_SYSTEM:'P1';")
$     WRITE open_file "$ ENDIF"
$     WRITE open_file "$ENDSUBROUTINE"
$  ENDIF
$
$  CLOSE open_file
$  file_open = 0
$
$  say "Performing Checksum on files being backed up..."
$
$  OPEN/WRITE open_file II_SYSTEM:[INGRES]PLIST.LIS;'backup_version'
$
$backup_checksum:
$
$  file_name = F$SEARCH("II_SYSTEM:[INGRES...]*.*;''backup_version'")
$
$  IF file_name .EQS. "" THEN GOTO backup_checksum_fin
$
$  IF file_name .EQS. F$PARSE("II_SYSTEM:[INGRES]PLIST.LIS;''backup_version'") THEN GOTO backup_checksum
$
$  CHECKSUMx 'file_name'
$
$  WRITE open_file F$FAO("!12ZL !AS", 'CHECKSUM$CHECKSUM, F$ELEMENT(1, ":", file_name))
$
$  GOTO backup_checksum
$
$backup_checksum_fin:
$  CLOSE open_file
$  file_open = 0
$
$  check_version  = backup_version
$  check_path     = "ii_system"
$  check_dev      = backdir
$  check_overhead = 400
$
$  say "Checking ''backdir' has sufficient space to backup modified files..."
$
$  GOSUB check_space
$
$  IF iipatch_status .NE. 0 THEN GOTO backup_inst_rename
$
$  backup_file = "''backdir'''patch_name'.UBK"
$
$  say "Backing up files to ""''backup_file'""..."
$
$  BACKUP/NOLOG II_SYSTEM:[INGRES...]*.*;'backup_version' 'backup_file';/SAVE
$
$  SET FILE/REMOVE II_SYSTEM:[INGRES...]*.*;'backup_version'
$
$  RETURN
$
$backup_inst_rename:
$  SET NOON
$
$  IF file_open .EQ. 1 THEN CLOSE open_file
$
$  say "Restoring modified files..."
$
$  IF F$SEARCH("II_SYSTEM:[INGRES...]*.*;''backup_version'") .NES. ""
$  THEN
$     SET FILE/REMOVE II_SYSTEM:[INGRES...]*.*;'backup_version'
$  ENDIF
$
$  SET ON
$
$  iipatch_status = EXIT_IIPATCH
$
$  RETURN
$
$backup_inst_ctrly:
$
$  ctrly_reason = "Backup Installation aborted. No Uninstall Savset created."
$
$  GOSUB report_ctrly
$
$  IF F$SEARCH( "''backup_file'" ) .NES. ""
$  THEN
$     DELETE 'backup_file';
$  ENDIF
$
$  GOTO backup_inst_rename
$
$backup_inst_err:
$
$  err_stat = $status
$
$  error_reason = "Error during Backup Installation. No Uninstall Saveset created"
$
$  GOSUB report_error
$
$  IF F$SEARCH( "''backup_file'" ) .NES. ""
$  THEN
$     DELETE 'backup_file';*
$  ENDIF
$
$  GOTO backup_inst_rename


$beginning:
$
$ON CONTROL_Y THEN EXIT
$
$IF "''P1'" .EQS. "UNINSTALL"
$THEN
$   GOSUB setup
$
$   action = "UNINSTALL"
$
$   steps = uninstaLL_steps
$
$   load_patch_version = install_version
$
$   GOTO perform_install
$ENDIF
$
$IF "''P1'" .EQS. "INSTALL"
$THEN
$   GOSUB setup
$
$   action = "INSTALL"
$
$   steps = instaLL_steps
$
$   load_patch_version = 1
$
$   GOTO perform_install
$ENDIF
$
$IF "Hh" - F$EXTRACT( 0, 1, P1) .NES. "Hh" THEN GOTO help
$
$TYPE SYS$INPUT

ERROR - Only options are UNINSTALL, INSTALL or HELP
$
$ GOTO help


$!CHECK_CHECKSUMS:
$! If the file [INGRES]PLIST.LIS exists, then check thatthe files to be
$! installed matched the CHECKSUMs listed in the file.
$!
$check_checksums:
$ IF F$SEARCH("iipatch:[ingres]plist.lis") .EQS. ""
$ THEN
$    RETURN
$ ENDIF
$
$ ON CONTROL_Y THEN GOTO check_checksums_ctrly
$ ON ERROR     THEN GOTO check_checksums_err
$
$ say "Checking Checksum of files being installed..."
$
$ OPEN/READ open_file iipatch:[ingres]plist.lis
$ file_open = 1
$
$checksum_loop:
$
$ READ/END_OF_FILE=checksum_fin open_file record
$
$ file_checksum = F$ELEMENT(0, ",", record)
$
$ file = F$ELEMENT(1, " ", record)
$
$ IF F$SEARCH("iipatch:''file';''load_patch_version'") .EQS. "" THEN GOTO checksum_loop
$
$ CHECKSUM iipatch:'file';'load_patch_version'
$
$ IF "''checksum$checksum'" .EQS. "''file_checksum'" THEN GOTO checksum_loop
$
$ CALL get_input "''file' checksum = ''CHECKSUM$CHECKSUM but ''file_checksum' expected. Abort/Continue ? [AC]" "AC"
$ IF iipatch_input_status .NE. 0
$ THEN
$    GOTO check_checksum_ctrly
$ ENDIF
$
$ IF "A" - iipatch_ans .NES. "A"
$ THEN
$    iipatch_status = EXIT_iipatch
$    CLOSE open_file
$    RETURN
$ ENDIF
$
$ GOTO checksum_loop
$
$checksum_fin:
$ CLOSE open_file
$ file_open = 0
$
$ RETURN
$
$check_checksums_ctrly:
$ ctrly_reason = "Abort while checking checksum of files to be Installed"
$
$ GOSUB report_ctrly
$
$ IF file_open .EQ. 1 THEN CLOSE open_file
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
$
$check_checksums_err:
$ err_stat = $status
$
$ error_reason = "Error while checking checksum of files to be Installed"
$
$ GOSUB report_error
$
$ IF file_open .EQ. 1 THEN CLOSE open_file
$
$ iipatch_status = EXIT_nocleanup
$
$ RETURN

$!CHECK_DIRECTORY:
$! Check that the specified directory specification is valid
$!
$!  directory == Name of the directory to parse
$!
$check_directory:
$ IF (F$PARSE(directory,,,"NAME")    .NES. "")  .OR. -
     (F$PARSE(directory,,,"TYPE")    .NES. ".") .OR. -
     (F$PARSE(directory,,,"VERSION") .NES. ";")
$ THEN
$    say """''directory'"" is not a valid DIRECTORY format"
$
$    iipatch_status = EXIT_nocleanup
$
$    RETURN
$ ENDIF
$
$ IF F$LOCATE("]", directory) .EQ. F$LENGTH(directory)
$ THEN
$    test_dir = directory + "[000000]"
$ ELSE
$    test_dir = directory
$ ENDIF
$
$ parse_dir = F$PARSE( "''test_dir'",,,,"NO_CONCEAL")
$
$ IF "''parse_dir'" .EQS. ""
$ THEN
$    say """''directory'"" is not a valid Directory format"
$    say " "
$
$    iipatch_status = EXIT_nocleanup
$    RETURN
$ ENDIF
$
$ directory = parse_dir - "000000]" - ".;" - "][" - ".]" - "]" + "]" - "[]"
$
$ GOSUB check_device
$
$ RETURN


$!CHECK_DEVICE
$! Check that the specified device is mounted and accessible.
$!
$!    Define Directory before calling.
$!
$check_device:
$
$ machine_type = F$GETDVI( "''directory'", "DEVCLASS" )
$
$ IF "''machine_type'" .EQS. "1"
$ THEN
$    IF F$LOCATE("[", "''directory'") .EQ. F$LENGTH(directory)
$    THEN
$       directory = "''directory'[000000]"
$    ENDIF
$
$    IF F$PARSE(directory) .EQS. ""
$    THEN
$       say "Directory ""''directory' does not exist"
$
$       iipatch_status = EXIT_nocleanup
$
$       RETURM
$    ENDIF
$ ELSE
$    IF "''machine_type'" .EQS. "2"
$    THEN
$       IF F$GETDVI( "''directory'", "MNT" ) .EQS. "FALSE"
$       THEN
$          say "    The specified Tape Device (''F$ELEMENT(0, ":",directory)':) is NOT MOUNTED."
$          say " "
$
$          iipatch_status = EXIT_nocleanup
$          RETURN
$       ENDIF
$    ELSE
$       say "   Unexpected Device Class (''machine_type')- Only DISKs (1) and TAPE (2) expected"
$       say " "
$
$       iipatch_status = EXIT_nocleanup
$       RETURN
$    ENDIF
$ ENDIF
$
$ RETURN



$!Search the II_SYSTEM:[INGRES...] directories for files with the
$!specified version number.
$!
$check_file_version:
$ ON CONTROL_Y THEN GOTO check_file_v_ctrly
$ ON ERROR     THEN GOTO check_file_v_err
$
$ IF F$SEARCH("II_SYSTEM:[INGRES...]*.*;''check_version'") .NES. ""
$ THEN
$    say "Files exist in II_SYSTEM:[INGRES...] with Version ''check_version'. This version is used by"
$    say "the IIPATCH installer to identify files installed from the patch. The utility"
$    say "cannot continue whilst these files exist."
$
$check_fv_input:
$    CALL get_input "Do you wish to View files/Rename files/Delete File//Abort [VRDA] ?" "ADRV"
$
$    IF iipatch_input_status .NE. 0
$    THEN
$       GOTO check_file_v_ctrly
$    ENDIF
$
$    IF "V" - iipatch_ans .NES. "V"
$    THEN
$       say " "
$       say "Listing of II_SYSTEM:[INGRES...]*.*;''check_version'"
$
$       DIR/NOHEAD/NOTRAIL II_SYSTEM:[INGRES...]*.*;'check_version'
$
$       say " "
$
$       GOTO check_fv_input
$    ENDIF
$
$    IF "A" - iipatch_ans .NES. "A"
$    THEN
$       iipatch_status = EXIT_nocleanup
$       RETURN
$    ENDIF
$
$    IF "R" - iipatch_ans .NES. "R"
$    THEN
$       say "Renaming all files with version ''check_version'"
$
$       RENAME/LOG II_SYSTEM:[INGRES...]*.*;'check_version' II_SYSTEM:[INGRES...]*.*;
$
$       say " "
$
$       RETURN
$    ENDIF
$
$    IF "D" - iipatch_ans .NES. "D"
$    THEN
$       say "Deleting all files with version ''check_version'"
$
$       DELETE/LOG II_SYSTEM:[INGRES...]*.*;'check_version'
$
$       say " "
$
$       RETURN
$    ENDIF
$
$    iipatch_status = EXIT_nocleanup
$ ENDIF
$
$ RETURN
$
$check_file_v_ctrly:
$ ctrly_reason = "Abort while checking installation for files version reserved for patch install"
$
$ GOSUB report_ctrly
$
$ iipatch_status = EXIT_nocleanup
$
$ RETURN
$
$check_file_v_err:
$ err_stat = $status
$
$ error_reason = "Error while checking installation for files version reserved for patch install"
$
$ GOSUB report_error
$
$ iipatch_status = EXIT_nocleanup
$
$ RETURN


$ !The version numbers 32767 and 32766 are used by this utility to identify files
$ !which are affected by the backup. If any files exist, in II_SYSTEM:[INGRES...]
$ !then need to renumber these files.
$
$check_inst:
$ say "Checking II_SYSTEM:[INGRES...] for file version reserved for IIPATCH use..."
$
$ check_version = install_version
$ GOSUB check_file_version
$ IF iipatch_status .NE. 0 THEN RETURN
$
$ check_version = backup_version
$ GOSUB check_file_version
$ IF iipatch_status .NE. 0 THEN RETURN
$
$ RETURN


$! Check that the saveset being installed is really an INSTALL patch
$!
$check_install_patch:
$
$ IF F$SEARCH( "iipatch:[INGRES]UNINSTALL_INFO.COM") .NES. ""
$ THEN
$    say "This is an UNINSTALL patch. Install aborted"
$
$    iipatch_status = EXIT_iipatch
$ ENDIF
$
$ RETURN

$! check_release - Check that the version of Ingres that the patch installs
$!                 has been identified.
$
$check_release:
$  rel_id = F$EDIT( ingres_version - "." - "/" - "(" - "." -"/" - ")", "COLLAPSE, LOWERCASE")
$
$  iigetres ii.'node_name.config.cluster.id cluster_id
$  cluster_id = f$trnlnm( "cluster_id" )
$
$  IF cluster_id .EQS. ""
$  THEN
$     node_list = "''node_name'"
$  ELSE
$     DEASSIGN "cluster_id"
$
$     SEARCH/OUT=IIPATCH:[TMP]CLUSTER.LIST II_SYSTEM:[INGRES.FILES]CONFIG.DAT ".config.cluster.id"
$
$     OPEN/READ cluster_list IIPATCH:[TMP]CLUSTER.LIST
$
$     node_list = ""
$
$check_release_clstr_l:
$     READ/END_OF_FILE=check_clustr_d cluster_list record
$
$     node_list = F$FAO( "!AS,!AS", node_list, F$ELEMENT( 1, ".", record))
$
$     GOTO check_release_clstr_l
$
$check_clustr_d:
$     CLOSE cluster_list
$     DELETE/NOLOG IIPATCH:[TMP]CLUSTER.LIST;*
$
$     node_list = node_list - ","
$  ENDIF
$
$  cr_elem = 0
$
$check_node_loop:
$
$  next_node = F$ELEMENT( cr_elem, ",", node_list)
$
$  IF next_node .NES. ","
$  THEN
$     elem = 0
$
$check_release_loop:
$     pkg = F$ELEMENT( elem, ",", products)
$
$     IF pkg .NES. ","
$     THEN
$        elem = elem + 1
$
$        IF F$LOCATE( "#''pkg'#", relid_pkgs) .NE. F$LENGTH( relid_pkgs )
$        THEN
$           CALL Check_Relid 'pkg' "''rel_id'" "''next_node'"
$        ENDIF
$
$        GOTO check_release_loop
$     ENDIF
$
$     cr_elem = cr_elem + 1
$
$     GOTO check_node_loop
$  ENDIF
$
$  RETURN


$! Check_RelID - SUBROUTINE
$!  This subroutine takes the package and release_id as parameters
$!  it then verifies that the package has the release id logged
$!  in CONFIG.DAT. If not, then the release is added with the
$!  same text as the last entry. If no previous entry exits, no
$!  new entry will be created.
$
$Check_Relid: SUBROUTINE
$   pkg = P1
$   rel = P2
$   node = P3
$
$   iigetres ii.'node'.config.'pkg'.'rel' pkg_rel_id
$
$   pkg_rel_id = F$TRNLNM( "pkg_rel_id" )
$
$   IF pkg_rel_id .NES. ""
$   THEN
$      DEASSIGN "pkg_rel_id"
$   ELSE
$      PIPE SEARCH/NOWARN II_SYSTEM:[INGRES.FILES]CONFIG.DAT ii.'node'.config.'pkg'. | -
            SORT/KEY=(POS:1,SIZE:100,DESC) SYS$PIPE IIPATCH:[TMP]relid.lis
$
$      OPEN/READ open_file IIPATCH:[TMP]relid.lis
$      READ/END_OF_FILE=check_Relid_nodef open_file line
$
$      pkg_rel_id = F$ELEMENT( 1, ":", F$EDIT("''line'", "COLLAPSE"))
$
$      iisetres ii.'node'.config.'pkg'.'rel' "''pkg_rel_id'"
$
$check_Relid_nodef:
$      CLOSE open_file
$      DELETE/NOLOG IIPATCH:[TMP]relid.lis;*
$   ENDIF
$
$   EXIT
$ENDSUBROUTINE

$!CHECK_SPACE:
$!  Check that there is sufficient space available on the device for the
$!  files being installed.
$!
$!      check_version  -- File version to be checked
$!      check_path     -- Logical device which hold files
$!      check_dev      -- Device to be checked for space
$!      check_overhead -- Space overhead per file (esp. for savesets)
$!
$check_space:
$ IF F$GETDVI( "''check_dev'", "DEVCLASS" ) .EQ. 2 THEN RETURN
$ 
$ ON CONTROL_Y THEN GOTO check_space_ctrly
$ ON ERROR     THEN GOTO check_space_err
$
$ space_needed = check_overhead
$
$check_space_loop:
$ file = F$SEARCH("''check_path':[INGRES...]*.*;''check_version'", 0)
$
$ IF file .EQS. "" THEN GOTO check_avail_space
$
$ space_needed = check_overhead + space_needed + 'F$FILE_ATTRIBUTE(file, "ALQ")
$
$ GOTO check_space_loop
$
$check_avail_space:
$ space_avail = F$GETDVI("''check_dev'", "FREEBLOCKS")
$
$ IF space_avail .LT. space_needed
$ THEN
$    p1_par = F$FAO( "Device !AS has !SL free blocks, !SL Blocks required to install patch.!/!AS", -
                       F$PARSE( "''check_dev'",,,"DEVICE", "NO_CONCEAL"), -
                       space_avail, space_needed, -
                       "Type A to Abort, C to Continue (when space available)." )
$
$    CALL get_input "''p1_par'" "AC"
$
$    IF iipatch_input_status .NE. 0
$    THEN
$       GOTO check_space_ctrly
$    ENDIF
$
$    IF "A" - iipatch_ans .NES. "A"
$    THEN
$       iipatch_status = EXIT_iipatch
$       RETURN
$    ENDIF
$
$    GOTO check_avail_space
$ ENDIF
$
$ RETURN
$
$check_space_ctrly:
$ ctrly_reason = "Abort during checking if sufficient space available"
$
$ GOSUB report_ctrly
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
$
$check_space_err:
$
$ err_stat = $STATUS
$
$ ctrly_reason = "Error during checking if sufficient space available"
$
$ GOSUB report_error
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN




$! Check that the saveset being installed is really an UNINSTALL patch
$!
$check_uninstall_patch:
$
$ ON CONTROL_Y THEN GOTO check_uninstall_ctrly
$ ON ERROR     THEN GOTO check_uninstall_err
$
$ IF F$SEARCH( "iipatch:[INGRES]UNINSTALL_INFO.COM") .EQS. ""
$ THEN
$    say "This is not an UNINSTALL patch. Uninstall aborted"
$
$    iipatch_status = EXIT_iipatch
$
$    RETURN
$ ENDIF
$
$ OPEN/READ file iipatch:[INGRES]UNINSTALL_INFO.COM
$ file_open = 1
$ READ file record
$ CLOSE file
$ file_open = 0
$
$ uninstall_patch_level = F$ELEMENT(1, "[", F$ELEMENT( 0, "]", record))
$
$ OPEN/READ file II_SYSTEM:[INGRES]VERSION.REL
$ file_open = 1
$ READ file record
$ READ file cur_patch_level
$ CLOSE file
$ file_open = 0
$
$ IF "''uninstall_patch_level'" .NES. "''cur_patch_level'"
$ THEN
$    say F$FAO( "ERROR - The uninstall saveset is to uninstall !AS but !AS installed", -
                             uninstall_patch_level, cur_patch_level)
$
$    iipatch_status = EXIT_iipatch
$
$    RETURN
$ ENDIF
$
$ RETURN
$
$check_uninstall_ctrly:
$
$ ctrly_reason = "Abort during checking of UNINSTALL saveset"
$
$ GOSUB report_ctrly
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
$
$check_uninstall_err:
$
$ err_stat = $STATUS
$
$ ctrly_reason = "Error during checking of UNINSTALL saveset"
$
$ GOSUB report_error
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN


$!Check User specified Parameters.
$!
$!    P2  == Name of Saveset
$!
$!    P3+ == Optional parameters
$!
$check_params:
$   saveset = F$PARSE( "''P2'",,,,"SYNTAX_ONLY")
$
$   IF "''saveset'" .EQS. ""
$   THEN
$      say """''P2' is not a valid FIle Descriptor"
$      say " "
$
$      iipatch_status = EXIT_nocleanup
$      RETURN
$   ENDIF
$
$ machine_type = F$GETDVI( "''saveset'", "DEVCLASS" )
$
$ IF "''machine_type'" .EQS. "1"
$ THEN
$    IF F$SEARCH( "''saveset'" ) .EQS. ""
$    THEN
$       say "   ""''P2'"" does not exist."
$       say " "
$
$       iipatch_status = EXIT_nocleanup
$       RETURN
$    ENDIF
$ ELSE
$    IF "''machine_type'" .EQS. "2"
$    THEN
$       IF F$GETDVI( "''saveset'", "MNT" ) .EQS. "FALSE"
$       THEN
$          say "    The specified Tape Device is NOT MOUNTED."
$          say " "
$
$          iipatch_status = EXIT_nocleanup
$          RETURN
$       ENDIF
$    ELSE
$       say "   Unexpected Device Class (''machine_type')- Only DISKs (1) and TAPE (2) expected"
$       say " "
$
$       iipatch_status = EXIT_nocleanup
$       RETURN
$    ENDIF
$ ENDIF
$
$ patch_name = F$PARSE( saveset,,,"NAME")
$
$ param_no = 3
$
$param_loop:
$
$ param_type = "WHAT"
$
$ user_param = P'param_no'
$
$ IF user_param .EQS. "" THEN RETURN
$
$ param_upp = F$EDIT(user_param, "UPCASE")
$
$ IF F$EXTRACT(0, 2, param_upp) .EQS. "-T"
$ THEN
$    param_type = "T"
$    param = param_upp - "-T"
$ ENDIF
$
$ IF F$EXTRACT(0, 2, param_upp) .EQS. "-U"
$ THEN
$    param_type = "U"
$    param = param_upp - "-U"
$ ENDIF
$
$ IF param_upp .EQS. "-N"
$ THEN
$    param_type = "N"
$ ENDIF
$
$ IF param_upp .EQS. "-P"
$ THEN
$    param_type = "P"
$ ENDIF
$
$ IF param_type .EQS. "WHAT"
$ THEN
$    say "ERROR - Unrecognised parameter [''user_param']"
$    say " "
$
$    iipatch_status = EXIT_nocleanup
$    RETURN
$ ENDIF
$
$ GOSUB param_type_'param_type'
$
$ IF iipatch_status .NE. 0 THEN RETURN
$
$ param_no = param_no + 1
$
$ IF param_no .GT. 9 THEN RETURN
$
$ GOTO param_loop


$! Delete All files and Directories under IIPATCH
$!
$delete_iipatch_files:
$
$ ON ERROR     THEN GOTO delete_iipatch_err
$ ON CONTROL_Y THEN GOTO delete_iipatch_ctrly
$
$ say "Removing all files in IIPATCH area..."
$
$ DELETE/NOLOG/EXCLUDE=*.DIR iipatch:[000000...]*.*;*
$
$ dir_count = 0
$
$ dir0 = "''tmpdir'iipatch.dir;"
$
$delete_dir_loop:
$
$ dir_name = F$SEARCH( "iipatch:[000000...]*.dir")
$
$ IF dir_name .EQS. "" THEN GOTO delete_dir
$
$ dir_count = dir_count + 1
$
$ dir'dir_count' = dir_name
$
$ GOTO delete_dir_loop
$
$delete_dir:
$ IF dir_count .LT. 0 THEN RETURN
$
$ dele_dir = dir'dir_count'
$
$ SET FILE/PROT=(O:WERD,W:WERD,G:WERD,S:WERD) 'dele_dir'
$ DELETE 'dele_dir'
$
$ dir_count = dir_count - 1
$
$ GOTO delete_dir
$
$delete_iipatch_err:
$ err_stat = $status
$
$ error_reason = "Error during DELETE of IIPATCH area"
$
$ GOSUB report_error
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
$
$delete_iipatch_ctrly:
$
$ ctrly_reason = "Aborting DELETE IIPATCH area"
$
$ GOSUB report_ctrly
$
$ iipatch_status = EXIT_nocleanup
$
$ RETURN


$!FILES_NEEDED:
$!    Determine from the Manifest what packages are licenced.
$!    Then determine what packages are installed.
$!    Finally determine which files need installing from the patch.
$!    The files to be installed will have INSTALL_VERSION as the
$!    file version.
$!
$files_needed:
$  ON CONTROL_Y THEN GOTO files_needed_ctrly
$  ON ERROR     THEN GOTO files_needed_err
$
$  mandir = F$SEARCH("iipatch:[ingres...]release.dat")
$
$  IF mandir .NES. ""
$  THEN
$     mandir = F$PARSE(mandir,,,"DIRECTORY")
$     DEFINE/NOLOG II_MANIFEST_DIR iipatch:'mandir'
$  ENDIF
$
$  IF (F$SEARCH( "II_SYSTEM:[INGRES.UTILITY]IIMANINF.EXE" ) .EQS. "") .OR. -
      ( (mandir .EQS. "") .AND. (F$SEARCH( "II_SYSTEM:[INGRES.INSTALL]RELEASE.DAT") .EQS. "") .AND. -
        ( F$SEARCH( "II_SYSTEM:[INGRES.MANIFEST]RELEASE.DAT") .EQS. ""))
$  THEN
$     say "IIMANINF utility unavailable."
$
$     CALL get_input "Do you wish to continue with installation ? Install ALL files ? [Y/N]" "YN"
$
$     IF iipatch_input_status .NE. 0
$     THEN
$        GOTO files_needed_ctrly
$     ENDIF
$
$     IF "Y" - iipatch_ans .NES "Y"
$     THEN
$        iipatch_install_version = 1
$
$        products = "ALL PATCH FILES"
$
$        RETURN
$     ELSE
$        iipatch_status = EXIT_iipatch
$        RETURN
$     ENDIF
$  ENDIF
$
$  iimaninf := $II_SYSTEM:[INGRES.UTILITY]IIMANINF.EXE
$
$  iimaninf -custom -output=iipatch:[tmp]packages.list
$
$  OPEN/READ open_file iipatch:[tmp]packages.list
$  file_open = 1
$
$  READ/END_OF_FILE=files_needed_err open_file record
$
$  len_title       = F$LENGTH(record)
$  licensed_offset = F$LOCATE( "Licensed", record)
$  
$  IF len_title .EQ. licensed_offset
$  THEN
$     licensed_offset = 0
$  ENDIF
$
$  READ/END_OF_FILE=files_needed_err open_file record
$
$  sep = ""
$
$files_needed_loop:
$
$  READ/END_OF_FILE=files_needed_fin open_file record
$
$  record = F$EDIT(record, "UPCASE")
$
$  IF licensed_offset .EQ. 0 .OR. F$EXTRACT(licensed_offset, 3, "''record'") .EQS. "YES"
$  THEN
$     product   = F$ELEMENT(0, " ", F$EDIT(record, "TRIM"))
$
$     known_pkg = F$LOCATE( F$FAO("#!AS#", product), valid_pkgs)
$
$     IF known_pkg .NE. F$LENGTH(valid_pkgs)
$     THEN
$        IF F$SEARCH(F$FAO("II_SYSTEM:!AS",pkg_'product')) .NES. ""
$        THEN
$           IF F$TYPE(SHELL_DEBUG) .NES. "" 
$           THEN
$              WRITE SYS$OUTPUT F$FAO("!AS!AS", sep, product)
$           ENDIF
$           products = products + F$FAO("!AS!AS", sep, product)
$
$           sep = ","
$        ENDIF
$     ELSE
$        say "Package [''product'] unknown to manifest."
$     ENDIF
$  ENDIF
$
$  GOTO files_needed_loop
$
$files_needed_fin:
$  CLOSE open_file
$  file_open = 0
$
$  DELETE iipatch:[tmp]packages.list;*
$
$  IF products .EQS. ""
$  THEN
$     say "No products are licensed for this installation."
$
$     iipatch_status = EXIT_iipatch
$     RETURN
$  ENDIF
$
$  iimaninf -output=iipatch:[tmp]installed.files -unique='products'
$
$  OPEN/READ open_file iipatch:[tmp]installed.files
$  file_open = 1
$
$files_needed_installed:
$  READ/END_OF_FILE=files_needed_completed open_file record
$
$  record = F$EDIT(record, "TRIM,COMPRESS,UPCASE")
$
$  file_installed = F$FAO("!AS!AS", F$ELEMENT(1, " ", record), F$ELEMENT(2, " ", record))
$  IF F$TYPE(SHELL_DEBUG) .NES. "" 
$  THEN
$     WRITE SYS$OUTPUT "''record'"
$  ENDIF
$
$  IF F$SEARCH( "iipatch:''file_installed'" ) .NES. ""
$  THEN
$     IF F$TYPE(SHELL_DEBUG) .NES. "" 
$     THEN
$        WRITE SYS$OUTPUT "''file_installed'"
$     ENDIF
$     RENAME iipatch:'file_installed' ;'iipatch_install_version'
$  ENDIF
$
$  GOTO files_needed_installed
$
$files_needed_completed:
$  CLOSE open_file
$  file_open = 0
$
$  DELETE iipatch:[tmp]installed.files;*
$
$  elem = 0
$
$add_always_copied:
$  IF F$ELEMENT( elem, ",", always_copy) .EQS. "," THEN RETURN
$
$  IF F$SEARCH(F$FAO("iipatch:!AS", F$ELEMENT( elem, ",", always_copy))) .NES. ""
$  THEN
$     RENAME iipatch:'F$ELEMENT( elem, ",", always_copy) ;'install_version'
$  ENDIF
$
$  elem = elem + 1
$
$  GOTO add_always_copied
$
$files_needed_ctrly:
$
$  ctrly_reason = "Abort while determing files to install"
$
$  GOSUB report_ctrly
$
$  IF file_open .EQ. 1 THEN CLOSE open_file
$
$  iipatch_status = EXIT_iipatch
$
$  RETURN
$
$files_needed_err:
$
$  err_stat = $status
$
$  error_reason = "Error while determing files to install"
$
$  GOSUB report_error
$
$  IF file_open .EQ. 1 THEN CLOSE open_file
$
$  iipatch_status = EXIT_iipatch
$
$  RETURN


$install_finished: 
$
$  ON CONTROL_Y THEN GOTO end_exit
$  ON ERROR     THEN GOTO end_exit
$
$ IF mandir .NES. ""
$ THEN
$    DEASSIGN II_MANIFEST_DIR
$ ENDIF
$
$ GOSUB exit_'iipatch_status'
$
$ IF F$TRNLNM("IIPATCH") .NES. "" THEN DEASSIGN IIPATCH
$
$ IF iipatch_status .NE. 0
$ THEN
$    say "''action' ABORTED at ''f$time()'"
$ ELSE
$    install_area = F$PARSE("II_SYSTEM:[000000]",,,,"NO_CONCEAL") - "[000000].;"
$    version      = "(" + F$ELEMENT(1, "(", ingres_version)
$
$    time = F$TIME()
$
$    IF F$SEARCH("sys$system:ingres_installations.dat") .NES. ""
$    THEN
$       OPEN/APPEND ingres_inst sys$system:ingres_installations.dat
$
$       WRITE ingres_inst F$FAO( "!AS !AS   !AS !AS !AS", time, version, install_area, f$user(), patch_name)
$       IF action .EQS. "INSTALL"
$       THEN
$          WRITE ingres_inst F$FAO( "             !AS INSTALLED pkgs = !AS", patch_level, products)
$       ELSE
$          WRITE ingres_inst F$FAO( "             !AS UNINSTALLED - Patch level now !AS", cur_patch_level, patch_level )
$       ENDIF
$
$       CLOSE ingres_inst
$    ENDIF
$
$    say "''action' COMPLETED at ''time'"
$ ENDIF
$
$end_exit:
$ EXIT
$
$exit_0: ! Normal exit condition.
$  GOTO delete_iipatch_files
$
$exit_1: ! IIPATCH_STATUS == EXIT_nocleanup
$  RETURN
$
$exit_2: ! iipatch_status == EXIT_iipatch
$  GOTO delete_iipatch_files


$! GET_INPUT:
$!     Gets user Input and checks that it is one of the valid
$!     options specified. If no permitted responses, then answer
$!     returned unchanged. Otherwise, input is returned in UPPER CASE.
$!
$!     P1 == Prompt
$!     P2 == Permitted responses.
$!
$!     iipatch_ans == Entered value
$
$get_input: SUBROUTINE
$
$ iipatch_input_status == 0
$
$ ON CONTROL_Y THEN GOTO get_input_ctrly
$
$get_input_l:
$   READ/PROMPT="''P1' " SYS$COMMAND ans
$
$   IF P2 .EQS. ""
$   THEN
$      iipatch_ans == ans
$   ELSE
$      iipatch_ans == F$EDIT(ans, "UPCASE")
$
$      IF "''P2'" - "''iipatch_ans'" .EQS. P2
$      THEN
$         say " "
$         say "Invalid option. Please try again. Valid Options ""''P2'"""
$         say " "
$         GOTO get_input_l
$      ENDIF
$   ENDIF
$
$   GOTO get_input_end
$
$get_input_ctrly:
$   iipatch_input_status == 1
$
$get_input_end:
$ENDSUBROUTINE


$!LOAD_FILES:
$!
$!   Install appropriate files from IIPATCH area to II_SYSTEM
$!
$load_files:
$    ON CONTROL_Y THEN GOTO load_files_ctrly
$    ON ERROR     THEN GOTO load_files_err
$
$    say "Checking II_SYSTEM device has sufficient space for new files..."
$
$    check_version  = iipatch_install_version
$    check_path     = "iipatch"
$    check_dev      = "ii_system"
$    check_overhead = 0
$
$    GOSUB check_space
$
$    IF iipatch_status .NE. 0 THEN RETURN
$
$    say "Installing appropriate files..."
$
$    BACKUP/EXCLUDE=*.DIR/NOLOG   IIPATCH:[INGRES...]*.*;'iipatch_install_version' -
                                  II_SYSTEM:[INGRES...]*.*;'install_version'/BY_OWNER=PARENT
$
$    IF ii_installation .EQS. "" THEN GOTO load_files_fin
$
$    elem = 0
$
$load_files_rename:
$
$    search_file = F$ELEMENT(elem, ",", inst_file_rename)
$
$    IF search_file .EQS. ","
$    THEN
$       GOTO load_files_fin
$    ENDIF
$
$    IF F$SEARCH( "II_SYSTEM:''search_file';''install_version'" )  .NES. ""
$    THEN
$       cp_dir  = F$PARSE(search_file,,,"DIRECTORY")
$       cp_name = F$PARSE(search_file,,,"NAME")
$       cp_type = F$PARSE(search_file,,,"TYPE")
$
$       COPY ii_system:'search_file';'install_version' -
             ii_system:'cp_dir''cp_name''ii_installation''cp_type';'install_version'
$
$    ENDIF
$
$    elem = elem + 1
$
$    GOTO load_files_rename
$
$load_files_fin:
$
$    lang =  F$TRNLNM( "II_LANGUAGE" )
$
$    IF lang .EQS. ""
$    THEN
$       lang = "ENGLISH"
$    ENDIF
$
$    IF (lang .NES. "ENGLISH") .AND. -
        ( F$SEARCH( "II_SYSTEM:[INGRES.FILES]''lang'.dir") .NES. "")
$    THEN
$       elem = 0
$
$lang_loop:
$       search_file = F$ELEMENT(elem, ",", lang_file_move)
$
$       IF search_file .EQS. ","
$       THEN
$          GOTO lang_file_fin
$       ENDIF
$
$       IF F$SEARCH( "II_SYSTEM:[INGRES.FILES.ENGLISH]''search_file';''install_version'" )  .NES. ""
$       THEN
$          COPY II_SYSTEM:[INGRES.FILES.ENGLISH]'search_file';'install_version' II_SYSTEM:[INGRES.FILES.'lang']
$       ENDIF
$
$       elem = elem + 1
$
$       GOTO lang_loop
$   ENDIF
$
$lang_file_fin:
$!   Rename the copied files
$
$    cur_control = F$ENVIRONMENT("CONTROL")
$
$    SET NOCONTROL=Y
$
$    IF F$SEARCH( "II_SYSTEM:[INGRES]UNINSTALL_INFO.COM;''install_version'") .NES. ""
$    THEN
$       @II_SYSTEM:[INGRES]UNINSTALL_INFO.COM;'install_version'
$
$       DELETE II_SYSTEM:[INGRES]UNINSTALL_INFO.COM;*
$    ENDIF
$
$    IF purge_req THEN GOSUB purge_prepare
$
$    RENAME II_SYSTEM:[INGRES...]*.*;'install_version' ;
$
$    IF purge_req THEN GOSUB purge_files
$
$    SET CONTROL=('cur_control')
$
$    RETURN
$
$load_files_ctrly:
$   ctrly_reason = "Aborting Patch file install..."
$
$   GOSUB report_ctrly
$
$   DELETE/NOLOG/EXCLUDE=*.DIR II_SYSTEM:[INGRES...]*.*;'install_version'
$
$   iipatch_status = EXIT_iipatch
$
$   RETURN
$
$load_files_err:
$   err_stat = $status
$
$   error_reason = "Error while installing Patch files"
$
$   GOSUB report_error
$
$   DELETE/NOLOG/EXCLUDE=*.DIR II_SYSTEM:[INGRES...]*.*;'install_version'
$
$   GOSUB backup_inst_rename
$
$   iipatch_status = EXIT_iipatch
$
$   RETURN



$!LOAD_PATCH:
$! Unload the patch from the specified saveset into the IIPATCH area.
$
$load_patch:
$   ON ERROR     THEN GOTO load_patch_err
$   ON CONTROL_Y THEN GOTO load_patch_ctrly
$
$   say "Unpacking Saveset into Temporary Working area..."
$
$   BACKUP/NOLOG 'saveset'/SAVE/SELECT=[INGRES...]*.*; iipatch:[ingres...]*.*;'load_patch_version'
$
$   IF F$SEARCH("iipatch:[ingres]version.rel") .NES. ""
$   THEN
$     OPEN/READ  open_file iipatch:[ingres]version.rel
$     file_open = 1
$     READ/ERROR=GET_VERS_ERROR_P/END_OF_FILE=GET_VERS_EOF_P open_file ingres_version
$     READ/ERROR=GET_PATCH_ERROR_P/END_OF_FILE=GET_PATCH_EOF_P open_file patch_level
$    GOTO GET_VERS_END_P
$GET_VERS_ERROR_P:
$GET_VERS_EOF_P:
$     ingres_version="unknown"
$GET_PATCH_ERROR_P:
$GET_PATCH_EOF_P:
$     patch_level=""
$GET_VERS_END_P:
$     CLOSE open_file
$     file_open = 0
$   ELSE
$
$     OPEN/READ  open_file II_SYSTEM:[ingres]version.rel
$     file_open = 1
$     READ/ERROR=GET_VERS_ERROR/END_OF_FILE=GET_VERS_EOF open_file ingres_version
$    GOTO GET_VERS_END
$GET_VERS_ERROR:
$GET_VERS_EOF:
$     ingres_version="unknown"
$     patch_level=""
$GET_VERS_END:
$     patch_level = ""
$     CLOSE open_file
$     file_open = 0
$
$   ENDIF
$
$   RETURN
$
$load_patch_err:
$   err_stat = $status
$
$   error_reason = "Unload Patch failed:"
$
$   GOSUB report_error
$
$   iipatch_status = EXIT_iipatch
$
$   IF file_open .EQ. 1 THEN CLOSE open_file
$
$   RETURN
$
$load_patch_ctrly:
$   ctrly_reason = "Aborting Unload Patch..."
$
$   GOSUB report_ctrly
$
$   iipatch_status = EXIT_iipatch
$
$   IF file_open .EQ. 1 THEN CLOSE open_file
$
$   RETURN


$!Make a IIPATCH.DIR in the specified working area.
$!
$make_tmpdir:
$   ON CONTROL_Y THEN GOTO make_tmpdir_ctrly
$   ON ERROR     THEN GOTO make_tmpdir_err
$
$   patch_dir = tmpdir - "]" + ".IIPATCH"
$
$   DEFINE/NOLOG/TRAN=CONC iipatch 'patch_dir'.]
$
$   IF F$SEARCH("''tmpdir'iipatch.dir") .NES. ""
$   THEN
$      say "IIPATCH.DIR exists in ''tmpdir'"
$
$      CALL get_input "Do you wish to purge this directory and Continue ?" "YN"
$
$      IF iipatch_input_status .NE. 0
$      THEN
$         GOTO make_tmpdir_ctrly
$      ENDIF
$         
$      IF "Y" - iipatch_ans .EQS. "Y"
$      THEN
$         say "Install aborted. IIPATCH area must be purged."
$
$         iipatch_status = EXIT_nocleanup
$         RETURN
$      ENDIF
$
$      GOSUB delete_iipatch_files
$      IF iipatch_status .NE. 0 THEN RETURN
$
$      CREATE/DIR iipatch:[tmp]
$   ELSE
$      CREATE/DIR 'patch_dir'.tmp]
$   ENDIF
$
$   RETURN
$
$make_tmpdir_ctrly:
$   ctrly_reason = "Aborted while making IIPATCH working area"
$
$   GOSUB report_ctrly
$
$   iipatch_status = EXIT_nocleanup
$
$   RETURN
$
$make_tmpdir_err:
$   err_stat = $status
$
$   error_reason = "ERROR while making IIPATCH working area"
$
$   GOSUB report_error
$
$   iipatch_status = EXIT_nocleanup
$
$   RETURN

$param_type_N:
$   Nobackup = 1
$
$   RETURN

$param_type_P:
$   Purge_req = 1
$
$   RETURN

$param_type_T:
$   directory = param
$
$   GOSUB check_directory
$
$   IF iipatch_status .NE. 0 THEN RETURN
$
$   tmpdir = directory
$
$   RETURN

$param_type_U:
$   directory = param
$
$   GOSUB check_directory
$
$   IF iipatch_status .NE. 0 THEN RETURN
$
$   backdir = directory
$
$   RETURN


$param_type_WHAT:
$   say "Unrecognised Option [''user_param']"
$
$   iipatch_status = EXIT_nocleanup
$
$   RETURN



$perform_install:
$   i = 0
$
$next_install_step:
$   next_step = F$ELEMENT(i, ",", steps)
$
$   IF "''next_step'" .EQS. "," THEN GOTO install_finished
$
$   GOSUB 'next_step
$
$   IF iipatch_status .NE. 0 THEN GOTO install_finished
$
$   i = i + 1
$
$   GOTO next_install_step


$purge_files:
$   say "Purging files..."
$   @IIPATCH:[TMP]patch_purge.com
$
$   RETURN

$purge_prepare:
$   ON CONTROL_Y THEN GOTO purge_prepare_ctrly
$   ON ERROR     THEN GOTO purge_prepare_err
$
$   OPEN/WRITE file IIPATCH:[TMP]patch_purge.com
$   file_open = 1
$
$purge_prepare_loop:
$   file_name = F$SEARCH( "II_SYSTEM:[INGRES...]*.*;''install_version'")
$
$   IF file_name .NES. ""
$   THEN
$      WRITE file F$FAO( "$ PURGE !AS", F$ELEMENT(0, ";", file_name))
$
$      GOTO purge_prepare_loop
$   ENDIF
$
$   CLOSE file
$   file_open = 0
$
$   RETURN

$purge_prepare_ctrly:
$ ctrly_reason = "Abort while creating list of files to PURGE. Purge aborted"
$
$ GOSUB report_ctrly
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ purge_req = 0
$
$ IF F$SEARCH( "IIPATCH:[TMP]patch_purge.com") .NES. ""
$ THEN
$    DELETE IIPATCH:[TMP]patch_purge.com;*
$ ENDIF
$
$ RETURN
$
$purge_prepare_err:
$ err_stat = $status
$
$ error_reason = "Error while creating list of files to PURGE. Purge aborted"
$
$ GOSUB report_error
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ purge_req = 0
$
$ IF F$SEARCH( "IIPATCH:[TMP]patch_purge.com") .NES. ""
$ THEN
$    DELETE IIPATCH:[TMP]patch_purge.com;*
$ ENDIF
$
$ RETURN

$report_ctrly:
$   say "''ctrly_reason'"
$   say " "
$
$   RETURN


$report_error:
$   say "''error_reason'"
$   say F$MESSAGE(err_stat)
$   say " "
$
$   RETURN


$setup:
$   say := WRITE SYS$OUTPUT
$
$   iipatch_status = 0
$   file_open      = 0
$   mandir         = ""
$
$   EXIT_nocleanup = 1
$   EXIT_iipatch   = 2  ! Exit and cleanup/delete IIPATCH
$
$   IF F$SEARCH( "ii_system:[ingres.utility]iigetres.exe") .EQS. ""
$   THEN
$      say "II_SYSTEM is undefined, or does not reference a valid Ingres installation"
$
$      EXIT
$   ENDIF
$
$   iigetres      := $ii_system:[ingres.utility]iigetres.exe
$
$   node_name      = F$GETSYI("NODENAME")
$
$   say "Obtaining Ingres Installation ID..."
$   iigetres ii.'node_name'.lnm.ii_installation ii_installation_value
$
$   ii_installation = F$TRNLNM("ii_installation_value")
$
$   tmpdir  = F$PARSE("SYS$SCRATCH",,,,"NO_CONCEAL") - "][" - ".;"
$   backdir = "II_SYSTEM:[INGRES]"
$
$   products = ""
$   backup_version  = 32766
$   install_version = 32767
$
$   iipatch_install_version = install_version
$
$   nobackup = 0
$   purge_req = 0
$
$   install_steps = "check_params,check_inst,make_tmpdir,load_patch,check_install_patch,"
$   install_steps = install_steps + "files_needed,check_checksums,backup_inst,stop_ingres,load_files,check_release,start_ingres"
$
$   uninstall_steps = "check_params,check_inst,make_tmpdir,load_patch,check_uninstall_patch,check_checksums,"
$   uninstall_steps = uninstall_steps + "stop_ingres,load_files,start_ingres"
$
$   valid_pkgs   = "#BRIDGE#C2AUDIT#DAS#DBMS#DOCUMENTATION#ESQL#JDBC#NET#ODBC#OLDMSG#OME#ORACLE#"
$   valid_pkgs   = valid_pkgs + "QR_RUN#QR_TOOLS#REP#RMS#SPATIAL#STAR#TM#VISPRO#"
$   relid_pkgs   = "#BRIDGE#C2AUDIT#DBMS#JDBC#NET#ORACLE#RMS#STAR#"
$   pkg_BRIDGE   = "[INGRES.BIN]IIGCB.EXE"
$   pkg_C2AUDIT  = "[INGRES.UTILITY]IIAUDLOC.EXE"
$   pkg_DAS      = "[INGRES.UTILITY]IICPYDAS.EXE"
$   pkg_DBMS     = "[INGRES.BIN]IIDBMS.EXE"
$   pkg_DOCUMENTATION = "[INGRES.FILES.ENGLISH]CMDREF.PDF"
$   pkg_ESQL     = "[INGRES.LIBRARY]FRONTMAIN.OBJ"
$   pkg_JDBC     = "[INGRES.BIN]IIJDBC.EXE"
$   pkg_NET      = "[INGRES.BIN]IIGCC.EXE
$   pkg_ODBC     = "[INGRES.BIN]IIODBCADMN.EXE"
$   pkg_OLDMSG   = "[INGRES.FILES.ENGLISH]FAST.MNX"
$   pkg_OME      = "[INGRES.DEMO.UDADTS]COMMON.C
$   pkg_ORACLE   = "[INGRES.BIN]IIGWORAD.EXE"
$   pkg_QR_RUN   = "[INGRES.BIN]INGMENU.EXE"
$   pkg_QR_TOOLS = "[INGRES.BIN]COPYFORM.EXE"
$   pkg_REP      = "[INGRES.BIN]REPMGR.EXE"
$   pkg_RMS      = "[INGRES.BIN]IIRMS.EXE"
$   pkg_SPATIAL  = "[INGRES.FILES]SPATIAL.H"
$   pkg_STAR     = "[INGRES.FILES]STAR.CRS"
$   pkg_TM       = "[INGRES.BIN]TM.EXE
$   pkg_VISPRO   = "[INGRES.BIN]IIABF.EXE"
$
$   always_copy = "[INGRES]PATCH.HTML,[INGRES]VERSION.REL,[INGRES.INSTALL]RELEASE.DAT"
$
$   inst_file_rename = F$FAO( "!AS,!AS,!AS,!AS,!AS,!AS,!AS", -
              "[INGRES.BIN]DMFJSP.EXE", -
              "[INGRES.LIBRARY]APIFELIB.EXE",    "[INGRES.LIBRARY]CLFELIB.EXE", -
              "[INGRES.LIBRARY]FRAMEFELIB.EXE",  "[INGRES.LIBRARY]IIUSERADT.EXE", -
              "[INGRES.LIBRARY]INTERPFELIB.EXE", "[INGRES.LIBRARY]LIBQFELIB.EXE" )
$
$   lang_file_move = F$FAO( "!AS,!AS", "FAST_V4.MNX", "SLOW_V4.MNX")
$
$   RETURN


$!START_INGRES:
$! Ask the user if we should start Ingres.
$!
$start_ingres:
$  ON CONTROL_Y THEN GOTO start_ingres_ctrly
$  ON ERROR     THEN GOTO start_ingres_err
$
$  CALL get_input "Do you wish to start Ingres processes ? [YN]" "YN"
$
$  IF iipatch_input_status .NE. 0
$  THEN
$     GOTO start_ingres_ctrly
$  ENDIF
$
$  IF "Y" - iipatch_ans .NES. "Y"
$  THEN
$     ingstart
$  ENDIF
$
$  RETURN
$
$start_ingres_ctrly:
$ ctrly_reason = "Abort. Ingres will not be started"
$
$ GOSUB report_ctrly
$
$ RETURN
$
$start_ingres_err:
$ err_stat = $status
$
$ error_reason = "Error. Ingres will not be started"
$
$ GOSUB report_error
$
$ RETURN


$!STOP_INGRES:
$!  Check that Ingres is not running. If Ingres is running, then need to INGSTOP
$!  it. But better check with user first.
$!
$!
$! Ingres process name formats:
$!
$!      DMFACP		DMPACPii
$!      II_GCN		II_GCN_ii
$!	II_abcd_xxxx	II_abcd_ii_xxxx
$!	RMCMD_xxxx	RMCMD_ii_xxxx
$!
$! Where "abcd" is string of (upto 4) characters (e.g. GCC, DBMS, IUSV)
$!       "ii"   is the installation ID (if non-system installation).
$!	 "xxxx" is the least 4 hex digits of the Process ID
$!
$! Ignore II_PC_xxxxx as this refers to processes created by FE applications
$! which have created a subprocess; not an Ingres server.
$!	
$stop_ingres:
$   ON CONTROL_Y THEN GOTO stop_ingres_ctrly
$   ON ERROR     THEN GOTO stop_ingres_err
$
$   IF ii_installation .EQS. ""
$   THEN
$      un_pr = ""
$   ELSE
$      un_pr = "_''ii_installation'"
$   ENDIF
$
$   sea_str = F$FAO(""" DMFACP!AS "", "" II_"", "" RMCMD!AS_""",-
                    ii_installation, un_pr)
$
$   SHOW SYSTEM /OUT=IIPATCH:[TMP]SYS.DAT
$
$   SORT/KEY=(POS:10,SIZE:16) IIPATCH:[TMP]SYS.DAT IIPATCH:[TMP]SYS.DAT
$
$   SEARCH/OUT=IIPATCH:[TMP]INGRES.PROCS/NOHEAD/NOWARN IIPATCH:[TMP]SYS.DAT 'sea_str'
$
$   IF $status .ne. 1 THEN RETURN
$
$   OPEN/READ file IIPATCH:[TMP]INGRES.PROCS
$   file_open = 1
$
$ingres_proc:
$   READ/END_OF_FILE=ingres_proc_fin file record
$
$   proc_name = F$EDIT(F$ELEMENT(1, " ", record), "UPCASE")
$
$   IF proc_name - "II_PC_" .NES. proc_name
$   THEN
$      GOTO ingres_proc
$   ENDIF
$
$   IF proc_name - "DMFACP" .NES. proc_name
$   THEN
$      IF (proc_name .EQS. "DMFACP''ii_installation'") THEN GOTO ingres_running
$
$      GOTO ingres_proc
$   ENDIF
$
$   IF proc_name - "II_GCN" .NES. proc_name
$   THEN
$      IF (proc_name .EQS. "II_GCN''un_pr'") THEN GOTO ingres_running
$
$      GOTO ingres_proc
$   ENDIF
$
$   IF ii_installation .EQS. ""
$   THEN
$      proc_name = proc_name - "II_" - "_"
$
$      IF proc_name - "_" .EQS. proc_name THEN GOTO ingres_running
$   ELSE
$      proc_name = proc_name - "II_"
$
$      IF proc_name - "_''ii_installation'_" .NES. proc_name THEN GOTO ingres_running
$   ENDIF
$
$   GOTO ingres_proc
$
$ingres_running:
$ CLOSE file
$ file_open = 0
$
$ ON CONTROL_Y THEN GOTO ingres_running_ctrly
$
$ CALL get_input "Ingres Installation is running. Enter Y to stop, N to abort PATCH Installation" "YN"
$
$ IF iipatch_input_status .NE. 0
$ THEN
$    GOTO ingres_running_ctrly
$ ENDIF
$
$ IF "N" - iipatch_ans .NES. "N"
$ THEN
$    iipatch_status = EXIT_iipatch
$    RETURN
$ ENDIF
$
$ INGSTOP
$
$ GOTO stop_ingres
$
$ingres_proc_fin:
$
$ CLOSE file
$ file_open = 0
$
$ RETURN
$
$ingres_running_ctrly:
$ ctrly_reason = "Ingstop Aborted. Patch Installation cancelled."
$
$ GOSUB report_ctrly
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
$
$stop_ingres_ctrly:
$ ctrly_reason = "Abort while checking if Ingres running"
$
$ GOSUB report_ctrly
$
$ iipatch_status = EXIT_iipatch
$
$ IF file_open .EQ. 1 THEN CLOSE file
$
$ RETURN
$
$stop_ingres_err:
$ err_stat = $status
$
$ error_reason = "Error while checking if Ingres running"
$
$ GOSUB report_error
$
$ iipatch_status = EXIT_iipatch
$
$ RETURN
