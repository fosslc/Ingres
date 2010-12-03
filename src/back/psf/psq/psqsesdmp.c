/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tr.h>
#include    <sl.h>
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

/**
**
**  Name: PSQSESDMP.C - Functions used to dump the session control block for a
**		    given session.
**
**  Description:
**      This file contains the functions necessary to dump the session control
**	block given a session id.
**
**          psq_sesdump - Dump the session control block for a given session
**	    psq_rngdmp  - Dump a user range variable
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**	18-mar-87 (stec)
**	    Added pss_resjour handling.
**	13-sep-90 (teresa)
**	    rewrote psq_sesdump to dump each flag in new field pss_flag and
**	    pss_ses_flag.
**	6-nov-90 (andre)
**	    replaced pss_flag with pss_dbp_flags and pss_stmt_flags
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	17-dec-93 (rblumer)
**	    removed all FIPS_MODE flag references.
**	10-oct-93 (tad)
**	    Bug #56449
**	    Replaced %x with %p for display of pointer values.
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBJOURNALING
**      14-jan-2004 (stial01)
**          set [no]blobjournaling only when table specified.
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added dump of
**	    session flag for SET CARDINALITY_CHECK
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_sesdump(
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);
void psq_rngdmp(
	PSS_RNGTAB *rv);

/*{
** Name: psq_sesdump	- Dump the session control block for a given session.
**
**  INTERNAL PSF call format: status = psq_sesdump((PSQ_CB *) NULL, &sess_cb);
**
**  EXTERNAL call format:
**		    status = psq_call(PSQ_SESDUMP, (PSQ_CB *) NULL, &sess_cb);
**
** Description:
**      The psq_sesdump function will format and print the session control 
**      block for a given session.  The output will go to the output terminal
**	and/or file named by the user in the "SET TRACE TERMINAL" and "SET TRACE
**	OUTPUT" commands.
**
** Inputs:
**	psq_cb				Ignored
**	sess_cb				Pointer to session control block to dump
**
** Outputs:
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Writes to output file and/or terminal as specified in the "set trace
**	    output" and "set trace terminal" commands.
**
** History:
**	02-oct-85 (jeff)
**          written
**      26-aug-85 (seputis)
**          removed reference to pss_yastr
**	13-sep-90 (teresa)
**	    rewrote psq_sesdump to dump each flag in new field pss_flag and
**	    pss_ses_flag.
**	28-mar-91 (andre)
**	    PSS_RGSET and PSS_RASET have been undefined.
**	18-nov-93 (andre)
**	    added code to display new bits in pss_stmt_flags, 
**	    pss_flattening_flags and pss_dbp_flags
**	17-dec-93 (rblumer)
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).  So I removed all FIPS_MODE flags.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation.
*/
DB_STATUS
psq_sesdump(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status;
    PSS_RNGTAB		*rv;

    TRdisplay("PSF session control block:\n");

    if ((status = psq_headdmp((PSQ_CBHEAD *) sess_cb)) != E_DB_OK)
	return (status);

    TRdisplay("\tpss_sessid:\t0x%x\n", sess_cb->pss_sessid);
    /* TRdisplay("\tpss_yastr:\t0x%x\n", sess_cb->pss_yastr); */
    TRdisplay("\tpss_numcursors:\t%d\n", sess_cb->pss_numcursors);

    TRdisplay("\tpss_lang:\t");
    if ((status = psq_lngdmp(sess_cb->pss_lang)) != E_DB_OK)
	return (status);
    TRdisplay("\n");

    TRdisplay("\tpss_decimal:\t%c\n", sess_cb->pss_decimal);

    TRdisplay("\tpss_distrib:\t");
    if ((status = psq_dstdmp(sess_cb->pss_distrib)) != E_DB_OK)
	return (status);

    /*
    ** dump bitflags in pss_ses_flag, which tend to persist throughout the
    ** session, though the values may be altered in response to user cmds 
    */
    TRdisplay("\n\tpss_ses_flag.PSS_CATUPD:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_CATUPD);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_ses_flag.PSS_WARNINGS:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_WARNINGS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_ses_flag.PSS_PROJECT:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_PROJECT);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_ses_flag.PSS_JOURNALING:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_JOURNALING);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_ses_flag.PSS_DBA_DROP_ALL:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_DBA_DROP_ALL);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_ses_flag.PSS_NOCHK_SINGLETON_CARD:\t");
    status = psq_booldmp(sess_cb->pss_ses_flag & PSS_NOCHK_SINGLETON_CARD);
    if (status != E_DB_OK)
	return (status);

    /* dump flags found in pss_stmt_flags and pss_dbp_flags */
    TRdisplay("\n\tpss_stmt_flags.PSS_AGINTREE:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_AGINTREE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_SUBINTREE:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_SUBINTREE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_TXTEMIT:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_TXTEMIT);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_QUAL_IN_PERMS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_QUAL_IN_PERMS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_QUEL_RPTQRY_TEXT:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_QUEL_RPTQRY_TEXT);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_DISREGARD_GROUP_ROLE_PERMS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & 
			     PSS_DISREGARD_GROUP_ROLE_PERMS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_SET_LOCKMODE_SESS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_SET_LOCKMODE_SESS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_REG_AS_NATIVE:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_REG_AS_NATIVE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_CP_DUMMY_COL:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_CP_DUMMY_COL);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_GATEWAY_SESS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_GATEWAY_SESS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_NEW_OBJ_NAME:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_NEW_OBJ_NAME);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_PARSING_PRIVS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_PARSING_PRIVS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_CALL_ADF_EXCEPTION:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_CALL_ADF_EXCEPTION);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_PARSING_CHECK_CONS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_PARSING_CHECK_CONS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_VALIDATING_CHECK_OPTION:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_VALIDATING_CHECK_OPTION);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_TXTEMIT2:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_TXTEMIT2);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_IMPL_COL_LIST_IN_DECL_CURS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & 
			     PSS_IMPL_COL_LIST_IN_DECL_CURS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_stmt_flags.PSS_RESOLVING_CHECK_CONS:\t");
    status = psq_booldmp(sess_cb->pss_stmt_flags & PSS_RESOLVING_CHECK_CONS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_flattening_flags.PSS_SINGLETON_SUBSELECT:\t");
    status = psq_booldmp(sess_cb->pss_flattening_flags & 
			     PSS_SINGLETON_SUBSELECT);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_flattening_flags.PSS_SUBSEL_IN_OR_TREE:\t");
    status = psq_booldmp(sess_cb->pss_flattening_flags & PSS_SUBSEL_IN_OR_TREE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_flattening_flags.PSS_ALL_IN_TREE:\t");
    status = psq_booldmp(sess_cb->pss_flattening_flags & PSS_ALL_IN_TREE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_flattening_flags.PSS_MULT_CORR_ATTRS:\t");
    status = psq_booldmp(sess_cb->pss_flattening_flags & PSS_MULT_CORR_ATTRS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_flattening_flags.PSS_CORR_AGGR:\t");
    status = psq_booldmp(sess_cb->pss_flattening_flags & PSS_CORR_AGGR);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_RECREATE:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_RECREATE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_IPROC:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_IPROC);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_DBPROC:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_DBPROC);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_DBPGRANT_OK:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_0DBPGRANT_CHECK:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_RUSET:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_RUSET);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_CHECK_IF_ACTIVE:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_CHECK_IF_ACTIVE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_MISSING_PRIV:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_MISSING_PRIV);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_MISSING_OBJ:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_MISSING_OBJ);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_PARSING_SET_PARAM:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_PARSING_SET_PARAM);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SET_INPUT_PARAM:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SYSTEM_GENERATED:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_NOT_DROPPABLE:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_NOT_DROPPABLE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SUPPORTS_CONS:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SUPPORTS_CONS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_CHECK_IF_ACTIVE:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_CHECK_IF_ACTIVE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_MISSING_PRIV:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_MISSING_PRIV);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_MISSING_OBJ:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_MISSING_OBJ);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_PARSING_SET_PARAM:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_PARSING_SET_PARAM);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SET_INPUT_PARAM:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SYSTEM_GENERATED:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_NOT_DROPPABLE:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_NOT_DROPPABLE);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_dbp_flags.PSS_SUPPORTS_CONS:\t");
    status = psq_booldmp(sess_cb->pss_dbp_flags & PSS_SUPPORTS_CONS);
    if (status != E_DB_OK)
	return (status);

    TRdisplay("\n\tpss_qbuf:\t0x%p\n", sess_cb->pss_qbuf);
    TRdisplay("\tpss_nxtchar:\t0x%p\n", sess_cb->pss_nxtchar);
    TRdisplay("\tpss_prvtok:\t0x%p\n", sess_cb->pss_prvtok);
    TRdisplay("\tpss_endbuf:\t0x%p\n", sess_cb->pss_endbuf);
    TRdisplay("\tpss_lineno:\t%d\n", sess_cb->pss_lineno);
    TRdisplay("\tpss_qualdepth:\t%d\n", sess_cb->pss_qualdepth);
    TRdisplay("\tpss_symstr:\t0x%p\n", sess_cb->pss_symstr);
    TRdisplay("\tpss_symtab:\t0x%p\n", sess_cb->pss_symtab);
    TRdisplay("\tpss_symnext:\t0x%p\n", sess_cb->pss_symnext);
    TRdisplay("\tpss_symblk:\t0x%p\n", sess_cb->pss_symblk);

    /*
    ** To dump pss_restab call pst_dmpres().
    */
    (VOID) pst_dmpres(&sess_cb->pss_restab);

    TRdisplay("\tpss_yacc:\t0x%p\n", sess_cb->pss_yacc);
    TRdisplay("\tpss_parser:\t0x%p\n", sess_cb->pss_parser);

    TRdisplay("\tpss_defqry:\t%d\n", sess_cb->pss_defqry);
    TRdisplay("\tpss_rsdmno:\t%d\n", sess_cb->pss_rsdmno);
    TRdisplay("\tpss_ostream:\t0x%p\n", &sess_cb->pss_ostream);
    TRdisplay("\tpss_tlist:\t0x%p\n", sess_cb->pss_tlist);
    TRdisplay("\tpss_dbid:\t0x%p\n", sess_cb->pss_dbid);
    TRdisplay("\tpss_crsr:\t0x%p\n", sess_cb->pss_crsr);
    TRdisplay("\tpss_highparm:\t%d\n", sess_cb->pss_highparm);
    TRdisplay("\tpss_memleft:\t%d\n", sess_cb->pss_memleft);
    TRdisplay("\tpss_user:\t%#s\n", sizeof (sess_cb->pss_user),
	&sess_cb->pss_user);
    TRdisplay("\tpss_dba:\t%#s\n", sizeof (sess_cb->pss_dba),
	&sess_cb->pss_dba);
    TRdisplay("\tpss_cstream:\t0x%p\n", sess_cb->pss_cstream);
    TRdisplay("\tpss_def_coll:\t%d\n", sess_cb->pss_def_coll);
    TRdisplay("\tpss_def_unicode_coll:\t%d\n", sess_cb->pss_def_unicode_coll);
    TRdisplay("\tpss_resrng:\n");
    psq_rngdmp(sess_cb->pss_resrng);
    TRdisplay("\n");

    TRdisplay("\tpss_usrrange:\n");
    for (rv = (PSS_RNGTAB *) sess_cb->pss_usrrange.pss_qhead.q_next;
	(QUEUE *) rv != &sess_cb->pss_usrrange.pss_qhead;
	rv = (PSS_RNGTAB *) rv->pss_rngque.q_next)
    {
	psq_rngdmp(rv);
	TRdisplay("\n");
    }
    return    (E_DB_OK);
}

/*{
** Name: psq_rngdmp	- Print a user range table entry for debugging purposes
**
** Description:
**      This function formats and prints out an entry in the user range table.
**
** Inputs:
**      rv                              Pointer to the range table entry
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	29-may-86 (jeff)
**          written
*/
VOID
psq_rngdmp(
	PSS_RNGTAB         *rv)
{
    if (rv == (PSS_RNGTAB *) NULL)
    {
	TRdisplay("\t\tNo range variable\n");
	return;
    }

    TRdisplay("\t\tpss_used:\t");
    if (psq_booldmp(rv->pss_used) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\t\tpss_rgno:\t%d\n", rv->pss_rgno);
    TRdisplay("\t\tpss_rgname:\t%#s\n", sizeof (rv->pss_rgname), 
	rv->pss_rgname);

    TRdisplay("\t\tpss_tabid:\t");
    if (psq_tbiddmp(&rv->pss_tabid) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\t\tpss_tabname:\t%#s\n", sizeof (rv->pss_tabname),
	&rv->pss_tabname);
    TRdisplay("\t\tpss_tabown:\t%#s\n", sizeof (rv->pss_ownname),
	&rv->pss_ownname);
    TRdisplay("\t\tpss_colhsh:\t0x%p\n", rv->pss_colhsh);
    TRdisplay("\t\tpss_tabdesc:\t0x%p\n", rv->pss_tabdesc);
    TRdisplay("\t\tpss_attdesc:\t0x%p\n", rv->pss_attdesc);
    TRdisplay("\t\tpss_rdrinfo:\t0x%p\n", rv->pss_rdrinfo);
}
