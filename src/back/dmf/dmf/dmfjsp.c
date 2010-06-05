/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
**
*/
#include    <compat.h>
#include    <gl.h>
#include    <id.h>
#include    <tm.h>
#include    <cv.h>
#include    <pc.h>
#include    <cm.h>
#include    <st.h>
#include    <nm.h>
#include    <cs.h>
#include    <ck.h>
#include    <jf.h>
#include    <tr.h>
#include    <si.h>
#include    <lo.h>
#include    <bt.h>
#include    <er.h>
#include    <ex.h>
#include    <pm.h>
#include    <me.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <cui.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dm0m.h>
#include    <sxf.h>
#include    <ulx.h>
#include    <dma.h>
#include    <dmxe.h>
#include    <dmpp.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2r.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm0j.h>
#include    <dmfjsp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0s.h>
#include    <dmfinit.h>
#include    <dm0d.h>
#include    <dm0llctx.h>
#include    <lgclustr.h>

/*
**
**  Name: DMFJSP.C - DMF Journaling Support Program.
**
**  Description:
**
**	Function prototypes in DMFJSP.H.
**
**      This file contains the main program that implements the journaling
**	support routines required by DMF.  These routines include: auditing
**	journal files, checkpointing databases, rolling forward a destroyed
**	database using checkpoints and journal files.
**
**          main - Main program of the JSP.
**
**	Internal routines.
**
**	    parse_command_line - Parse the command line.
**	    allocate_dcb - Allocate a DCB.
**	    search_dcb - Search for required DCB's.
**	    audit_jsp_action - audit a user action with dma_write_audit()
**
**  History:
**      02-nov-1986 (Derek)
**          Created for Jupiter.
**	30-sep-1986 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	24-oct-1988 (greg)
**	    Must check the return status of LOingpath().  Should only
**	    fail if the installation is hosed but results are disasterous.
**	21-jan-1989 (Edhsu)
**	    Add code to support online backup
**      22-jun-89 (jennifer)
**          Added in a new role for a user named an Operator denoted
**          by DU_UOPERATOR in iiuser.  An operator can backup any
**          database.  This is all that he can do, unless a DBA allows
**          him access to a database indicated by the iidbaccess table.
**      07-aug-1989 (fred)
**          Added DYNLIBS MING hint for user defined datatype support
**	26-sep-1989 (cal)
**	    Update MODE, PROGRAM, and DEST ming hints for user defined
**	    datatypes.
**	13-oct-1989 (rogerk)
**	    Made -j/+j flags to checkpoint imply the -l flag as well.
**	 1-dec-1989 (rogerk)
**	    Added #w flag - this is used for online checkpoint testing.  It
**	    causes the checkpoint to stall in the backup phase.
**	 6-dec-1989 (rogerk)
**	    Made CHECKPOINT default mode be OFFLINE.
**	    Added -o flag to force online checkpoint.
**	    This is a temporary change until bugs in online checkpoint are
**	    fixed.
**	 7-dec-1989 (rogerk)
**	    Took out above patch to turn off ONLINE CHECKPOINT.
**	26-dec-1989 (greg)
**	    Don't use obsolete DB arg to LOingpath
**	    Add II_DMF_MERGE ifdef for Unix guys to use
**	14-may-1990 (rogerk)
**	    Changed STRUCT_ASSIGN_MACRO's of different typed objects in
**	    search_dcb to MEcopy's.
**	21-may-1990 (bryanp)
**	    Added the new, undocumented (for now) '#c<ckpno>' flag for
**	    rollforward. This flag is used to request a specific checkpoint
**	    sequence number to be used as the 'rollback point' for recovery.
**	    If #c is ommitted, the current checkpoint is used. If #c is given
**	    with a number, the checkpoint with that sequence number is used.
**	    If #c is given without a number, the last valid checkpoint is used.
**	    Also, in support of the ability to recover a database when the
**	    config file has been lost, changed the DCB building code to build
**	    the database's dump location as well as its data location. The
**	    dump location may be needed by rollforward if the config file
**	    cannot be found (we keep a backup copy of the config file in the
**	    dump location and rollforward can use that copy if it is given
**	    the dump location).
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	30-oct-1990 (bryanp)
**	    Bug #34120: jsp utilities don't work right on distributed database.
**	    A distributed database (du_dbservice & DU_1SER_DDB) has neither
**	    a database directory nor a config file, so the jsp utilities cannot
**	    process it correctly. Changed search_dcb to skip such databases
**	    when making a list of all databases you own, and to reject any
**	    attempt to explicitly name such a database. Also fixed a few
**	    error message problems in the search_dcb code so that it could give
**	    better error messages.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    10-jan-1991 (stevet)
**		Added CMset_attr call to initialize CM attribute table.
**	    25-feb-1991 (rogerk)
**		Added support for the Archiver Stability project.
**		Added alterdb command line option with -disable_journal flag.
**		Added JSX_O_ALTERDB and JSX_JDISABLE to correspond to above.
**		Added JSX_NOLOGGING option to specify that operations should
**		not be logged during command processing.  This is turned on
**		for the disable_journaling option of alterdb.
**	    29-may-1991 (bryanp)
**		B37799: infodb bry1 bry2 loops because database list is broken.
**	    11-jun-1991 (bryanp)
**		B37942: Check dmf_init status if no database names given.
**	    12-aug-1991 (bryanp)
**		B39106: Don't need to open iidbdb when rolling forward iidbdb.
**	    12-aug-91 (stevet)
**		Change read-in character set support to use II_CHARSET symbol.
**	17-oct-1991 (mikem)
**	    Eliminated unix compile warnings ("warning: statement not reached")
**	    changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**	20-dec-1991 (bryanp) Integrated: 14-aug-91 (markg)
**	    Added start and end date checking to parse_command_line() for
**	    auditdb. Bug 39150.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	03-nov-1992 (robf)
**	    Update security auditing to use SXF sessions.
**	    Since  a session is database-specific, we use a session per
**	    database, plus one for the initial search for dcb's in iidbdb
**	16-nov-1992 (bryanp)
**	    Fix some argument passing problems (chg by value to by reference).
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	04-feb-93 (jnash)
**	    Reduced logging project.
**	      - Cast MEcmp operators to eliminate compiler warnings.
**	      - Add support for AUDITDB #a.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added #norollback flag to bypass undo phase of rollforward.
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include dmfinit.h to pick up function prototyes.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Modified calling sequence to dmf_init to make more explicit the
**		    control over buffer manager allocation and UDT lock values.
**	26-apr-93 (jnash)
**	    Add support for:
**		AUDITDB -table, -file
**	  	ALTERDB -target_jnl_blocks
**		ALTERDB -jnl_block_size
**		ALTERDB -init_jnl_blocks
**		change ROLLFORWARD #nologging and -nologging
**	    Also, add LRC requested changes:
**		change ROLLFORWARD #norollback and -norollback
**		change AUDITDB #a to -all
**		change AUDITDB #v to -verbose
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Added csp_lock_list argument to dmf_init()
**	26-jul-1993 (jnash)
**	    Add support for auditdb -inconsistent to open inconsistent dbs.
**	    Add support for auditdb -wait to wait for archiver to purge up to
**		current log file eof.
**	    Add -help usage.
**	23-aug-1993 (jnash)
**	    Fix AV introduced by -help when program has no arguments.
**          Add -all to auditdb -help.
**	20-sep-1993 (jnash)
**	    Delimited Ids:
**	     - Add support for delimited auditdb table names and '-i' flags,
**	       as well as the '-u' flag used by several utilities.
**	     - Restrict auditdb, alterdb, rollforwarddb and ckpdb to operating
**	       on a single database.
**	    ALTERDB -delete_oldest_ckp.
**	     - Add command line syntax.
**	     - Move file_maint() from dmfcpp.c to this file, rename it
**	       to jsp_file_maint().
**	    All utilities: -verbose and -v are equivalent.
**	1-oct-93 (robf)
**	    Replaced dmxe_ calls by wrappers
**	18-oct-1993 (jnash)
**	    Issue ALTERDB completion message even if no dump files deleted.
**	    Perform additional param checking.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dm0d.h>.
**	25-dec-1993 (rogerk)
**	    Added exception handler (copied from DMFACP.C).
**	    Added forward declaration of static parse_table_list.
**	    Cleaned up DCB memory allocated in search_dcb for iidbdb open.
**	31-jan-1994 (jnash)
**	    During rollforwarddb #f, set JSX_FORCE instead of JSX_JDISABLE.
**	    This now causes rollforward to "force" a recovery in a number of
**	    circumstances, not just when the database is journal disabled.
**	15-feb-1994 (andyw)
**	    Added new error message E_DM1051_JSP_NO_INSTALL displays a
**	    more appropriate message if the installation is not running
**	    Bug 55193
**	15-feb-1994 (andyw)
**	    Added new error message E_DM1052_JSP_VALID_USAGE is displayed
**	    if an invalid command argument is specified. Shows usage
**	    Bug 56789
**	17-feb-94 (stephenb)
**	    Fix audit call in search_dcb() and add additionl call for
**	    user action. Also move dmf_sxf_bgn_session() from main() into
**	    search_dcb(). Add routine audit_jsp_action().
**	21-feb-1994 (jnash)
**	  - Add support for -start_lsn and -end_lsn auditdb and rollforwarddb
**	    parameters.
**	  - Add support for auditdb #c.  Unlike rolldb, a checkpoint sequence
**	    number is required.
**	  - Remove error generated on rollforwarddb #c -c requests.
**	24-feb-1994 (jnash)
**	    Remove requirement for #f on rolldb +j -c requests.
**	    This applies only to rolldb -b and -start_lsn.
**	9-mar-94 (stephenb)
**	    add dbname to dma_write_audit() calls and move dmf_sxf_bgn_session()
**	    further up search_dcb to cope with additional B1 audits.
**      28-mar-1993 (jnash)
**          Initialize uninitialized variable that was causing
**	    auditdb -table to fail.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	21-apr-1994 (rogerk)
**	    Changed dm2r_get calls to explicitly request DM2R_GETNEXT mode
**	    rather than implicitly expect it by passing a zero get mode.
**	23-may-1994 (bryanp) B58498
**	    If an EXINTR signal is received, notify LG and LK, then
**		continue. This allows LG and LK to hold off the ^c while
**		under the mutex, then to respond to it once the mutex has
**		been released.
**	23-may-1994 (jnash)
** 	    If error encountered in jsp_get_case() during rolldb of iidbdb,
**	    set default case semantics to mixed and continue.  This fixes
**	    a problem where rolldb of iidbdb can't be performed if
**	    iidbdepends inaccessible.
**	24-jun-1994 (jnash)
** 	    When rolling forward the iidbdb:
**	     - Unconditionally hand build the iidbdb DCB.  We can't rely on
**	       any catalog info.
**	     - Write an sxf audit log
**	     - Check the operator priv against SERVER CONTROL in config.dat.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Added the following command line flags to rollforwarddb:
**		    -table=
**		    -nosecondary_index
**		    -on_error_continue
**		    -online
**		    -status_rec_count
**		    -noblobs
**	   	    -force_logical_consistent
**		    -Default rollforward level is database
**      18-nov-1994 (alison)
**          Database Relocation Project:
**              Added the following command line flags to rollforwarddb:
**                  -relocate
**                  -new_ckp_location=
**                  -new_dump_location=
**                  -new_work_location=
**                  -new_jnl_location=
**                  -new_location=
**                  -new_database=
**                  -location=
**              Added the routine parse_loc_list()
**      04-jan-1995 (alison)
**          Added JSX_O_RELOC to remove pure relocate functions from RFP.
**          parse_command_line() added/corrected checking for reloc params
**               comment out -status_rec_count, -force_logical_consistent
**          jsp_echo_usage() print usage for JSX_O_RELOC.
**               comment out -status_rec_count, -force_logical_consistent
**               and -online.
**               added -statistics
**          audit_jsp_action() added auditing for JSX_O_RELOC
**      05-dec-1994 (sarjo01)
**          Ensure that we have an arg for '-m' option - bug 51298
**          Cross integration of 19-sep-1994 (nick)
**	28-dec-1994 (shero03)
**	    Disable -online for now. B66007.
**	06-jan-1995 (shero03)
**	    Support Partial Checkpoint
**      23-jan-1995 (stial01)
**          BUG 66430: parse_command_line(): -d illegal for table checkpoint,
**          It would leave the db without a full checkpoint.
**	21-feb-1995 (shero03)
**	    Bug #67004, #67005
**	    Ensure that -relocate -new_location= -location= 
**	    -on_eror_continue.
**	    only happens for table level actions.
**      07-mar-1995 (stial01)
**          Bug 67329: jsp_file_maint(): there may not be a ckp file for each
**          data location, even if one doesn't exist, continue to delete others
**	20-mar-1995 (shero03)
**	    Bug #67437
**	    Ensure that invalid flag prefixes are not mistaken 
**	    for dbnames.
**	10-apr-95 (angusm)
**          Add 'infodb1' for "DBALERT sponsors for INGRES " project.
**      05-apr-1995 (stial01)
**          jsp_echo_usage() fixed help for relocatedb
**      11-may-1995 (harpa06)
**          In RELOCATEDB, if "-location" is specified, check to see if
**          "-new_location" is also specified. -- bug 68602
**      12-may-1995 (harpa06)
**          Removed E_DM1052_JSP_VALID_USAGE from being displayed if a command
**          line argument is specified. It makes no sense to the user. 
**          Especially if they are using the ROLLFORWARDDB or RELOCATEDB
**          scripts to rollforward a database or relocate database locations.
**      29-jun-1995 (thaju02)
**	    Modified parse_command_line routine.
**          Added new ckpdb parameter '-max_stall_time=xx' where stall
**          time is specified in seconds.  Store the stall time in
**          DMF_JSX structure (jsx_stall_time) and the stall time
**          specified flag (JSX1_STALL_TIME) in jsx_status1.
**      30-jun-1995 (shero03)
**          Added -ignore for rollforwarddb
**	12-july-1995 (thaju02)
**	    Modified parse_command_line routine.  Added call to STindex
**	    for parsing of the max stall time, and added error checking of
**	    max_stall_time value.  Stall time must be of the format
**	    mm:ss.
**	10-aug-1995 (sarjo01)
**          Added fastload command support in main() and parse_command_line().
**	
**      13-july-1995 (thaju02)
**          Modified the parsing and validity checking of the timeout
**          value specified on command line.
**          bug #69979 Modified '-max_stall_time' argument to
**          '-timeout'.  If user specified the max_stall_time
**          argument misspelled, ckpdb will attempt to checkpoint
**          to a tape device.
**          bug #69981 - added the initialization of
**          jsx->jsx_ignored_errors during parsing of -ignore arg.
**	21-aug-1995 (harpa06)
**	    Bug #70655 - It is possible that the checkpoint file where the 
**	    database tables are stored is in fact a directory. If so, delete the
**	    files in the directory and then remove the directory.
**      11-sep-1995 (thaju02)
**	    Support for viewing contents of checkpoint table list file.
**	 6-feb-1996 (nick)
**	    Add suite of functions to support the new table list file - the
**	    existing code was broken and in diverse files.
**      13-sep-1995 (tchdi01)
**          Added support for the -production=(on|off) option for alterdb
**	13-feb-1996 (canor01)
**	    Added support for the -[no]online_checkpoint option for alterdb
**	    Changed syntax for production mode flag to "-[no]allow_ddl"
**	    which is more descriptive of what the flag does.
**	20-feb-1996 (canor01)
**	    Put in error checking in parse_command_line() for -allow_ddl.
**	12-mar-96 (nick)
**	    Allow '#c' without a sequence number for infodb - we'll use
**	    the last checkpoint sequence number in this case.
**      06-mar-1996 (stial01)
**          Variable page size project:
**          parse_command_line() Pass new buffers parameter to dmf_init
**          Defer dmf_init() until all jsp parms have been processed. 
**          Also check for new dmf_cache_size parms.
**          jsp_echo_usage() Display new dmf_cache_size parms. 
**	20-may-1996 (hanch04)
**	    Added parse_device_list, device is now an array of devices
**	12-jun-96 (nick)
**	    Add E_DM105A to differentiate some parameter errors.
**	13-jun-96 (nick)
**	    An uninitialised pointer was being passed to DIdirdelete() from
**	    jsp_file_maint().
**	14-jun-96 (nick)
**	    Disallow [+|-]j at table level.
**	25-jun-96 (nick)
**	    More use of E_DM105A to fix #77262.
**	 2-jul-96 (nick)
**	    Ensure empty location lists are trapped ; remove a FIX ME which
**	    should never have made it to submitted code.
**	17-jul-96 (nick)
**	    Correct usage message errors. #77826
**	20-aug-96 (nick)
**	    If the error is some form of incorrect command line syntax
**	    or invalid arg combo then display usage. #78090
**	24-oct-96 (nick)
**	    Fix parsing problem.
**	    Some LINT stuff.
**	23-sep-1996 (canor01)
**	    Move dmf_write_msg() and dmf_put_line() into dmfio.c.
**	    Move jsp_get_case(), jsp_set_case(), and jsp_file_maint() 
**	    and tbllist_delete into dmfjnl.c.
**	26-Oct-1996 (jenjo02)
**	        Added *name parm to dm0s_minit() calls.
**	05-nov-1996 (hanch04)
**		removed dbname checking for FLOAD, use default checking
**	03-jan-1997 (shero03)
**	    Enforce -c|+c, -j|+j, -w|+w.  #79869
**	    Enforce -relocate has a checkpoint #79932
**	06-jan-96 (mcgem01)
**		Fix the usage message to display date format correctly.
**	07-jan-96 (nanpr01)
**	    Takeout the -noblob option from rollforwarddb. Refer 79913.
**	15-jan-96 (nanpr01)
**	    Distinguish between invalid argument and argument value.
**	    Bug 88001 & 88038. 
**	30-jan-97 (shero03)
**	    B80384 - Enforce -relocate has a checkpoint (enhance #79932)
** 	14-apr-1997 (wonst02)
** 	    Allow new -read_only and -read_write flags to set access mode.
** 	18-jun-1997 (wonst02)
** 	    Remove change of 14-apr-1997; not needed for readonly database.
**	23-jun-1997 (shero03)
**	    Support alterdb -next_journal_file for Cheyenne.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	23-oct-1997 (hanch04)
**	    Remove +jx option from ckpdb help
**	13-Nov-1997 (jenjo02)
**	    Added -s option (JSX_CKP_STALL_RW) for online checkpoint.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	15-Mar-1999 (jenjo02)
**	    Removed much-maligned "-s" online CKPDB option,
**	    JSX_CKP_STALL_RW, stall mechanism completely redone.
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**  17-may-1999 (kitch01)
**     Bug 91603. In parse_command_line for relocatedb ensure 
**     database name is valid.
**      05-Aug-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**	18-jan-2000 (gupsh01)
**	    Added switches -aborted_transactions and -internal_data for
**	    auditdb for use of visual journal analyser.
**      26-jan-2000 (gupsh01)
**          Added switches -no_header and -limit_output for
**          auditdb for use of visual journal analyser.
**      28-feb-2000 (gupsh01)
**          Added support for table names with format owner_name.table_name
**          in parse_table_list.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	27-mar-2001 (somsa01)
**	    Added '-expand_lobjs' flag to auditdb.
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in parse_table_list(), parse_device_list() and parse_loc_list()
**          routines.
**      21-Nov-2001 (horda03) Bug 106230
**         When performing a table level checkpoint, enable ALL
**         cache page sizes.
**	15-may-2002 (somsa01)
**	    Added the running of CMset_locale() before doing anything.
**	17-sep-2002 (somsa01)
**	    In parse_table_list(), make sure we at least initialize the
**	    tbl_owner.db_own_name for each table found to NULL. Also,
**	    in parse_command_line(), initialize jsx_status to zero if this
**	    is an "alterdb" or "fastload" operation, and jsx_status2 to
**	    zero if this is an "auditdb" operation.
**	10-jun-2003 (somsa01)
**	    In build_location(), updated code to account for embedded spaces
**	    in path names.
**	28-jul-2003 (somsa01)
**	    In parse_command_list(), if we are using the "-t" option, make
**	    sure we initialize the table owner field to NULLs. Also, in
**	    parse_table_list(), do the same thing if we are not using the
**	    "dot" case.
**	8-oct-2004 (shust01)
**	    In parse_command_line(), initialize jsx->jsx_status when doing
**	    the infodb processing.  Otherwise possible to get E_DM1006 when
**	    running infodb command without specifying a database name.
**	    bug 112188, INGCBT548
**      19-Jan-2004 (hanje04)
**	    BUG 111637
**          Initialize jsx_tbl_cnt in parse_command_lnx to stop SEGV in 
**	    cpp_read_tables() when NOT performing a table level checkpoint.
**	    (Default behavior)
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      13-Apr-2004 (nansa02) 
**          Added support for -delete_invalid_ckp and -convert_unicode.
**  2-Aug-2004 (kodse01)
**      BUG 112531
**      Rollforwarddb -noblobs flag support is removed by commenting out 
**      the relevant portion in the code.
**      01-sep-2004 (stial01)
**          Online cluster checkpoint
**	29-sep-2004 (devjo01)
**	    Replace du_chk3_locname with cui_chk3_locname.
**      01-apr-2005 (stial01)
**         More online cluster checkpoint error handling
**      07-apr-2005 (stial01)
**          Added rollforwarddb flag -on_error_ignore_index 
**      05-may-2005 (stial01)
**          Added rollforwarddb flag -on_error_prompt
**      18-may-2005 (bolke01)
**          Moved all tbllst_* functions to dmfjnl.c as part of VMS Cluster 
**          port (s114136)
**	14-oct-2005 (mutma03/devjo01) b115540
**	    Have E_DM1200 error message display database name.
**      25-oct-2005 (stial01)
**          Init jsx_ingored_tbl_err, jsx_ignored_idx_err.
**          Removed -on_error_ignore_index, it is now default error handling
**	9-Jan-2006 (kschendel)
**	    Changes to allow fastload to attach to shared cache.
**	    This permits use of fastload in fast-commit environments.
**	    This has to be restricted (to privileged users), as kill -9 while
**	    holding a cache attachment wipes out the entire installation.
**	    (Future: track a "someone's in the BM" counter, so that kills
**	    can be nonfatal;  and perhaps add a specific privilege for this.)
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      04-may-2007 (stial01)
**          Added jsp_log_info()
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      27-jun-2007 (stial01)
**          jsp_echo_usage() Added new rollforwarddb argument -incremental
**      09-oct-2007 (stial01)
**          Require explicit -rollback OR -norollback for incremental rfp.
**      10-jan-2008 (stial01)
**          Support fastload [+w|-w] (default was and still is +w)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	9-May-2008 (kibro01) b120373
**	    Added support for -keep=n for checkpoints
**	03-Nov-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat
**	    and functions.
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dm0p_? functions converted to DB_ERROR *
**      06-Jan-2009 (stial01)
**          Fix buffer sizes dependent on DB_MAXNAME
**      26-jan-2009 (stial01)
**          Fix call to jsp_log_info (b121547)
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      14-Apr-2010 (hanal04) SIR 123574
**          Added -disable_mustlog and -enable_mustlog options to alterdb.
**	    
*/
/*
MODE=         PARTIAL_LD

PROGRAM=      dmfjsp

DEST=	      bin

NEEDLIBS=     DMFLIB SCFLIB ADFLIB GWFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

DYNLIBS =     LIBIIUSERADT
*/

/*
**  Forward and/or External function references.
*/

GLOBALREF DMP_DCB *dmf_jsp_dcb ;	/* Current DCB  */

static i4  fload_bsize = 0;   /* buffer size for fastload */
 
static DB_STATUS parse_command_line(
    i4	argc,
    char	*argv[],
    DMF_JSX	*journal_context);

static DB_STATUS allocate_dcb(
    DMF_JSX	*jsx,
    DB_DB_NAME	*dbname,
    DMP_DCB	**dcb);

static DB_STATUS search_dcb(
    DMF_JSX	*journal_context);

static DB_STATUS build_location(
    DMP_DCB	    *dcb,
    i4		    l_path,
    char	    *path,
    char	    *loc_type,
    DMP_LOC_ENTRY   *location);

static VOID jsp_echo_usage(
    DMF_JSX	    *jsx);

static DB_STATUS jsp_normalize(
    DMF_JSX 	    *jsx,
    u_i4	    untrans_len,
    u_char	    *obj,
    u_i4	    *trans_len,
    u_char	    *unpad_obj);

static STATUS ex_handler(
    EX_ARGS	*ex_args);

static DB_STATUS parse_table_list(
    DMF_JSX	    *jsx,
    char	    *cmd_line);

static DB_STATUS parse_device_list(
    DMF_JSX	    *jsx,
    char	    *cmd_line);

static DB_STATUS parse_lsn(
    char	    *cmd_line,
    LG_LSN	    *lsn);

static DB_STATUS audit_jsp_action(
	i4		action,
	bool		succeed,
	char		*obj_name,
	DB_OWN_NAME	*obj_owner,
	bool		force_write,
	DB_ERROR	*dberr);

static    DU_USER		user; /* User info */

static bool jsp_chk_priv(
	char *user_name,
	char *priv_name );

static DB_STATUS parse_loc_list(
    DMF_JSX         *jsx,
    JSX_LOC         *loc_list,
    char            *cmd_line,
    i4         maxloc);

static VOID jsp_log_info(
    DMF_JSX	*jsx,
    i4		argc,
    char 	*argv[]);


/*
**  Structures.
*/
typedef struct
{
	STATUS	    error;
	char	    *path_name;	
	int	    path_length;
	int	    pad;
}   DELETE_INFO;

# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	dmfjsp_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*{
** Name: main	- Main program.
**
** Description:
**      This routine gets control from the operating system and prepares
**	to execute any of the Journal Support Program utility routines.
**
** Inputs:
**      argc                            Count of arguments passed to program.
**	argv				Array of pointers to arguments.
**
** Outputs:
**	Returns:
**	    OK
**	    ABORT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-nov-1986 (Derek)
**          Created for Jupiter.
**	14-Sept-1988 (Edhsu)
**	    Added maxdb interface to dmf_init
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    10-jan-1991 (stevet)
**		Added CMset_attr to initialize CM attribute table.
**	    25-feb-1991 (rogerk)
**		Added support for the Archiver Stability project.
**		Added alterdb command option.
**	    11-jun-1991 (bryanp)
**		B37942: Move ule_initiate call up so that msgs contain "II_JSP".
**	    12-aug-91 (stevet)
**		Change read-in character set support to use II_CHARSET symbol.
**
**	03-nov-1992 (robf)
**	    Update security auditing to use SXF sessions.
**	    Since  a session is database-specific, we use a session per
**	    database, plus one for the initial search for dcb's in iidbdb
**	16-nov-1992 (bryanp)
**	    Fix some argument passing problems (chg by value to by reference).
**	18-feb-93 (jnash)
**	    Fix problem where status returned my dmfrfp was clobbered
**	    by call to dmf_sxf_end_session().
**	26-jul-1993 (jnash)
**	    Add -help support.
**	25-dec-1993 (rogerk)
**	    Added exception handler (copied from DMFACP.C).
**	17-feb-94 (stephenb)
**	    Move initial dmf_sxf_bgn_session() into search_dcb() so that we
**	    can take advantage of case translated usernames.
**
**      15-feb-1994 (andyw)
**          Added new error message E_DM1052_JSP_VALID_USAGE is displayed
**          if an invalid command argument is specified. Shows usage
**          Bug 56789
**	12-may-1995 (harpa06)
**	    Removed E_DM1052_JSP_VALID_USAGE from being displayed if a command
**	    line argument is specified. It makes no sense to the user. 
**	    Especially if they are using the ROLLFORWARDDB or RELOCATEDB
**	    scripts to rollforward a database or relocate database locations.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**      18-jan-2000 (gupsh01)
**          Added switches -aborted_transactions and -internal_data for
**          auditdb for use of visual journal analyser.
**      26-jan-2000 (gupsh01)
**          Added switches -no_header and -limit_output for
**          auditdb for use of visual journal analyser.
**	27-mar-2001 (somsa01)
**	    Added '-expand_lobjs' flag to auditdb.
**	15-may-2002 (somsa01)
**	    Added the running of CMset_locale() before doing anything.
**      29-sep-2002 (devjo01)
**          Call MEadvise only if not II_DMF_MERGE.
**	19-jun-2003 (gupsh01)
**	    Fixed auditdb -all case, if the -b, -e flags are specified,
**	    we handle this by allowing -aborted_transactions case to be
**	    enabled which will handle this correctly. 
**       7-Oct-2003 (hanal04) Bug 110889 INGSRV2521
**          Call EXsetclient() before declaring an exception handler
**          in order to prevent our type defaulting to EX_USER_APPLICATION.
**          EX_INGRES_TOOL is used as EX_INGRES_DBMS ignores SIGINT.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	9-Jan-2006 (kschendel)
**	    Detach from buffer manager when exiting, if necessary.
**	27-May-2008 (jonj)
**	    Prepend ULE message with nodename in a cluster, hostname
**	    otherwise.
*/
# ifdef	II_DMF_MERGE
VOID MAIN(argc, argv)
# else
VOID 
main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
    DMF_JSX		jsx;
    DMP_DCB		*dcb;
    DB_STATUS		status;
    STATUS              ret_status;
    CL_ERR_DESC         cl_err;
    DB_OWN_NAME 	eusername;
    EX_CONTEXT		context;
    char		dbname[DB_DB_MAXNAME+1];
    char		NodeName[CX_MAX_NODE_NAME_LEN +1];
    i4			error;
    DB_ERROR		local_dberr;

# ifndef        II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif

    CMset_locale();

    /* Prepend nodename or hostname to errlog.log messages */
    if ( CXcluster_enabled() )
	CXnode_name(NodeName);
    else
	GChostname(NodeName, sizeof(NodeName));

    ule_initiate(NodeName, STlength(NodeName), "II_JSP", 6);

    /*
    ** Declare exception handler.
    ** All exceptions in the jsp are fatal.
    **
    ** When an exception occurs, execution will continue here.  The
    ** exception handler will have logged an error message giving the
    ** exception type and hopefully useful information indicating where
    ** it occurred.
    */
    (void) EXsetclient(EX_INGRES_TOOL);

    /* Init JSX */
    MEfill(sizeof(DMF_JSX), '\0', &jsx);
    jsx.jsx_next = (DMP_DCB*)&jsx;
    jsx.jsx_prev = (DMP_DCB*)&jsx;

    CLRDBERR(&jsx.jsx_dberr);

    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	if (jsx.jsx_status2 & JSX2_MUST_DISCONNECT_BM)
	    (void) dm0p_deallocate(&local_dberr);
	dmfWriteMsg(NULL, E_DM904A_FATAL_EXCEPTION, 0);
	PCexit(FAIL);
    }

    /* Set the proper character set for CM */

    ret_status = CMset_charset(&cl_err);

    if ( ret_status != OK)
    {
	uleFormat(NULL, E_UL0016_CHAR_INIT, (CL_ERR_DESC *)&cl_err, ULE_LOG ,
	    NULL, (char * )0, 0L, (i4 *)0, &error, 0);
	PCexit(FAIL);
    }

    /*
    **	Initialize PM
    */
    PMinit();

    switch(PMload((LOCATION *)NULL, (PM_ERR_FUNC *)NULL))
    {
    case OK:
	/* Loaded sucessfully */
	break;
    case PM_FILE_BAD:
        SETDBERR(&jsx.jsx_dberr, 0, E_DM1039_JSP_BAD_PM_FILE);
	break;
    default:
        SETDBERR(&jsx.jsx_dberr, 0, E_DM103A_JSP_NO_PM_FILE);
	break;
    }

    if (jsx.jsx_dberr.err_code)
    {
	dmfWriteMsg(&jsx.jsx_dberr, 0, 0);
	PCexit(FAIL);
    }

    /*
    ** Prepare command and args for shared memory info string and
    ** (in some cases) write a message to the errlog
    */ 

    status = parse_command_line(argc, argv, &jsx);
    jsp_log_info(&jsx, argc, argv);
    if (status != E_DB_OK)
    {
	if (jsx.jsx_status2 & JSX2_MUST_DISCONNECT_BM)
	    (void) dm0p_deallocate(&local_dberr);
	/*
	** Write message unless set to 0
	*/
	if (jsx.jsx_dberr.err_code > 0)
	{
	    dmfWriteMsg(&jsx.jsx_dberr, 0, 0);
	}
	PCexit(FAIL);
    }

    if (jsx.jsx_status1 & JSX_HELP)
    {
	/*
	** Echo correct command line usage.
	*/
	jsp_echo_usage(&jsx);
	PCexit(OK);
    }

    for(;;)
    {
	/*  Fill in information about the selected databases. */

	MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ',
           sizeof(eusername.db_own_name), eusername.db_own_name);

	status = search_dcb(&jsx);
	if (status != E_DB_OK)
	    break;

	/* If connecting with shared cache, check out user, has to be
	** a SERVER_CONTROL user (since kill -9 of fastload can wipe out the
	** installation).
	** It may be that simply being a security user is good enough
	** (since they can do any desired amount of damage to the data),
	** but for starters I'm sticking with SERVER_CONTROL.  -u is
	** available if need be.
	*/
	if ((jsx.jsx_status2 & JSX2_MUST_DISCONNECT_BM)
	  && (jsx.jsx_status2 & JSX2_SERVER_CONTROL) == 0)
	{
	    SETDBERR(&jsx.jsx_dberr, 0, E_DM100E_JSP_NO_SUPERUSER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	**	Should have user info now, so save real/effective user
	*/

	if( jsx.jsx_status & JSX_UNAME)
	{
		/* Effective user different */
		MEcopy(jsx.jsx_username.db_own_name,
			sizeof(eusername.db_own_name),
			eusername.db_own_name);
	}
	else
	{
		/* Effective same as real user */
		MEcopy(user.du_name.db_own_name,
			sizeof(eusername.db_own_name),
			eusername.db_own_name);
	}
	/*
	**	If no databases found then break
	*/
	if (jsx.jsx_next == (DMP_DCB *)&jsx.jsx_next)
	{
	    SETDBERR(&jsx.jsx_dberr, 0, E_DM1011_JSP_DB_NOTFOUND);
	    break;

	}

	/*  Loop through all selected database, and perform operation. */

	while (jsx.jsx_next != (DMP_DCB *)&jsx.jsx_next)
	{

	    /*	Get current context. */

	    dcb = (DMP_DCB *)jsx.jsx_next;

	    /* Set global context */
	    dmf_jsp_dcb = dcb;

	    STncpy( dbname, dcb->dcb_name.db_db_name, DB_DB_MAXNAME);
	    dbname[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(dbname);

	    /*  Start an SXF audit session */

	    status = dmf_sxf_bgn_session (
		&(user.du_name),   /* Real user */
		&(eusername),	   /* Effective user */
		dcb->dcb_name.db_db_name,	   /* Database name */
		user.du_status	   /* User status */
	    );

	    if (status != E_DB_OK)
		break;

	    /*	Perform operation. */

	    switch (jsx.jsx_operation)
	    {
	    case JSX_O_CPP:
		/*  Checkpoint the database. */

		status = dmfcpp(&jsx, dcb);
		break;

	    case JSX_O_ATP:
		/*  Generate an audit trail. */

		status = dmfatp(&jsx, dcb);
		break;

	    case JSX_O_DTP:
		/*  Generate an dump trail. */

		status = dmfdtp(&jsx, dcb);
		break;

	    case JSX_O_RFP:
		/*  Roll forward a database. */

		status = dmfrfp(&jsx, dcb);
		break;

	    case JSX_O_INF:
		/*  Display database information. */

		status = dmfinfo(&jsx, dcb);
		break;
#ifdef UNIX
       	   case JSX_O_IN1:
        	/*  Display database information. DBALERT*/
 
        	status = dmfinfo1(&jsx, dcb);
        	break;
#endif 
	    case JSX_O_ALTER:
		/*  Alter database state. */

		TRdisplay ("calling dmfalter\n");
		status = dmfalter(&jsx, dcb);
		break;

	    case JSX_O_RELOC:
		/* Relocate ckp,jnl,dmp,work locations, relocate db */

		TRdisplay ("calling dmfreloc\n");
		status = dmfreloc(&jsx, dcb);
		break;

            case JSX_O_FLOAD:
                status = dmffload(&jsx, dcb,
                                   eusername.db_own_name, fload_bsize);
                break;

	    }

	    if (status != E_DB_OK && status!=E_DB_INFO && status!=E_DB_WARN)
	    {
		    /*
	   	    **	Terminate the SXF session before breaking
		    */
	    	    _VOID_ dmf_sxf_end_session();

		    break;
      	    }
	    /*	Remove DCB from JSX. */

	    dcb->dcb_q_next->dcb_q_prev = dcb->dcb_q_prev;
	    dcb->dcb_q_prev->dcb_q_next = dcb->dcb_q_next;

	    /*	Deallocate the DCB. */

	    dm0m_deallocate((DM_OBJECT **)&dcb);

	    /*  End the SXF session */
	    _VOID_ dmf_sxf_end_session();

	    if( status!= E_DB_OK)
		break;
	}

	/*  Check for any errors. */

	if (status == E_DB_OK)
	{
	    if (jsx.jsx_status2 & JSX2_MUST_DISCONNECT_BM)
		(void) dm0p_deallocate(&local_dberr);
	    /*	Terminate SXF facility */
	    _VOID_ dmf_sxf_terminate(&jsx.jsx_dberr);
	    PCexit(OK);
	}
	break;
    }

    /*	Handle error recovery. */

    if (jsx.jsx_status2 & JSX2_MUST_DISCONNECT_BM)
	(void) dm0p_deallocate(&local_dberr);

    if (jsx.jsx_dberr.err_code == E_DM1200_ATP_DB_NOT_JNL)
    {
	dmfWriteMsg(&jsx.jsx_dberr, 0, 1, 0, dbname );
    }
    else if (jsx.jsx_dberr.err_code != 0)
    {
	dmfWriteMsg(&jsx.jsx_dberr, 0, 0);
    }

    /*  Flush the DCB work queue. */

    while (jsx.jsx_next != (DMP_DCB *)&jsx.jsx_next)
    {
	dcb = jsx.jsx_next;
	dcb->dcb_q_next->dcb_q_prev = dcb->dcb_q_prev;
	dcb->dcb_q_prev->dcb_q_next = dcb->dcb_q_next;
	dm0m_deallocate((DM_OBJECT **)&dcb);
    }

    /*	Terminate SXF facility */
    _VOID_ dmf_sxf_terminate(&jsx.jsx_dberr);

    /*  Exit with error status. */

    PCexit(FAIL);
}

/*{
** Name: parse_command_line	- Parse the command line.
**
** Description:
**      This routine parses the command line and builds any contexts required
**	by the operation requested.
**
** Inputs:
**      arcg                            Number of arguments.
**	argv				Array of pointers to arguments.
**	journal_context			Pointer journal support context.
**
** Outputs:
**	err_code			Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-nov-1986 (Derek)
**          Created for Jupiter.
**	31-aug-1989 (Sandyh)
**	    Fix database queue mgmt.
**	13-oct-1989 (rogerk)
**	    Made -j/+j flags to checkpoint imply the -l flag as well.
**	 1-dec-1989 (rogerk)
**	    Added #w flag - this is used for online checkpoint testing.  It
**	    causes the checkpoint to stall in the backup phase.
**	 6-dec-1989 (rogerk)
**	    Made CHECKPOINT default mode be OFFLINE.
**	    Added -o flag to force online checkpoint.
**	    This is a temporary change until bugs in online checkpoint are
**	    fixed.
**	 7-dec-1989 (rogerk)
**	    Took out above patch to turn off ONLINE CHECKPOINT.
**	08-dec-1989 (sandyh)
**	    Removed check for database specified on command line. If no db's
**	    are passed then all db's for the user should be audited, bug 9089.
**	21-may-90 (bryanp)
**	    Added the new, undocumented (for now) '#c<ckpno>' flag for
**	    rollforward. This flag is used to request a specific checkpoint
**	    sequence number to be used as the 'rollback point' for recovery.
**	    If #c is ommitted, the current checkpoint is used. If #c is given
**	    with a number, the checkpoint with that sequence number is used.
**	    If #c is given without a number, the last valid checkpoint is used.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    25-feb-1991 (rogerk)
**		Added alterdb command line option with -disable_journal flag.
**		These were added as part of the Archiver Stability project.
**		Added JSX_O_ALTERDB and JSX_JDISABLE to correspond to above.
**		Added JSX_NOLOGGING option to specify that operations should
**		not be logged during command processing.  This is turned on
**		for the disable_journaling option of alterdb.
**	    29-may-1991 (bryanp)
**		B37799: infodb bry1 bry2 loops because database list is broken.
**	    11-jun-1991 (bryanp)
**		B37942: Check dmf_init() return code when no db names given.
**	14-aug-91 (markg)
**	    When a user enters both start and end dates for auditdb
**	    proccessing, check that the start date is not greater than the
**	    end date. Bug 39150.
**	21-dec-92 (robf)
**	    When a user enters an invalid or inconsistent argument, print
**	    a message saying which argument is wrong. We add new errors,
**	    E_DM103B_JSP_INVALID_ARG/E_DM103C_JSP_INCON_ARG, with a parameter
**	    since we can't change the number of parameters passed to an
**	    old message.
**	10-feb-1993 (jnash)
**	    Add AUDITDB #a and #v flags, translating to JSX_AUDIT_ALL and
**	    JSX_AUDIT_VERBOSE.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added #norollback flag to bypass undo phase of rollforward.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Modified calling sequence to dmf_init to make more explicit the
**		    control over buffer manager allocation and UDT lock values.
**	26-apr-1993 (jnash)
**	    Add support for AUDITDB -table -file.
**	    Add support for ALTERDB -target_jnl_blocks, -jnl_block_size and
**	        -init_jnl_blocks.
**	    Change ROLLFORWARD #norollback to -norollback
**	    Add ROLLFORWARD -nologging.
**	    Change AUDITDB #a to -all
**	    Change AUDITDB #v to -verbose
**	26-jul-1993 (bryanp)
**	    Added csp_lock_list argument to dmf_init()
**	26-jul-1993 (jnash)
**	    Add support for auditdb -inconsistent and -wait.
**	    Add support for -help.
**	23-aug-1993 (jnash)
**	    Fix AV introduced by -help when program has no arguments.
**	20-sep-1993 (jnash)
**	  - Add delim id support.
**	  - Restrict auditdb, alterdb, rollforwarddb, ckpdb to operating on a
**	    single database.
**	  - Add alterdb -delete_oldest_ckp.
**	  - Add -verbose, equivalent to -v.
**	18-oct-1993 (jnash)
**	    Further param checking.  Use STbcompare() to check for exact
**	    string match, MEcmp() to check for partial match.
**	31-jan-1994 (jnash)
**	    During rollforwarddb #f, set JSX_FORCE instead of JSX_JDISABLE.
**	    This now causes rollforward to "force" a recovery in a number of
**	    circumstances, not just when the database is journal disabled.
**      15-feb-1994 (andyw)
**          Added new error message E_DM1051_JSP_NO_INSTALL displays a
**          more appropriate message if the installation is not running
**          Bug 55193
**	21-feb-1994 (jnash)
**	  - Add support for auditdb & rollforwarddb -start_lsn, -end_lsn.
**	  - Add support for auditdb #c.
**	  - Allow rollforwarddb #c -c requests if #f specified.
**	  - Remove redundant alterdb param check.
**	24-feb-1994 (jnash)
**	    Remove requirement for #f on rolldb +j -c requests.
**	    This applies only to rolldb -b and -start_lsn.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**              Added the following command line flags to rollforwarddb:
**                  -table=
**                  -nosecondary_index
**                  -on_error_continue
**                  -online
**                  -status_rec_count
**                  -noblobs
**                  -force_logical_consistent
**      05-dec-1994 (sarjo01)
**          Ensure that we have an arg for '-m' option - bug 51298
**          Cross integration of 19-sep-1994 (nick)
**	28-dec-1994 (shero03)
**	    Disable -online for now. B66007.
**	06-jan-1995 (shero03)
**	    Support Partial Checkpoint
**		Added -table= to ckpdb command
**      23-jan-1995 (stial01)
**          BUG 66430: Disallow -d for table level checkpoints.
**	21-feb-1995 (shero03)
**	    Bug #67004, #67005
**	    Ensure that -relocate -new_location= -location= 
**	    -on_eror_continue.
**	    only happens for table level actions.
**	20-mar-1995 (shero03)
**	    Bug #67437
**	    Ensure that invalid flag prefixes are not mistaken 
**	    for dbnames.
**	11-may-1995 (harpa06)
**	    In RELOCATEDB, if "-location" is specified, check to see if
**	    "-new_location" is also specified. -- bug 68602
**	29-jun-1995 (thaju02)
**	    Added new ckpdb parameter '-max_stall_time=xx' where stall 
**	    time is specified in seconds.  Store the stall time in
**	    DMF_JSX structure (jsx_stall_time) and the stall time
**	    specified flag (JSX1_STALL_TIME) in jsx_status1.
**      12-july-1995 (thaju02)
**          Added call to STindex for parsing of the max stall time.
**	    Added error checking of the stall time.
**	12-jul-1995 (harpa06) Bug #69865
**	    Convert the name specified on the command line to lowercase.
**      13-july-1995 (thaju02)
**          Modified the parsing and validity checking of the timeout
**          value specified on command line.
**          bug #69979 Modified '-max_stall_time' argument to
**          '-timeout'.  If user specified the max_stall_time
**          argument misspelled, ckpdb will attempt to checkpoint
**          to a tape device.
**          bug #69981 - added the initialization of
**          jsx->jsx_ignored_errors during parsing of -ignore arg.
**	11-sep-1995 (thaju02)
**	    Support for viewing contents of checkpoint table list file.
**      13-sep-1995 (tchdi01)
**          Added support for the -production=(on|off) option for alterdb
**	13-feb-1996 (canor01)
**	    Added support for the -[no]online_checkpoint option for alterdb
**	    Changed syntax for production mode flag to "-[no]allow_ddl"
**	    which is more descriptive of what the flag does.
**	20-feb-1996 (canor01)
**	    Put in error checking for -allow_ddl.
**	12-mar-96 (nick)
**	    Allow '#c' without sequence number if we're 'infodb'.
**      06-mar-1996 (stial01)
**          Variable page size project:
**          Added new buffers parameter to dmf_init
**          Defer dmf_init() all jsp parms have been processed.
**	06-may-1996 (nanpr01)
**	    Get rid of the compiler warning message by casting the return
**	    value of STlength.
**	20-may-1996 (hanch04)
**	    Added multiple devices
**	12-jun-96 (nick)
**	    Add E_DM105A to differentiate some parameter errors.
**	14-jun-96 (nick)
**	    [+|-]j are database level operations for checkpoint
**	25-jun-96 (nick)
**	    Some more use of E_DM105A. #77262
**	20-aug-96 (nick)
**	    Print usage for command line errors.
**	24-oct-96 (nick)
**	    Parsing of '-h' fell through.
**	03-jan-1997 (shero03)
**	    Enforce -c|+c, -j|+j, -w|+w.  #79869
**	    Enforce a checkpoint file when relocating during table recovery 
**	23-jun-1997 (shero03)
**	    Support alterdb -next_journal_file for Cheyenne.
**      07-Jul-1998 (wanya01)
**          Fix bug 91833, relocatedb with -w gives syntax error.
**  17-may-1999 (kitch01)
**      Bug 91603. relocatedb does not support creating new databases on
**      a remote node. Therefor ensure that the database name is valid.
**      02-may-2000 (gupsh01)
**          Removed the check for auditdb parameters -start_lsn and
**          -end_lsn to be only provided when -all is used.
**          Visual Journal Analyser requires these flags for 
**          auditdb output.  
**	27-mar-2001 (somsa01)
**	    Added '-expand_lobjs' flag to auditdb.
**      21-Nov-2001 (horda03) Bug 106230
**          Enable all DMF cache sizes when performing a table level
**          checkpoint.
**	17-sep-2002 (somsa01)
**	    Initialize jsx_status to zero if this is an "alterdb" or
**	    "fastload" operation, and jsx_status2 to zero if this is an
**	    "auditdb" operation.
**	28-jul-2003 (somsa01)
**	    If we are using the "-t" option, make sure we initialize the
**	    table owner field to NULLs.
**	8-oct-2004 (shust01)
**	    Initialize jsx->jsx_status when doing the infodb processing.
**	    Otherwise possible to get E_DM1006 when running infodb command
**	    without specifying a database name. bug 112188, INGCBT548
**      19-Jan-2004 (hanje04)
**          Initialize jsx_tbl_cnt in parse_command_lnx to stop SEGV in 
**	    cpp_read_tables() when NOT performing a table level checkpoint.
**	    (Default behavior)
**	05-May-2004 (hanje04)
**	    Extend initializing of jsx_tbl_cnt to 0 in for ckpdb to all
**	    dmfjsp functions.
**	06-May-2004 (hanje04)
**	    Initialize jsx_status2 to 0 for all dmfjsp operations as it is now
**	    always checked in jsp_file_maint().
**	10-May-2004 (hanje04)
**	    Initialize jsx_status to 0 as well.
**	21-feb-05 (inkdo01)
**	    Add support for "alterdb -i" for Unicode NFC.
**	9-Jan-2006 (kschendel)
**	    Add -cache_name=shared-cache-name option for fastload.
**	10-Dec-2007 (jonj)
**	    Init jsx->jsx_ckp_phase = 0 instead of using the
**	    define now moved to csp.h and not easily accessible.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add -enable_mvcc, -disable_mvcc
**	    to alterdb.
*/
static DB_STATUS
parse_command_line(
i4		    argc,
char		    *argv[],
DMF_JSX		    *journal_context)
{
    DMF_JSX	*jsx = journal_context;
    i4		operator;
    DB_STATUS	status;
    i4		i;
    i4		j;
    char	error_buffer[ER_MAX_LEN];
    i4	    	error_length;
    u_i4	len_trans;
    u_char	unpad_owner[DB_OWN_MAXNAME + 1];
    i4		c_buffers[DM_MAX_CACHE];
    i4		dmf_cache_size[DM_MAX_CACHE];
    i4		cache_ix;
    i4		dbname_ix[32];
    char	cused = 'N';		/* Keep track if any "c" option is used */
    char	jused = 'N';		/* Keep track if any "j" option is used */
    char	wused = 'N';		/* Keep track if any "w" option is used */
    bool	connect_only;
    char	cache_name[DB_MAXNAME+1];
    i4		error;

    CLRDBERR(&jsx->jsx_dberr);


    /* Init dmf_cache_size to illegal buffer size */
    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	dmf_cache_size[cache_ix] = -1;
    connect_only = FALSE;
    cache_name[0] = '\0';

    /*
    **	Standard command line is parsed as follows:
    **
    **	argv[0]	    -   Program name.
    **	argv[1]	    -   Function.
    **	argv[2-n]   -	Fucntion specific parameters.
    */

    status = E_DB_OK;

    /*	Check for minimum number of arguments. */

    if (argc < 2)
    {
	jsx->jsx_operation = -1;
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1002_JSP_MISS_ARGS);
	return (E_DB_ERROR);
    }

    /*	Check function selection. */

    jsx->jsx_status = 0;
    jsx->jsx_status1 = 0;
    jsx->jsx_status2 = 0;
    jsx->jsx_lock_list = 0;
    jsx->jsx_pid = (PID)-1;
    jsx->jsx_ckp_crash = 0;
    if (CXcluster_enabled())
    {
	jsx->jsx_node = CXnode_number( NULL );
	jsx->jsx_ckp_node = jsx->jsx_node;
    }
    else
    {
	jsx->jsx_node = -1;
	jsx->jsx_ckp_node = -1;
    }

    jsx->jsx_ckp_phase = 0;
    jsx->jsx_lockid.lk_unique = 0;	/* LK_LKID for LK_CKP_DB lock */
    jsx->jsx_lockid.lk_common = 0;

    /* 
    ** Default operation is database wide so table count should
    ** default to 0
    */
    jsx->jsx_tbl_cnt = 0;

    if (STcompare("auditdb", argv[1]) == 0)
    {
	operator = JSX_O_ATP;
    }
    else if (STcompare("dumpdb", argv[1]) == 0)
    {
	operator = JSX_O_DTP;
    }
    else if (STcompare("ckpdb", argv[1]) == 0)
    {
	operator = JSX_O_CPP;
        jsx->jsx_stall_time = 0;
	jsx->jsx_dev_cnt = 1;
        jsx->jsx_device_list[0].dev_pid = -1;

        /*
        ** Default mode is database level checkpoint 
	** so jsx_tbl_cnt should be 0 by default.
        */
        jsx->jsx_status1 = JSX1_CKPT_DB;
	PCpid(&jsx->jsx_pid);
    }
    else if (STcompare("rolldb", argv[1]) == 0)
    {
	operator = JSX_O_RFP;
	jsx->jsx_status = JSX_USECKP | JSX_USEJNL;
	jsx->jsx_ckp_number = -1;
        jsx->jsx_ignored_errors = FALSE;
	jsx->jsx_ignored_tbl_err = FALSE;
	jsx->jsx_ignored_idx_err = FALSE;
	jsx->jsx_dev_cnt = 1;
        jsx->jsx_device_list[0].dev_pid = -1;

        /*
        ** Default mode is database level rollforward
        */
        jsx->jsx_status1 = JSX1_RECOVER_DB;
    }
    else if (STcompare("infodb", argv[1]) == 0)
    {
	operator = JSX_O_INF;
    }
#ifdef UNIX
    else if (STcompare("infodb1", argv[1]) == 0)
    {
    operator = JSX_O_IN1;
    jsx->jsx_status = JSX_VERBOSE;
    }
#endif
    else if (STcompare("alterdb", argv[1]) == 0)
    {
	operator = JSX_O_ALTER;
    }
    else if (STcompare("relocdb", argv[1]) == 0)
    {
	operator = JSX_O_RELOC;
    }
    else if (STcompare("fastload", argv[1]) == 0)
    {
        operator = JSX_O_FLOAD;
	jsx->jsx_status |= JSX_WAITDB; /* default */
    }
    else
    {
	jsx->jsx_operation = -1;
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1003_JSP_UNRECOGNIZED);
	return (E_DB_ERROR);
    }
    jsx->jsx_db_cnt = 0;
    jsx->jsx_operation = operator;

    /*	Parse the rest of the command line. */

    for (i = 2; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'l':
		if (operator == JSX_O_CPP)
		    jsx->jsx_status |= JSX_CKPXL;
		/*
		** For Rollforward:
		** For Relocate:
		**   "location" - To specify a list of locations to be
		**                relocated.
		*/
		else if (operator == JSX_O_RFP || operator == JSX_O_RELOC)
		{
		    if (MEcmp(&argv[i][1], "location=", 9) == 0)
		    {
			status = parse_loc_list(jsx, &jsx->jsx_loc_list[0],
					&argv[i][10], JSX_MAX_LOC);
			if (status != E_DB_OK)
			    break;
			else
			    jsx->jsx_status1 |= JSX_LOC_LIST;
		    }
		    else if (MEcmp(&argv[i][1], "location", 8) == 0)
			     SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
	        else if (operator == JSX_O_ATP)
                {
		/* Auditdb option -limit_output will restrict the output of 
		** auditdb to a single line.  
		*/ 
                    if (STbcompare(&argv[i][1], 0, "limit_output", 0, 0) == 0)
                    {
                        jsx->jsx_status2 |= JSX2_LIMIT_OUTPUT;
                    }
		    else 
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
	        else	
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'a':
		/*
		** There are two auditdb options starting with "a", one to
		** audit all system catalog updates, another to request
		** that everything in the journal file be displayed.
		** Another case is when the rollbacked transactions are
		** also required in the output. This was added for visual
		** journal analyser's internal use. 
		*/
		if (operator == JSX_O_ATP)
		{
		    if (STcmp(&argv[i][1],  "a" ) == 0)
		    {
			jsx->jsx_status |= JSX_SYSCATS;
		    }
		    else if (STcmp(&argv[i][1], "all" ) == 0)
		    {
			jsx->jsx_status |= JSX_AUDIT_ALL;
		    }
            	    else if(STbcompare(&argv[i][1], 0, 
                                    "aborted_transactions", 0, 0) == 0)
            	    {
                	jsx->jsx_status2 |= JSX2_ABORT_TRANS;
                    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
                else if (operator & JSX_O_ALTER)
                {
                    if (MEcmp(&argv[i][1], "allow_ddl", 9) == 0)
                    {
                        jsx->jsx_status |= JSX_JOFF;
                        jsx->jsx_status2 |= JSX2_PMODE;
                    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'b':
                if (operator == JSX_O_FLOAD)
                {
                    if (MEcmp(&argv[i][1], "bsize=", 6) == 0)
                    {
                        status = CVan(&argv[i][7], &fload_bsize);
			if (status != E_DB_OK)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                    }
                    else if (MEcmp(&argv[i][1], "bsize", 5) == 0)
			     SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                    break;
                }

		/*  Transaction begin time. */

		if ((operator == JSX_O_ATP) || (operator == JSX_O_RFP))
		{
		    i4	length = STlength(&argv[i][2]);

		    if (length)
		    {
			status = TMstr_stamp(&argv[i][2],
			    (TM_STAMP *)&jsx->jsx_bgn_time);
			if (status == OK)
			    jsx->jsx_status |= JSX_BTIME;
			else
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105A_JSP_INC_ARG);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'c':
		if (operator == JSX_O_RFP)
		{
		    if (cused == 'Y')
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
			break;
		    }
		    else
			cused = 'Y';

		    /*  Don't use checkpoint for RFP. */

		    jsx->jsx_status &= ~JSX_USECKP;
		    break;
		}
		else if (operator == JSX_O_FLOAD)
		{
		    /* -cache_name=xxxx for fastload/fastdump */
		    if (MEcmp(&argv[i][1], "cache_name=",11) == 0)
		    {
			STlcopy(&argv[i][12],&cache_name[0], sizeof(cache_name));
			connect_only = TRUE;
			break;
		    }
		}
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'd':
		/*
		** dmf_cache_size: Override default buffers for specified 
		**                 page size.
		**
		** Operation dependent:
		**
		** For ckpdb -d:
		**	Destroy previous journals, dumps and checkpoints.
		**
		** For Alterdb -disable_journaling:
		**	Disable journaling on the database.  We also turn
		**	on the JSX_NOLOGGING flag so that we will not log
		**	the opendb operation in search_dcb.  This allows
		**	alterdb to run while the logfile is full.
		**	Note - we enforce the full spelling of alterdb options.
		**
		** For Alterdb -delete_oldest_ckp:
		**	Destroy oldest journal, dump and checkpoint.
		**
		** For Alterdb -disable_mvcc:
		**	Disable use of MVCC on the next open of the 
		**	database.
		*/
		if (MEcmp(&argv[i][1], "dmf_cache_size=", 15) == 0)
		{
		    status = CVal(&argv[i][16], &dmf_cache_size[0] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if (MEcmp(&argv[i][1], "dmf_cache_size_4k=", 18) == 0)
		{
		    status = CVal(&argv[i][19], &dmf_cache_size[1] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if (MEcmp(&argv[i][1], "dmf_cache_size_8k=", 18) == 0)
		{
		    status = CVal(&argv[i][19], &dmf_cache_size[2] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if (MEcmp(&argv[i][1], "dmf_cache_size_16k=", 19) == 0)
		{
		    status = CVal(&argv[i][20], &dmf_cache_size[3] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if (MEcmp(&argv[i][1], "dmf_cache_size_32k=", 19) == 0)
		{
		    status = CVal(&argv[i][20], &dmf_cache_size[4] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if (MEcmp(&argv[i][1], "dmf_cache_size_64k=", 19) == 0)
		{
		    status = CVal(&argv[i][20], &dmf_cache_size[5] );
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else if ((operator == JSX_O_CPP) &&
		    (STcmp(&argv[i][1], "d" ) == 0))
		{
		    jsx->jsx_status |= JSX_DESTROY;
		}
		else if ((operator == JSX_O_ALTER) &&
		        (STcompare("delete_oldest_ckp", &argv[i][1]) == 0))
		{
		    jsx->jsx_status |= JSX_ALTDB_CKP_DELETE;
		    jsx->jsx_keep_ckps = 1;
		}
                else if ((operator == JSX_O_ALTER) &&
                        (STcompare("delete_invalid_ckp", &argv[i][1]) == 0))  
                {
                     jsx->jsx_status2 |= JSX2_ALTDB_CKP_INVALID;  

                }  
		else if ((operator == JSX_O_ALTER) &&
		        (STcompare("disable_journaling",  &argv[i][1]) == 0))
		{
		    jsx->jsx_status |= (JSX_JDISABLE | JSX_NOLOGGING);
		}
		else if ((operator == JSX_O_ALTER) &&
		        (STcompare("disable_mvcc",  &argv[i][1]) == 0))
		{
		    if ( !(jsx->jsx_status2 & JSX2_ALTDB_EMVCC) )
			jsx->jsx_status2 |= JSX2_ALTDB_DMVCC;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		}
                else if ((operator == JSX_O_ALTER) &&
                        (STcompare("disable_mustlog",  &argv[i][1]) == 0))
                {
                    if ( !(jsx->jsx_status2 & JSX2_ALTDB_EMUSTLOG) )
                        jsx->jsx_status2 |= JSX2_ALTDB_DMUSTLOG;
                    else
                        SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
                }
		else if (MEcmp(&argv[i][1], "dmf_cache_size", 14) == 0)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);

		break;

	    case 'e':

		/*
		** auditdb and rollforward support -end_lsn.
		*/
		if (operator == JSX_O_ATP &&
		    STbcompare(&argv[i][1], 0, "expand_lobjs", 0, 0) == 0)
		{
		    jsx->jsx_status2 |= JSX2_EXPAND_LOBJS;
		}
		else if ( (operator == JSX_O_ATP || operator == JSX_O_RFP) &&
			  (MEcmp(&argv[i][1], "end_lsn=", 8) == 0) )
		{
		    status = parse_lsn(&argv[i][9], &jsx->jsx_end_lsn);
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			jsx->jsx_status1 |= JSX_END_LSN;
		}
		else if ((operator == JSX_O_ATP) || (operator == JSX_O_RFP))
		{
		    /*  Transaction end time. */

		    i4	length = STlength(&argv[i][2]);

		    if (length)
		    {
			status = TMstr_stamp(&argv[i][2],
			    (TM_STAMP *) &jsx->jsx_end_time);
			if (status == OK)
			    jsx->jsx_status |= JSX_ETIME;
			else
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105A_JSP_INC_ARG);
		}
		else if ((operator == JSX_O_ALTER) &&
		        (STcompare("enable_mvcc",  &argv[i][1]) == 0))
		{
		    if ( !(jsx->jsx_status2 & JSX2_ALTDB_DMVCC) )
			jsx->jsx_status2 |= JSX2_ALTDB_EMVCC;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		}
                else if ((operator == JSX_O_ALTER) &&
                        (STcompare("enable_mustlog",  &argv[i][1]) == 0))
                {
                    if ( !(jsx->jsx_status2 & JSX2_ALTDB_DMUSTLOG) )
                        jsx->jsx_status2 |= JSX2_ALTDB_EMUSTLOG;
                    else
                        SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
                }
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);

		break;

	    case 'f':
                if (operator == JSX_O_FLOAD)
                {
                    if (MEcmp(&argv[i][1], "file=", 5) == 0)
                    {
                        PTR         src = &argv[i][6];
                        PTR         dest = (PTR)&jsx->jsx_fname_list[0];
 
                        j = 0;
                        MEfill(sizeof(jsx->jsx_fname_list), '\0', dest);
 
                        while (*dest++ = *src++)
                        {
                            if (*src == ',')
                            {
                                j++, src++;
                                if (dest >= (PTR)&jsx->jsx_fname_list[j])
                                {
                                    /*
                                    ** File name too long.
                                    */
				    SETDBERR(&jsx->jsx_dberr, 0, E_DM104F_JSP_NAME_LEN);
                                    break;
                                }
                                dest = (PTR)&jsx->jsx_fname_list[j];
                            }
                        }
 
                        *--dest = '\0';
                        jsx->jsx_fname_cnt = ++j;
                        jsx->jsx_status |= JSX_FNAME_LIST;
                    }
                    else if (MEcmp(&argv[i][1], "file", 4) == 0)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }

		/*  Name of audit trail file. */

		else if (operator == JSX_O_ATP)
		{
		    /*
		    ** There are two cases to handle, the older -f
		    ** and the newer -file=.
		    ** The older -t -f permits audit information to
		    ** be written to a single file named audit.trl.
		    ** -table and -file options allow audit
		    ** information to be written to multiple files, the
		    ** names of which are either specified here, or are
		    ** derived from the file name.  -file with out a
		    ** file name list requests that AUDITDB create
		    ** file names of the format <tbl_name>.trl.
		    */
		    if (STcmp(&argv[i][1], "f" ) == 0)
		    {
			/*
			** -f case, hardcode file name inserted by dmfatp.c
			*/
			jsx->jsx_fname_cnt = 1;
			jsx->jsx_status |= JSX_AUDITFILE;
		    }
		    else if (STcmp(&argv[i][1], "file" ) == 0)
		    {
			/*
			** This is a request to have AUDITDB create .trl
			** files automatically.
			*/
			jsx->jsx_fname_cnt = 0;
			jsx->jsx_status |= JSX_FNAME_LIST;
		    }
		    else if (MEcmp(&argv[i][1], "file=", 5) == 0)
		    {
			PTR	    src = &argv[i][6];
			PTR    	    dest = (PTR)&jsx->jsx_fname_list[0];

			/*
			** Copy the list of file names to jsx_tbl_list.
			*/
			j = 0;
			MEfill(sizeof(jsx->jsx_fname_list), '\0', dest);

			while (*dest++ = *src++)
			{
			    if (*src == ',')
			    {
				j++, src++;
				if (dest >= (PTR)&jsx->jsx_fname_list[j])
				{
				    /*
				    ** File name too long.
				    */
				    SETDBERR(&jsx->jsx_dberr, 0, E_DM104F_JSP_NAME_LEN);
				    break;
				}
				dest = (PTR)&jsx->jsx_fname_list[j];
			    }
			}

			*--dest = '\0';
			jsx->jsx_fname_cnt = ++j;
			jsx->jsx_status |= JSX_FNAME_LIST;
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
# ifdef NOTYET
                else if ( operator == JSX_O_RFP )
                {
                    if (STncmp( &argv[i][1],
                                   "force_logical_consistent", 24 ) == 0)
                    {
                        /*
                        ** If "point-in-time" recovery donot mark tables
                        ** logically inconsistent
                        */
                        jsx->jsx_status1 |= JSX1_F_LOGICAL_CONSIST;
                    }
                }
# endif
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'h':

		if (STcompare(&argv[i][1], "help") == 0)
		{
		    jsx->jsx_status1 |= JSX_HELP;
		    break;
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'i':

		if (operator == JSX_O_ATP)
		{
		    if (STncmp(&argv[i][1], "inconsistent", 12) == 0)
		    {
		        /*
			** auditdb - open inconsistent database.
			*/
			jsx->jsx_status1 |= JSX_ATP_INCONS;
		    }
            	    else if(STbcompare(&argv[i][1], 13,
					 "internal_data", 13, 0) == 0)
            	    {
           		/*
			** auditdb -internal_data option to print out lsn
			** and tid information. added to support the visual
			** journal analyser project. 
			*/ 
			jsx->jsx_status2 |= JSX2_INTERNAL_DATA;
            	    }
		    else
		    {
		        /*
			** Assume we have name of user to restrict audit.
			** Bug if username is "nconsistent".
			*/

			jsx->jsx_aname_delim = FALSE;
			jsx->jsx_status |= JSX_ANAME;

			if (argv[i][2] == '\"')
			{
			    /*
			    ** Delimited id found.  Normalize w/o case xlation,
			    ** which is per-database and performed later.
			    */
			    len_trans = DB_OWN_MAXNAME;
			    status = jsp_normalize(jsx, 0,
				(u_char *)&argv[i][2], &len_trans, unpad_owner);
			    if (status != E_DB_OK)
			    {
				dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
				SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
			    }
			    else
			    {
				jsx->jsx_aname_delim = TRUE;
				MEmove(len_trans, (char *)unpad_owner, ' ',
				    DB_OWN_MAXNAME,
				    (char *)jsx->jsx_a_name.db_own_name);
			    }
			}
			else
			{
			    STmove(&argv[i][2], ' ', sizeof(jsx->jsx_a_name),
				jsx->jsx_a_name.db_own_name);
			}
		    }
		}
		else if (operator == JSX_O_ALTER)
		{
		  if (MEcmp(&argv[i][1], "init_jnl_blocks=", 16) == 0)
		  {
		    status = CVal(&argv[i][17], &jsx->jsx_init_jnl_blocks);
		    if (status == OK)
			jsx->jsx_status1 |= JSX_JNL_INIT_BLOCKS;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		  }
                  else 
                  {
                       if (argv[i][2] !=EOS)
                       {
                        if (cui_chk3_locname(&argv[i][2]) != OK)
                         {
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1406_ALT_INV_UCOLLATION);
                         }
                       else
                         {
                          /*Take the user input for collation table*/
                          STcopy(&argv[i][2],jsx->jsx_ucollation);
                          jsx->jsx_status2  |= JSX2_ALTDB_UNICODEC;
                          break;
                         }
                       }
                       else
                        {
                         /* for unicode, use the default collation table unless
                             another is specified */
                          STcopy("udefault",jsx->jsx_ucollation);
                         jsx->jsx_status2  |= JSX2_ALTDB_UNICODEC;
                        }
                  }
		}
                else if (operator == JSX_O_RFP)
		{
		    if (MEcmp(&argv[i][1], "ignore", 6) == 0)
		    {
			jsx->jsx_status1 |= JSX1_IGNORE_FLAG;
		    }
		    else if (MEcmp(&argv[i][1], "incremental", 11) == 0)
		    {
			jsx->jsx_status2 |= JSX2_INCR_JNL;
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'j':
		/*  Operation dependent. */

		if (STcmp(&argv[i][1], "j" ) == 0)
		{
		   if (jused == 'Y')
		   {
		      SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		      break;
		   }
		   else
		      jused = 'Y';

		    if (operator == JSX_O_CPP)
		    {
			if (((jsx->jsx_status & JSX_JON) == 0) &&
			     (jsx->jsx_status1 & JSX1_CKPT_DB))
			    jsx->jsx_status |= JSX_JOFF;
			else
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
			}

			/*
			** Turning on/off journaling on database requires
			** exclusive lock.
			*/
			jsx->jsx_status |= JSX_CKPXL;
		    }
		    else if (operator == JSX_O_RFP)
		    {
			jsx->jsx_status &= ~JSX_USEJNL;
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		else if ((operator == JSX_O_ALTER) &&
		         (MEcmp(&argv[i][1], "jnl_block_size=", 15) == 0))
		{
		    status = CVal(&argv[i][16], &jsx->jsx_jnl_block_size);
		    if (status == OK)
			jsx->jsx_status1 |= JSX_JNL_BLK_SIZE;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'k':
                if ((operator == JSX_O_ALTER) &&
                    (MEcmp(&argv[i][1], "keep=", 5) == 0))
                {
		    jsx->jsx_status2 |= JSX2_ALTDB_KEEP_N;
		    jsx->jsx_status |= JSX_ALTDB_CKP_DELETE;
                    status = CVan(&argv[i][6], &jsx->jsx_keep_ckps);
		    if (status != E_DB_OK || (jsx->jsx_keep_ckps <= 0))
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;
	    case 'm':

		if ((operator == JSX_O_CPP) || (operator == JSX_O_RFP))
                {
                    /*
                    ** -m case
                    ** Insert device names into jsx, update thangs.
                    */
                    status = parse_device_list(jsx, &argv[i][2]);
                    if (status != E_DB_OK)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105A_JSP_INC_ARG);
                        break;
		    }
                 }
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'n':
		/*
		** For Rollforward:
		**   "norollback" - option specifies to bypass the rollback
		** 	phase of rollforward and leave the database in the
		**      exact state described by the journal files.
		**
		**      If used in conjuction with the -e flag, then the
		**      database is left in the state described by the journal
		**      files up to the time specified.  Any incomplete
		**      transactions are left incomplete.
		**
		**   "nologging" - option specifies to bypass logging
		** 	CLR records during ROLLFORWARD UNDO.  ET log records
		**	are still written.  Presumably one would perform a
		** 	checkpoint immediately after this operation.
		**
                **   "nosecondary_index" - If performing table level
                **    point-in-time recovery the default is to recover
                **    all tables seconadry indicies. This flag overrides
                **    these default behaviour.
                **
                **   "noblobs" - If performing table level recovery
                **    the user can override recovery of blobs on this
                **    table.
		**
		**   "new_location" - To give a list of new data locations
		**
		*/
		if (operator == JSX_O_RFP)
		{
		    if (STcompare("norollback", &argv[i][1]) == 0)
		    {
			if (jsx->jsx_status2 & JSX2_RFP_ROLLBACK)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
			else
			    jsx->jsx_status |= JSX_NORFP_UNDO;
		    }
		    else if (STcompare("nologging", &argv[i][1]) == 0)
			jsx->jsx_status1 |= JSX_NORFP_LOGGING;
                 /* Following code is commented out to remove -noblobs support
                 ** else if (STncmp( &argv[i][1], "noblobs", 7) == 0)
                 ** {
                 **     jsx->jsx_status1 |= JSX1_NOBLOBS;
                 ** }
                 */
                    else if (STncmp( &argv[i][1], "nosecondary_index", 17) == 0)
                    {
                        jsx->jsx_status1 |= JSX1_NOSEC_IND;
                    }
		    else if (MEcmp(&argv[i][1], "new_location=", 13) == 0)
		    {
			status = parse_loc_list(jsx, &jsx->jsx_nloc_list[0],
						&argv[i][14], JSX_MAX_LOC);
			if (status != E_DB_OK)
			    break;
			else
			    jsx->jsx_status1 |= JSX_NLOC_LIST;
		    }
		    else if (MEcmp(&argv[i][1], "new_location", 12) == 0)
				SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		/*
		** For relocate:
		**
		**   "new_ckp_location" - To relocate the default ckp location
		**
		**   "new_dump_location" - To relocate the default dmp location
		**
		**   "new_work_location" - To relocate the default work location
		**
		**   "new_jnl_location" - To relocate the default jnl location
		**
		**   "new_location" - To give a list of new data locations
		**
		**   "new_database" - To make a copy of the db
		**
		*/
		else if (operator == JSX_O_RELOC)
		{
		    if (MEcmp(&argv[i][1], "new_ckp_location=", 17) == 0)
		    {
			status = parse_loc_list(jsx,
						&jsx->jsx_def_list[JSX_CKPTLOC],
						&argv[i][18], 1);
			if (status != E_DB_OK)
			    break;
			else
			{
			    jsx->jsx_status1 &= ~JSX1_RECOVER_DB;
			    jsx->jsx_status1 |= JSX_CKPT_RELOC;
			}
		    }
		    else if (MEcmp(&argv[i][1], "new_dump_location=",18) == 0)
		    {
			status = parse_loc_list(jsx,
						&jsx->jsx_def_list[JSX_DMPLOC],
						&argv[i][19], 1);
			if (status != E_DB_OK)
			    break;
			else
			{
			    jsx->jsx_status1 &= ~JSX1_RECOVER_DB;
			    jsx->jsx_status1 |= JSX_DMP_RELOC;
			}
		    }
		    else if (MEcmp(&argv[i][1], "new_work_location=",18) == 0)
		    {
			status = parse_loc_list(jsx,
					      &jsx->jsx_def_list[JSX_WORKLOC],
						&argv[i][19], 1);
			if (status != E_DB_OK)
			    break;
			else
			{
			    jsx->jsx_status1 &= ~JSX1_RECOVER_DB;
			    jsx->jsx_status1 |= JSX_WORK_RELOC;
			}
		    }
		    else if (MEcmp(&argv[i][1], "new_jnl_location=", 17) == 0)
		    {
			status = parse_loc_list(jsx,
						&jsx->jsx_def_list[JSX_JNLLOC],
						&argv[i][18], 1);
			if (status != E_DB_OK)
			    break;
			else
			{
			    jsx->jsx_status1 &= ~JSX1_RECOVER_DB;
			    jsx->jsx_status1 |= JSX_JNL_RELOC;
			}
		    }
		    else if (MEcmp(&argv[i][1], "new_location=", 13) == 0)
		    {
			status = parse_loc_list(jsx, &jsx->jsx_nloc_list[0],
						&argv[i][14], JSX_MAX_LOC);
			if (status != E_DB_OK)
			    break;
			else
			    jsx->jsx_status1 |= JSX_NLOC_LIST;
		    }
		    else if (MEcmp(&argv[i][1], "new_database=",13) == 0)
		    {
			   /* Bug 91603. Ensure database name is valid */
			   if (cui_chk1_dbname(&argv[i][14]) != OK) 
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
			else
			{
			    CVlower(&argv[i][14]);
			    MEmove(STlength(&argv[i][14]), &argv[i][14], ' ',
				DB_DB_MAXNAME, (char *)&jsx->jsx_newdbname);
			    jsx->jsx_status1 |= JSX_DB_RELOC;
			}
		    }
		    else if ((MEcmp(&argv[i][1], "new_ckp_location", 16) == 0) 
			    || (MEcmp(&argv[i][1], "new_dump_location",17) == 0)
			    || (MEcmp(&argv[i][1], "new_work_location",17)== 0)
			    || (MEcmp(&argv[i][1], "new_jnl_location", 16) == 0)
			    || (MEcmp(&argv[i][1], "new_database",12) == 0))
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
                else if (operator & JSX_O_ALTER)
                {
                  if (MEcmp(&argv[i][1], "noonline_checkpoint", 19) == 0)
                  {
                      jsx->jsx_status |= JSX_JOFF;
                      jsx->jsx_status2 |= JSX2_OCKP;
                  }
                  else if (MEcmp(&argv[i][1], "noallow_ddl", 11) == 0)
                  {
                          jsx->jsx_status |= JSX_JON;
                          jsx->jsx_status2 |= JSX2_PMODE;
                  }
                  else if (MEcmp(&argv[i][1], "next_jnl_file", 13) == 0)
                  {
                          jsx->jsx_status2 |= JSX2_SWITCH_JOURNAL;
                  }
                  else 
                  {
                       if (argv[i][2] !=EOS)
                       {
                        if (cui_chk3_locname(&argv[i][2]) != OK)
                         {
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1406_ALT_INV_UCOLLATION);
                         }
                       else
                         {
                          /*Take the user input for collation table*/
                          STcopy(&argv[i][2],jsx->jsx_ucollation);
                          jsx->jsx_status2  |= JSX2_ALTDB_UNICODED;
                          break;
                         }
                       }
                       else
                        {
                         /* for unicode, use the default collation table unless
                             another is specified */
                          STcopy("udefault",jsx->jsx_ucollation);
                         jsx->jsx_status2  |= JSX2_ALTDB_UNICODED;
                        }
                  }
                }
		/*
		** For Auditdb 
		**	-no_header: this option will avoid printing out the page print 
		**		    headers from the output of auditdb
		*/
                else if (operator == JSX_O_ATP)
                {
                    if (STbcompare(&argv[i][1], 0, "no_header", 0, 0) == 0)
                    {
                        jsx->jsx_status2 |= JSX2_NO_HEADER;
                    }
		    else if (STbcompare(&argv[i][1], 0, "expand_lobjs", 0, 0)
			     == 0)
		    {
                        jsx->jsx_status2 |= JSX2_EXPAND_LOBJS;
		    }
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }                                                              
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);

		break;

	    case 'o':
		/*  Use online checkpoint - turn off EX-LOCK status */
                if (operator == JSX_O_CPP)
                {
                    jsx->jsx_status &= ~JSX_CKPXL;
                }
                else if ( operator == JSX_O_RFP )
                {
                    if (STncmp( &argv[i][1], "online", 6) == 0)
                    {
		    /* Disable until implemented
                        jsx->jsx_status1 |= JSX1_ONLINE;
		    */
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                    }
                    else if (STncmp( &argv[i][1], 
                                         "on_error_continue", 17) == 0)
                    {
                        jsx->jsx_status1 |= JSX1_ON_ERR_CONT;
                    }
		    else if (STncmp( &argv[i][1],
					  "on_error_prompt", 15) == 0)
		    {
			jsx->jsx_status2 |= JSX2_ON_ERR_PROMPT;
		    }
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
                else if (operator & JSX_O_ALTER)
                {
                  if (MEcmp(&argv[i][1], "online_checkpoint", 17) == 0)
                  {
                      jsx->jsx_status |= JSX_JON;
                      jsx->jsx_status2 |= JSX2_OCKP;
                  }
                  else
                  {
		      SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                  }
                }
                else
                {
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
                break;

	    case 'p':
		/*  Audit by process id, not supported. */
		break;

	    case 'q':
		/*  Audit by QUEL command, not supported. */
		break;

	    case 'r':
		/*
		** For Rollforward:
		**   "relocate" - To specify table/location/database relocation
		*/
		if (operator == JSX_O_RFP)
		{
		    if (MEcmp(&argv[i][1], "relocate", 8) == 0)
		    {
			jsx->jsx_status1 |= JSX_RELOCATE;
		    }
		    else if (MEcmp(&argv[i][1], "rollback", 8) == 0)
		    {
			/*
			** -rollback is the default for normal rollforwarddb
			** However it must be EXPLICITLY specified for
			** incremental rollforwarddb,
			** and cannot be requested with -norollback
			*/
			if (jsx->jsx_status & JSX_NORFP_UNDO)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
			else
			    jsx->jsx_status2 |= JSX2_RFP_ROLLBACK;
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 's':

	  	/*
		** auditdb and rollforward support -start_lsn.
		*/
		if ((operator == JSX_O_ATP || operator == JSX_O_RFP) &&
		    (MEcmp(&argv[i][1], "start_lsn=", 10) == 0))
		{
		    status = parse_lsn(&argv[i][11], &jsx->jsx_start_lsn);
		    if (status != OK)
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		    else
			jsx->jsx_status1 |= JSX_START_LSN;
		}
                else if ( operator == JSX_O_RFP)
                {
                    if (STncmp( &argv[i][1], "statistics", 10) == 0)
                    {
                        jsx->jsx_status1 |= JSX1_STATISTICS;
                    }
/* ---> Removed until implemented
                    else if (STncmp( &argv[i][1], 
                                    "status_rec_count=", 17) == 0)
                    {
                        status = CVal( &argv[i][18],
                                       &jsx->jsx_rfp_status_rec_cnt);
                        if (status != OK)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                        else
                            jsx->jsx_status1 |= JSX1_STAT_REC_COUNT;
                    }
 <---- */
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
		else if (STcmp(&argv[i][1], "s" ) == 0)
		{
		    /*
		    **	No-op. Issue a warning per LRC decision
		    */
		    dmfWriteMsg(NULL, W_DM1038_JSP_SUPU_IGNORED, 0);
		}

		break;

	    case 't':
                if (operator == JSX_O_FLOAD)
                {
                    if (MEcmp(&argv[i][1], "table=", 6) == 0)
                    {
                        /*
                        ** -table= case
                        ** Insert table names into jsx, update thangs.
                        */
                        status = parse_table_list(jsx, &argv[i][7]);
                        if (status != E_DB_OK)
                            break;
                    }
                    else if (MEcmp(&argv[i][1], "table", 5) == 0)
				SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
		else if (operator == JSX_O_ATP)
		{
		    /*
		    ** AUDITDB, select list of tables to audit
		    */

		    /*
		    ** There are two cases to handle here, the older -t
		    ** and the newer -table=.  -t allows auditing of a single
		    ** table and, when used in combination with -f, creates
		    ** an audit trail file of the name audit.trl.  -table=
		    ** allows multiple tables and when used with -f creates
		    ** multiple .trl files.
		    */
		    if (MEcmp(&argv[i][1], "table=", 6) == 0)
		    {
			/*
			** -table= case
			** Insert table names into jsx, update thangs.
			*/
			status = parse_table_list(jsx, &argv[i][7]);
			if (status != E_DB_OK)
			    break;
		    }
		    else
		    {
			/*
			** -t case remains for backward compatibility,
			** does not support delimited id's
			*/
			STmove(&argv[i][2], ' ', sizeof(DB_TAB_NAME),
			    (char *)&jsx->jsx_tbl_list[0].tbl_name.db_tab_name);

			MEfill(
			    sizeof(jsx->jsx_tbl_list[0].tbl_owner.db_own_name),
			    0,
			    (char *)jsx->jsx_tbl_list[0].tbl_owner.db_own_name);

			jsx->jsx_tbl_list[0].tbl_delim = FALSE;
			jsx->jsx_tbl_cnt = 1;
			jsx->jsx_status |= JSX_TNAME;
		    }
		}
		else if ((operator == JSX_O_ALTER) &&
			 (MEcmp(&argv[i][1], "target_jnl_blocks=", 18) == 0))
		{
		    /*
		    ** ALTERDB -target_jnl_blocks
		    */

		    status = CVal(&argv[i][19], &jsx->jsx_max_jnl_blocks);
		    if (status == OK)
			jsx->jsx_status1 |= JSX_JNL_NUM_BLOCKS;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		}
                else if ( operator == JSX_O_RFP )
                {
                    if (STncmp( &argv[i][1], "table=", 6 ) == 0)
                    {
                        /*
                        ** Turn off database level recovery
                        */
                        jsx->jsx_status1 &= ~JSX1_RECOVER_DB;

                        /*
                        ** -table= case
                        ** Insert table names into jsx, update thangs.
                        */
                        status = parse_table_list(jsx, &argv[i][7]);
                        if (status != E_DB_OK)
                            break;
                    }
                    else if (STncmp( &argv[i][1], "table", 5 ) == 0)
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
                else if ( operator == JSX_O_CPP )
                {
                    if (STncmp( &argv[i][1], "table=", 6 ) == 0)
                    {
                        /*
                        ** Turn off database level checkpoint
                        */
			if ((jsx->jsx_status & (JSX_JON | JSX_JOFF)) == 0) 
			{
                            jsx->jsx_status1 &= ~JSX1_CKPT_DB;
			    jsx->jsx_status1 |= JSX1_CKPT_TABLE;
			}
			else
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
			    break;
			}

                        /*
                        ** -table= case
                        ** Insert table names into jsx, update thangs.
                        */
                        status = parse_table_list(jsx, &argv[i][7]);
                        if (status != E_DB_OK)
                            break;
                    }
                    else if (STncmp( &argv[i][1], "timeout=", 8) == 0)
                    {
                        char       *colonpos;
                        i4    val = 0;

                        colonpos = STchr(&argv[i][1], ':');
                        if (colonpos)
                        {
                            if (STlength(colonpos + 1) != 2)
                            {
				SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                                break;
                            }
                            *colonpos = '\0';
                        }

                        /* get minutes */
                        status = CVal (&argv[i][9], &val);
                        if ((status != OK) || (val < 0))
                        {
                            *colonpos = ':';
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                            break;
                        }

                        /* change minutes to seconds */
                        jsx->jsx_stall_time = val * 60;

                        if (colonpos)
                        {
                            status = CVal((colonpos + 1), &val);
                            if ((status != OK) || (val < 0) ||
                                (val > 59))
                            {
                                *colonpos = ':';
				SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                                break;
                            }
                            jsx->jsx_stall_time += val;
                        }
                        jsx->jsx_status1 |= JSX1_STALL_TIME;
                    }
                    else if ((STncmp(&argv[i][1], "table",5) == 0)
		    	  || (STncmp(&argv[i][1], "timeout",7) == 0))
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
                }
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);

		break;

	    case 'u':

		/*
		** Username override.
		*/
		jsx->jsx_status |= JSX_UNAME;
		jsx->jsx_username_delim = FALSE;

		if (argv[i][2] == '\"')
		{
		    /*
		    ** Delimited id found.
		    ** Normalize without case translation.  Case xlation is
		    ** per-database and is performed later.
		    */
		    len_trans = DB_OWN_MAXNAME;
		    status = jsp_normalize(jsx, 0, (u_char *)&argv[i][2],
			&len_trans, unpad_owner);
		    if (status != E_DB_OK)
		    {
			dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		    }
		    else
		    {
			jsx->jsx_username_delim = TRUE;
			MEmove(len_trans, (char *)unpad_owner, ' ',
			    DB_OWN_MAXNAME,
			    (char *)jsx->jsx_username.db_own_name);
		    }
		}
		else
		{
		    STmove(&argv[i][2], ' ', sizeof(jsx->jsx_username),
			(char *)&jsx->jsx_username.db_own_name);
		}

		break;

	    case 'v':
		/*
		** Verbose operation.
		** Accept either -v or -verbose.
		*/
		jsx->jsx_status |= JSX_VERBOSE;

		if (! ((STcmp(&argv[i][1], "verbose" ) == 0) ||
		       (STcmp(&argv[i][1], "v" ) == 0)) )
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}

		break;

	    case 'w':
		if (wused == 'Y')
		{
		  SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		  break;
		}
		else
		  wused = 'Y';

		/*
		** Two -w cases: wait for DB lock and auditdb -wait.
		*/
                if ((   (operator == JSX_O_CPP)
                     || (operator == JSX_O_RFP)
                     || (operator == JSX_O_RELOC)
		     || (operator == JSX_O_FLOAD)
                     || (operator == JSX_O_ALTER)) &&
		     (STcmp(&argv[i][1], "w" ) == 0))
	  	{
		    /*  Don't wait for DB lock. */
		    jsx->jsx_status &= ~JSX_WAITDB;
		}
		else if ((operator == JSX_O_ATP) &&
		        (STcompare(&argv[i][1], "wait") == 0))
		{
		    /*
		    ** Auditdb wait for archiver purge.
		    */
		    jsx->jsx_status1 |= JSX_ATP_WAIT;
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);

		break;

	    case 'z':
		/*  Just cause the log file to be drained. */

		if (operator == JSX_O_CPP)
		{
		    jsx->jsx_status |= JSX_DRAINDB;
		    /* Turn off checkpointing data indicators */
		    jsx->jsx_status1 &= ~JSX1_CKPT_DB;
		    jsx->jsx_status1 &= ~JSX1_CKPT_TABLE;
		}   
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    default:
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    }

	}
	else if (argv[i][0] == '+')
	{
	    switch(argv[i][1])
	    {
	    case 'c':
		if (cused == 'Y')
		{
		  SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		  break;
		}
		else
		  cused = 'Y';
		
		/*  Use checkpoint for RFP. */

		if (operator == JSX_O_RFP)
		    jsx->jsx_status |= JSX_USECKP;
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'j':
		if (jused == 'Y')
		{
		   SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		   break;
		}
		else
		   jused = 'Y';
		
		/*  Operation dependent. */

		if (STcmp(&argv[i][1], "j" ) == 0)
		{
		    if (operator == JSX_O_CPP)
		    {
			if (((jsx->jsx_status & JSX_JOFF) == 0) &&
			     (jsx->jsx_status1 & JSX1_CKPT_DB))
			   jsx->jsx_status |= JSX_JON;
			else
			  SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		        /*
		        ** Turning on/off journaling on database requires
		        ** exclusive lock.
		        */
		        jsx->jsx_status |= JSX_CKPXL;
		    }
		    else if (operator == JSX_O_RFP)
			jsx->jsx_status |= JSX_USEJNL;
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'w':
		if (wused == 'Y')
		{
		  SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		  break;
		}
		else
		  wused = 'Y';
		
		/*  Wait for DB lock. */

		jsx->jsx_status |= JSX_WAITDB;
		break;

	    default:
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    }
	}
	else if (argv[i][0] == '#')
	{
	    switch(argv[i][1])
	    {
	    case 'c':
		
		/*
		** Control which checkpoint sequence number to use
		** in rollforward or auditdb processing.
		*/
		if ((operator == JSX_O_RFP) || (operator == JSX_O_ATP))
		{
		    if (jsx->jsx_status & JSX_CKP_SELECTED)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		    }
		    else if (argv[i][2] != EOS)
		    {
			status = CVal(&argv[i][2], &jsx->jsx_ckp_number );
			if (status == OK)
			    jsx->jsx_status |= JSX_CKP_SELECTED;
			else
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
			}
		    }
		    else if (operator == JSX_O_ATP)
		    {
			/*
			** AUDITDB requires specification of a
			** checkpoint sequence number.
			*/
			dmfWriteMsg(NULL, E_DM1219_ATP_INV_CKP, 0);
			SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
		    }
		    else
		    {
			jsx->jsx_status |= JSX_LAST_VALID_CKP;
		    }
		}
                else if (operator == JSX_O_INF)
                {
                    /*
                    ** 11-sep-95 (thaju02)
                    ** enables viewing of table list file.
                    */
		    if (argv[i][2] != EOS)
                    {
                        status = CVal(&argv[i][2], &jsx->jsx_ckp_number);
                        if (status == OK)
                            jsx->jsx_status1 |= JSX1_OUT_FILE;
                        else
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
                    }
		    else
		    {
			jsx->jsx_status1 |= JSX1_OUT_FILE;
			jsx->jsx_ckp_number = 0;
		    }
                }
		else
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		break;

	    case 'f':
		/*
		** Flag to allow rollforward to run on a journal
		** disabled database.
		*/
		if (operator == JSX_O_RFP)
		{
		    jsx->jsx_status1 |= JSX_FORCE;
		}
		else
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		}
		break;

	    case 'm':
		if ((operator == JSX_O_CPP) || (operator == JSX_O_RFP))
                {
                    /*
                    ** #m[n] case
                    ** Number of parallel cpp of rfp to run at one time.
                    */
		    if (argv[i][2] != EOS)
		    {
			status = CVal(&argv[i][2], &jsx->jsx_dev_cnt );
			if (status != OK)
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM105C_JSP_INV_ARG_VAL);
			}
			if (jsx->jsx_dev_cnt < 1)
		 	    jsx->jsx_dev_cnt = 1;
			for( j=0; j<jsx->jsx_dev_cnt; j++)
			    jsx->jsx_device_list[j].dev_pid = -1;
		    }
		    else 
		    {
                    	/*
                    	** Default to single device.
                    	*/
		 	jsx->jsx_dev_cnt = 1;
		    }
                 }
                    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;

	    case 'x':

		jsx->jsx_status |= JSX_TRACE;
                
		if (operator == JSX_O_CPP &&
		    MEcmp(&argv[i][0], "#x_csp_crash=", 13) == 0 &&
		    CMdigit(&argv[i][13]))
		{
		    CVal(&argv[i][13], &jsx->jsx_ckp_crash);
		}
		break;

	    case 'w':
		/*
		** Stall checkpoint for online-checkpoint tests.
		*/
		jsx->jsx_status |= JSX_CKP_STALL;
		break;

	    default:
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    }
	}
	else if (CMnmstart(argv[i]))
	{
	    /*
	    ** Just save index of database name until we process all
	    ** JSP parameters. We need to process dmf_cache_size parameters 
	    ** before calling dmf_init().
	    */
	    if (jsx->jsx_db_cnt == 32)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;
	    }

	    dbname_ix[jsx->jsx_db_cnt++] = i;
	}
	else 
	{
	    /*
	    ** Bug 67437 - ensure dname starts with a valid character
	    */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
    	}

	/*
	** Perform additional auditdb checks.
	*/
 	if (operator == JSX_O_ATP)
	{
	    /*
	    ** Can't have both -t and -table=
	    */
	    if ((jsx->jsx_status & JSX_TNAME) &&
		(jsx->jsx_status & JSX_TBL_LIST))
	    {
	        SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    }

	    /*
	    ** Disallow -t along with either -table= or -file=
	    */
	    if ((jsx->jsx_status & JSX_TNAME) &&
		(jsx->jsx_status & (JSX_TBL_LIST | JSX_FNAME_LIST)))
	    {
	        SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    }
	}

	/*
	** Perform additional checkpoint checks
	*/
	if (operator == JSX_O_CPP)
	{
	    /* BUG 66430: disallow -d for table level checkpoint */
	    if (jsx->jsx_status & JSX_DESTROY)
	    {
		if (jsx->jsx_status & JSX_TBL_LIST)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		}
	    }
	}

	/*  Return with any error. */

	if (jsx->jsx_dberr.err_code != 0)
	{
	    /*
	    ** If got an invalid/inconsistent argument, be nice and tell
	    ** the user which argument is wrong. We create a new error
	    ** since we can't change the number of parameter to the old
	    ** error codes.
	    */
	    if (jsx->jsx_dberr.err_code == E_DM1004_JSP_BAD_ARGUMENT ||
		jsx->jsx_dberr.err_code == E_DM1005_JSP_BAD_ARG_COMBO ||
		jsx->jsx_dberr.err_code == E_DM105C_JSP_INV_ARG_VAL ||
		jsx->jsx_dberr.err_code == E_DM105A_JSP_INC_ARG)
	    {
		i4 new_err;

		if (jsx->jsx_dberr.err_code == E_DM1004_JSP_BAD_ARGUMENT)
		    new_err = E_DM103B_JSP_INVALID_ARG;
		else if (jsx->jsx_dberr.err_code == E_DM1005_JSP_BAD_ARG_COMBO)
		    new_err = E_DM103C_JSP_INCON_ARG;
		else
		    new_err = jsx->jsx_dberr.err_code;
		

		uleFormat(NULL, new_err, (CL_ERR_DESC *) NULL,
			ULE_LOOKUP, (DB_SQLSTATE *) NULL, error_buffer,
			ER_MAX_LEN, &error_length, &new_err, 2,
			STlength(argv[1]),argv[1],
			STlength(argv[i]),argv[i]);
		dmf_put_line(0, error_length, error_buffer);
		jsp_echo_usage(jsx);
		CLRDBERR(&jsx->jsx_dberr);
	    }
	    return (E_DB_ERROR);
	}
    }

    if (jsx->jsx_status1 & JSX_HELP)
	return(E_DB_OK);

    if (status != E_DB_OK)
	return (E_DB_ERROR);

    /*
    ** Most jsp utilities require specification of one and only one database.
    */
    if (((operator == JSX_O_ALTER) ||
	 (operator == JSX_O_ATP) ||
	 (operator == JSX_O_FLOAD) ||
	 (operator == JSX_O_CPP) ||
	 (operator == JSX_O_RELOC) ||
	 (operator == JSX_O_RFP)) &&
	 (jsx->jsx_db_cnt != 1))
    {
	if (operator == JSX_O_ALTER)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM104B_JSP_ALTDB_ONE_DB);
	else if (operator == JSX_O_ATP)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM104C_JSP_ATP_ONE_DB);
	else if (operator == JSX_O_CPP)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1050_JSP_CPP_ONE_DB);
	else if (operator == JSX_O_RELOC)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1056_JSP_RELOC_ONE_DB);
	else if (operator == JSX_O_FLOAD)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1057_JSP_FLOAD_ONE_DB);
	else
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM104D_JSP_RFP_ONE_DB);

	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1001_JSP_INIT);
	return (E_DB_ERROR);
    }

    if (jsx->jsx_db_cnt == 0)
    {
	if (jsx->jsx_dberr.err_code != 0)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1001_JSP_INIT);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Allocate Buffer Manager:
    **
    ** For rollforwarddb, the default is to allocate a cache for each page size
    **
    **       If we don't have sufficient memory to allocate all pools, we
    **       want to know before we begin the rollforward.
    ** 
    ** For all other JSP operations, just allocate a 2k cache.
    ** 
    **       If FLOAD needs another cache, it will be added before we 
    **       open the table being loaded.
    **
    ** Bug 106230. For Table level Checkpoint, also need caches for each page size.
    */
    if ( (jsx->jsx_operation == JSX_O_RFP) ||
         (jsx->jsx_status & JSX_TBL_LIST) )
    {
	c_buffers[0] = 256; /* Number of 2k buffers */
	c_buffers[1] = 200; /* Number of 4k buffers  */
	c_buffers[2] = 200; /* Number of 8k buffers  */
	c_buffers[3] = 200; /* Number of 16k buffers */
	c_buffers[4] = 200; /* Number of 32k buffers */
	c_buffers[5] = 200; /* Number of 64k buffers */
    }
    else
    {
	/* Other JSP operations only require 2k buffer pool
	** Note fastload (JSX_O_FLOAD) may need another buffer pool
	** but it can be added when the table is opened             
	*/
	c_buffers[0] = 256;/* Number of 2k buffers        */
	c_buffers[1] = 0;  /* Don't alloc any 4k buffers  */
	c_buffers[2] = 0;  /* Don't alloc any 8k buffers  */
	c_buffers[3] = 0;  /* Don't alloc any 16k buffers */
	c_buffers[4] = 0;  /* Don't alloc any 32k buffers */
	c_buffers[5] = 0;  /* Don't alloc any 64k buffers */
    }

    /* Check to see if the default cache sizes have been overridden */
    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
    {
	if (dmf_cache_size[cache_ix] >= 0)
	    c_buffers[cache_ix] = dmf_cache_size[cache_ix];
    }

    status = dmf_init(
		(jsx->jsx_operation == JSX_O_CPP) ? DM0L_CKPDB : 0,
		connect_only,
		cache_name,
		c_buffers,
		0,     /* no special lock list flags needed */
		0, 0, 32, &jsx->jsx_dberr, jsx->jsx_lgk_info);

    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM101A_JSP_CANT_CONNECT_BM)
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 1,
			STlength(cache_name), cache_name);
	else
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    dmfWriteMsg(NULL, E_DM1051_JSP_NO_INSTALL, 0);
	}

        SETDBERR(&jsx->jsx_dberr, 0, E_DM1001_JSP_INIT);
	return (E_DB_ERROR);
    }

    /* If connecting to shared cache, we have to disconnect cleanly
    ** when we exit, or the shared BM will get fu'ed.
    */
    if (connect_only)
	jsx->jsx_status2 |= JSX2_MUST_DISCONNECT_BM;

    /* Allocate dcb's */
    if (jsx->jsx_db_cnt != 0)
    {
	DB_DB_NAME      dbname;
	DMP_DCB         *dcb;

	for (i = 0; i < jsx->jsx_db_cnt; i++)
	{
	    j = dbname_ix[i];
	    MEmove(STlength(argv[j]), argv[j], ' ', sizeof(dbname),
		    dbname.db_db_name);

	    /* Convert the database name to lowercase */
	    CVlower(dbname.db_db_name);

	    status = allocate_dcb(jsx, &dbname, &dcb);
	    if (status != E_DB_OK)
		break;

	    /*	Insert DCB on tail of the work queue. */

	    dcb->dcb_q_next = (DMP_DCB *)&jsx->jsx_next;
	    dcb->dcb_q_prev = jsx->jsx_prev;
	    jsx->jsx_prev->dcb_q_next = dcb;
	    jsx->jsx_prev = dcb;
	}
    }

    /*	Make some consistency checks on the arguments. */

    if ((jsx->jsx_status & JSX_DEVICE) && jsx->jsx_db_cnt != 1)
    {
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1006_JSP_TAPE_ONE_DB);
	return (E_DB_ERROR);
    }

    /*
    ** If the user supplied both a "-b" begin time and a "-e" end time, then
    ** the begin time should not be after the end time (B39150):
    */
    if ((jsx->jsx_status & (JSX_BTIME|JSX_ETIME)) == (JSX_BTIME|JSX_ETIME))
    {
	if (TMcmp_stamp((TM_STAMP *)&jsx->jsx_bgn_time,
	    (TM_STAMP *)&jsx->jsx_end_time) > 0)
	{
	    char	    bgn_time[TM_SIZE_STAMP];
	    char	    end_time[TM_SIZE_STAMP];

	    TMstamp_str((TM_STAMP *)&jsx->jsx_bgn_time, bgn_time);
	    TMstamp_str((TM_STAMP *)&jsx->jsx_end_time, end_time);

	    uleFormat(NULL, E_DM1025_JSP_BAD_DATE_RANGE, (CL_ERR_DESC *) NULL,
		ULE_LOOKUP, (DB_SQLSTATE *) NULL, error_buffer,
		ER_MAX_LEN, &error_length, &error, 2,
		TM_SIZE_STAMP, bgn_time, TM_SIZE_STAMP, end_time);

	    dmf_put_line(0, error_length, error_buffer);

	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    return (E_DB_ERROR);
	}
    }

    /* For auditdb if -all is specified and -b / -e are also specified, 
    ** we automatically allow the -aborted_transactions option enabled
    ** to correctly allow screening for begin and end transaction records.
    */
    if((jsx->jsx_status & (JSX_BTIME|JSX_ETIME)) &&
	(jsx->jsx_status & JSX_AUDIT_ALL) && 
		(operator == JSX_O_ATP))
      jsx->jsx_status2 |= JSX2_ABORT_TRANS;

    /*
    **	Perform rollforward param checks.
    */
    if (operator == JSX_O_RFP)
    {
	if ((jsx->jsx_status & (JSX_USEJNL|JSX_USECKP)) == 0)
	{
	    /* you didn't use checkpoint or journal */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}

	/*
	** -b requires #f, simply because its not likely to work.
	*/
	if ( (jsx->jsx_status & JSX_BTIME) &&
	     (jsx->jsx_status1 & JSX_FORCE) == 0 )
	{
	    dmfWriteMsg(NULL, E_DM1351_RFP_BTIME_NOFORCE, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    return (E_DB_ERROR);
	}

	/*
	** -start_lsn also requires #f.
	*/
	if ( (jsx->jsx_status1 & JSX_START_LSN) &&
	     (jsx->jsx_status1 & JSX_FORCE) == 0 )
	{
	    dmfWriteMsg(NULL, E_DM1353_RFP_SLSN_NOFORCE, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    return (E_DB_ERROR);
	}

	/*
	** Make sure -new_location and -location are specified together
	*/
	if (((jsx->jsx_status1 & JSX_NLOC_LIST) &&
	     !(jsx->jsx_status1 & JSX_LOC_LIST)) ||
	    (!(jsx->jsx_status1 & JSX_NLOC_LIST) &&
	     (jsx->jsx_status1 & JSX_LOC_LIST)) )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}

	/*
	** Bug #67004
	** Make sure -location and -relocate specified together
	*/
	if (((jsx->jsx_status1 & JSX_LOC_LIST) && 
	     !(jsx->jsx_status1 & JSX_RELOCATE)) ||
	    (!(jsx->jsx_status1 & JSX_LOC_LIST) &&
	     (jsx->jsx_status1 & JSX_RELOCATE)))
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}

	/*
	** Bug #67004
	** Make sure -relocate is only specified for table level recovery
	*/
	if ((jsx->jsx_status1 & JSX_RELOCATE) &&
	    (jsx->jsx_status1 & JSX1_RECOVER_DB))
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}

	/*
	** Bug #67005
	** Make sure -on_error_continue is only specified for table level rf
	*/
	if ((jsx->jsx_status1 & JSX1_ON_ERR_CONT) &&
	    (jsx->jsx_status1 & JSX1_RECOVER_DB))
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}

	/*
	** Bug #79932 & 80384
	** Make sure -relocate is only specified for table level recovery with check point
	*/
	if ((jsx->jsx_status1 & JSX_RELOCATE) &&
	    !(jsx->jsx_status & JSX_USECKP))	
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Any time -new_location= is specified, -location must also be
    ** specified, and there must be the same number of entries on
    ** each list
    */
    if (operator == JSX_O_RFP || operator == JSX_O_RELOC)
    {
	/*
	** bug #68502 - if "-location" is specified, "-new_location" must also
        ** be specified.
	*/
	if ((jsx->jsx_status1 & JSX_NLOC_LIST) || 
	    (jsx->jsx_status1 & JSX_LOC_LIST))
	{
	    char *loc, *nloc;

	    jsx->jsx_loc_cnt = 0;
       	    if (((jsx->jsx_status1 & JSX_NLOC_LIST) &&
                !(jsx->jsx_status1 & JSX_LOC_LIST)) ||
               (!(jsx->jsx_status1 & JSX_NLOC_LIST) &&
                 (jsx->jsx_status1 & JSX_LOC_LIST)) )
            {
	        SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
                return (E_DB_ERROR);
            }

	    for (i = 0; i < JSX_MAX_LOC; i++)
	    {
		loc = jsx->jsx_loc_list[i].loc_name.db_loc_name;
		nloc = jsx->jsx_nloc_list[i].loc_name.db_loc_name;
		if (loc[0] == '\0' && nloc[0] == '\0')
		    break;
		if ((loc[0] == '\0' || nloc[0] == '\0') && loc[0] != nloc[0])
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
		    return (E_DB_ERROR);
		}
		jsx->jsx_loc_cnt++;
	    }
	}
    }

    /*
    ** Perform fastload param checks
    */
    if (operator == JSX_O_FLOAD)
    {
	;
    }
    /*
    ** Perform Relocate param checks
    */
    if (operator == JSX_O_RELOC)
    {
        /* Make sure ONE relocation request specified */
        i = jsx->jsx_status1 & (JSX_CKPT_RELOC | JSX_DMP_RELOC |
                JSX_JNL_RELOC | JSX_WORK_RELOC | JSX_DB_RELOC);
        if (BTcount((char *)&i, (sizeof i) * BITSPERBYTE) != 1)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
            return (E_DB_ERROR);
        }
 
        /* -location and -new_location only valid for db relocation */
        if ( (jsx->jsx_status1 & (JSX_LOC_LIST | JSX_NLOC_LIST)) &&
                ((jsx->jsx_status1 & JSX_DB_RELOC) == 0))
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
            return (E_DB_ERROR);
        }
    }

    /*
    ** Perform more AUDITDB parameter checks.
    */
    if (operator == JSX_O_ATP)
    {
	/*
	** If audit file specified, must also specify tables to be audited.
	*/
	if ( ((jsx->jsx_status & JSX_AUDITFILE ) &&
	      ((jsx->jsx_status & JSX_TNAME) == 0)) ||
	     ((jsx->jsx_status & JSX_FNAME_LIST ) &&
	        (jsx->jsx_status & JSX_TBL_LIST) == 0) )
	{
	    dmfWriteMsg(NULL, E_DM1045_JSP_FILE_REQ_TBL, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return(E_DB_ERROR);
	}

	/*
	** If -file specified, must have same number of files and tables
	*/
	if ( (jsx->jsx_status & JSX_FNAME_LIST) &&
	     (jsx->jsx_fname_cnt) &&
	     (jsx->jsx_fname_cnt != jsx->jsx_tbl_cnt) )
	{
	    dmfWriteMsg(NULL, E_DM1044_JSP_FILE_TBL_EQ, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Perform Additional AUDITDB and ROLLDB param checks.
    */
    if ( (operator == JSX_O_RFP) || (operator == JSX_O_ATP) )
    {
	if ( (jsx->jsx_status1 & JSX_START_LSN) &&
	     (jsx->jsx_status1 & JSX_END_LSN)  &&
	     (LSN_LT(&jsx->jsx_end_lsn, &jsx->jsx_start_lsn)) )
	{
	    dmfWriteMsg(NULL, E_DM1054_JSP_SLSN_GT_ELSN, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
	    return(E_DB_ERROR);
	}

	/*
	** Can't mix times and lsn's.
	*/
	if ( (jsx->jsx_status & (JSX_BTIME|JSX_ETIME)) &&
	     (jsx->jsx_status1 & (JSX_START_LSN|JSX_END_LSN)) )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Perform infodb checks
    */
    if ((operator == JSX_O_INF) &&
        (jsx->jsx_status1 & JSX1_OUT_FILE) &&
        (jsx->jsx_db_cnt != 1))
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1005_JSP_BAD_ARG_COMBO);
	return(E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: allocate_dcb	- Allocate a DCB.
**
** Description:
**      This routine allocates a DCB and makes a first pass at
**	initializing the DCB.
**
**	In order for rollforward to be able to automatically use the backup
**	copy of the config file which we have placed in the dump location,
**	we must build the dump location as well as the data location. It's
**	still not really a complete DCB -- this is just the minimal info
**	needed for the JSP operations. It is also unusual in that our
**	dump location is allocated memory, where 'standard' DCB's have their
**	dump location pointer set to point into the in-memory copy of the
**	config file. This means that we will free our dump location
**	when we free the dcb, where other users of the dcb count on the
**	closing of the config file to free the dump location.
**
** Inputs:
**      dbname                          Pointer to database name.
**
** Outputs:
**	dcb				Pointer to pointer to allocated DCB.
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-nov-1986 (Derek)
**          Created for Jupiter.
**	21-may-90 (bryanp)
**	    Allocate space for the dump location's information and store
**	    the address of that space in the dcb.
[@history_template@]...
*/
static DB_STATUS
allocate_dcb(
DMF_JSX		    *jsx,
DB_DB_NAME	    *dbname,
DMP_DCB		    **dcb)
{
    DB_STATUS           status;
    i4			error;
    DMP_DCB		*d;

    CLRDBERR(&jsx->jsx_dberr);

    /*	Allocate the DCB. */

    status = dm0m_allocate((i4)(sizeof(DMP_DCB) + sizeof(DMP_LOC_ENTRY)),
	DM0M_ZERO, (i4)DCB_CB, (i4)DCB_ASCII_ID, (char *)0,
	(DM_OBJECT **)dcb, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1007_JSP_MEM_ALLOC);
	return (status);
    }

    /*	Initialize pointer to root location. Set dcb->dcb_dmp to point to
    **	the extra DMP_LOC_ENTRY which we allocated 'at the end' of the DCB.
    */

    d = *dcb;
    d->dcb_dmp = (DMP_LOC_ENTRY *)((char *)d + sizeof(DMP_DCB));

    /*	Store the database name. */

    STRUCT_ASSIGN_MACRO(*dbname, d->dcb_name);

    /*	Return. */

    return (E_DB_OK);
}

/*{
** Name: search_dcb	- Search for DCB's.
**
** Description:
**      This routine scans the list of dcb representing database names from
**	the command line.  For each DCB found, this routine fills in the DCB
**	with information found in the DBDB database.  If no database names were
**	given on the command line, then all database names owned by the user
**	are located in the DBDB database, and DCB's are created for them.
**
**	One interesting 'extra' piece of information which we fill in is the
**	dump location for each database. This is done so that rollforward can
**	locate the special backup copy of the config file in the dump location
**	in case it cannot open the standard database location's config file.
**
**	A distributed database (du_dbservice & DU_1SER_DDB) has neither
**	a database directory nor a config file, so the jsp utilities cannot
**	process it correctly. We skip over such databases when building a list
**	of all databases you own, and we reject any attempt to specifically
**	name a distributed database on the command line.
**
** Inputs:
**      journal_context                 Pointer to journal context.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-nov-1986 (Derek)
**          Created for Jupiter.
**	30-sep-1986 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	11-apr-1989 (EdHsu)
**	    Online  backup cluster support
**	17-apr-1989 (Sandyh)
**	    bug 4545 any user could use the -u flag
**	14-may-1990 (rogerk)
**	    Changed STRUCT_ASSIGN_MACRO's of different typed objects to MEcopy.
**	21-may-90 (bryanp)
**	    Fill in dcb->dcb_dmp with information read from iidatabase and
**	    iilocations.
**	30-oct-1990 (bryanp)
**	    Bug #34120: jsp utilities don't work right on distributed databases.
**	    Changed search_dcb to skip such databases when making a list of all
**	    databases you own, and to reject any attempt to explicitly name
**	    such a database. Also fixed a few error message problems in the
**	    search_dcb code so that it could give better error messages.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    25-feb-1991 (rogerk)
**		Added JSX_NOLOGGING support.  This specifies that no logging
**		should be done.  When this flag is asserted, we do not log the
**		opendb of the iidbdb here.  This flag is currently used in
**		alterdb so that journaling can be disabled while the logfile is
**		full.
**	    25-feb-1991 (rogerk)
**		Initialize rcb so we don't try to close table when it is not
**		yet open.
**	    12-aug-1991 (bryanp)
**		B39106: Don't need to open iidbdb when rolling forward iidbdb.
**		The iidbdb is always found in II_DATABASE, and its dump
**		location is always II_DUMP, so we can build the DCB by hand.
**		This allows us to roll forward the iidbdb even if you lose the
**		data directory for the iidbdb.
**	17-oct-1991 (mikem)
**	    Eliminated unix compile warnings ("warning: statement not reached")
**	    changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**      2-oct-1992 (ed)
**	    - fix MEcmp paramter, which was incorrectly passed by value
**          - Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	18-nov-1992 (robf)
**	     Initialize the security audit state for SXF in single user.
**	04-feb-93 (jnash)
**	     Cast ME function params to eliminate compiler warnings.
**	1-oct-93 (robf)
***          Removed unused scb variable
**	25-dec-93 (rogerk)
**	    Cleaned up DCB memory allocated for the IIDBDB open on exit.
**	    Leaving it around was causing an AV when dmd_checks occurred
**	    because the memory cb was found but its contents were no longer
**	    valid.
**	17-feb-94 (stephenb)
**	    Alter user authentication audit so that we are consistent with
**	    other records of this type in the dbms.
**	    Add a call to audit the user acton on failure to access
**	    database. Also move dmf_sxf_bgn_session() here so that we can take
**	    advantage of case translated usernames.
**	1-mar-94 (robf)
**          Fixed error handling problem, if SXF MAC call failed then
**          ule_format() reset err_code which when dereferenced was 0
**	    causing an AV. Pass err_code, not &err_code to ule_format.
**	9-mar-94 (stephenb)
**	    move dmf_sxf_bgn_session() futher up this routine to cope with
**	    additional B1 audits and add dbname to dma_write_audit() calls.
**	23-may-1994 (jnash)
** 	    If error encountered in jsp_get_case() during rolldb of iidbdb,
**	    set default case semantics to mixed and continue.  This fixes
**	    a problem where rolldb of iidbdb can't be performed if
**	    iidbdepends inaccessible.
**	24-jun-1994 (jnash)
** 	    When rolling forward the iidbdb:
**	     - Unconditionally hand build the iidbdb DCB.  We can't rely on
**	       any catalog info.
**	     - Write an sxf audit log
**	     - Check the operator priv against SERVER CONTROL in config.dat.
**	19-jul-94 (stephenb)
**	    fix 24-jun jnash compilation errors and make sure we write an
**	    audit in the failure case for iidbdb.
**	17-feb-95 (carly01)
**		66897 - alter audit state only if enabled.  
**		E_DM1036_JSP_ALTER_SXF issued when attempting to alter
**		audit state when disabled.
**	7-nov-1995 (angusm)
**		bug 72427 - location names may be delimited identifiers
**		containing embdedded spaces.
**	26-Oct-1996 (jenjo02)
**	        Added *name parm to dm0s_minit() calls.
**	9-Jan-2005 (kschendel)
**	    Record server-control priv for others to see.
[@history_template@]...
*/
static DB_STATUS
search_dcb(
DMF_JSX             *journal_context)
{
    DMF_JSX		*jsx = journal_context;
    DMP_DCB		*dcb;
    DMP_RCB		*rcb = 0;
    DM_TID		tid;
    DB_TRAN_ID		tran_id;
    DB_TAB_TIMESTAMP	stamp;
    DB_STATUS		error,local_error;
    DB_STATUS		status,local_status;
    DB_STATUS		close_status;
    DU_DATABASE		database;
    DU_LOCATIONS	location;
    DB_OWN_NAME		username;
    i4		log_id = 0;
    i4		lock_id;
    i4             tmp_lock_id;
    i4		dbdb_flag;
    DB_TAB_TIMESTAMP	ctime;
    DM2R_KEY_DESC	key_list[1];
    DB_TAB_ID		iiuser;
    DB_TAB_ID		iidatabase;
    DB_TAB_ID		iilocation;
    DB_TAB_ID		iisecstate;
    DU_SECSTATE		secstate;
    SXF_ACCESS          access = SXF_A_AUTHENT;
    SXF_RCB   	        sxf_rcb;
    DB_OWN_NAME 	cased_username;
    DB_OWN_NAME		cased_realuser;
    char		termid[16];
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /* default initialization */

    iiuser.db_tab_base = DM_B_USER_TAB_ID;
    iiuser.db_tab_index = 0;
    iidatabase.db_tab_base = DM_B_DATABASE_TAB_ID;
    iidatabase.db_tab_index = 0;
    iilocation.db_tab_base = DM_B_LOCATIONS_TAB_ID;
    iilocation.db_tab_index = 0;
    iisecstate.db_tab_base = DM_B_SECSTATE_TAB_ID;
    iisecstate.db_tab_index = 0;

    {
	char	    *cp = (char *)&username;

	IDname(&cp);
	MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);

    }
    if (jsp_chk_priv((char *)&username, "SERVER_CONTROL"))
	jsx->jsx_status2 |= JSX2_SERVER_CONTROL;

    /*	Allocate a DCB for the database database. */

    status = dm0m_allocate((i4)sizeof(DMP_DCB),
	DM0M_ZERO, (i4)DCB_CB, (i4)DCB_ASCII_ID, (char *)0,
	(DM_OBJECT **)&dcb, &jsx->jsx_dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    /*	Fill the in DCB for the database database. */

    MEmove(sizeof(DU_DBDBNAME) - 1, DU_DBDBNAME, ' ',
           sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);
    MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ',
           sizeof(dcb->dcb_db_owner), dcb->dcb_db_owner.db_own_name);
    dcb->dcb_type = DCB_CB;
    dcb->dcb_access_mode = DCB_A_READ;
    dcb->dcb_served = DCB_MULTIPLE;
    dcb->dcb_bm_served = DCB_MULTIPLE;
    dcb->dcb_db_type = DCB_PUBLIC;
    dcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    dm0s_minit(&dcb->dcb_mutex, "DCB mutex");
    MEmove(8, "$default", ' ', sizeof(dcb->dcb_location.logical),
           (PTR)&dcb->dcb_location.logical);

    if (build_location(dcb, sizeof(ING_DBDIR), ING_DBDIR, LO_DB,
			&dcb->dcb_location) != E_DB_OK)
    {
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1017_JSP_II_DATABASE);
	return (E_DB_ERROR);
    }

    /*
    ** If rolling forward the iidbdb, we don't try to open the
    ** iidbdb because we have no idea of what state it is in.  In this
    ** case, hand build a DCB.
    **
    ** In order to be able to do this, we depend on the fact that the
    ** iidbdb never has alternate locations; it always has II_DATABASE
    ** as its database location, and II_DUMP as its dump location (B39106).
    **
    ** Although this code is designed to handle the rolldb case, we follow
    ** the same action for all jsp iidbdb operations.
    */
    if ( (jsx->jsx_db_cnt == 1) &&
	 (MEcmp((PTR)&jsx->jsx_next->dcb_name, (PTR)DB_DBDB_NAME,
		    sizeof(DB_DB_NAME)) == 0) )
    {
	DMP_DCB	    *ndcb = jsx->jsx_next;

 	/*
 	** Find default case translation semantics, set results into jsx.
	** Ok that db unopened.
 	*/
 	status = jsp_get_case((DMP_DCB *)0, jsx);
 	if (status != E_DB_OK)
	    return (E_DB_ERROR);

	/*
	** Apply proper case to -u name.
	*/
	cased_username.db_own_name[0] = 0;
	if (jsx->jsx_status & JSX_UNAME)
	{
	    jsp_set_case(jsx, jsx->jsx_reg_case, DB_OWN_MAXNAME,
		(char *)&jsx->jsx_username, (char *)&cased_username);
	}

	/*
	** User names obtained from the operating system
	** must have "real user case" applied.
	*/
	jsp_set_case(jsx, jsx->jsx_real_user_case, DB_OWN_MAXNAME,
		(char *)&username, (char *)&cased_realuser);

	/*
	** Hand-build the IIDBDB DCB
	*/
	MEmove(sizeof(DU_DBDBNAME) - 1, DU_DBDBNAME, ' ',
	       sizeof(ndcb->dcb_name), ndcb->dcb_name.db_db_name);
	MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ',
	       sizeof(ndcb->dcb_db_owner), ndcb->dcb_db_owner.db_own_name);
	MEmove(8, "$default", ' ', sizeof(ndcb->dcb_location.logical),
	       (PTR)&ndcb->dcb_location.logical);
	MEmove(8, "$default", ' ', sizeof(ndcb->dcb_dmp->logical),
	    (PTR)&ndcb->dcb_dmp->logical);
	ndcb->dcb_access_mode = DCB_A_WRITE;
	ndcb->dcb_served = DCB_MULTIPLE;
	ndcb->dcb_bm_served = DCB_MULTIPLE;
	ndcb->dcb_db_type = DCB_PUBLIC;
	ndcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
	dm0s_minit(&ndcb->dcb_mutex, "DCB mutex");

	if (build_location(ndcb, sizeof(ING_DBDIR), ING_DBDIR, LO_DB,
			    &ndcb->dcb_location) != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1017_JSP_II_DATABASE);
	    return (E_DB_ERROR);
	}

	if (build_location(ndcb, sizeof(ING_DMPDIR), ING_DMPDIR, LO_DMP,
			ndcb->dcb_dmp) != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1024_JSP_BAD_LOC_INFO);
	    return (E_DB_ERROR);
	}
	/*
	** Start an SXF audit session.
	*/
	local_status = dmf_sxf_bgn_session (
	    &cased_realuser,
	    (jsx->jsx_status & JSX_UNAME) ? &cased_username : &cased_realuser,
	    DB_DBDB_NAME, 0);
	if (local_status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1031_JSP_SXF_SES_INIT);
	    return (E_DB_ERROR);
	}
	/*
	** Get terminal name
	*/
	if (TEname(termid) != OK)
	    termid[0] = EOS;


 	/*
	** As we can't assume successful access to iiuser, check
	** operator priv's again config.dat "SERVER CONTROL".
	**
	** also audit access failure if user does not have SERVER_CONTROL
	** (nasty !! we realy should have a better way of determining wether
	** a user can rollforward the iidbdb).
	*/
	if ((jsx->jsx_status2 & JSX2_SERVER_CONTROL) == 0)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		local_status = dma_write_audit( SXF_E_USER,
		    access | SXF_A_FAIL, termid, STlength(termid),
		    (DB_OWN_NAME *) DU_DBA_DBDB, I_SX2024_USER_ACCESS, TRUE,
		    &local_dberr, NULL);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1037_JSP_NO_PRIV_UNAME);
	    return (E_DB_ERROR);
	}
	jsx->jsx_status2 |= JSX2_SERVER_CONTROL;

    	/*
    	**	Record user authentication.
    	*/
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	    local_status = dma_write_audit( SXF_E_USER,
		access | SXF_A_SUCCESS, termid, STlength(termid),
		(DB_OWN_NAME *) DU_DBA_DBDB, I_SX2024_USER_ACCESS, TRUE,
		&local_dberr, NULL);

	/*
	** End SXF session for iidbdb
	*/
	local_status = dmf_sxf_end_session();
	if (status == E_DB_OK && local_status != E_DB_OK)
	{
	    error = local_error;
	    return (E_DB_ERROR);
	}

	dmf_jsp_dcb = dcb;

	return (E_DB_OK);
    }

    /*
    ** Set global context for iidbdb
    */
    dmf_jsp_dcb = dcb;

    /*
    ** Open the database database.
    **
    ** Some operations must be allowed while the Ingres Log File is full.
    ** In these cases we open the dbdb with the DM2D_NOLOGGING flag to bypass
    ** logging the open operation.
    */
    dbdb_flag = 0;
    if (jsx->jsx_status & JSX_NOLOGGING)
	dbdb_flag = DM2D_NOLOGGING;

    status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS,
                          dmf_svcb->svcb_lock_list, dbdb_flag, &jsx->jsx_dberr);
	
    if (status == E_DB_OK)
    {
 	/*
 	** Find case translation semantics, set results into jsx.
 	*/
 	status = jsp_get_case(dcb, jsx);
 	if (status != E_DB_OK)
 	    (VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
 		dbdb_flag, &local_dberr);
    }

    /*
    ** Check return status from the opendb/jsp_get_case calls.
    */
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1009_JSP_DBDB_OPEN);
	return (E_DB_ERROR);
    }

    /*	Begin a transaction. */
    status = dmf_jsp_begin_xact(DMXE_READ, 0, dcb, dcb->dcb_log_id, &username,
	dmf_svcb->svcb_lock_list, &log_id, &tran_id, &lock_id, &jsx->jsx_dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    /*
    ** Apply proper case to -u name.
    */
    cased_username.db_own_name[0] = 0;
    if (jsx->jsx_status & JSX_UNAME)
    {
	jsp_set_case(jsx, jsx->jsx_reg_case, DB_OWN_MAXNAME,
	    (char *)&jsx->jsx_username, (char *)&cased_username);
    }

    /*
    ** User names obtained from the operating system
    ** must have "real user case" applied.
    */
    jsp_set_case(jsx, jsx->jsx_real_user_case, DB_OWN_MAXNAME,
	    (char *)&username, (char *)&cased_realuser);

    /*
    ** First start a SXF audit session.
    ** This session will be used to audit the user authentication, since
    ** the user is only authenticated once, and may be refferencing a number
    ** of databases we use the "iidbdb" as a generic handle for the database
    ** name in SXF. We will also use this session to audit any failure
    ** to access other databases, the database name should be explicitly
    ** passed in this case.
    */
    local_status = dmf_sxf_bgn_session (
	&cased_realuser,   /* Real user */
	(jsx->jsx_status & JSX_UNAME) ? &cased_username : &cased_realuser,
	DB_DBDB_NAME,	   /* iidbdb */
	0	   	   /* User status */
	);

    if (local_status != E_DB_OK)
    {
	/*
	** 	Couldn't talk to SXF, so stop processing to
	**		avoid further errors later.
	*/
	return (E_DB_ERROR);
    }
    for(;;)
    {
	if (status != E_DB_OK)
	    break;

	/*  Open the user table. */

	status = dm2t_open(dcb, &iiuser, DM2T_IS, DM2T_UDIRECT, DM2T_A_READ,
	    0, 20, 0, 0, dmf_svcb->svcb_lock_list, 0, 0, 0, &tran_id,
	    &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM100A_JSP_OPEN_IIUSER);
	    break;
	}

	/*
	** Position on case adjusted username.
	*/
	key_list[0].attr_number = DM_1_USER_KEY;
	key_list[0].attr_operator = DM2R_EQ;
	key_list[0].attr_value = (char *)&cased_realuser;
	status = dm2r_position(rcb, DM2R_QUAL, key_list,
                               1, &tid, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)&user, &jsx->jsx_dberr);
	    if (status == E_DB_OK)
	    {
		/*
		**	Check if using -uflag - needs SECURITY or OPERATOR
		**	priv.
		*/
		if (( (jsx->jsx_status & JSX_UNAME)) &&
		     (user.du_status & (DU_USECURITY|DU_UOPERATOR)) == 0)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1037_JSP_NO_PRIV_UNAME);
		    status = E_DB_ERROR;
		    break;
		}

		if (user.du_status & DU_UOPERATOR)
		{
		    jsx->jsx_status |= JSX_OPER;
		}

		if (user.du_status & DU_USECURITY)
		{
		    jsx->jsx_status |= JSX_SECURITY;
		}
	        /*
	        ** Tell SXF the user status
	        */
	        sxf_rcb.sxf_length = sizeof(SXF_RCB);
	        sxf_rcb.sxf_cb_type = SXFRCB_CB;
	        sxf_rcb.sxf_alter= SXF_X_USERPRIV;
	        sxf_rcb.sxf_ustat= user.du_status;

	        status = sxf_call(SXC_ALTER_SESSION, &sxf_rcb);
	        if (status)
	        {
		    *&jsx->jsx_dberr = sxf_rcb.sxf_error;
	    	    break;
	        }
	    }
	}
	if (status == E_DB_OK && (jsx->jsx_status & JSX_UNAME))
	{
	    /*
	    ** Check for username override.
	    */

	    key_list[0].attr_value = (char *)&cased_username;
	    status = dm2r_position(rcb, DM2R_QUAL, key_list,
                                   1, &tid, &jsx->jsx_dberr);
	    if (status == E_DB_OK)
	    {
		status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
				    (char *)&user, &jsx->jsx_dberr);
	    }
	    if ( status )
		SETDBERR(&jsx->jsx_dberr, 0, E_DM100C_JSP_NOT_OVERUSER);
	}
	break;
    }
    /*
    ** If user is not a valid ingres user then they will fail authentication
    ** and we need to audit as a fail
    */
    if (jsx->jsx_dberr.err_code == E_DM100B_JSP_NOT_USER && status != E_DB_OK)
	access |= SXF_A_FAIL;
    else
	access |= SXF_A_SUCCESS;

    /*  Close the user table. */

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM100D_JSP_CLOSE_IIUSER);
	    status = close_status;
	}
	rcb = 0;
    }

    /*
    **	Load the security state info from the iidbdb into
    **	SXF for later use.
    **  We do this regardles of status because we will need to write an audit
    **  record anyway (e.g. if user is not a valid ingres user).
    */
    for(;;)
    {
	/*  Open the security state table. */

	local_status = dm2t_open(dcb, &iisecstate, DM2T_IS, DM2T_UDIRECT,
	    DM2T_A_READ, 0, 20, 0, 0, dmf_svcb->svcb_lock_list, 0, 0, 0,
	    &tran_id, &stamp, &rcb, (DML_SCB *)0, &local_dberr);

	/*  Scan  */
	if ( local_status == E_DB_OK )
	    local_status = dm2r_position(rcb, DM2R_ALL, key_list,
                               0, &tid, &local_dberr);
	if (local_status == E_DB_OK)
	{
	    do
	    {
	        local_status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
		    (char *)&secstate, &local_dberr);
	        if (local_status == E_DB_OK)
	        {
		    DB_STATUS tmp_status;

	    	    /*
	    	    **	Tell SXF about the new security state info
	    	    */
	    	    sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	    	    sxf_rcb.sxf_length = sizeof (sxf_rcb);
	    	    sxf_rcb.sxf_cb_type = SXFRCB_CB;
	    	    sxf_rcb.sxf_status = 0;
	    	    if (secstate.du_state==DU_STRUE)
	    		sxf_rcb.sxf_auditstate = SXF_S_ENABLE;
	    	    else
	    		sxf_rcb.sxf_auditstate = SXF_S_DISABLE;

	    	    if ( sxf_rcb.sxf_auditstate == SXF_S_ENABLE )
		    {
			/*66897 - alter audit state only if enabled*/
			sxf_rcb.sxf_auditevent = secstate.du_id;
	    	        tmp_status = sxf_call(SXS_ALTER, &sxf_rcb);
	    	        if (tmp_status != E_DB_OK)
	    	        {
		            uleFormat(&sxf_rcb.sxf_error, 0, NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
			    SETDBERR(&local_dberr, 0, E_DM1036_JSP_ALTER_SXF);
			    local_status=tmp_status;
	    	            break;
	    	        }
		    } /*end of 66897*/
	        }
	     } while (local_status==E_DB_OK);

	     if ( local_dberr.err_code == E_DM0055_NONEXT)
	     {
		local_status=E_DB_OK;
	     }
	}
	else
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1034_JSP_OPEN_SECSTATE);
	break;
    }
    if (status == E_DB_OK && local_status != E_DB_OK)
    {
	status = local_status;
	jsx->jsx_dberr = local_dberr;
    }

    /*  Close the security state table. */

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1035_JSP_CLOSE_SECSTATE);
	    status = close_status;
	}
	rcb = 0;
    }

    /*
    ** If C2/B1 must audit attempt to access INGRES by user
    */
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	/*
	** Get terminal name
	*/
	if (TEname(termid) != OK)
	    termid[0] = EOS;
    	/*
    	**	Record user authentication.
    	*/
    	local_status = dma_write_audit( SXF_E_USER,
	    access,
	    termid, /* Object name (terminal) */
	    STlength(termid), /* Object name (terminal) */
	    NULL, /* Object owner */
	    I_SX2024_USER_ACCESS,/*  Message */
	    TRUE,		    /* Force */
	    &local_dberr, NULL);
    }


    for(;;)
    {
	if (status != E_DB_OK)
	    break;

	/*  Open the database table. */

	/* FIX ME !!!! For relocate WORK/JNL/CKP/DUMP, later on we will
	** need to update iidatabase inside a transaction!!!
	** We can't do this if we pass session lock list
	** (dmf_svcb->svcb_lock_list) for this open.
	** For now pass transaction lock list (lock_id), so that when we
	** commit below, we release the lock S lock on the iidatabase tuple
	** we need to update later in rfp_iidatabase.
	** I don't know if it is important for us to hold this lock
	*/
        if (   (jsx->jsx_status1 & (JSX_CKPT_RELOC | JSX_DMP_RELOC |
                JSX_WORK_RELOC | JSX_JNL_RELOC | JSX_DB_RELOC))
            || (jsx->jsx_status2 & (JSX2_PMODE | JSX2_OCKP | 
			JSX2_ALTDB_UNICODEC | JSX2_ALTDB_UNICODED)))
	    tmp_lock_id = lock_id;
	else
	    tmp_lock_id = dmf_svcb->svcb_lock_list;

	status = dm2t_open(dcb, &iidatabase, DM2T_IS, DM2T_UDIRECT, DM2T_A_READ,
	    0, 20, 0, 0, tmp_lock_id, 0, 0, 0, &tran_id,
	    &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM100F_JSP_OPEN_IIDB);
	    break;
	}

	if (jsx->jsx_db_cnt == 0)
	{
	    /*  Scan for all databases. */

	    status = dm2r_position(rcb, DM2R_ALL, key_list,
                                   1, &tid, &jsx->jsx_dberr);
	    if (status == E_DB_OK)
	    {
		for(;;)
		{
		    DMP_DCB	    *ndcb;

		    status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
					(char *)&database, &jsx->jsx_dberr);
		    if (status != E_DB_OK)
			break;
		    /*
                    ** OPERATOR/SECURITY user can operate on any database
		    */
		    if ((jsx->jsx_status & (JSX_OPER | JSX_SECURITY)) == 0)
		    {
			/* Not an operator/security, check user name. */
			if ((jsx->jsx_status & JSX_UNAME) == 0 )
			{
			    /*
			    ** This compares a user name obtained from
			    ** the operating system with one in the catalogs.
			    */
			    if (MEcmp((char *)&database.du_own,
				(char *)&cased_realuser,
				sizeof(DB_OWN_NAME)))
			    {
				continue;
			    }
			}
			else
			{
			    /*
			    **	Alternate user specified
			    */
			    if (MEcmp((char *)&database.du_own,
				(char *)&cased_username,
				sizeof(DB_OWN_NAME)) != 0)
			    {
				continue;
			    }
			}
		    }
		    /*
		    ** Skip all distributed databases:
		    */
		    if (database.du_dbservice & DU_1SER_DDB)
			continue;

		    /*	Allocate a DCB for this database. */

		    status = allocate_dcb(jsx,
			(DB_DB_NAME *)database.du_dbname, &ndcb);
		    if (status != E_DB_OK)
			break;

		    /*	Finish initializing the DCB. */

		    MEmove(sizeof(database.du_dbname), database.du_dbname, ' ',
			sizeof(ndcb->dcb_name), (PTR)&ndcb->dcb_name);
		    MEmove(sizeof(database.du_own), (PTR)&database.du_own, ' ',
			sizeof(ndcb->dcb_db_owner), (PTR)&ndcb->dcb_db_owner);
		    MEmove(sizeof(database.du_dbloc),
			(PTR)&database.du_dbloc, ' ',
			sizeof(ndcb->dcb_location.logical),
                        (PTR)&ndcb->dcb_location.logical);
		    MEmove(sizeof(database.du_dmploc),
			(PTR)&database.du_dmploc, ' ',
			sizeof(ndcb->dcb_dmp->logical),
                        (PTR)&ndcb->dcb_dmp->logical);
		    ndcb->dcb_type = DCB_CB;
		    ndcb->dcb_access_mode = DCB_A_WRITE;
		    ndcb->dcb_served = DCB_MULTIPLE;
		    ndcb->dcb_bm_served = DCB_MULTIPLE;
		    ndcb->dcb_db_type = DCB_PUBLIC;
		    ndcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;

		    dm0s_minit(&ndcb->dcb_mutex, "DCB mutex");

		    /*  Queue DCB to JSX. */

		    ndcb->dcb_q_next = jsx->jsx_next;
		    ndcb->dcb_q_prev = (DMP_DCB*)&jsx->jsx_next;
		    jsx->jsx_next->dcb_q_prev = ndcb;
		    jsx->jsx_next = ndcb;
		}
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    status = E_DB_OK;
	    }
	}
	else
	{
	    DMP_DCB	    *ndcb;

	    for (ndcb = jsx->jsx_next;
		ndcb != (DMP_DCB*)&jsx->jsx_next;
		ndcb = ndcb->dcb_q_next)
	    {
		/*  Probe for each listed database. */

		SETDBERR(&jsx->jsx_dberr, 0, E_DM1011_JSP_DB_NOTFOUND);
		key_list[0].attr_value = (char *)&ndcb->dcb_name;
		key_list[0].attr_operator = DM2R_EQ;
		key_list[0].attr_number = DM_1_DATABASE_KEY;
		status = dm2r_position(rcb, DM2R_QUAL, key_list,
                                       1, &tid, &jsx->jsx_dberr);
		if (status == E_DB_OK)
		{
		    status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
					(char *)&database, &jsx->jsx_dberr);
		}
		if (status == E_DB_OK)
		{
		    /*
		    **  A user with OPERATOR or SECURITY privilege can
                    **  operate on any database.
                    */
		    if ((jsx->jsx_status & (JSX_OPER | JSX_SECURITY)) == 0)
		    {
			/* Not an operator/security, check user name. */
			if ( MEcmp((char *)&database.du_own,
			      (jsx->jsx_status & JSX_UNAME) ?
			      (char *)&cased_username : (char *)&cased_realuser,
			      sizeof(DB_OWN_NAME)))
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1014_JSP_NOT_DB_OWNER);
			    /*
			    ** We have a DAC failure
			    ** Audit failure to perform action
			    */
			    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
				local_status = audit_jsp_action(jsx->jsx_operation,
				    FALSE, database.du_dbname,
				    &database.du_own, TRUE, &local_dberr);
			    status = E_DB_ERROR;
			    break;
			}

		    }
		    /*
		    ** Reject an attempt to specify a distributed database:
		    */
		    if (database.du_dbservice & DU_1SER_DDB)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1022_JSP_NO_DISTRIB_DB);
			status = E_DB_ERROR;
			break;
		    }

		    /*	Initialize the DCB. */

		    MEmove(sizeof(database.du_dbname), database.du_dbname, ' ',
			sizeof(ndcb->dcb_name), (PTR)&ndcb->dcb_name);
		    MEmove(sizeof(database.du_own), (PTR)&database.du_own, ' ',
			sizeof(ndcb->dcb_db_owner), (PTR)&ndcb->dcb_db_owner);
		    MEmove(sizeof(database.du_dbloc),
			(PTR)&database.du_dbloc, ' ',
			sizeof(ndcb->dcb_location.logical),
                        (PTR)&ndcb->dcb_location.logical);
		    MEmove(sizeof(database.du_dmploc),
			(PTR)&database.du_dmploc, ' ',
			sizeof(ndcb->dcb_dmp->logical),
                        (PTR)&ndcb->dcb_dmp->logical);
		    ndcb->dcb_type = DCB_CB;
		    ndcb->dcb_access_mode = DCB_A_WRITE;
		    ndcb->dcb_served = DCB_MULTIPLE;
		    ndcb->dcb_bm_served = DCB_MULTIPLE;
		    ndcb->dcb_db_type = DCB_PUBLIC;
		    ndcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;

		    dm0s_minit(&ndcb->dcb_mutex, "DCB mutex");
		    continue;
		}
		break;
	    }
	}
	break;
    }

    /*  Close the database table. */

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1010_JSP_CLOSE_IIDB);
	    status = close_status;
	}
	rcb = 0;
    }

    /*  Scan the location table and fill in location information. Here we are
    **	completing the DMP_LOC_ENTRY information for the data and dump
    **	locations for each database.
    */

    for(;;)
    {
	DMP_DCB		*ndcb;

	if (status != E_DB_OK)
	    break;

	/*  Open the database table. */

	status = dm2t_open(dcb, &iilocation, DM2T_IS, DM2T_UDIRECT, DM2T_A_READ,
	    0, 20, 0, 0, dmf_svcb->svcb_lock_list, 0, 0, 0, &tran_id,
	    &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1018_JSP_II_LOCATION);
	    break;
	}

	/*  Probe for each listed database. */

	for (ndcb = jsx->jsx_next;
	    ndcb != (DMP_DCB*)&jsx->jsx_next;
	    ndcb = ndcb->dcb_q_next)
	{
	    struct
	    {
		/*
		** This nifty structure is used to compute a counted-string
		** version of the logical name for searching. We assume
		** that the dcb_location and *dcb_dmp data types are identical
		** and so we can share the structure between them.
		*/

		i2	    count;
		char	    logical[sizeof(ndcb->dcb_location.logical)];
	    }	logical_text;
		char	*place;

	    /* Find the tuple for the data location and fill in the
	    ** DMP_LOC_ENTRY inforamtion for it. Then find the tuple for
	    ** the dump location and fill in the DMP_LOC_ENTRY info for it.
	    */

	    /*		Data location:	    */
/*
Location names may be delimited ids containing
embedded spaces. Bug 72427.
*/
		place = STrindex(ndcb->dcb_location.logical.db_loc_name,
							" ",
					(i4)sizeof(ndcb->dcb_location.logical.db_loc_name));

		logical_text.count = (i2)((char *)place - (char *)ndcb->dcb_location.logical.db_loc_name);
		MEcopy((PTR)&ndcb->dcb_location.logical,
		sizeof(logical_text.logical), (PTR)logical_text.logical);
	    key_list[0].attr_number = DM_1_LOCATIONS_KEY;
	    key_list[0].attr_value = (char *)&logical_text;
	    status = dm2r_position(rcb, DM2R_QUAL, key_list,
				   1, &tid, &jsx->jsx_dberr);

	    if (status == E_DB_OK)
	    {
		status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
				    (char *)&location, &jsx->jsx_dberr);

		if (status == E_DB_OK)
		{
		    if (build_location(ndcb, location.du_l_area,
				    location.du_area, LO_DB,
				    &ndcb->dcb_location) != E_DB_OK)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1024_JSP_BAD_LOC_INFO);
			status = E_DB_ERROR;
		    }
		}
		else
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1023_JSP_NO_LOC_INFO);
		}
	    }

	    if (status != E_DB_OK)
		break;

	    /*		DUMP location:	    */

/*
Location names may be delimited ids containing
embedded spaces. Bug 72427.
*/
		place = STrindex(ndcb->dcb_location.logical.db_loc_name,
							" ",
						(i4)sizeof(ndcb->dcb_location.logical.db_loc_name));

		logical_text.count = (i2)((char *)place - (char *)ndcb->dcb_location.logical.db_loc_name);

	    MEcopy((PTR)&ndcb->dcb_dmp->logical,
		sizeof(logical_text.logical), (PTR)logical_text.logical);
	    key_list[0].attr_number = DM_1_LOCATIONS_KEY;
	    key_list[0].attr_value = (char *)&logical_text;
	    status = dm2r_position(rcb, DM2R_QUAL, key_list,
				   1, &tid, &jsx->jsx_dberr);

	    if (status == E_DB_OK)
	    {
		status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
				    (char *)&location, &jsx->jsx_dberr);

		if (status == E_DB_OK)
		{
		    if (build_location(ndcb, location.du_l_area,
				    location.du_area, LO_DMP,
				    ndcb->dcb_dmp) != E_DB_OK)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1024_JSP_BAD_LOC_INFO);
			status = E_DB_ERROR;
		    }
		}
		else
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1023_JSP_NO_LOC_INFO);
		}
	    }

	    if (status != E_DB_OK)
		break;
	}
	break;
    }

    /*  Close the location table. */

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1013_JSP_CLOSE_IILOC);
	    status = close_status;
	}
	rcb = 0;
    }

    /*
    ** End SXF session for iidbdb
    */
    local_status = dmf_sxf_end_session();
    if (status == E_DB_OK && local_status != E_DB_OK)
    {
	status = local_status;
        SETDBERR(&jsx->jsx_dberr, 0, local_error);
    }

    /*	End the transaction. */

    if (log_id)
    {
	close_status = dmf_jsp_commit_xact(&tran_id, log_id, lock_id, DMXE_ROTRAN,
			    &ctime, &local_dberr);
	if ( close_status && status == E_DB_OK )
	{
	    jsx->jsx_dberr = local_dberr;
	    status = close_status;
	}
    }

    /*	Close the database. */

    close_status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
			dbdb_flag, &local_dberr);
    if (close_status != E_DB_OK && status == E_DB_OK)
    {
	jsx->jsx_dberr = local_dberr;
	status = close_status;
    }

    /*
    ** Free the DCB memory used for the iidbdb open.
    */
    dm0m_deallocate((DM_OBJECT **)&dcb);

    if (status != E_DB_OK)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: build_location	- Build a physical location into DCB.
**
** Description:
**      This routine takes the DCB and the default database information and
**	creates the appropriate DCB location entry.
**
**	The location will be either the database location or the dump location,
**	as specified by the caller.
**
** Inputs:
**      dcb                             Pointer to DCB.
**      l_path                          Length of location path.
**      path                            Pointer to location path.
**	loc_type			The "what" flag to LOingpath
**	location			Which location is to be filled in
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-mar-1987 (Derek)
**          Created for Jupiter.
**	24-oct-1988 (greg)
**	    return status so LOingpath can be checked
**	21-may-90 (bryanp)
**	    pass loc_type and location to allow building of dump locations
**	    as well as database locations.
**	10-jun-2003 (somsa01)
**	    Updated code to account for embedded spaces in path names.
*/
static DB_STATUS
build_location(
DMP_DCB            *dcb,
i4                 l_path,
char		    *path,
char		    *loc_type,
DMP_LOC_ENTRY	    *location)
{
    i4		    i;
    char	    *cp;
    LOCATION	    loc;
    char	    db_name[DB_DB_MAXNAME+1];
    char	    area[256];
    DB_STATUS	    status;

    for (i = 0; i < sizeof(dcb->dcb_name); i++)
    {
	db_name[i] = dcb->dcb_name.db_db_name[i];

	if (dcb->dcb_name.db_db_name[i] == ' ')
	    break;
    }

    db_name[i] = '\0';

    STlcopy(path, area, l_path);
    STtrmwhite(area);

    status = LOingpath(area, db_name, loc_type, &loc);
    if (status == OK)
    {
	LOtos(&loc, &cp);

	if (*cp == EOS)
	{
	    status = E_DB_ERROR;
	}
	else
	{
	    MEmove(STlength(cp), cp, ' ',
		   sizeof(location->physical),
		   (PTR)&location->physical);
	    location->phys_length = STlength(cp);
	}
    }

    return(status);
}

/*{
** Name: jsp_echo_usage	- Write command line usage.
**
** Description:
**	Echo command line usage message.
**
** Inputs:
**      jsx                        jsx context
**
** Outputs:
**	    VOID
**
** Side Effects:
**	    none
**
** History:
**      26-jul-1993 (jnash)
**          Created.
**      20-sep-1993 (jnash)
**	    Add -all to auditdb, "-" to "mdevice", #x to ckpdb.
**	    Move rollforwarddb #c to supported list.
**	    Eliminate erroneous err_code from SIprintf() arg list.
**	31-jan-1993 (jnash)
**	    Fix hh:mm:ss display on rollforward help.
**	21-feb-1994 (jnash)
**	  - Add auditdb/rollforwarddb -start_lsn, -end_lsn, auditdb #c
**	    and -verbose.
**	  - Remove ckpdb, rollforwarddb -s.
**	  - Fix alterdb format, add unsupported rolldb #x.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Show New Argument for rfp
**	06-jan-1995 (shero03)
**	    Support Partial Checkpoint
**	    	Show -table= for CPP 
**      05-apr-1995 (stial01)
**          Fixed help for relocatedb
**      06-mar-1996 (stial01)
**          Variable page size project:
**          Added help for dmf_cache_size parameters
**	07-jul-1996 (hanch04)
**	    added multiple devices for parallel backup and restore.
**	17-jul-96 (nick)
**	    Correct incorrect usage messages.
**	05-dec-1996 (nanpr01)
**	    Bug 79106 : Incorrect online usage as reported. Apparently
**	    fastload accepts both the syntax.
**	31-Jul-1997 (shero03)
**	    Add next_jnl_file to ALTERDB
**	13-Nov-1997 (jenjo02)
**	    Added -s option (JSX_CKP_STALL_RW) for online checkpoint.
**	15-Mar-1999 (jenjo02)
**	    Removed much-maligned "-s" online CKPDB option,
**	    JSX_CKP_STALL_RW, stall mechanism completely redone.
**	27-Oct-2008 (wanfr01)
**	    Bug 121138
**	    Added -i option into alterdb usage screen.
**      11-Aug-2009 (hanal04) Bug 122441
**          Added -timeout option to usage output.
**      24-Sep-2009 (coomi01) b122629
**          Enhance usage of dbname parameter with optional serverclass
**          for rollforwarddb. 
**
[@history_template@]...
*/
static VOID
jsp_echo_usage(
DMF_JSX		*jsx)
{
    switch (jsx->jsx_operation)
    {
    case JSX_O_CPP:

	SIprintf(
	    "usage: ckpdb [-d] [+j|-j] [-l] [#m ] [-mdevice1,device2,...] \n");

	SIprintf(
	    "\t[-table=tbl1,tbl2,...] [-uusername] [-v] [+w|-w] \n");

	SIprintf(
	    "\t[-timeout=mm:ss] dbname\n");

#ifdef xDEBUG
	SIprintf("------------ unsupported --------------\n");
	SIprintf(
	    "\t[-o] [-z] [#x] [#w]\n");
#endif

	break;

    case JSX_O_ATP:
	SIprintf(
	    "usage: auditdb [-a] [#cn] [-bdd-mmm-yyyy[:hh:mm:ss]] [-edd-mmm-yyyy[:hh:mm:ss]]\n");

	SIprintf(
	    "\t[-iusername] [-inconsistent] [-file[=file1,file2,...]]\n");

	SIprintf(
	    "\t[-table=tbl1,tbl2,...] [-uusername] [-wait] dbname\n");

#ifdef xDEBUG
	SIprintf("------------ obsolete but supported --------------\n");
	SIprintf(
	    "\t[-f] [-t]\n");

	SIprintf("------------ unsupported --------------\n");
	SIprintf(
	    "\t[-all] [-start_lsn=a,b] [-end_lsn=c,d] [-verbose] \n");
#endif

	break;

    case JSX_O_RFP:

	SIprintf(
	    "usage: rollforwarddb dbname[/server_class] [+c|-c] [#c[n]] [+j|-j] [-mdev1,dev2,...] [-uusername]\n");

	SIprintf(
	    "\t[#m[n]] [-v] [+w|-w] [-bdd-mmm-yyyy[:hh:mm:ss]] [-edd-mmm-yyyy[:hh:mm:ss]]\n");

	SIprintf(
	    "\t[-incremental] [-table=] [-nosecondary_index] [-on_error_continue]\n");

/* ---> Removed until implemented
	SIprintf(
	    "\t[-online] [-status_rec_count=] [-force_logical_consistent]\n");
 <--- */

	SIprintf(
	    "\t[-relocate -location= -new_location=] [-statistics]\n");
	SIprintf(
	    "\t[-dmf_cache_size=] [-dmf_cache_size_4k=] [-dmf_cache_size_8k=]\n");
	SIprintf(
	    "\t[-dmf_cache_size_16k=] [-dmf_cache_size_32k=] [-dmf_cache_size_64k=]\n");

#ifdef xDEBUG
	SIprintf("------------ unsupported --------------\n");
	SIprintf(
	    "\t[#f] [#x] [-nologging] [-norollback] [-start_lsn=a,b] [-end_lsn=c,d]\n");
#endif

	break;

    case JSX_O_INF:

	SIprintf(
	    "usage: infodb [#c[n]] [-uusername] {dbname}\n");

	break;

    case JSX_O_ALTER:
	SIprintf(
	    "usage: alterdb [-disable_journaling] [-delete_oldest_ckp]\n");
	SIprintf(
	    "\t[-init_jnl_blocks=nnn] [-jnl_block_size=nnn] [-next_jnl_file]\n");
	SIprintf(
	    "\t[-target_jnl_blocks=nnn] [-delete_invalid_ckp]\n");
	SIprintf(
	    "\t[-n|-i[ucollation_name]] [-keep=nnn]\n");
	SIprintf(
	    "\t[-disable_mvcc] [-enable_mvcc]\n");
        SIprintf(
            "\t[-disable_mustlog] [-enable_mustlog]\n");


# ifdef PMODE_DOCUMENTED
	SIprintf(
	    "\t[-[no]online_checkpoint] [-[no]allow_ddl]\n");
# endif /* PMODE_DOCUMENTED */

	SIprintf("\tdbname\n");

	break;

    case JSX_O_FLOAD:
        SIprintf(

            "usage: fastload dbname [+w|-w] -file=<filename> -table=<tablename>\n");
        break;

    case JSX_O_RELOC:
	SIprintf(
	    "usage: relocatedb [-new_ckp_location= | -new_dump_location=  |\n");
	SIprintf(
	    "\t-new_work_location= | -new_jnl_location=  |\n");
	SIprintf(
	    "\t-new_database= [-location= -new_location= ]]\n");
	SIprintf(
	    "\t[+w -w]\n");
	SIprintf(
	    "\tdbname\n");
	break;
    }
}

/*{
** Name: jsp_normalize		- Normalize command line parameter.
**
** Description:
**
**	Normalize command line parameter, return unpadded version.
**	Don't modify the case of the command line parameter, it is determined 
**	per-database.
**
** Inputs:
**      jsx                     jsx context
**      untrans_len		Length of object, or zero if null terminated 
**      trans_len		max length of translated object
**      obj			object to be masticated, null terminated
**
** Outputs:
**      obj			translated object 
**      trans_len		actual length of translated object
**	err_code		reason for DB_STATUS != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**      20-sep-1993 (jnash)
**          Created in support of delim id case semantics in jsp utilites.
[@history_template@]...
*/
static DB_STATUS 
jsp_normalize(
DMF_JSX		*jsx, 
u_i4		untrans_len,
u_char		*obj,
u_i4		*trans_len,
u_char		*unpad_obj)
{
    u_i4		ret_mode;
    i4	    	status;

    /*
    ** Normalize the delimited name, leave the case as is.
    ** Caller has initialized trans_len 
    */
    status = cui_idxlate(obj, &untrans_len, unpad_obj, trans_len, 
	CUI_ID_DLM_M, &ret_mode, &jsx->jsx_dberr);
    if (DB_FAILURE_MACRO(status))
    {
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: parse_table_list		- Parse table list.
**
** Description:
**
**	Parse a comma separated list of command line table names, insert 
**	into jsp_tbl_list.  Normalize the names as required.
**
**	Routine exists to reduce inline code.
**
** Inputs:
**      jsx                     jsx context
**      src			pointer to string of table names
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      jsx_tbl_list		filled in 
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**      20-sep-1993 (jnash)
**          Created in support of delim id case semantics in jsp utilites.
**      28-mar-1993 (jnash)
**          Initialize uninitialized variable that was causing
**	    auditdb -table to fail.
**      28-feb-2000 (gupsh01)
**          Added support for table names with format owner_name.table_name.
**	17-sep-2002 (somsa01)
**	    Make sure we at least initialize the tbl_owner.db_own_name for
**	    each table found to NULL.
**	28-jul-2003 (somsa01)
**	    Initialize the tbl_owner.db_own_name field if we are not using
**	    the "dot" case.
*/
static DB_STATUS 
parse_table_list(
DMF_JSX		*jsx, 
char		*cmd_line)
{
    DB_STATUS		status = E_DB_OK;
    i4		i = 0;
    u_i4		tbl_name_len;
    u_i4		len_trans;
    u_char		*tmp_end;
    u_char		*src = (u_char *)cmd_line;
    u_char		unpad_name[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + 20];
    bool    		comma_found;
    char		error_buffer[ER_MAX_LEN];
    i4		error_length;
    u_i4                own_name_len;
    u_char              *tmp_end2;
    bool                dot_found;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Copy table names to jsx_tbl_list.  
    */
    while (*src)
    {
	/*
	** Delimited id's have a leading double quote.
	*/
	if (*src == '\"')
	{
	    /*
	    ** Delimited id found, find its length and normalize it.
	    */
	    status = cui_idlen(src, ',', &tbl_name_len, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM104E_JSP_NORMALIZE_ERR);
		break;
	    }

            /*
            ** Check for '.' eg table of type owner.tablename.
            */
            status = cui_idlen(src, '.', &own_name_len, &jsx->jsx_dberr);
            if (status != E_DB_OK)
            {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM104E_JSP_NORMALIZE_ERR);
                break;
            }

            if(own_name_len >= tbl_name_len)
            {
                own_name_len = 0;
                dot_found = FALSE;
            }
            else
            {
                tbl_name_len -= own_name_len - 1;
                dot_found = TRUE;
            }

            if (dot_found == TRUE)
            {
		len_trans = DB_OWN_MAXNAME;
                status = jsp_normalize(jsx, own_name_len, (u_char *)src,
					&len_trans, unpad_name);
                if (status != E_DB_OK)
		    break;

                MEmove(len_trans, (char *)unpad_name, ' ', DB_OWN_MAXNAME,
                (char *)jsx->jsx_tbl_list[i].tbl_owner.db_own_name);

                src = src + own_name_len + 1;
            }
	    else
	    {
		MEfill( sizeof(jsx->jsx_tbl_list[i].tbl_owner.db_own_name), 0,
			(char *)jsx->jsx_tbl_list[i].tbl_owner.db_own_name );
	    }
	
	    /*
	    ** Comma or NULL terminator.
	    */
	    tmp_end = src + tbl_name_len;
	    if (*tmp_end == ',') 
	    {
		*tmp_end = EOS;
		comma_found = TRUE;
	    }
	    else 
		comma_found = FALSE;

	    /*
	    ** Normalize and save the name, flag it as delimited. 
	    */
	    len_trans = DB_TAB_MAXNAME;
	    status = jsp_normalize(jsx, tbl_name_len, (u_char *)src, 
		&len_trans, unpad_name);
	    if (status != E_DB_OK)
		break;

	    MEmove(len_trans, (char *)unpad_name, ' ', DB_TAB_MAXNAME, 
		(char *)jsx->jsx_tbl_list[i].tbl_name.db_tab_name);

	    jsx->jsx_tbl_list[i].tbl_delim = TRUE;

	    /*
	    ** Skip to next entry.
	    */
	    src = tmp_end;
	    if (comma_found)
		src++;
	}
	else
	{
	    /*
	    ** Table name not delimited, save in jsx after checking length.
	    ** No comma implies NULL terminated name.
	    */

	    tmp_end = (u_char *)STchr((char *)src, ',');
	    tmp_end2 = (u_char *)STchr((char *)src, '.'); 

            if (tmp_end != NULL)
            {
                  /* more than one tables */
                  if ((tmp_end2 != NULL) && ((tmp_end > tmp_end2)))
                  {
                  /* First table contains the name of owner as well */
                  /* eg ownername.tablename */
                      tbl_name_len = tmp_end - tmp_end2 - 1;
                      own_name_len = tmp_end2 - src;
                   }
                   else
                   {
                       tbl_name_len = tmp_end - src;
                       own_name_len = 0;
                   }
             }
             else if (tmp_end2 != NULL)
             {
                own_name_len = tmp_end2 - src;
                tbl_name_len = STlength((char *)src) - own_name_len - 1;
              }
              else
              {
                 tbl_name_len = STlength((char *)src);
                 own_name_len = 0;
              }

            if ((tbl_name_len > DB_TAB_MAXNAME) 
			|| (own_name_len > DB_OWN_MAXNAME))
	    {
		status = E_DB_ERROR;
		SETDBERR(&jsx->jsx_dberr, 0, E_DM104F_JSP_NAME_LEN);
		break;
	    }

            if(own_name_len != 0)
            {
                MEmove(own_name_len, (char *)src, ' ', DB_OWN_MAXNAME,
                   (char *)jsx->jsx_tbl_list[i].tbl_owner.db_own_name);
                src += own_name_len + 1;
            }
	    else
	    {
		MEfill( sizeof(jsx->jsx_tbl_list[i].tbl_owner.db_own_name), 0,
			(char *)jsx->jsx_tbl_list[i].tbl_owner.db_own_name );
	    }

	    MEmove(tbl_name_len, (char *)src, ' ', DB_TAB_MAXNAME, 
		(char *)jsx->jsx_tbl_list[i].tbl_name.db_tab_name);

	    jsx->jsx_tbl_list[i].tbl_delim = FALSE;

	    /*
	    ** Skip to next entry.
	    */
	    src += tbl_name_len;
	    if ((src) && (*src == ','))
		src++;
	}

	if (status != E_DB_OK)
	   break;

	/*
	** Skip to next table entry.
	*/
	i++; 

    }

    if (status == E_DB_OK)
    {
	jsx->jsx_status |= JSX_TBL_LIST;
	jsx->jsx_tbl_cnt = i;
	return(E_DB_OK);
    }

    /*
    ** Error handling.  Output the specific error and then reduce them all
    ** to a bad argument.
    */
    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	(DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	&error, 0);
    dmf_put_line(0, error_length, error_buffer);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
    
    return(E_DB_ERROR);
}

/*{
** Name: parse_device_list		- Parse device list.
**
** Description:
**
**	Parse a comma separated list of command line device names, insert 
**	into jsp_dev_list.  Normalize the names as required.
**
**	Routine exists to reduce inline code.
**
** Inputs:
**      jsx                     jsx context
**      src			pointer to string of table names
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      jsx_dev_list		filled in 
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**	20-may-1996 (hanch04)
**	    Created from parse_table_list
[@history_template@]...
*/
static DB_STATUS 
parse_device_list(
DMF_JSX		*jsx, 
char		*cmd_line)
{
    DB_STATUS		status = E_DB_OK;
    i4		i = 0;
    u_i4		dev_name_len;
    u_char		*tmp_end;
    u_char		*src = (u_char *)cmd_line;
    char		error_buffer[ER_MAX_LEN];
    i4		error_length;
    i4		error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Copy table names to jsx_tbl_list.  
    */
    while ( (*src) && i < (JSX_MAX_LOC) )
    {
   	/*
	** Save dev in jsx after checking length.
	** No comma implies NULL terminated name.
	*/

	tmp_end = (u_char *)STchr((char *)src, ',');
	if (tmp_end == NULL)
		dev_name_len = STlength((char *)src);
	else
		dev_name_len = tmp_end - src;

	if (dev_name_len > DI_PATH_MAX)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM104F_JSP_NAME_LEN);
	    break;
	}

	MEmove(dev_name_len, (char *)src, ' ', DI_PATH_MAX, 
	      (char *)jsx->jsx_device_list[i].dev_name);
	jsx->jsx_device_list[i].l_device = dev_name_len;
	jsx->jsx_device_list[i].dev_pid = -1;
	jsx->jsx_device_list[i].status = E_DB_OK;

	/*
	** Skip to next entry.
	*/
	src += dev_name_len;
	if ((src) && (*src == ','))
	    src++;

	if (status != E_DB_OK)
	   break;

	/*
	** Skip to next table entry.
	*/
	i++; 

    }

    if (status == E_DB_OK)
    {
        jsx->jsx_status |= JSX_DEVICE;
	jsx->jsx_dev_cnt = i;
	return(E_DB_OK);
    }

    /*
    ** Error handling.  Output the specific error and then reduce them all
    ** to a bad argument.
    */
    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	(DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	&error, 0);
    dmf_put_line(0, error_length, error_buffer);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
    
    return(E_DB_ERROR);
}

/*{
** Name: ex_handler	- DMF exception handler.
**
** Description:
**	Exception handler for JSP program.  This handler is declared at process
**	startup.  All exceptions which come through here will cause the
**	program to fail.
**
**	An error message including the exception type and PC will be written
**	to the log file before exiting.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	EXDECLARE causes us to return execution to dmfacp routine.
**
** History:
**	25-dec-1993 (rogerk)
**	    Copied exception handler from DMFACP.C
**	23-may-1994 (bryanp) B58498
**	    If an EXINTR signal is received, notify LG and LK, then
**		continue. This allows LG and LK to hold off the ^c while
**		under the mutex, then to respond to it once the mutex has
**		been released.
*/
static STATUS
ex_handler(
EX_ARGS	    *ex_args)
{
    i4	    err_code;

    if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else if (ex_args->exarg_num == EXINTR)
    {
	dmf_put_line(0, sizeof("Exiting due to interrupt...")-1,
			"Exiting due to interrupt...");
	LGintrcvd();
	LKintrcvd();
	return (EXCONTINUES);
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    ** Note: the fourth argument to ulx_exception must be FALSE so
    ** that it does not try to call scs_avformat() since there is no
    ** session in the ACP.
    */
    (VOID) ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    /*
    ** Returning with EXDECLARE will cause execution to return to the
    ** top of the DMFJSP routine.
    */
    return (EXDECLARE);
}

/*{
** Name: parse_lsn		- Parse a command line lsn.
**
** Description:
**
**	Parse an lsn of the form: xx,yy
**
** Inputs:
**      cmd_line		command line where lsn exists
**
** Outputs:
**      lsn			pointer to the output lsn
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**
** Side Effects:
**	    none
**
** History:
**      21-feb-1994 (jnash)
**          Created in support of -start_lsn and -end_lsn parameters.
[@history_template@]...
*/
static DB_STATUS 
parse_lsn(
char		*cmd_line,
LG_LSN		*lsn)
{
    DB_STATUS	status;
    char      	*tmp_ptr;

    for (;;)
    {
	/*
	** CVal requires a terminating space or null.
	*/
	if (tmp_ptr = STchr(cmd_line, ','))
	    *tmp_ptr++ = EOS;
	else
	    break;

	status = CVal(cmd_line, (i4 *)&lsn->lsn_high);
	if (status != OK)
	    break;

	status = CVal(tmp_ptr, (i4 *)&lsn->lsn_low);
	if (status != OK)
	    break;

        return(E_DB_OK);
    }

    return(E_DB_ERROR);
}

/*
** Name: audit_jsp_action - write an audit record for jsp action.
**
** Description:
**	This routine writes an audit record using dma_write_audit given the
**	jsp action being performed.
**
** Inputs:
**	action		- action being performed
**	succeed		- wether action succeeded
**	obj_name	- Object name
**	obj_owner	- Object owner
**	force_write	- Force write to disk
**	err_code	- Error info
**
** Outputs:
**	none
**
** Returns
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	18-feb-94 (stephenb)
**	    Initial createion.
**	9-mar-94 (stephenb)
**	    Add dbname data to dma_write_audit() call.
*/
static DB_STATUS
audit_jsp_action(
	i4		action,
	bool		succeed,
	char		*obj_name,
	DB_OWN_NAME	*obj_owner,
	bool		force_write,
	DB_ERROR	*dberr)
{
    SXF_ACCESS		access;
    i4		msgid;
    DB_STATUS		status;
    DB_DB_NAME		dbname;

    /*
    ** construct database name
    */
    MEcopy(obj_name, DB_DB_MAXNAME, dbname.db_db_name);

    if (succeed)
	access = SXF_A_SUCCESS;
    else
	access = SXF_A_FAIL;
    /*
    ** determin access, and message id using action
    */
    switch (action)
    {
    case JSX_O_CPP:	/* checkpointing a database */
	access |= SXF_A_SELECT;
	msgid = I_SX270B_CKPDB;
	break;
    case JSX_O_ATP:	/* generating an audit trail */
	access |= SXF_A_SELECT;
	msgid = I_SX2709_AUDITDB;
	break;
    case JSX_O_DTP:	/* Generating a dump trail */
	access |= SXF_A_SELECT;
	msgid = I_SX2747_DUMPDB;
	break;
    case JSX_O_RFP:	/* Rolling forward a database */
	access |= SXF_A_UPDATE;
	msgid = I_SX270C_ROLLDB;
	break;
    case JSX_O_INF:	/* displaying database info */
	access |= SXF_A_SELECT;
	msgid = I_SX2711_INFODB;
	break;
    case JSX_O_ALTER:	/* altering database state */
	access |= SXF_A_ALTER;
	msgid = I_SX270D_ALTERDB;
	break;
    case JSX_O_RELOC:   /* relocate jnl,dmp,ckp,work locations or db */
	access |= SXF_A_UPDATE;
	msgid = I_SX2752_RELOCATE;  
	break;
    }
    /*
    ** Write audit
    */
    status = dma_write_audit(SXF_E_DATABASE, access, 
	obj_name, 
	GL_MAXNAME, 
	obj_owner, 
	msgid, 
	force_write, 
	dberr,
	&dbname);

    return (status);
}

/*{
**
**  Name: jsp_chk_priv - validate a given user's ownership of a given privilege
**
**  Description:
**	Calls PM to determine the user's allotted privileges, then
**	checks whether the specific required privilege is among them.
**
**  Inputs:
**	usr  -	user to validate
**	priv -	name of required privilege
**
**  Returns:
**	TRUE/FALSE	- user has the privilege
**	
**  History:
**      24-jun-1994 (jnash)
**          Copied verbatim from scs_chk_priv because I don't know
**          where routines common to both facilities belong (gl?).
*/
static
bool
jsp_chk_priv( 
char *user_name, 
char *priv_name )
{
	char	pmsym[128+DB_MAXNAME], userbuf[DB_OWN_MAXNAME+1], *value, *valueptr ;
	char	*strbuf = 0;
	int	priv_len;

        /* 
        ** privileges entries are of the form
        **   ii.<host>.*.privilege.<user>:   SERVER_CONTROL,NET_ADMIN,...
	**
	** Currently known privs are:  SERVER_CONTROL,NET_ADMIN,
	**  	    MONITOR,TRUSTED
        */

	STncpy( userbuf, user_name, DB_OWN_MAXNAME );
	userbuf[ DB_OWN_MAXNAME ] = '\0';
	STtrmwhite( userbuf );
	STprintf(pmsym, "$.$.privileges.user.%s", userbuf);
	
	/* check to see if entry for given user */
	/* Assumes PMinit() and PMload() have already been called */

	if( PMget(pmsym, &value) != OK )
	    return FALSE;
	
	valueptr = value ;
	priv_len = STlength(priv_name) ;

	/*
	** STindex the PM value string and compare each individual string
	** with priv_name
	*/
	while ( *valueptr != EOS && (strbuf=STchr(valueptr, ',' )))
	{
	    if (!STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	    /* skip blank characters after the ','*/	
	    valueptr = STskipblank(strbuf+1, 10); 
	}

	/* we're now at the last or only (or NULL) word in the string */
	if ( *valueptr != EOS  && 
              !STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	return FALSE;
}

/*
** Name: parse_loc_list			-Parse location list
**
** Description:
**
**      Parse a comma separated list of command line location names, insert
**      into loc_list. Normalize the names as required.
**
** Inputs:
**      jsx                     jsx context
**      loc_list                location list 
**      src                     pointer to string of location names
**      maxloc                  max number of location names loc_list can hold
**      err_code                reason for status != E_DB_OK
**
** Outputs:
**      Returns:
**          E_DB_OK, E_DB_ERROR
**      loc_list filled in
**      err_code                reason for status != E_DB_OK
**
** Side Effects:
**          none
**
** History:
**      18-nov-94 (alison)
**          Created in support of location relocation
**	 2-jul-96 (nick)
**	    Error if no locations found ; remove a FIX ME by defining a
**	    new message.
[@history_template@]...
*/
static DB_STATUS 
parse_loc_list(
DMF_JSX		*jsx, 
JSX_LOC         *loc_list,
char		*cmd_line,
i4          maxloc)
{
    DB_STATUS		status = E_DB_OK;
    i4		i;
    u_i4		loc_name_len;
    u_char		*tmp_end;
    u_char		*src = (u_char *)cmd_line;
    char		error_buffer[ER_MAX_LEN];
    i4		error_length;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /* Init loc_list */
    
    for (i = 0; i < maxloc; i++)
    {
	loc_list[i].loc_name.db_loc_name[0] = '\0';
    }

    /*
    ** Copy location names to loc_list
    */

    i = 0;
    while (*src)
    {
	/*
	** Location name not delimited, save in jsx after checking length.
	** No comma implies NULL terminated name.
	*/

	tmp_end = (u_char *)STchr((char *)src, ',');
	if (tmp_end == NULL)
	    loc_name_len = STlength((char *)src);
	else
	    loc_name_len = tmp_end - src;

	if (loc_name_len > DB_LOC_MAXNAME)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM104F_JSP_NAME_LEN);
	    break;
	}

	if (i >= maxloc)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM105B_JSP_NUM_LOCATIONS);
	    break;
	}

	MEmove(loc_name_len, (char *)src, ' ', DB_LOC_MAXNAME, 
		(char *)loc_list[i].loc_name.db_loc_name);

	/*
	** Skip to next entry.
	*/
	src += loc_name_len;
	if ((src) && (*src == ','))
	    src++;

	/*
	** Skip to next location entry.
	*/
	i++; 

    }

    if (status == E_DB_OK)
    {
	if (i == 0)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM105B_JSP_NUM_LOCATIONS);
	}
	else
	{
	    return(E_DB_OK);
	}
    }

    /*
    ** Error handling.  Output the specific error and then reduce them all
    ** to a bad argument.
    */
    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	(DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	&error, 0);
    dmf_put_line(0, error_length, error_buffer);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
    
    return(E_DB_ERROR);
}


/*
** Name: jsp_log_info			Write informational msg to errlog	
**
** Description: Write informational msg to errlog
**
** Inputs:
**      argc
**      argv
**
** Outputs:
**      Returns:
**          None
**
** Side Effects:
**          none
**
** History:
**      04-may-2007 (stial01)
**          Created.
[@history_template@]...
*/
static VOID
jsp_log_info(DMF_JSX *jsx, i4 argc, char *argv[])
{
    i4		error;
    char	buf[ER_MAX_LEN];
    char	*cp = buf;
    i4		i;
    i4		start_arg;

    buf[0] = '\0';
    start_arg = 1;
    if (argc >= 2 && STcompare("rolldb", argv[1]) == 0)
    {
	STcat(cp, "rollforwarddb ");
	start_arg++;	
    }

    /*  Parse the rest of the command line. */
    for (i = start_arg; i < argc; i++)
    {
	if (STlength(cp) + STlength(argv[i]) + 2 < sizeof(buf)) 
	{
	    STcat(cp, argv[i]);
	    STcat(cp, " ");
	}
    }

    i = STlcopy(buf, jsx->jsx_lgk_info, JSX_LGK_INFO_SIZE-1);

    if (jsx->jsx_operation == JSX_O_CPP || jsx->jsx_operation == JSX_O_RFP ||
	jsx->jsx_operation == JSX_O_RELOC || jsx->jsx_operation == JSX_O_ALTER)
	    uleFormat(NULL, E_DM105D_JSP_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, STlength(buf), buf);

}
