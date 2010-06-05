/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#if defined(axp_osf)
#include    <pthread_exception.h>
#endif
#include    <ex.h>

/**
**
**  Name: PSQPARSE.C - Functions used to parse a query
**
**  Description:
**      This file contains the functions necessary to parse a query.
**
**          psq_parseqry - Parse a query and return the parsed form.
**	    psq_cbinit	 - Initialize call and session CB before processing.
**	    psq_cbreturn - Reset call and session CB after processing.
**
**
**  History:
**      01-oct-85 (jeff)    
**          written
**	25-may-88 (stec)
**	    Made changes in connection with DB procedures.
**	23-aug-88 (stec)
**	    Initialize qso_handle in QSO_OBIDs in psq_cb.
**	21-apr-89 (neil)
**	    Extracted initialization and cleanup from psq_parseqry to allow it
**	    to be called from other routines as well.
**	12-sep-90 (teresa)
**	    fix faulty pss_retry logic; also the following booleans are now
**	    bitflags: pss_agintree, pss_subintree, pss_txtemit
**	6-nov-90 (andre)
**	    replaced pss_flag with pss_dbp_flags and pss_stmt_flags.
**	11-sep-91 (andre)
**	    added initialization of PSS_SESBLK.pss_indep_objs to psq_cbinit()
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      24-nov-92 (ralph)
**          CREATE SCHEMA:
**          Initialize pss_prvgoval
**	22-dec-92 (rblumer)
**	    initialize pointers for statement-level rule list and pss_tchain2;
**	    clean up after pss_tchain2 just like pss_tchain.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made psf_display() portable. The old psf_display() took a variable
**	    number of variable-sized arguments. It could not be re-written as
**	    a varargs function as this practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function, TRformat,
**	    directly.
**	    The psf_display() function has now been deleted, but for flexibilty
**	    psf_display has been #defined to TRformat to accomodate future
**	    change.
**	    All calls to psf_display() changed to pass additional arguments
**	    required by TRformat:
**	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, ...)
**	    this emulates the behaviour of the old psf_display() except that
**	    the trbuf must be supplied.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	02-feb-94 (vijay)
**	    Cast one more qsf_owner to PTR.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	25-Jun-2008 (kibro01)
**	    Add in SC930 tracing inside ult_always_trace() blocks.
**	26-Aug-2009 (kschendel) b121804
**	    Move adu_valuetomystr def to adf.h for gcc 4.3.
**      15-Feb-2010 (maspa05)
**          output server_class in SC930 output as part of "BEGINS" line
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type adu_valuetomystr param.
**/


FUNC_EXTERN	DB_STATUS   	pslparse();
FUNC_EXTERN	DB_STATUS   	pslsparse();
FUNC_EXTERN	VOID	   	adu_2prvalue();
GLOBALREF       PSF_SERVBLK    *Psf_srvblk;

/*{
** Name: print_qry_buffer	- Output the SC930 tracing for a query
**
** Description:
**	Separate this printing so that the large strings required do not
**	get left on the stack during query parsing, since then the stack
**	can run out, leading to SIGSEGV.
**
** History
**	23-Jul-2009 (kibro01)
**	    Written.
**	11-Aug-2009 (kibro01) b122393
**	    Add in role and group to session begins line.
**      15-feb-2010 (maspa05) SIR 123293
**          Added server_class to output
**
*/
static void
print_qry_buffer(PSQ_CB *psq_cb,
		 PSQ_QDESC *qdesc,
		 PSS_SESBLK *sess_cb)
{
	i4 i;
        void *f = ult_open_tracefile((void*)psq_cb->psq_sessid);
	i2 qtype;
        if (f)
        {
	    char val[DB_MAXSTRING + 80];
	    char val2[DB_MAXSTRING + 1];
	    DB_DATA_VALUE d;
	    d.db_datatype = DB_VCH_TYPE;
	    d.db_length = DB_MAXSTRING;
	    d.db_data = val;
	    if (!(sess_cb->pss_ses_flag & PSS_SESSION_STARTED))
	    {
                char tmp[1000];
	        STprintf(tmp,"(DBID=%d)(%*.*s)(%*.*s)(%*.*s)(SVRCL=%*s)",
			psq_cb->psq_udbid,
                        sizeof(sess_cb->pss_user.db_own_name),
                        sizeof(sess_cb->pss_user.db_own_name),
                        sess_cb->pss_user.db_own_name,
                        sizeof(sess_cb->pss_aplid.db_tab_own.db_own_name),
                        sizeof(sess_cb->pss_aplid.db_tab_own.db_own_name),
                        sess_cb->pss_aplid.db_tab_own.db_own_name,
                        sizeof(sess_cb->pss_group.db_tab_own.db_own_name),
                        sizeof(sess_cb->pss_group.db_tab_own.db_own_name),
                        sess_cb->pss_group.db_tab_own.db_own_name,
			SVR_CLASS_MAXNAME,
			Psf_srvblk->psf_server_class
                        );
		sess_cb->pss_ses_flag |= PSS_SESSION_STARTED;
                ult_print_tracefile(f,SC930_LTYPE_BEGINTRACE,tmp);
	    }
	    if (sess_cb->pss_dbp_flags & PSS_RECREATE)
	    {
		qtype = (psq_cb->psq_qlang == DB_QUEL) ? SC930_LTYPE_REQUEL : SC930_LTYPE_REQUERY;
	    } else
	    {
		qtype = (psq_cb->psq_qlang == DB_QUEL) ? SC930_LTYPE_QUEL : SC930_LTYPE_QUERY;
	    }
	    ult_print_tracefile(f, qtype, qdesc->psq_qrytext);
	    for (i = 0; i < qdesc->psq_dnum; i++)
	    {
		STprintf(val,"%d:%d=%s",qdesc->psq_qrydata[i]->db_datatype,
			i,adu_valuetomystr(val2,
			qdesc->psq_qrydata[i],sess_cb->pss_adfcb));
		ult_print_tracefile(f,SC930_LTYPE_PARM,val);
	    }
            ult_close_tracefile(f);
        }
}

/*
** The only standard way of avoiding inlining is to make a global function
** pointer and call through that, so that's what we're doing here to avoid
** the big strings being on the stack when we head down into the parsing,
** which in turn requires a much larger stack size
*/
void (*print_qry_buffer_ptr)(PSQ_CB*,PSQ_QDESC*,PSS_SESBLK*) = print_qry_buffer;


/*{
** Name: psq_parseqry	- Parse a query and return a data structure
**
**  INTERNAL PSF call format: status = psq_parseqry(&psq_cb, &sess_cb);
**
**  EXTERNAL call format:    status = psq_call(PSQ_PARSEQRY, &psq_cb, &sess_cb);
**
** Description:
**	This function will parse a query in QSF memory.  It will return a
**	data structure (such as a query tree), and a code telling what kind of
**	query was just parsed (e.g. a copy statement).  This code will be
**	called the "query mode".  For qrymod definitions, it will store a
**	modified version of the query text in QSF, for later insertion in the
**	iiqrytext system relation.
**
**	This function will check the syntax of each statement, and will perform
**	some semantic validations.  The syntax checking will be rather simple:
**	each statement will either be right or wrong, and a syntax error will
**	cause the whole go block to be aborted.  Semantic checks will be more
**	complicated; some of them will cause warnings (which will allow the
**	statement to continue), and some will cause errors (causing the
**	statement to be aborted).  It should be noted that the semantic checks
**	will not be the same in the Jupiter version as in previous versions;
**	in the Jupiter version, for instance, the parser won't check for whether
**	the statement is valid inside a multi-query transaction.
**
**	Sometimes it will be found that the definition of a view, permit, or
**	integrity is out of date.  When this happens, this function will return
**	an error status saying so, and an id for finding the query text in the
**	iiqrytext relation, so that the definition can be re-parsed and
**	re-stored.  Accordingly, this function has an option by which one can
**	tell it to get the query text out of the iiqrytext relation instead of
**	QSF.
**
**	When a statement is parsed that creates a new cursor, the parser will
**	assign a cursor id to the cursor, create a cursor control block
**	containing information about the cursor, and return the cursor id inside
**	with the query tree representing the cursor.  This cursor id will be
**	used throughout the session to uniquely identify the cursor.  When a
**	"close cursor" statement is parsed, the parser will deallocate the
**	control block associated with the cursor.
**
**	The parser will automatically apply view, permit, and integrity
**	processing to any data manipulation query it parses.  This will not
**	require a separate call to the parser.
**
**	Multi-statement go blocks are no longer allowed, as they used to be in
**	previous versions.  This parser can handle only one statement at a time.
**
** Inputs:
**      psq_cb
**          .psq_qid                    A unique identifier for getting the
**					query text from QSF.
**	    .psq_iiqrytext		TRUE means to get the query text from
**					the iiqrytext relation, not QSF.
**	    .psq_txtid			Query text id key into iiqrytext
**					relation; used only if above is true.
**	sess_cb				Pointer to the session control block
**
** Outputs:
**      psq_cb
**	    .psq_qlang			The language of the query text.
**          .psq_mode                   The query mode (a code telling what
**					kind of query was just parsed).
**	    .psq_result			QSF id for the data structure produced
**					(query tree, or control block stored as
**					QEP).
**	    .psq_txtid			Query text id key into iiqrytext
**					relation; filled in if some qrymod
**					object needs redefining.
**	    .psq_mnyfmt			Set on a "set money_format" or
**					"set money_prec" statement
**	    .psq_dtefmt			Set on a "set date_format" statement
**	    .psq_decimal		Set on a "set decimal" statement
**          .psq_error                  Standard error block
**		E_PS0000_OK		    Success
**		E_PS0001_USER_ERROR	    Mistake by user
**		E_PS0002_INTERNAL_ERROR	    Internal inconsistency inside PSF
**		E_PS0B01_OUTDATED_VIEW	    View out of date; must re-define
**		E_PS0B02_OUTDATED_PERMIT    Permit out of date; must re-define
**		E_PS0B03_OUTDATED_INTEG	    Integrity out of date; must re-def.
**		E_PS0B04_CANT_GET_TEXT	    Can't get query text
**	    .psq_txtout			QSF id for the create procedure stmt to
**					be stored in the system catalog.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores query tree or control block in QSF.
**	    Can open a cursor.
**
** History:
**	01-oct-85 (jeff)
**          written
**	19-sep-86 (daved)
**	    end of qry should point to last char. This char should be a space.
**	    this makes the scanner's job easier
**	27-jan-87 (daved)
**	    add the printqry set command.
**	02-oct-87 (stec)
**	    Removed pss_journaling flag initialization;
**	    must be initialized in psqbgnses.c
**	19-jan-88 (stec)
**	    Changed initialization od pst_resloc.
**	25-may-88 (stec)
**	    Made changes in connection with DB procedures.
**	    QSF object of QP type has to be destroyed if
**	    translate_or_define worked for CREATE PROCEDURE. 
**	23-aug-88 (stec)
**	    Initialize qso_handle in QSO_OBIDs in psq_cb.
**	21-apr-89 (neil)
**	    Extracted some initialization (psq_cbinit) from psq_parseqry to
**	    allow it to be called from other routines as well.
**	11-dec-89 (ralph)
**	    Change interface to QSO for dbprocs
**	12-sep-90 (teresa)
**	    fix faulty pss_retry logic.
**	15-jun-92 (barbara)
**	    Sybil merge.  Pass in sess control block to pst_clrrng.
**	23-nov-92 (barbara)
**	    For Star, accept range statement as the one and only allowable
**	    QUEL statement.  This is to support old FE's which use the range
**	    statement as a quick way to ascertain table existence.  FEs of
**	    >= 6.5 vintage use a table_resolve() function, so at some point
**	    we can remove the QUEL range table statement support for Star.
**      24-nov-92 (ralph)
**          CREATE SCHEMA:
**          Initialize pss_prvgoval
**	22-dec-92 (rblumer)
**	    clean up after pss_tchain2 just like pss_tchain.
**	25-may-93 (rog)
**	    Move clean-up/exit code into psq_cbreturn() and then call it.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
**	16-mar-94 (andre)
**	    if performing an internal PSF retry and trace point ps129 (same as 
*8	    SET PRINTQRY) is set, instead of redisplaying the query we will 
*8	    tell the user that we are retrying the last query.
**	28-feb-2005 (wanfr01)
**	    Bug 64899, INGSRV87
**	    Add stack overflow handler for TRU64.
**	17-mar-06 (dougi)
**	    Init pss_hintcount to 0 for optimizer hints project.
**	23-june-06 (dougi)
**	    Init pss_stmtno to 1 for procedure debugging.
**	05-sep-06 (toumi01)
**	    Init pss_stmtno to 0 (to match new "help procedure" numbering)
**	    lest we point, PC register like, to the _following_ statement.
**	15-Sep-2008 (kibro01) b120571
**	    Use same session ID as available in iimonitor
**	16-Sep-2008 (kibro01) b120571
**	    Remove compilation error from cast of sessid
**	16-Feb-2009 (kibro01) b121674
**	    Add version to SESSION BEGINS message in sc930 and give out
**	    data type of parameters.
**	10-Jul-2009 (kibro01) b122299
**	    Increase the version number due to dates being printed out now.
**	15-Jul-2009 (kibro01) b122172
**	    Change QUERY or QUEL to REQUERY or REQUEL when a DB procedure is
**	    reparsed due to being invalidated.
**	23-Jul-2009 (kibro01) b122172
**	    Separate the print_qry_buffer logic to avoid stack size problems.
**	3-Aug-2009 (kibro01) b122393
**	    Use print_qry_buffer_ptr to avoid inlining.
**	4-Aug-2009 (kibro01) b122172
**	    Allow REQUERY/REQUEL through even if the RECREATE flag is set so
**	    we get the useful debug output.
**     28-Oct-2009 (maspa05) b122725
**          ult_print_tracefile now uses integer constants instead of string
**          for type parameter - SC930_LTYPE_PARM instead of "PARM" and so on
**          Also moved function definitions for SC930 tracing to ulf.h
**          The functions involved were - ult_always_trace, ult_open_tracefile
**          ult_print_tracefile and ult_close_tracefile
*/
DB_STATUS
psq_parseqry(
	register PSQ_CB     *psq_cb,
	register PSS_SESBLK *sess_cb)
{
    DB_STATUS		    status;
    DB_STATUS		    ret_val;
    QSF_RCB		    qsf_rb;
    i4		    err_code;
    PSQ_QDESC		    *qdesc;
    i4		    val1 = 0;
    i4		    val2 = 0;
    i4			    i;
    char		    trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    if ((status = psq_cbinit(psq_cb, sess_cb)) != E_DB_OK)
	return (status);

    /*
    ** The following is particular to queries that must be parsed.
    ** Get query text from QSF and put it in session control block
    ** Initialize the qbuf, nextchar, prevtok, and bgnstmt pointers
    */

    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;

    qsf_rb.qsf_obj_id.qso_handle = psq_cb->psq_qid;
    status = qsf_call(QSO_INFO, &qsf_rb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0B04_CANT_GET_TEXT, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    qdesc		    = (PSQ_QDESC*) qsf_rb.qsf_root;

    /* print the qry buffer as long as this isn't a retry 
    ** - although allow through the RECREATE case since it's useful
    ** for debugging to log reparsing an object */
    if (ult_always_trace() && 
	( ((sess_cb->pss_retry & PSS_REFRESH_CACHE)==0) ||
	  (sess_cb->pss_dbp_flags & PSS_RECREATE) != 0 ) )
    {
	(*print_qry_buffer_ptr)(psq_cb, qdesc, sess_cb);
    }
    if (ult_check_macro(&sess_cb->pss_trace, 1, &val1, &val2))
    {
	if (psf_in_retry(sess_cb, psq_cb))
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\n...retrying last query...\n");
	}
	else
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\nQUERY BUFFER:\n");
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "%.#s\n", qdesc->psq_qrysize, qdesc->psq_qrytext);
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\nQUERY PARAMETERS:\n");
	    for (i = 0; i < qdesc->psq_dnum; i++)
	    {
	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		  "Parameter : %d\n", i);
	        adu_2prvalue(psf_relay, qdesc->psq_qrydata[i]);
	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, "\n");
	    }
	}
    }

    sess_cb->pss_bgnstmt    = (u_char*) qdesc->psq_qrytext;
    sess_cb->pss_prvgoval   = (u_char*) NULL;
    sess_cb->pss_prvtok	    = (u_char*) qdesc->psq_qrytext;
    sess_cb->pss_qbuf	    = (u_char*) qdesc->psq_qrytext;
    sess_cb->pss_nxtchar    = (u_char*) qdesc->psq_qrytext;
    sess_cb->pss_endbuf	    = sess_cb->pss_qbuf + qdesc->psq_qrysize - 1;
    sess_cb->pss_dmax	    = qdesc->psq_dnum;
    sess_cb->pss_qrydata    = qdesc->psq_qrydata;
    *sess_cb->pss_endbuf    = ' ';
    sess_cb->pss_lineno	    = 1;	/* Start out at line one */
    sess_cb->pss_stmtno	    = 0;	/* and statement at zero */
    sess_cb->pss_dval	    = 0;
    sess_cb->pss_hintcount  = 0;

    psl_yinit(sess_cb);
    if (psq_cb->psq_qlang == DB_QUEL)
    {
	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    char	*c;
	    char	*r = "range";

	    /* skip leading white space chars, if any */
	    for (c = qdesc->psq_qrytext;
		 c <= (char *) sess_cb->pss_endbuf && CMwhite(c);
		 c = CMnext(c)
		)
	    ;

	    /* compare the first word with "range" */
	    for (;
		 *r != EOS && c <= (char *) sess_cb->pss_endbuf &&
		 !CMcmpnocase(c,r);
		 c = CMnext(c), r = CMnext(r)
		)	
	    ;

	    /*
	    ** we will go on to parse this statement iff
	    ** 1) first non-white chars are "range"     AND
	    ** 2) 'e' is followed by a white space
	    */
	    if (*r != EOS || c >= (char *) sess_cb->pss_endbuf || !CMwhite(c))
	    {
		(VOID) psf_error(5212L, 0L, PSF_USERERR, &err_code,
				    &psq_cb->psq_error,0);
		return(E_DB_ERROR);
	    }
	}
	sess_cb->pss_parser = pslparse;
    }
    else
    {
	sess_cb->pss_parser = pslsparse;
    }

    IIEXtry
    {
         status = (*sess_cb->pss_parser)(sess_cb, psq_cb);
    }
    IIEXcatch(pthread_stackovf_e)
    {
	(VOID) psf_error(5212L, 0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error,0);
    }
    IIEXendtry
    
    ret_val = psq_cbreturn(psq_cb, sess_cb, status);

    return (ret_val);
}

/*{
** Name: psq_cbinit	- Initialize call and session CB before query.
**
** Description:
**	This routine is a common routine that sets up the call (psq_cb) and
**	session CB (sess_cb) before processing a query.  Since there are
**	other entry points into the parser for query building (ie, PSQ_RECREATE
**	and PSQ_DELCURS) this common processing was extracted into a single
**	routine.
**
** Inputs:
**      psq_cb
**          .psq_qid                    NULL (if no text) or a unique identifier
**					for getting the query text from QSF.
**	sess_cb				Pointer to the session control block
**
** Outputs:
**      psq_cb
**	sess_cb				Both CB's have all initial query
**					processing filed cleared.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	21-apr-89 (neil)
**	    Extracted initialization from psq_parseqry to allow it to be called
**	    from other routines as well.
**	11-sep-91 (andre)
**	    initialize members of pss_indep_objs to NULL
**	19-feb-92 (andre)
**	    members of pss_indep_objs will not be initialized if we are about to
**	    reparse a dbproc definition to determine if it is grantable (as a
**	    part of processing GRANT ON PROCEDURE) or active (as a part of
**	    processing [re]CREATE PROCEDURE)
**	22-dec-92 (rblumer)
**	    initialize pointers for statement-level rule list and pss_tchain2.
**	08-apr-93 (andre)
**	    names of rule list headers in sess_cb have changed (and their
**	    number has doubled)
**	17-nov-93 (andre)
**	    Added initialization for pss_flattening_flags
**      04-08-13 (wanfr01)
**        Bug 112828, INGSRV2799
**        Added initialization for targparm
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**      8-Mar-2008 (kschendel)
**          Re-init the aux range table, don't just reset the max used.
**          If this isn't done, range entries are used in different orders
**          depending on what the last query did, which can cause view
**          processing to proceed in a different order from query to query.
**          (since view processing goes thru the table in position order, not
**          varno order.)  This isn't bad, necessarily, but it causes
**          query processing to appear to change from one iteration to the
**          next and makes it hard to isolate problems.
**       5-Oct-2008 (hanal04) Bug 120702
**        Added initialization for sess_cb->pss_audit.
**	04-may-2010 (miket) SIR 122403
**	    Init new sess_cb->pss_stmt_flags2.
*/
DB_STATUS
psq_cbinit(
	PSQ_CB     *psq_cb,
	PSS_SESBLK *sess_cb)
{
    DB_STATUS	status = E_DB_OK;

    psq_cb->psq_error.err_code= E_PS0000_OK;		/* No errors */
    psq_cb->psq_mode = 0;				/* No query mode */
    sess_cb->pss_stmt_flags = sess_cb->pss_stmt_flags2 = 0L;
    sess_cb->pss_audit = NULL;
    sess_cb->pss_flattening_flags = 0;
    sess_cb->pss_symblk = sess_cb->pss_symtab;		/* No scanner symtab */
    sess_cb->pss_symnext = sess_cb->pss_symtab->pss_symdata;
    sess_cb->pss_qualdepth = 0;				/* No quals */
    sess_cb->pss_rsdmno = 0;				/* No result domains */
    sess_cb->pss_tlist = NULL;				/* No target list */
    sess_cb->pss_defqry = 0;				/* No repeat queries */
    sess_cb->pss_highparm = -1;				/* No params */
    sess_cb->pss_targparm = -1;                         /* No params */
    sess_cb->pss_ostream.psf_mstream.qso_handle = NULL;	/* No output streams */
    sess_cb->pss_tstream.psf_mstream.qso_handle = NULL;
    sess_cb->pss_cbstream.psf_mstream.qso_handle = NULL;
    sess_cb->pss_cstream = NULL;			/* No open-csr stream */
    sess_cb->pss_crsr = NULL;				/* No cursors */
    sess_cb->pss_resrng = NULL;				/* No range table */
    sess_cb->pss_usrrange.pss_maxrng = -1;		/* No range vars used */
    /* Fully reinit the SQL range table each time for repeatability */
    (void) pst_rginit(&sess_cb->pss_auxrng);
    sess_cb->pss_tchain  = NULL;
    sess_cb->pss_tchain2 = NULL;
    sess_cb->pss_funarg_stream = NULL;

    /* no rule tree yet */
    sess_cb->pss_row_lvl_usr_rules  =
    sess_cb->pss_row_lvl_sys_rules  =
    sess_cb->pss_stmt_lvl_usr_rules =
    sess_cb->pss_stmt_lvl_sys_rules =
    sess_cb->pss_row_lvl_usr_before_rules  =
    sess_cb->pss_row_lvl_sys_before_rules  =
    sess_cb->pss_stmt_lvl_usr_before_rules =
    sess_cb->pss_stmt_lvl_sys_before_rules = (PST_STATEMENT *) NULL;

    /*
    ** initialize the descriptor of independent objects and privileges; the 
    ** exceptions are the cases when we are reparsing the text of a dbproc to
    ** determine if it is grantable or active, in which case we want to keep
    ** around the lists of all objects permissions on which have been checked
    */
    if (!(sess_cb->pss_dbp_flags & (PSS_0DBPGRANT_CHECK | PSS_CHECK_IF_ACTIVE)))
    {
	sess_cb->pss_indep_objs.psq_objs = (PSQ_OBJ *) NULL;
	sess_cb->pss_indep_objs.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	sess_cb->pss_indep_objs.psq_colprivs = (PSQ_COLPRIV *) NULL;
    }

    /* Initialize the result table contents */
    MEfill(sizeof (PST_RESTAB), (u_char) 0, (PTR) &sess_cb->pss_restab);
    MEfill(sizeof (DB_TAB_NAME), (u_char) ' ',
	(PTR) &sess_cb->pss_restab.pst_resname);
    MEfill(sizeof (DB_OWN_NAME), (u_char) ' ',
	(PTR) &sess_cb->pss_restab.pst_resown);
    MEfill(sizeof (DM_DATA), (u_char) 0,
	(PTR) &sess_cb->pss_restab.pst_resloc);
    sess_cb->pss_restab.pst_resvno = -1;

    psq_cb->psq_result.qso_handle = NULL;		/* No results yet */
    psq_cb->psq_txtout.qso_handle = NULL;

    sess_cb->pss_lang = psq_cb->psq_qlang;		/* Query language */

    return (status);
} /* psq_cbinit */

/*{
** Name: psq_cbreturn	- Clear call and session CB after processing.
**
** Description:
**	This routine is a common routine that clears up the call (psq_cb) and
**	session CB (sess_cb) after processing a query.  The routine
**	psq_parseqry still does its own work as it handles various errors
**	associated with textual queries.
**
** Inputs:
**      psq_cb				Pointer to call CB.
**	sess_cb				Pointer to the session control block
**
** Outputs:
**      psq_cb			
**		.psq_result		Result query tree.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	21-apr-89 (neil)
**	    Extracted this from psq_parseqry to allow it to be called from
**	    other routines as well.
**	15-jun-92 (barbara)
**	    Sybil merge.  Pass in sess control block to pst_clrrng.
**	25-may-1993 (rog)
**	    Moved clean-up/exit code here from psq_parseqry() above, and added
**	    status argument so that we know whether to execute the good exit
**	    code or the bad exit code.
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
**	27-aug-93 (andre)
**		(part of fix for bug 54348)
**	    moved code responsible for destroying a dbproc QEP into a separate 
**	    function (psq_destr_dbp_qep()) which will be called from 
**	    psq_cbreturn() and from psq_recreate() if an error occurs AFTER the
**	    dbproc QEP QSF object has been created
**	16-mar-94 (andre)
**	    use psf_retry() to determine whether we are going to retry parsing 
**	    this query and, therefore, whether we should destroy any QSF objects
**	    created during the just completed attempt 
**	28-jan-2004 (schka24)
**	    Close partition def memory if open.
**	15-Mar-2006 (kschendel)
**	    Close function-arg stream if open.
**	28-nov-2007 (dougi)
**	    Different logic for PSQ_REPDYN (for cached dynamic queries).
*/
DB_STATUS
psq_cbreturn(
	PSQ_CB     *psq_cb,
	PSS_SESBLK *sess_cb,
	DB_STATUS   ret_val)
{
    DB_STATUS		status;
    i4		err_code;
    QSF_RCB		qsf_rb;

    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;

    /* Tasks that are common to both a normal return and an error return. */

    do	/* Something to break out of */
    {
	/*
	** Clear out the user's range table.
	*/
	status = pst_clrrng(sess_cb, &sess_cb->pss_usrrange,
			    &psq_cb->psq_error);
	if (status == E_DB_FATAL)
	    ret_val = status;

	/*
	** Clear out the auxliary range table.
	*/
	status = pst_clrrng(sess_cb, &sess_cb->pss_auxrng, &psq_cb->psq_error);
	if (status == E_DB_FATAL)
	    ret_val = status;

	/*
	** If the statement has emitted query text, close the stream.
	*/
	if (sess_cb->pss_tchain != (PTR) NULL)
	{
	    status = psq_tclose(sess_cb->pss_tchain, &psq_cb->psq_error);
	    if (status == E_DB_FATAL)
		ret_val = status;

	    sess_cb->pss_tchain = (PTR) NULL;
	}

	if (sess_cb->pss_tchain2 != (PTR) NULL)
	{
	    status = psq_tclose(sess_cb->pss_tchain2, &psq_cb->psq_error);
	    if (status == E_DB_FATAL)
		ret_val = status;

	    sess_cb->pss_tchain2 = (PTR) NULL;
	}

	/* Close partition definition working memory stream if in use */
	if (sess_cb->pss_ses_flag & PSS_PARTDEF_STREAM_OPEN)
	{
	    status = ulm_closestream(&sess_cb->pss_partdef_stream);
	    /* Toss any error */
	    sess_cb->pss_ses_flag &= ~PSS_PARTDEF_STREAM_OPEN;
	}

	/* Close nested-function-call arg list stack if it was needed */
	if (sess_cb->pss_funarg_stream != NULL)
	{
	    ULM_RCB ulm;

	    ulm.ulm_facility = DB_PSF_ID;
	    ulm.ulm_poolid = Psf_srvblk->psf_poolid;
	    ulm.ulm_streamid_p = &sess_cb->pss_funarg_stream;
	    ulm.ulm_memleft = &sess_cb->pss_memleft;
	    status = ulm_closestream(&ulm);
	    /* Ignore error */
	    sess_cb->pss_funarg_stream = NULL;
	}

    } while (0);

    for (; ret_val == E_DB_OK; )	/* Something to break out of */
    {
	/*
	** This is the path we take for a successful exit.
	** If we have a problem here, we break out of this loop and
	** fall through to the failure exit code.
	*/

	/*
	** Set the QSF id of the return object and unlock it, if any.
	*/
	if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    qsf_rb.qsf_obj_id.qso_handle =
		sess_cb->pss_ostream.psf_mstream.qso_handle;
	    qsf_rb.qsf_lk_id = sess_cb->pss_ostream.psf_mlock;

	    if (psq_cb->psq_mode == PSQ_REPDYN)
	    {
		/* If cached dynamic qp already exists, destroy parse
		** tree object. */
		if (ret_val = qsf_call(QSO_DESTROY, &qsf_rb))
		{
		    (VOID) psf_error(E_PS0A09_CANTDESTROY,
			qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code,
			&psq_cb->psq_error, 0);

		    /* break out to the failure code. */
		    break;
		}
	    }
	    else
	    {
		/* If not cached dynamic, copy object ID and unlock
		** parse tree. */
		STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream.psf_mstream,
				psq_cb->psq_result);
		if (ret_val = qsf_call(QSO_UNLOCK, &qsf_rb))
		{
		    (VOID) psf_error(E_PS0B05_CANT_UNLOCK,
			qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code,
			&psq_cb->psq_error, 0);

		    /* break out to the failure code. */
		    break;
		}
	    }
	    sess_cb->pss_ostream.psf_mlock = 0;
	}

	/*
	** Unlock the text stream if it was used.
	** this is unlocked after the text stream is created.
	** (Don't know why this was taken out.)
	*/
#ifdef NO
	if (sess_cb->pss_tstream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    qsf_rb.qsf_obj_id.qso_handle =
		sess_cb->pss_tstream.psf_mstream.qso_handle;
	    qsf_rb.qsf_lk_id = sess_cb->pss_tstream.psf_mlock;
	    if (ret_val = qsf_call(QSO_UNLOCK, &qsf_rb))
	    {
		(VOID) psf_error(E_PS0B05_CANT_UNLOCK,
				 qsf_rb.qsf_error.err_code, PSF_INTERR,
				 &err_code, &psq_cb->psq_error, 0);

		/* break out to the failure code. */
		break;
	    }
	    sess_cb->pss_tstream.psf_mlock = 0;
	}
#endif
	/* Unlock the control block stream if it was used */
	if (sess_cb->pss_cbstream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    qsf_rb.qsf_obj_id.qso_handle =
		sess_cb->pss_cbstream.psf_mstream.qso_handle;
	    qsf_rb.qsf_lk_id = sess_cb->pss_cbstream.psf_mlock;
	    if (ret_val = qsf_call(QSO_UNLOCK, &qsf_rb))
	    {
		(VOID) psf_error(E_PS0B05_CANT_UNLOCK,
				 qsf_rb.qsf_error.err_code, PSF_INTERR,
				 &err_code, &psq_cb->psq_error, 0);

		/* break out to the failure code. */
		break;
	    }
	    sess_cb->pss_cbstream.psf_mlock = 0;
	}

	/*
	** We need to deallocate memory if the same query is to be tried
	** again. This is supposed to fix The 'drop table, nonexistent_table'
	** memory leak problem.
	*/
	if (psf_retry(sess_cb, psq_cb, ret_val))
	{
	    if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_ostream, &psq_cb->psq_error);
		sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
	    }

	    if (sess_cb->pss_tstream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_tstream, &psq_cb->psq_error);
		sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
	    }

	    if (sess_cb->pss_cbstream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_cbstream, &psq_cb->psq_error);
		sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;
	    }
	}

	/* end with no output streams */
	sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
	sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
	sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;

	if (ret_val == E_DB_OK)
	{
	    /*
	    ** this IF statement is here to keep acc from complaining - we would
	    ** never reach this point unless ret_val was E_DB_OK
	    */
	    return (ret_val);
	}
    }

    /* This is the failure exit code. */

    /*
    ** On an error, the yacc error handling function will call psf_error.
    ** All that's necessary here is to return a status indicating that
    ** an error has occurred.  If an output object has been allocated,
    ** and the error caused the statement to fail, deallocate the object
    ** before returning.  Same for miscellaneous objects, like control
    ** blocks and query text.
    */
    if (ret_val == E_DB_ERROR || ret_val == E_DB_FATAL ||
	ret_val == E_DB_SEVERE)
    {
	/* 
	** In case of CREATE PROCEDURE statement we also need to 
	** destroy the QEP object in QSF if it was created.
	*/
	if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL &&
	    psq_cb->psq_mode == PSQ_CREDBP
	   )
	{
	    DB_STATUS	stat;

	    stat = 
		psq_destr_dbp_qep(sess_cb, 
		    sess_cb->pss_ostream.psf_mstream.qso_handle, 
		    &psq_cb->psq_error);
	    if (stat > ret_val)
		ret_val = stat;
        }

	if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    (VOID) psf_mclose(sess_cb, &sess_cb->pss_ostream, &psq_cb->psq_error);
	    sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
	}

	if (sess_cb->pss_tstream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    (VOID) psf_mclose(sess_cb, &sess_cb->pss_tstream, &psq_cb->psq_error);
	    sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
	}

	if (sess_cb->pss_cbstream.psf_mstream.qso_handle != (PTR) NULL)
	{
	    (VOID) psf_mclose(sess_cb, &sess_cb->pss_cbstream, &psq_cb->psq_error);
	    sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;
	}
    }

    /*
    ** If the statement was a "define cursor" or a "define repeat cursor",
    ** we have to deallocate the cursor control block, if it was allocated.
    */
    if (sess_cb->pss_cstream != (PTR) NULL)
    {
	status = psq_delcursor(sess_cb->pss_crsr, &sess_cb->pss_curstab,
	    &sess_cb->pss_memleft, &psq_cb->psq_error);
	if (status == E_DB_FATAL)
	    ret_val = status;
    }

    return(ret_val);
}

/*
** Name: psq_destr_dbp_qep - destroy QSF object created for a dbproc
**
** Description:
**	If an error occurs in PSF in the course of parsing a dbproc definition
**	or recreating a dbproc QEP, we need to destroy the QEP object created 
** 	in PSF before exiting.  Otherwise, subsequent attempt to parse or 
**	recreate the dbproc will result in SC0206 because the QP object created
**	during the first (aborted) attempt will still be locked in QSF
**
** Input:
**	handle			QP object handle
**
** Output:
**	err_blk			filled in if an error occurs
**
** Side effects:
**
** History:
**	27-aug-93 (andre)
**	    extracted from psq_cbreturn() to enable psq_recreate() call it as 
**	    well
*/
DB_STATUS
psq_destr_dbp_qep(
		PSS_SESBLK	*sess_cb,
		PTR		handle,
		DB_ERROR	*err_blk)
{
    DB_STATUS	    status;
    PST_PROCEDURE   *pnode;
    QSF_RCB	    qsf_rb;
    i4	    err_code;

    /*
    ** first get the root of the query tree object - using it, we will 
    ** derive dbp QP object alias
    */
    
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;

    qsf_rb.qsf_obj_id.qso_handle = handle;

    status = qsf_call(QSO_INFO, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
        (VOID) psf_error(E_PS0A0A_CANTGETINFO, qsf_rb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return(status);
    }

    /* 
    ** We are interested in the proc node, because it holds the identifier for 
    ** the QP object we want to destroy.
    */
    pnode = (PST_PROCEDURE *) qsf_rb.qsf_root;

    /* 
    ** We can only proceed if root has been set - otherwise it is safe to 
    ** assume that the QP object has not been created - so we are done 
    */
    if (pnode == (PST_PROCEDURE *) NULL)
	return(E_DB_OK);

    qsf_rb.qsf_obj_id.qso_type = QSO_QP_OBJ;
    qsf_rb.qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID);
    (VOID) MEcopy((PTR) &pnode->pst_dbpid, sizeof(DB_CURSOR_ID),
        (PTR) qsf_rb.qsf_obj_id.qso_name);

    /* Lock it in shared mode before destroying. */
    qsf_rb.qsf_lk_state = QSO_SHLOCK;

    /* Get handle for the QP object to be destroyed */
    status = qsf_call(QSO_GETHANDLE, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
        /* 
	** looks like the error occurred before the dbproc QP object was 
	** created - we are done
	*/
        if (qsf_rb.qsf_error.err_code == E_QS0019_UNKNOWN_OBJ)
	    return(E_DB_OK);

        (VOID) psf_error(E_PS0A0B_CANTGETHNDLE, qsf_rb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return(status);
    }

    /* Now it is ready to be destroyed */
    status = qsf_call(QSO_DESTROY, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
        (VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return(status);
    }

    return(E_DB_OK);
}
