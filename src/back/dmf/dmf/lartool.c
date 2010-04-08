/*
**Copyright (c) 2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <pc.h>
#include    <st.h>
#include    <cs.h>
#include    <tr.h>
#include    <lo.h>
#include    <si.h>
#include    <me.h>
#include    <nm.h>
#include    <ep.h>
#include    <er.h>
#include    <cm.h>
#include    <si.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmve.h>
#include    <dmfrcp.h>
 
/*
**	mkming hints
**
MODE =	SETUID PARTIAL_LD

NEEDLIBS = SCFLIB GCFLIB ULFLIB COMPATLIB MALLOCLIB

UNDEFS = scf_call

OWNER =	INGUSER

PROGRAM = lartool
*/

/* Static funtion prototype definitions. */

static STATUS lartool(
    i4	    argc,
    char    *argv[]);

static STATUS get_input(
    i4	*abort,
    i4	*data);

static DB_STATUS term_tran(
    i4	abort,
    i4	data);

/*
** TRrequest won't flush it reads the number of characters
** specified.  If user types in more than OP_ASCII characters
** at first prompt, the left-over will go into next prompt.
*/
# define	OP_ASCII	16	
# define	HEX_ASCII	2 * sizeof(i4) + 1

/**
**  Name: LARTOOL.C - Tool for manipulating transactions in the logging system.
**
**  Description:
**	    This file contains routines to manipulate transactions in the 
**	    logging system.
**
**  History:
**      6-Feb-1989 (ac)
**          Created.
**	12-oct-1989 (walt)
**	    Fix bug 8383 - ule_format calls were missing a parameter.
**	04-nov-1989 (greg)
**	    BUG 8550 -- handle ^D input (to TRrequest) on UNIX.
**	    add MING hints.
**	    Someday we'll need to get the strings out of the program.
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**	13-may-1991 (bonobo)
**	    Added the PARTIAL_LD mode ming hint.
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	25-sep-1991 (mikem) integrated following change:  7-mar-1991 (bryanp)
**	    Bug 34740 -- give a nice(r) msg when can't commit/abort a xact.
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**	    Change read-in character set support to use II_CHARSET symbol.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	4-aug-1992 (bryanp)
**	    Prototyping LG and LK.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Initialize SCF so that CS semaphores work properly.
**	23-aug-1993 (jnash)
**	    Support -help command line option.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so ule_format them.
**      16-oct-1997 (thaju02)
**          Bug #86238 - Unable to commit/abort via lartool when
**          transaction id is large. Modified get_input().
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in lartool() routine.
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Use new form uleFormat.
[@history_template@]...
**/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	lartool_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*{
** Name: main	- Main program
**
** Description:
**      This is the main entry point of the LARTOOL.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    Uses PCexit() to indicate SUCCESS/FAILURE to installation program
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-Feb-1989 (ac)
**          Created.
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**      25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
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
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(lartool(argc, argv));
}

/*{
**  Name: LARTOOL.C - Tool for manipulating transactions in the logging system.
**
**  Description:
**	    Currently, this routine manually commits or aborts a transaction 
**	    in the logging system.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**  History:
**      6-Feb-1989 (ac)
**          Created.
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	26-jul-1993 (bryanp)
**	    Initialize SCF so that CS semaphores work properly.
**	23-aug-1993 (jnash)
**	    Support command line -help.
**	31-jan-1994 (jnash)
**	    CSinitiate() (called by scf_call(SCU_MALLOC)) now returns errors, 
**	    so dump them.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**    	22-Jun-2007 (drivi01)
**          On Vista, this binary should only run in elevated mode.
**          If user attempts to launch this binary without elevated
**          privileges, this change will ensure they get an error
**          explaining the elevation requriement.
*/
static STATUS
lartool(
i4                  argc,
char		    *argv[])
{
    DB_STATUS		status = E_DB_ERROR;
    STATUS              ret_status;
    CL_ERR_DESC		sys_err;
    CL_ERR_DESC         cl_err;
    i4		data;
    i4		abort;
    i4		err_code;
    SCF_CB		scf_cb;

    /* Set the proper character set for CM */

    ret_status = CMset_charset(&cl_err);
    if (ret_status != OK)
    {
	uleFormat(NULL, E_UL0016_CHAR_INIT, (CL_ERR_DESC *)&cl_err, ULE_LOG , 
	    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(FAIL);
    }

    /*
    ** Set interactive input/output file.
    */
 
    if (ret_status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err))
    {
        uleFormat(NULL, ret_status, &sys_err, ULE_LOG , NULL,
                (char *)NULL, 0L, (i4 *)NULL,
                &err_code, 0);
	return (FAIL);
    }

    if (ret_status = TRset_file(TR_I_OPEN, TR_INPUT, TR_L_INPUT, &sys_err))
    {
        uleFormat(NULL, ret_status, &sys_err, ULE_LOG , NULL,
                (char *)NULL, 0L, (i4 *)NULL,
                &err_code, 0);
	return (FAIL);
    }

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    /*
    ** Support command line -help.
    */
    if (argv[1] && STcasecmp(&argv[1][0], "-help" ) == 0)
    {
	TRdisplay("\n");
	TRdisplay("    LARTOOL OPTIONS:\n");
	TRdisplay("\n");
	TRdisplay("\tABORT   - Manually ABORT a transaction.\n");
	TRdisplay("\tCOMMIT  - Manually COMMIT a transaction.\n");
	TRdisplay("\tEXIT    - Exit the program.\n");
	TRdisplay("\n");

	return (OK);
    }


    /* Open II_CONFIG!rcpconfig.log */
    {
	LOCATION		loc;
	char    *path;
	i4	l_path;

	if (NMloc(UTILITY, FILENAME, "lartool.log", &loc))
	{
	    TRdisplay("Error while building path/filename to lartool.log.\n");
	    return (E_DB_ERROR);
	}

	LOtos(&loc, &path);
	l_path = STlength(path);

	if (ret_status = TRset_file(TR_F_OPEN, path, l_path, &sys_err))
	{
            uleFormat(NULL, ret_status, &sys_err, ULE_LOG , NULL,
                	(char *)NULL, 0L, (i4 *)NULL,
                	&err_code, 0);
	    return (FAIL);
	}
    }

    /*
    ** Initialize lartool to be a single user server.
    **
    ** (Tongue-in-cheek explanation follows):
    ** This is done in a completely straightforward and natural way here by
    ** calling scf to allocate memory.  It turns out that this memory
    ** allocation request will result in SCF initializing SCF and CS
    ** processing (In particular: CSinitiate and CSset_sid).  Before this
    ** call, CS semaphore requests will fail.  The memory allocated here is
    ** not used.
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
	char		error_buffer[ER_MAX_LEN];
	i4 	error_length;
	i4 	count;

	uleFormat(NULL, status, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	    &err_code, 0);
	error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);
	if (scf_cb.scf_error.err_code)
	    (VOID) uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &err_code, 0);

	return (status);
    }

    /* Initialize the Logging system. */
 
    if (LGinitialize(&sys_err, ERx("lartool")))
    {
	(VOID) uleFormat(NULL, E_DM9011_BAD_LOG_INIT, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }
    else if (get_input(&abort, &data) == OK)
    {
        TRdisplay("Manually %s the transaction %x \n",
    	          abort ? "abort" : "commit", data);
    	status = term_tran(abort, data);
     }

     TRdisplay("\nExiting LARTOOL\n");
     TRflush();

     return(status);
}

/*{
**  Name: get_input - Get the input commands.
**
**  Description:
**	    Get the input command, abort or commit a transaction in the
**	    logging system.
**
** Inputs:
**	    none
** Outputs:
**	    abort			TRUE if to abort the transaction.
**	    data			The transaction ID to commit or abort.
**
**  History:
**      6-Feb-1989 (ac)
**          Created.
**	4-nov-1989 (greg)
**	    Handle TRrequest()'s problem with ^D on UNIX.
**      16-oct-1997 (thaju02)
**          Bug #86238 - Unable to commit/abort via lartool when
**          transaction id is large. Changed call to CVahxl() to
**          CVuahxl(). Added checking/reporting of CVuahxl() return
**          status.
*/
static STATUS
get_input(
i4		*abort,
i4		*data)
{
    char		buffer[OP_ASCII + 1];
    i4		amount_read;
    CL_ERR_DESC		sys_err;
    char		*prompt;
    STATUS		ret_val;

    for (;;)
    {
	MEfill(OP_ASCII, '\0', buffer);
	(VOID)TRdisplay("Manually ABORT or COMMIT a transaction.\n");
	prompt = "Enter ABORT, COMMIT or EXIT : ";

	if (ret_val = TRrequest(buffer, OP_ASCII, &amount_read,
				&sys_err, prompt))
	{
	    break;
	}

	CVlower(buffer);

	if (buffer[0] == 'e')
	{
	    /* Return FAIL to indicate user EXIT */
	    ret_val = FAIL;
	    break;
	}
	else if ((*abort = (buffer[0] == 'a')) ||
	         (buffer[0] == 'c'))
	{
	    char    hex_buf[HEX_ASCII + 1];

	    do
	    {
		MEfill(HEX_ASCII, '\0', hex_buf);
		TRdisplay("The ID of the transaction is the tx_id displayed \n\
	    under the list of active transactions in the LOGSTAT utility.\n");
		prompt = "Enter the ID of the transaction to ABORT/COMMIT : ";
		
	    } while ( ((ret_val = TRrequest(hex_buf, HEX_ASCII,
					    &amount_read,
				            &sys_err, prompt)) == OK)	&&
			(*hex_buf == 0) );

	    if (ret_val == OK)
	    {
		ret_val = CVuahxl(hex_buf, (u_i4 *)data);
		if (ret_val == CV_SYNTAX)
		    TRdisplay("\nInvalid transaction id specified.\n");
		if (ret_val == CV_OVERFLOW)
		    TRdisplay("\nConversion of transaction id resulted in overflow.\n");
	    }
	    break;
	}
	else	    /* useless clause for "clarity" */
	{
	    continue;
	}
    }

    return(ret_val);
}

/*{
**  Name: term_tran - Terminate a transaction.
**
**  Description:
**	    Terminate a transaction in the logging system.
**
** Inputs:
**	    abort			TRUE if to abort the transaction.
**	    data			The transaction ID to commit or abort.
** Outputs:
**	
**
**  History:
**      6-Feb-1989 (ac)
**          Created.
**	25-sep-1991 (mikem) integrated following change: 7-mar-1991 (bryanp)
**	    Bug 34740 -- give a nice(r) msg when can't commit/abort a xact.
*/
static DB_STATUS
term_tran(
i4	    abort,
i4	    data)
{
    LG_TRANSACTION	tr;
    i4		length;
    CL_ERR_DESC		sys_err;
    i4		err_code;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_stat;

    MEcopy((char *)&data, sizeof(data), (char *)&tr);

    if (LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr), &length, 
	       &sys_err))
    {
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
		    LG_S_ATRANSACTION);
	TRdisplay("Transaction %x not found in the logging system.\n", data);
	status = E_DB_ERROR;
    }
    else if (length == 0)
    {
	TRdisplay("Transaction %x not found in the logging system.\n", data);
	status = E_DB_ERROR;
    }
    else if (abort)
    {
	if ((cl_stat = LGalter(LG_A_ABORT, (PTR)&data,
				sizeof(data), &sys_err)) == OK)
	{
	    TRdisplay("Manual ABORT of transaction %x successful\n", data);
	}
	else if (cl_stat == LG_BADPARAM)
	{
	    TRdisplay("The specified transaction, %x, could not be aborted.\n",
			data);
	    TRdisplay("Perhaps you mis-typed the transaction ID.\n");
	    TRdisplay("If the transaction ID was correctly typed, then the\n");
	    TRdisplay("LARTOOL utility is unable to abort this transaction.\n");
	    TRdisplay(
	    "For more information, please contact Ingres Technical Support.\n");
	    status = E_DB_ERROR;
	}
	else
	{
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
	    	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
		    LG_A_ABORT);
	    TRdisplay("Error altering the status of the transaction %x\n",
			data);
	    status = E_DB_ERROR;
	}
    }
    else if ((cl_stat = LGalter(LG_A_COMMIT, (PTR)&data,
			        sizeof(data), &sys_err)) == OK)
    {
    	TRdisplay("Manual COMMIT the transaction %x successful\n", data);
    }
    else if (cl_stat == LG_BADPARAM)
    {
	TRdisplay("The specified transaction, %x, could not be committed.\n",
			data);
	TRdisplay("Perhaps you mis-typed the transaction ID.\n");
	TRdisplay("If the transaction ID was correctly typed, then the\n");
	TRdisplay("LARTOOL utility is unable to commit this transaction.\n");
	TRdisplay(
	    "For more information, please contact Ingres Technical Support.\n");
	status = E_DB_ERROR;
    }
    else if (cl_stat == LG_SYNCHRONOUS)
    {
	TRdisplay("The transaction %x is owned by a server.\n", data);
TRdisplay("Can't manually commit a transaction that is owned by a server.\n");
TRdisplay("Stop the thread and then manually commit the transaction.\n");

        status = E_DB_ERROR;
    }
    else
    {
    	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
	    	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
		    LG_A_COMMIT);
    	TRdisplay("Error altering the status of the transaction %x\n", data);

        status = E_DB_ERROR;
    }

    return(status);
}
