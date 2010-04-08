$!
$ goto _START_ADBTOFST
$!
$! Copyright (c) 2008 Ingres Corporation
$!
$! Name: adbtofst.dcl  -  pipes auditdb output direcly to fastload.
$!
$! Description:
$!
$! This is the wrapper to enable output from the auditdb
$! to be able to be piped to the fastload utility for
$! copying into ingres tables.
$!
$! This is equivalent to the following command.
$!
$! auditdb -table=<db table> -file=- <audited dbname> |
$!                      fastload <fload dbname> -table=<fst table> -file=-
$!
$! usage : adbtofst [-uusername] <fload_table> <fload_dbname> <db_table>
$!                      <audited_database> [other_auditdb_parameters]
$!
$!     [-uusername]: optional "-uusername" is specified for user id.
$!                   should be same for both fastload and audited database.
$!     <fload_table>: non journalled table into which fastloading.
$!    <fload_dbname>: fastload database name
$!        <db_table>: journaled table name on which we are running auditdb.
$!  <audited_dbname>: journaled database name
$!  [other_auditdb_parameters] : optional other auditdb parameters like
$!                              -abort_transactions, -internal_data.. etc.
$!
$!! History:
$!!     22-jan-2008 (upake01) b115383
$!!        Created from adbtofst.sh for VMS.
$!!     15-aug-2008 (upake01) b115383
$!!        Change "-file" to "-file=-" for "-internal_data" option
$!!            of "auditdb" to write lsnhigh, lsnlow and tidno.
$!!        Change the appended string for optional auditdb parameters to
$!!            lowercase as "-INTERNAL_DATA" and "-ABORTED_TRANSACTIONS"
$!!            give syntax errors.
$!!        Modify to have only one "pipe" command for both - username
$!!            specified with a "-u" or not specified.
$!!        Change the "pipe" command from ";" to "&&" to not execute
$!!            "fastload" command in case there are errors with the
$!!            "auditdb" command.
$!!
$!
$ _START_ADBTOFST:
$!================
$!
$ self = "ADBTOFST"
$ echo  := write sys$output
$ params := call _set_params
$ usage := call _usage
$ addparam := call _addparam
$ msg = F$ENVIRONMENT( "MESSAGE" )
$!
$ FALSE = 0
$ TRUE = 1
$ on CONTROL_Y then goto _EXIT_FAIL
$!
$ userset = FALSE
$ if (f$length(P1) .ge. 2)
$ then
$    param = P1
$    if (param - "-u" .nes. param) .OR. (param - "-U" .nes. param) THEN userset = TRUE
$ endif
$!
$ if (userset .eq. TRUE)
$ then
$    if ( (p2 .eqs. "") .or. (p3 .eqs. "") .or. (p4 .eqs. "") .or. (p5 .eqs. "") )
$    then
$       usage "''self'" "''p1'" "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$       goto _EXIT_FAIL
$    endif
$ else
$    if ( (p1 .eqs. "") .or. (p2 .eqs. "") .or. (p3 .eqs. "") .or. (p4 .eqs. "") )
$    then
$       usage "''self'" "''p1'" "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$       goto _EXIT_FAIL
$    endif
$ endif
$!
$ on error then goto _EXIT_FAIL
$!
$!! Copy parameters P1 to P8 into P_1 to P_8 changing case (if necessary) as follows:
$ cnt = 1
$param_loop:
$   IF cnt .LE. 8
$   THEN
$      param = P'cnt'
$      IF param .EQS. "" .OR. param - """" .NES. param .OR. -
          param - "-" .NES. param
$      THEN
$         P_'cnt' = F$EDIT(param, "LOWERCASE") - """" - """"
$      ELSE
$         IF F$EDIT( param, "UPCASE" ) .EQS. param
$         THEN
$            param = F$EDIT( param, "LOWERCASE" )
$         ENDIF
$         P_'cnt' = param
$      ENDIF
$      cnt = cnt + 1
$      GOTO param_loop
$    ENDIF
$!
$!
$ added_par == ""
$ if (userset .eq. TRUE)
$ then
$    username  = "''P_1'"
$    fastldtab = "''P_2'"
$    dbname    = "''P_3'"
$    audittab1 = "''P_4'"
$ else
$    username  = ""
$    fastldtab = "''P_1'"
$    dbname    = "''P_2'"
$    audittab1 = "''P_3'"
$    addparam  "''P_4'"
$ endif
$!! append auditdb parameters into "added_par" symbol
$ addparam  "''P_5'"
$ addparam  "''P_6'"
$ addparam  "''P_7'"
$ addparam  "''P_8'"
$!
$ auditeddb = f$edit( added_par, "compress, trim" )
$!
$!
$ audittab2=f$element(1,"_",audittab1)      !   Extract table name from "IJA_table_number"
$ if (audittab2 .eqs. "_")
$ then
$    audittab=audittab1
$ else
$    audittab=audittab2
$ endif
$!
$!Set output filename to table name (i.e. extract table name from "owner.tablename")
$ filename=f$element(1,".",audittab)
$ if (filename .eqs. ".")
$ then
$    filename=audittab
$ endif
$!
$ auditdb_str = "auditdb   -table=''audittab'  -file=-   "
$!
$!
$ SET MESSAGE/NOFACILITY/NOIDENTIFICATION/NOSEVERITY/NOTEXT
$ PIPE   'auditdb_str'   'username'   'auditeddb'   >   'filename'.trl   &&   (fastload   'username'   'dbname'   -file='filename'.trl   -table='fastldtab')
$!
$ goto _EXIT_OK
$!
$!-------------------------------------------------------------------
$!
$! Exit labels
$!
$ _EXIT_FAIL:
$!===========
$!
$ echo "ADBTOFST aborted."
$ goto _EXIT_OK
$!
$!
$!
$ _EXIT_OK:
$!=========
$!
$! This is the only clean exit point in the routine.
$!
$!! Delete the "trl" file
$ if f$search("''filename'.trl") .nes. "" then -
$    delete/noconfirm/nolog   'filename'.trl;*
$!
$ SET MESSAGE 'msg'
$ exit
$!
$!------------------------------------------------------------------------
$!
$!
$!!  Complain about bad input.
$!
$ _usage: SUBROUTINE
$!==================
$!
$ echo  "ADBTOFST - Error! Incorrect Parameters"
$!
$ echo "Invalid Parameter  ''p1' ''p2' ''p3' ''p4' ''p5' ''p6' ''p7' ''p8'."
$!
$ echo  "  "
$ echo  "ADBTOFST - error! incorrect parameters"
$ echo  "usage : adbtofst [-uusername] fload_table fload_dbname db_table"
$ echo  "                 audited_database [other_auditdb_parameters]"
$ echo  "  "
$ echo  "  [-uusername]: optional ""-uusername"" is specified for user id."
$ echo  "         should be same for both fastload and audited database."
$ echo  "  "
$ echo  "  fastld_table: non journalled table into which fastloading."
$ echo  "  fastld_db: name of the fastload database."
$ echo  "  "
$ echo  "  audited_table: journaled table name on which we are running auditdb."
$ echo  "  audited_db: journaled database name."
$ echo  "  [auditdb parameters]: other auditdb parameters if any."
$ echo  "  "
$!
$ exit
$ ENDSUBROUTINE
$!
$!
$!-----------------------------------------------------------------------------
$!
$!! Add parameters - The adbtofst command may have additional auditdb parameters
$!!     at the end of the command.  All these parameters need to be packed into
$!!     one symbol and retaining the case.  This subroutine will pack (append)
$!!     each parameter into global symbol "added_par".
$!
$ _addparam: SUBROUTINE
$!=====================
$ IF P1 .NES. "" THEN added_par == added_par + " " + "''P1'"
$!
$ exit
$ ENDSUBROUTINE
