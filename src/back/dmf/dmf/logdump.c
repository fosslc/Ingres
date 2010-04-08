/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <nm.h>
#include    <cv.h>
#include    <lo.h>
#include    <st.h>
#include    <tr.h>
#include    <me.h>
#include    <cm.h>
#include    <ep.h>
#include    <er.h>
#include    <si.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0llctx.h>
#include    <dm0s.h>
#include    <dmd.h>

/**
**
**  Name: LOGDUMP.C - {@comment_text@}
**
**  Description:
**	Logdump is a debugging tool, primarily. Its main use is to dump the
**	contents of the Ingres log file, for capture of unusual situations
**	which indicate problems/bugs. Secondarily, logdump is useful for
**	examining the records which are logged, e.g. for performance analysis.
**
**	Logdump is invoked as follows:
**
**	    logdump {[logfile name]}
**
**	where logfile name is optional. If logfile name specified, then all the logfiles
**                                      for a partitoned TX log must be specified.
**	Logdump also takes a number of parameters:
**
**	-bsnumber
**	    start with logfile block 'number'
**	-benumber
**	    end with logfile block 'number'.
**	-as<number,number,number>
**	    start with logaddress <number,number,number>
**	-ae<number,number,number>
**	    end with logaddress <number,number,number>
**	-ddb_id
**	    dump only log records for database with ID number 'db_id'
**	-full
**	    dump then entire logfile ignoring the logical BOF/EOF in the 
**	    log header.  This option accepted only in standalone logdump.
**	-rrec_type
**	    dump only log records of type 'rec_type'
**	-ttablename
**	    dump only log records for table named 'tablename'
**	-xtx_id
**	    dump only log records for transaction 'tx_id'.
**	-sknumber
**	    skip 'number' of records before printing.
**	-stnumber
**	    stop after 'number' of records have been printed.
**	-verbose
**	    dump header and tuple/key information in hex.
**
**	The transaction ID and database ID must be given in hexadecimal. All
**	other numbers must be given in decimal.
**
**
PROGRAM =	ntlogdmp
**
**  History:    
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**      08-aug-1990 (jonb)
**          Add spaces in front of "main(" so mkming won't think this
**          is a main routine on UNIX machines where II_DMF_MERGE is
**          always defined (reintegration of 14-mar-1990 change).
**	22-aug-1990 (bryanp)
**	    Added a number of new parameters to allow filtering of portions
**	    of the log file.
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	16-jul-1991 (bryanp)
**	    B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**      19-aug-1991 (jnash) merged 13-may-1991 (bonobo)
**          Added ming hints from ingres63p logdump.
**      26-sep-1991 (jnash) merged 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added support for variable length
**	    table and owner names.  Also removed system catalog log records
**	    (sdel, sput, srep) as they are now collapsed with normal records.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	16-feb-1992 (rogerk)
**	    Change CS initialization to use scf memory allocation rather than
**	    CS initiate so that semaphore calls in DI will work properly.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format - moved the database and transaction
**		id's out of the lctx_header and into the log record itself.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Pass DM0L_FULL_FILENAME flag to dm0l_allocate when doing a
**		    standalone logdump.
**	26-apr-1993 (jnash)
**	    LRC mandated change to use -verbose rather than -v.
**	24-may-1993 (andys)
**	    Deal with newly initialized logfiles by finding EOF then searching
**	    backwards to find BOF. 	
**	    Clean up a few comments.
**	    Return 0 at end of log_dump.
**	    Disallow standalone logdump if logging system online
**	21-jun-1993 (rogerk)
**	    Added -full parameter to allow offline logdump to process the
**	    entire logfile rather than just the records between the logical
**	    BOF and EOF.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (jnash)
**	    Add -help.
**	20-sep-1993 (andys)
**	    Slight alteration to help message.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called by scf_call(SCF_MALLOC)), 
**	    so dump them.
**	31-jan-1996 (canor01)
**	    The initial TRset_file() for all non-VMS platforms is the same.
**	    Allow both '-verbose' and '-v'.
**	26-Oct-1996 (jenjo02)
**	    Added *name to dm0s_minit() call.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in log_dump() routine.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      30-mar-2005 (stial01)
**          Init svcb_pad_bytes
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Use new form of uleFormat
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**      30-Oct-2009 (horda03) Bug 122823
**          Allow multiple filenames to handle a partitioned TX log file.
[@history_template@]...
**/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	logdump_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*
** Name: LDMP_CB	- Logdump runtime control block.
**
** Description:
**	This control block is used to hold interesting parameters about
**	this invocation of logdump.
**
** History:
**	22-aug-1990 (bryanp)
**	    Created.
**	15-mar-1993 (jnash)
**	    Add verbose command line option and associated code.
**	21-jun-1993 (rogerk)
**	    Added -full parameter (LDMP_FULLFILE) to allow offline logdump to 
**	    process the entire logfile rather than just the records between 
**	    the logical BOF and EOF.
**      30-Oct-2009 (horda03) Bug 122823
**          Replaced ldmp_l_filename and ldmp_filename with ldmp_no_partitions and
**          ldmp_partitions to allow multiple filenames to be specified for a partitioned
**          TX log file.
*/
typedef struct
{
    u_i4	    ldmp_flags;
#define			LDMP_STANDALONE		0x000001
			    /*
			    ** 'standalone' logdump -- i.e., access the logfile
			    ** directly through LGC routines -- VMS only.
			    */
#define			LDMP_BLOCKNO_START	0x000002
#define			LDMP_BLOCKNO_END	0x000004
#define			LDMP_LOGADDR_START	0x000008
#define			LDMP_LOGADDR_END	0x000010
#define			LDMP_DB_ID		0x000020
#define			LDMP_TNAME		0x000040
#define			LDMP_RECTYPE		0x000080
#define			LDMP_SKIP_COUNT		0x000100
#define			LDMP_STOP_COUNT		0x000200
#define			LDMP_TXID		0x000400
			    /*
			    ** Selection by transaction ID
			    */
#define			LDMP_VERBOSE		0x000800
#define			LDMP_FULLFILE		0x001000

    i4              ldmp_no_partitions;
    LG_PARTITION    ldmp_partitions [LG_MAX_FILE];
    DB_TRAN_ID	    ldmp_tran_id;
    i4	    ldmp_block_start;
    i4	    ldmp_block_end;
    LG_LA	    ldmp_la_start;
    LG_LA	    ldmp_la_end;
    LG_DBID	    ldmp_db_id;
    char	    ldmp_t_name [DB_MAXNAME];
    i4	    ldmp_rectype;
    i4	    ldmp_skip_count;
    i4	    ldmp_stop_count;
} LDMP_CB;

/*
** Forward function declarations:
*/

static STATUS log_dump(
    i4		argc,
    char	**argv,
    LDMP_CB	*ldmp);

static DB_STATUS parse_args(
    i4		argc,
    char	**argv,
    i4	first_flag_arg,
    i4	size,
    LDMP_CB	*ldmp);

static DB_STATUS log_addr(
    char	    *arg,
    i4	    size,
    LG_LA	    *la);

static i4  check_table_name(
    LDMP_CB	*ldmp,
    PTR		record);

static VOID logdump_usage(void);


/*{
** Name: main	- Main program.
**
** Description:
**      This routine gets control from the operating system and prepares
**	to execute any of the Journal Support Program utility routines.
**
**	This routine calls log_dump to do the real work.
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
**	12-dec-1988 (greg)
**	    Add ADFLIB to NEELIBS (per Roger's request)
**	05-jan-1989 (mmm)
**	    bug #4405.  We must call CSinitiate(0, 0, 0) from any program 
**	    which will connect to the logging system or locking system.
**      08-aug-1990 (jonb)
**          Add spaces in front of "main(" so mkming won't think this
**          is a main routine on UNIX machines where II_DMF_MERGE is
**          always defined (reintegration of 14-mar-1990 change).
**	22-aug-1990 (bryanp)
**	    Introduced the LDMP_CB to hold parameters passed to this routine.
**	18-dec-1990 (bonobo)
**	    Added the PARTIAL_LD mode, and moved "main(argc, argv)" back to 
**	    the beginning of the line.
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**      19-aug-1991 (jnash) merged 13-may-1991 (bonobo)
**          Added ming hints from ingres63p logdump.
**      26-sep-1991 (jnash) merged 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	26-jul-1993 (jnash)
**	    Added -help.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**/

/*
NEEDLIBS =      DMFLIB ADFLIB SCFLIB GWFLIB  ULFLIB GCFLIB COMPATLIB MALLOCLIB  

MODE =		PARTIAL_LD

OWNER =         INGUSER

PROGRAM =       logdump
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
    STATUS	    status;
    CL_ERR_DESC	    sys_err;
    CL_ERR_DESC	    cl_err;
    LDMP_CB	    ldmp;

# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif

    MEfill(sizeof(LDMP_CB), (u_char)'\0', (PTR)&ldmp);

#ifdef VMS
    status = TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &sys_err);
#else
    status = TRset_file(TR_T_OPEN, "stdio", 5, &sys_err);
#endif

    if (status)
	PCexit(FAIL);

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    /* Set the proper character set for CM */

    status = CMset_charset(&cl_err);
    if ( status != OK)
    {
	TRdisplay("\n Error while processing character set attribute file.\n");
	PCexit(FAIL);
    }

    PCexit(log_dump(argc, argv, &ldmp));
}

/*{
** Name: log_dump	- Dump log file.
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	19-jun-1987 (rogerk)
**	    Took hex constants out of LGadd call.  This won't work in
**	    byte_swapped environments.
**	30-nov-1988 (rogerk)
**	    Force current log buffers in log file so we can read the latest
**	    log records.
**	30-Jan-1989 (ac)
**	    Added arguments to LGbegin().
**	22-aug-1990 (bryanp)
**	    Introduced LDMP_CB to hold arguments.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	16-feb-1992 (rogerk)
**	    Change CS initialization to use scf memory allocation rather than
**	    CS initiate so that semaphore calls in DI will work properly.
**	10-feb-93 (jnash)
**	    Add verbose code, move previously inline TRdisplay of log
**	    address information into dmd_put_la_info().
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format - moved the database and transaction
**		id's out of the lctx_header and into the log record itself.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Pass DM0L_FULL_FILENAME flag to dm0l_allocate when doing a
**		    standalone logdump.
**	24-may-1993 (andys)
**	    Deal with newly initialized logfiles by finding EOF then searching
**	    backwards to find BOF. 	
**	    Clean up a few comments.
**	    Return 0 at end of log_dump.
**	    Disallow standalone logdump if logging system online
**	21-jun-1993 (rogerk)
**	    Added -full logdump flag to direct logdump to scan the entire
**	    logfile rather than just reading those records between the logical
**	    bof and eof (as indicated by the log file header).
**	    Moved call to parse_args routine up so it will be processed before
**	    the dm0l_allocate call (so the -full flag will have been parsed).
**	26-jul-1993 (jnash)
**	    Added -help.  
**	31-jan-1994 (jnash)
**	    CSinitiate() (called by scf_call(SCU_MALLOC)) now returns errors, 
**	    so dump them.
**	10-Apr-2005 (schka24)
**	    Position the log to the position given by -as or -bs, don't
**	    churn through the entire log throwing away records.  (Also
**	    allows logdump when the BOF is garbled, or contains a partial
**	    or invalid record.)
**	15-Aug-2005 (jenjo02)
**	    Call dm0m_init for SINGLEUSER use.
**      25-Apr-2006 (hanal04) Bug 116032
**          logdump with no file name can not be run if the logging
**          system is not on-line. Trap this and advise use of
**          logdump standalone.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      30-Oct-2009 (horda03) Bug 122823
**          Handle multiple file names and indicate to the logging
**          system that the "name" is really a list of LG_PARTITIONs.
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read().
*/
static STATUS
log_dump(
i4	    argc,
char	    **argv,
LDMP_CB	    *ldmp)
{
    CL_ERR_DESC	    sys_err;
    DMP_LCTX	    *lctx;
    LG_DBID	    db_id;
    LG_LXID	    lx_id;
    DB_TRAN_ID	    tran_id;
    LG_LA	    la;
    LG_RECORD	    *lctx_header;
    DM0L_HEADER	    *record;
    DB_STATUS	    status;
    DB_STATUS	    err_code;
    DM0L_ADDDB	    add_info;
    DM0L_HEADER	    *h;
    i4		    l_add_info;
    static DM_SVCB  svcb;
    i4	    lgd_status;
    i4	    size = 64;
    i4		    length;
    LG_LSN	    nlsn;
    DB_OWN_NAME	    user_name;
    SCF_CB	    scf_cb;
    i4	    dm0l_flags;
    i4	    block_no;
    i4	    num_qualifying_records;
    i4	    num_records_printed;
    i4	    first_flag_arg = 1;
    i4		    first_read = 0;	    /* is this the first log record? */
    i4		    count = 0;		    /* count log recs read backwards */
    i4	    local_err_code;	    /* dummy for use in ule_format   */
    i4      partition;
    DB_ERROR	    local_dberr;

    /*
    ** Service -help request.
    */
    if ((argc == 2) && (STcasecmp(&argv[1][0], "-help" ) == 0))
    {
	logdump_usage();
	return(OK);
    }

    if (argc >= 2)
    {
	for( partition = 1;
             (partition < argc) && 
                ((argv[partition][0] != '-') && (argv[partition][0] != '+') && (argv[partition][0] != '#'));
             partition++)
	{
	    /*  Get Log file name. */

	    ldmp->ldmp_partitions [partition-1].lg_part_len = STlength(argv[partition]);

            if (ldmp->ldmp_partitions [partition-1].lg_part_len > MAX_LOC)
            {
               TRdisplay("\n Error maximum length for a filename is %d characters.\n", MAX_LOC);
               TRdisplay("    \"%s\" is %d characters\n", argv[partition],
                         ldmp->ldmp_partitions [partition-1].lg_part_len);

               PCexit(FAIL); 
            }

	    STmove(argv[partition], ' ', sizeof(ldmp->ldmp_partitions [partition-1].lg_part_name),
		    ldmp->ldmp_partitions [partition-1].lg_part_name);
	    ldmp->ldmp_flags |= LDMP_STANDALONE;
        }

	first_flag_arg = partition;

	ldmp->ldmp_no_partitions = partition - 1;
    }

    /*
    ** Initialize CS/SCF to be a single user server.
    ** This is done in a completely straightforward and natural way here
    ** by calling scf to allocate memory.  It turns out that this memory
    ** allocation request will result in SCF initializing SCF and CS
    ** processing (In particular: CSinitiate and CSset_sid).  Before this call,
    ** CS semaphore requests will fail.  The memory allocated here is not used.
    */
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = ((42 + SCU_MPAGESIZE - 1)
	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

    status = scf_call(SCU_MALLOC, &scf_cb);
    if (status != OK)
    {
	char	    	error_buffer[ER_MAX_LEN];
	i4 	error_length;
	i4 	tmp_count;

	uleFormat(NULL, status, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	    &err_code, 0);
	error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &tmp_count, stdout);
        SIwrite(1, "\n", &tmp_count, stdout);
        SIflush(stdout);
	return (status);
    }

    dmf_svcb = &svcb;
    dmf_svcb->svcb_status = SVCB_SINGLEUSER;
    /* Init DMF memory manager */
    dm0m_init();

    if (ldmp->ldmp_flags & LDMP_STANDALONE)
    {
	/* Initialize the Logging system. */
     
	if (LGinitialize(&sys_err, ERx("logdump")))
	{
	    TRdisplay("Can't initialize logging system\n");
	    return(FAIL);
	}
	status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
                        &length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show logging system status.\n");
	    return(status);
	}
	if ((lgd_status & 1) == 0)
	{
	    TRdisplay("Logging system not ONLINE.\n");
	    /* Allocate dynamic memory in logging system. */
 
	    if ((status = LGalter(LG_A_BLKS, (PTR)&size,
				    sizeof(size), &sys_err)) != OK)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		(VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err,
			ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code,
			1, 0, LG_A_BLKS);
 
		return (status);
	    }
 
	    if ((status = LGalter(LG_A_LDBS, (PTR)&size, sizeof(size),
			&sys_err)) != OK)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		(VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err,
			ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code,
				1, 0, LG_A_LDBS);
 
		return (status);
	    }
	}
	else
	{
	    /*
	    ** If you run standalone logdump while the logging system is 
	    ** online, then you call LGopen with the LG_MASTER flag.
	    ** This means that logdump is pretending to be the RCP.
	    ** So, when we exit (LGclose) the logging system thinks the 
	    ** RCP has exited and forces everyone else out. 
	    ** So, don't let standalone logdump run while the logging system
	    ** online.
	    */
	    TRdisplay("Can't run standalone logdump while logging system online.\n"); 
	    return (1);
	}
    }

    /*
    ** Parse the command line flags. This will fill in options in the ldmp cb.
    */
    status = parse_args( argc, argv, first_flag_arg, size, ldmp );
    if (status != E_DB_OK)
	PCexit(FAIL);

    if (ldmp->ldmp_flags & LDMP_STANDALONE)
    {
	/*
	** Pass standalone flags to dm0l_allocate (for LGopen call).
	** Check for -full parameter and pass LGopen flag to scan entire
	** logfile accordingly.
	*/
	dm0l_flags = DM0L_RECOVER | DM0L_FULL_FILENAME;
	if (ldmp->ldmp_flags & LDMP_FULLFILE)
	    dm0l_flags |= DM0L_SCAN_LOGFILE;

	status = dm0l_allocate(dm0l_flags | DM0L_PARTITION_FILENAMES, 0, 0, 
                                    (char *)ldmp->ldmp_partitions,
				    ldmp->ldmp_no_partitions,
				    (DMP_LCTX **)0,
				    &local_dberr);
    }
    else
    {
        if (LGinitialize(&sys_err, ERx("logdump")))
        {
            TRdisplay("Can't initialize logging system\n");
            return(FAIL);
        }
        status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status),
                        &length, &sys_err);
        if (status)
        {
            uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
                        ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
            TRdisplay("Can't show logging system status.\n");
            return(status);
        }
        if ((lgd_status & 1) == 0)
        {
            TRdisplay("Logging system is OFFLINE. Use logdump <filename>\n");
            return (1);
        }
	status = dm0l_allocate(0, 0, 0, 0, 0, (DMP_LCTX **)0, &local_dberr);
    }
    if (status != E_DB_OK)
    {
	if (ldmp->ldmp_flags & LDMP_STANDALONE)
	    TRdisplay("Can't read standalone log.\n");
	else
	    TRdisplay("Can't initialize logging system.\n");
	return (1);
    }
    lctx = dmf_svcb->svcb_lctx_ptr;
    size = lctx->lctx_bksz;

    STmove((PTR)DB_LOGDUMP_INFO, ' ', DB_MAXNAME, (PTR) &add_info.ad_dbname);
    MEcopy((PTR)DB_INGRES_NAME, DB_MAXNAME, (PTR) &add_info.ad_dbowner);
    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
    add_info.ad_dbid = 0;
    add_info.ad_l_root = 4;
    l_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

    status = LGadd(lctx->lctx_lgid, LG_NOTDB, (char *)&add_info, 
		   l_add_info, &db_id, &sys_err);

    if (status)
    {
	TRdisplay("Error Adding the NOTDB.\n");
	return (1);
    }

    /*	Start an archiver transaction so that we can read the log. */

    STmove((PTR)DB_LOGDUMP_INFO, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
    status = LGbegin(LG_NOPROTECT, db_id, &tran_id, &lx_id,
		    sizeof(DB_OWN_NAME), &user_name.db_own_name[0],
		    (DB_DIS_TRAN_ID*)NULL, &sys_err);
    if (status)
    {
	TRdisplay("Error beginning a NONPROTECT transaction.\n");
	return (1);
    }

    if ((ldmp->ldmp_flags & LDMP_STANDALONE) == 0)
    {
	/*
	** If Logging system is active, then flush the log records in the
	** current log buffer so that we will see them in the log dump.
	*/
	status = LGforce(LG_LAST, lx_id, (LG_LSN *)0, &nlsn, &sys_err);
	if (status)
	{
	    TRdisplay("Error Forcing Log file.\n");
	    TRdisplay("Logdump may not display most recently written log records.\n");
	}
    }

    if (ldmp->ldmp_flags & LDMP_LOGADDR_START)
	status = dm0l_position(DM0L_FLGA, lx_id, &ldmp->ldmp_la_start,
			lctx, &local_dberr);
    else if (ldmp->ldmp_flags & LDMP_BLOCKNO_START)
    {
	/* Position will take zeros for sequence and offset, but then
	** LGread gets all upset.  So, use BOF instead, but twiddle the
	** block number part.
	*/
	STRUCT_ASSIGN_MACRO(lctx->lctx_lg_header.lgh_begin, la);
	if (ldmp->ldmp_block_start > lctx->lctx_lg_header.lgh_end.la_block)
	    -- la.la_sequence;
	la.la_block = ldmp->ldmp_block_start;
	status = dm0l_position(DM0L_FLGA, lx_id, &la, lctx, &local_dberr);
    }
    else
	status = dm0l_position(DM0L_FIRST, lx_id, 0, lctx, &local_dberr);

    if (status != E_DB_OK)
    {
	TRdisplay("Can't position log file.\n");
	return (1);
    }

    num_qualifying_records = num_records_printed = 0;

    for (;;)
    {
	status = dm0l_read(
	    lctx, lx_id, (DM0L_HEADER **)&record, &la, (i4*)NULL, &local_dberr);
	if (first_read++ == 0 && status == E_DB_WARN 
			      && local_dberr.err_code == E_DM9238_LOG_READ_EOF)
	{
	    /*
	    ** If the first log record we read got EOF then we probably have
	    ** a newly initialized logfile. In LGopen/scan_bof_eof the BOF
	    ** is set to be just after the EOF with the sequence number 
	    ** decremnted. This doesn't work if the logfile is newly 
	    ** initialized as there will be a jump in LAs. So, we will position
	    ** ourselves at the EOF and read backwards to find BOF and take
	    ** it from there. 
	    */
	    TRdisplay("EOF was detected where we expect BOF to be.\n");
	    TRdisplay("This is probably caused by a newly initialised logfile.\n");
	    TRdisplay("Position to EOF and search backwards for BOF.\n");
	    status = dm0l_position(DM0L_LAST, lx_id, 0, lctx, &local_dberr);
	    if (status != E_DB_OK)
	    {
		TRdisplay("Can't position logfile at EOF.\n");
		return (1);
	    }
	    /*
	    ** Read backwards to find BOF..
	    */
	    for (;;)
	    {
		if (status = dm0l_read(lctx, lctx->lctx_lxid, &record,
					&la, (i4*)NULL, &local_dberr))
		{
		    break;
		}
		if (++count % 100 == 1)
		    TRdisplay("Have read backwards through %d record(s)...\n", 
								    count);
	    }
	    if (local_dberr.err_code == E_DM9238_LOG_READ_EOF && count)
	    {
		status = E_DB_OK;
		TRdisplay("Found EOF reading backwards.. assume this is BOF\n");
		TRdisplay("position to BOF @ <%x,%x,%x>\n", 
				    la.la_sequence,
				    la.la_block,
				    la.la_offset);  
		
		status = dm0l_position(DM0L_FLGA, lx_id, &la,
					lctx, &local_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, 
				&local_err_code, 0);
		    return (1);
		}
		/*
		** Now that BOF and EOF are set to more "reasonable" values,
		** jump back to start the main loop
		*/
		TRdisplay("New BOF and EOF set.. continue logdump.\n");
	    	continue;	
	    }
	    else
	    {
		TRdisplay("Error finding BOF after reading back %d records\n",
								    count);
	    }
	    return (1);

	}
	if (status != E_DB_OK)
    	    break;

	lctx_header = (LG_RECORD *)lctx->lctx_header;
	h = (DM0L_HEADER *)record;

	block_no = la.la_block;
	/*
	** Apply the various log file filters to determine whether this
	** record should be kept or not.
	*/
	if (ldmp->ldmp_flags & LDMP_TXID)
	{
	    if (h->tran_id.db_high_tran != ldmp->ldmp_tran_id.db_high_tran ||
		h->tran_id.db_low_tran  != ldmp->ldmp_tran_id.db_low_tran)
	    {
		continue;
	    }
	}
	if (ldmp->ldmp_flags & LDMP_BLOCKNO_START)
	{
	    if (block_no < ldmp->ldmp_block_start)
		continue;
	}
	if (ldmp->ldmp_flags & LDMP_BLOCKNO_END)
	{
	    if (block_no > ldmp->ldmp_block_end)
		continue;
	}
	if (ldmp->ldmp_flags & LDMP_LOGADDR_START)
	{
	    /*
	    ** If this record's log addr is less than the one you said to
	    ** start with, we'll skip this log record.
	    */
	    if (la.la_sequence < ldmp->ldmp_la_start.la_sequence ||
		(la.la_sequence == ldmp->ldmp_la_start.la_sequence &&
		 la.la_block < ldmp->ldmp_la_start.la_block ) ||
		(la.la_sequence == ldmp->ldmp_la_start.la_sequence &&
		 la.la_block == ldmp->ldmp_la_start.la_block &&
		 la.la_offset < ldmp->ldmp_la_start.la_offset))
	    {
		continue;
	    }
	}
	if (ldmp->ldmp_flags & LDMP_LOGADDR_END)
	{
	    /*
	    ** If this record's log addr is greater than the one you said to
	    ** end with, we'll skip this log record.
	    */
	    if (la.la_sequence > ldmp->ldmp_la_end.la_sequence ||
		(la.la_sequence == ldmp->ldmp_la_end.la_sequence &&
		 la.la_block > ldmp->ldmp_la_end.la_block) ||
		(la.la_sequence == ldmp->ldmp_la_end.la_sequence &&
		 la.la_block == ldmp->ldmp_la_end.la_block &&
		 la.la_offset > ldmp->ldmp_la_end.la_offset))
	    {
		continue;
	    }
	}
	if (ldmp->ldmp_flags & LDMP_DB_ID)
	{
	    if (h->database_id != ldmp->ldmp_db_id)
		continue;
	}
	if (ldmp->ldmp_flags & LDMP_TNAME)
	{
	    /*
	    ** If the record is for some other table, skip it
	    */
	    if (check_table_name(ldmp, (PTR)record) == 0)
		continue;
	}
	if (ldmp->ldmp_flags & LDMP_RECTYPE)
	{
	    /*
	    ** If the log record is not of this type, skip it.
	    */
	    if (record->type != ldmp->ldmp_rectype)
		continue;
	}

	/*
	** At this point, we have a log record which we wish to print.
	*/
	if (ldmp->ldmp_flags & LDMP_SKIP_COUNT)
	{
	    num_qualifying_records++;

	    if (num_qualifying_records <= ldmp->ldmp_skip_count)
		continue;
	}

	/*
	** Dump log record header information.
	*/
  	dmd_format_lg_hdr(dmd_put_line, &la, lctx_header, size);

	/*
	** Dump log record information, verbose if requested.
	*/
	dmd_log((ldmp->ldmp_flags & LDMP_VERBOSE) ? TRUE : FALSE, 
		(PTR)record, size);

	/*
	** Unless we were instructed to stop after printing N records, loop 
	** back around and process the next record.
	*/
	if (ldmp->ldmp_flags & LDMP_STOP_COUNT)
	{
	    num_records_printed++;
	    if (num_records_printed >= ldmp->ldmp_stop_count)
		break;
	}
    }

    status = LGend(lx_id, 0, &sys_err);
    status = LGremove(db_id, &sys_err);

    return (0);
}

/*
** Name: parse_args - Parse the command line arguments
**
** Description:
**	This code structure allows for the eventual use of argument flags
**	beginning with '-', '+', and '#', but the current logdump only uses
**	'-' flags.
**
** History:
**	22-aug-1990 (bryanp)
**	12-feb-93 (jnash)
**	    Reduced logging project.  Add verbose (-v) option.
**	26-apr-1993 (jnash)
**	    LRC mandated change to use -verbose rather than -v.
**	21-jun-1993 (rogerk)
**	    Added -full parameter to allow offline logdump to process the
**	    entire logfile rather than just the records between the logical
**	    BOF and EOF.
*/
static DB_STATUS
parse_args(
i4	    argc,
char	    **argv,
i4	    first_flag_arg,
i4	    size,
LDMP_CB	    *ldmp)
{
    i4	i;
    STATUS	status = OK;
    char	work_buf [30];

    /*
    **	Standard command line is parsed as follows:
    **
    **	argv[0]	    -   Program name.
    **	argv[1]	    -   File Name of LOG to dump. (optional)
    **	argv[2-n]   -	Additional arguments.
    **
    **	This routine need only deal with the additional arguments -- the first
    **	two have already been dealt with.
    */

    /*	Parse the rest of the command line. */

    for (i = first_flag_arg; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'a':
		if (argv[i][2] == 's')
		{
		    status = log_addr(&(argv[i][3]), size,
					&ldmp->ldmp_la_start);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_LOGADDR_START;
		}
		else if (argv[i][2] == 'e')
		{
		    status = log_addr(&(argv[i][3]), size,
					&ldmp->ldmp_la_end);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_LOGADDR_END;
		}
		else
		{
		    TRdisplay("logdump: use -as or -ae, not '%s'\n",
				&argv[i][0]);
		    status = FAIL;
		}
		break;

	    case 'b':
		if (argv[i][2] == 's')
		{
		    status = CVal(&(argv[i][3]), &ldmp->ldmp_block_start);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_BLOCKNO_START;
		}
		else if (argv[i][2] == 'e')
		{
		    status = CVal(&(argv[i][3]), &ldmp->ldmp_block_end);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_BLOCKNO_END;
		}
		else
		{
		    TRdisplay("logdump: use -bs or -be, not '%s'\n",
				&argv[i][0]);
		    status = FAIL;
		}
		break;

	    case 'd':
		status = CVahxl(&(argv[i][2]), (u_i4 *)&ldmp->ldmp_db_id);
		if (status)
		    TRdisplay("Parameter error: '%s'\n", &(argv[i][0]));
		else
		    ldmp->ldmp_flags |= LDMP_DB_ID;
		break;

	    case 'f':
		if (STcompare("full", &argv[i][1]) != 0)
		{
		    TRdisplay("Parameter error: '%s'\n", &(argv[i][0]));
		    status = FAIL;
		}
		else
		{
		    ldmp->ldmp_flags |= LDMP_FULLFILE;
		    if ((ldmp->ldmp_flags & LDMP_STANDALONE) == 0)
		    {
			TRdisplay("Parameter error: -full parameter can only be used in standalone logdump.\n");
			status = FAIL;
		    }
		}
		break;

	    case 'r':
		status = CVal(&(argv[i][2]), &ldmp->ldmp_rectype);
		if (status)
		    TRdisplay("Parameter error: '%s'\n", &(argv[i][0]));
		else
		    ldmp->ldmp_flags |= LDMP_RECTYPE;
		break;
		

	    case 's':
		if (argv[i][2] == 'k')
		{
		    status = CVal(&(argv[i][3]), &ldmp->ldmp_skip_count);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_SKIP_COUNT;
		}
		else if (argv[i][2] == 't')
		{
		    status = CVal(&(argv[i][3]), &ldmp->ldmp_stop_count);
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &argv[i][0]);
		    else
			ldmp->ldmp_flags |= LDMP_STOP_COUNT;
		}
		else
		{
		    TRdisplay("logdump: use -sk or -st, not '%s'\n",
				&argv[i][0]);
		    status = FAIL;
		}
		break;

	    case 't':
		/*  Table name audit. */

		STmove(&argv[i][2], ' ', sizeof(ldmp->ldmp_t_name),
			ldmp->ldmp_t_name);
		ldmp->ldmp_flags |= LDMP_TNAME;
		break;

	    case 'v':
		/*  Verbose option. */

		if (STcompare("verbose", &argv[i][1]) == 0 ||
		    STcompare("v"      , &argv[i][1]) == 0 )
		    ldmp->ldmp_flags |= LDMP_VERBOSE;
		else
		{
		    TRdisplay("logdump: use -verbose, not '%s'\n",
				&argv[i][0]);
		    status = FAIL;
		}
		break;

	    case 'x':
		if (STlength(&(argv[i][2])) != 16)
		{
		    TRdisplay("Enter all 16 characters of the trans. ID\n");
		    status = FAIL;
		}
		else
		{
		    STncpy( work_buf, &(argv[i][2]), 8);
		    work_buf[8] = '\0';
		    status = CVahxl(work_buf, &ldmp->ldmp_tran_id.db_high_tran);
		    if (status == OK)
		    {
			STncpy(work_buf, &(argv[i][10]), 8);
			work_buf[8] = '\0';
			status = CVahxl(work_buf,
				    &ldmp->ldmp_tran_id.db_low_tran);
		    }
		    if (status)
			TRdisplay("Parameter error: '%s'\n", &(argv[i][0]));
		    else
			ldmp->ldmp_flags |= LDMP_TXID;
		}
		break;

	    default:
		status = E_DB_ERROR;
	    }

	}
	else if (argv[i][0] == '+')
	{
	    switch(argv[i][1])
	    {
		
	    default:
		status = E_DB_ERROR;
	    }
	}
	else if (argv[i][0] == '#')
	{
	    switch(argv[i][1])
	    {

	    default:
		status = E_DB_ERROR;
	    }
	}
	else
	{
	    status = E_DB_ERROR;
	}

	/*  Return with any error. */

	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: logdump_usage   - Echo logdump usage format.
**
** Description:
**	 Echo logdump usage format.
**
** History:
**	26-jul-1993 (jnash)
**	    Created.
**	20-sep-1993 (andys)
**	    Slight alteration to help message (logfile_name first).  
*/
static VOID
logdump_usage(void)
{
    TRdisplay(
	"usage: logdump {logfile_name} [-bs<number>] [-be<number>] [-ddb_id] [-full]\n");

    TRdisplay(
	"\t[-as<lga1,lga2,lga3>] [-ae<lga1,lga2,lga3>] [-rrec_type]\n");

    TRdisplay(
	"\t[-ttablename] [-xtx_id] [-sk<number>] [-st<number>] [-verbose]\n");

    return;
}

/*
** Name: log_addr   - parse a log address given in a parameter
**
** Description:
**	This subroutine is used by '-as' and '-ae' to parse a log address.
**
**	Log addresses are expected in their standard format:
**
**	    <number,number,number>
**
**	Where the three numbers are the logfile sequence number, logfile
**	block number, and logpage offset, all in decimal.
**
** History:
**	22-aug-1990 (bryanp)
**	    Created.
*/
static DB_STATUS
log_addr(
char	    *arg,
i4	    size,
LG_LA	    *la)
{
    char    *cur_pos = arg;
    char    *first_comma;
    char    *second_comma;
    char    *end;
    char    work_buf [30];
    i4 offset;

    if (*cur_pos != '<')
    {
	TRdisplay("Expected '<', found '%c'\n", *cur_pos);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }

    cur_pos++;

    first_comma = STchr(cur_pos, ',');
    if (first_comma == 0)
    {
	TRdisplay("First comma was missing from '%s'\n", cur_pos);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }
    STncpy( work_buf, cur_pos, first_comma - cur_pos );
    work_buf[first_comma - cur_pos] = '\0';
    if (CVal(work_buf, (i4 *)&la->la_sequence) != OK)
    {
	TRdisplay("Error converting sequence number '%s' to integer\n",
		    work_buf);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }
    cur_pos = first_comma + 1;

    second_comma = STchr(cur_pos, ',');
    if (second_comma == 0)
    {
	TRdisplay("Second comma was missing from '%s'\n", cur_pos);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }
    STncpy( work_buf, cur_pos, second_comma - cur_pos );
    work_buf[second_comma - cur_pos] = '\0';
    if (CVal(work_buf, (i4 *)&la->la_block) != OK)
    {
	TRdisplay("Error converting block number '%s' to integer.\n",
		    work_buf);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }
    cur_pos = second_comma + 1;

    end = STchr(cur_pos, '>');
    if (end == 0)
    {
	TRdisplay("Terminating '>' missing from '%s'\n", cur_pos);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }
    STncpy( work_buf, cur_pos, end - cur_pos );
    work_buf[end - cur_pos] = '\0';
    if (CVal(work_buf, (i4 *)&la->la_offset) != OK)
    {
	TRdisplay("Error converting offset '%s' to integer.\n",
		    work_buf);
	TRdisplay("Log address format is '<number,number,number>, not '%s'\n",
		    arg);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: check_table_name   - is this log record for this table?
**
** Description:
**	Some log records are not table specific. For those that are, if the
**	log record is for the indicated table, this routine returns non-zero,
**	and if the log record is NOT for the indicated table, this routine
**	returns 0.
**
**	The 'owner_name' code is nascent -- if we ever decide to incorporate
**	a logdump filter by owner name, this routine will be ready for it.
**
** History:
**	23-aug-1990 (bryanp)
**	    Created.
**	16-jul-1991 (bryanp)
**	    B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added support for variable length
**	    table and owner names.  Also removed system catalog log records
**	    (sdel, sput, srep) as they are now collapsed with normal records.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Delete old-index record.
*/
static i4
check_table_name(
LDMP_CB	    *ldmp,
PTR	    record)
{
    char	    *table_name = NULL;
    char	    *owner_name = NULL;
    i4	    table_length = DB_MAXNAME;
    i4	    owner_length = DB_MAXNAME;
    DM0L_HEADER	    *h = (DM0L_HEADER *)record;

    if (ldmp->ldmp_flags & LDMP_TNAME)
    {
	/*  Now check for a specific table & owner. */

	switch (h->type)
	{
	case DM0LPUT:
	    table_length = ((DM0L_PUT *)h)->put_tab_size;
	    owner_length = ((DM0L_PUT *)h)->put_own_size;
	    table_name = &((DM0L_PUT *)h)->put_vbuf[0];
	    owner_name = &((DM0L_PUT *)h)->put_vbuf[table_length];
	    break;

	case DM0LDEL:
	    table_length = ((DM0L_DEL *)h)->del_tab_size;
	    owner_length = ((DM0L_DEL *)h)->del_own_size;
	    table_name = &((DM0L_DEL *)h)->del_vbuf[0];
	    owner_name = &((DM0L_DEL *)h)->del_vbuf[table_length];
	    break;

	case DM0LREP:
	    table_length = ((DM0L_REP *)h)->rep_tab_size;
	    owner_length = ((DM0L_REP *)h)->rep_own_size;
	    table_name = &((DM0L_REP *)h)->rep_vbuf[0];
	    owner_name = &((DM0L_REP *)h)->rep_vbuf[table_length];
	    break;

	case DM0LCREATE:
	    table_name = ((DM0L_CREATE *)h)->duc_name.db_tab_name;
	    owner_name = ((DM0L_CREATE *)h)->duc_owner.db_own_name;
	    break;

	case DM0LDESTROY:
	    table_name = ((DM0L_DESTROY *)h)->dud_name.db_tab_name;
	    owner_name = ((DM0L_DESTROY *)h)->dud_owner.db_own_name;
	    break;

	case DM0LRELOCATE:
	    table_name = ((DM0L_RELOCATE *)h)->dur_name.db_tab_name;
	    owner_name = ((DM0L_RELOCATE *)h)->dur_owner.db_own_name;
	    break;

	case DM0LOLDMODIFY:
	    table_name = ((DM0L_OLD_MODIFY *)h)->dum_name.db_tab_name;
	    owner_name = ((DM0L_OLD_MODIFY *)h)->dum_owner.db_own_name;
	    break;

	case DM0LMODIFY:
	    table_name = ((DM0L_MODIFY *)h)->dum_name.db_tab_name;
	    owner_name = ((DM0L_MODIFY *)h)->dum_owner.db_own_name;
	    break;

	case DM0LINDEX:
	    table_name = ((DM0L_INDEX *)h)->dui_name.db_tab_name;
	    owner_name = ((DM0L_INDEX *)h)->dui_owner.db_own_name;
	    break;

	case DM0LALTER:
	    table_name = ((DM0L_ALTER *)h)->dua_name.db_tab_name;
	    owner_name = ((DM0L_ALTER *)h)->dua_owner.db_own_name;
	    break;

	case DM0LBSF:
	    table_name = ((DM0L_BSF *)h)->bsf_name.db_tab_name;
	    owner_name = ((DM0L_BSF *)h)->bsf_owner.db_own_name;
	    break;

	case DM0LESF:
	    table_name = ((DM0L_ESF *)h)->esf_name.db_tab_name;
	    owner_name = ((DM0L_ESF *)h)->esf_owner.db_own_name;
	    break;
	}

	/*
	** Lastly, check for a specific table name & system catalogs.
	*/

	if (table_name &&
	    (MEcmp(ldmp->ldmp_t_name, table_name, table_length) == 0))
	{
	    return (1);
	}
	else
	{
	    return (0);
	}
    }
    return (0);
}
