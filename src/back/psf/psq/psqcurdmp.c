/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tr.h>
#include    <bt.h>
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
**  Name: PSQCURDMP.C - Functions used to dump cursor control block for given
**		    cursor and session
**
**  Description:
**      This file contains the functions necessary to dump a cursor control
**	block given a cursor id and a session id.
**
**          psq_crdump - Dump cursor control block for given cursor and session
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**	27-oct-88 (stec)
**	    Dump psc_iupdmap.
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	22-dec-92 (rblumer)
**	    Added tracing of new statement-level rules.
**	10-mar-93 (andre)
**	    dump psc_expmap
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in psq_crdump().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_crdump(
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psq_crdump	- Dump cursor control block given cursor and session ids
**
**  INTERNAL PSF call format: status = psq_crdump(&psq_cb, &sess_cb);
**
**  EXTERNAL call format:    status = psq_call(PSQ_CURDUMP, &psq_cb, &sess_cb);
**
** Description:
**      The psq_crdump function will format and print a cursor control block
**	given the cursor id and the session id that identify it.  The output
**	will go to the output terminal and/or file named by the user in the
**	"SET TRACE TERMINAL" and "SET TRACE OUTPUT" commands.
**
** Inputs:
**      psq_cb
**          .psq_cursid                 Cursor id
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**	psq_cb
**	    .psq_error			Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0205_SRV_NOT_INIT	Server not initialized
**		    E_PS0401_CUR_NOT_FOUND	Cursor not found
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
**	27-oct-88 (stec)
**	    Dump psc_iupdmap.
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	22-dec-92 (rblumer)
**	    Added tracing of new statement-level rules.
**	10-mar-93 (andre)
**	    dump psc_expmap
**	07-apr-93 (andre)
**	    psc_tbl_mask, psc_rchecked, psc_rules, and psc_stmt_rules have all
**	    been moved from PSC_CURBLK into PSC_TBL_DESCR.  A list of one or
**	    more PSC_TBL_DESCR structures will hang off PSC_CURBLK for 
**		updatable cursors
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
*/
DB_STATUS
psq_crdump(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    PSC_CURBLK		*cursor;
    DB_STATUS		status;
    i4			i;
    i4			thisline;
    PSC_RESCOL		*column;
    extern PSF_SERVBLK	*Psf_srvblk;

    /*
    ** Make sure server is initialized.
    */
    if (!Psf_srvblk->psf_srvinit)
    {
	psq_cb->psq_error.err_code = E_PS0205_SRV_NOT_INIT;
	return (E_DB_ERROR);
    }

    /*
    ** Get pointer to cursor control block.
    */
    status = psq_crfind(sess_cb, &psq_cb->psq_cursid, &cursor,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    /*
    ** NULL means no such cursor.
    */
    if (cursor == (PSC_CURBLK *) NULL)
    {
	psq_cb->psq_error.err_code = E_PS0401_CUR_NOT_FOUND;
	return (E_DB_ERROR);
    }

    /*
    ** Now print out everything in the cursor control block.
    */

    TRdisplay("Cursor Control Block for Cursor:");
    status = psq_ciddmp(&psq_cb->psq_cursid);
    if (status != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n\n");

    /* First, the control block header */
    if ((status = psq_headdmp((PSQ_CBHEAD *) cursor)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }

    /* The cursor id */
    TRdisplay("\tpsc_blkid:\n");
    if ((status = psq_ciddmp(&cursor->psc_blkid)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }

    /* Used / Not Used flag */
    TRdisplay("\tpsc_used:\t");
    if ((status = psq_booldmp(cursor->psc_used)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }

    TRdisplay("\n");

    /* Stream pointer */
    TRdisplay("\tpsc_stream:\t%p\n", cursor->psc_stream);

    /* Query language */
    TRdisplay("\tpsc_lang:\t(%d) ", cursor->psc_lang);
    if ((status = psq_lngdmp(cursor->psc_lang)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    /* Delete permission flag */
    TRdisplay("\tpsc_delall:\t");
    if ((status = psq_booldmp(cursor->psc_delall)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    /* For update flag */
    TRdisplay("\tpsc_forupd:\t");
    if ((status = psq_booldmp(cursor->psc_forupd)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    /* Readonly flag */
    TRdisplay("\tpsc_readonly:\t");
    if ((status = psq_booldmp(cursor->psc_readonly)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    /* Repeat cursor flag */
    TRdisplay("\tpsc_repeat:\t");
    if ((status = psq_booldmp(cursor->psc_repeat)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    /* Open flag */
    TRdisplay("\tpsc_open:\t");
    if ((status = psq_booldmp(cursor->psc_open)) != E_DB_OK)
    {
	psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
	return (status);
    }
    TRdisplay("\n");

    TRdisplay("\tList of descriptions of cursor's underlying table/views:\n");
    if (cursor->psc_tbl_descr_queue.q_next == &cursor->psc_tbl_descr_queue)
    {
	TRdisplay("\t\tNONE\n");
    }
    else
    {
	PSC_TBL_DESCR	    *descr, *last_descr;
	i4		    i = 0;

	descr = (PSC_TBL_DESCR *) cursor->psc_tbl_descr_queue.q_next;
	last_descr = (PSC_TBL_DESCR *) cursor->psc_tbl_descr_queue.q_prev;
	
	do
	{
	    if (i++)
		descr = (PSC_TBL_DESCR *) descr->psc_queue.q_next;

	    /* element number (starting at 1) */
	    TRdisplay("\t\telement %d:\n", i);

	    /* table/view id */
	    TRdisplay("\t\t\t\tpsc_tabid:\t(%d,%d)\n",
		descr->psc_tabid.db_tab_base, descr->psc_tabid.db_tab_index);

	    /* table mask */
	    TRdisplay("\t\t\tpsc_tbl_mask:\t0x%x\n", descr->psc_tbl_mask);

	    /* Row-level after user-defined rules */
	    TRdisplay("\t\t\tpsc_row_lvl_usr_rules:\t(address) 0x%p\n",
		descr->psc_row_lvl_usr_rules);

	    /* Row-level after system-generated rules */
	    TRdisplay("\t\t\tpsc_row_lvl_sys_rules:\t(address) 0x%p\n",
		descr->psc_row_lvl_sys_rules);

	    /* statement-level after user-defined rules */
	    TRdisplay("\t\t\tpsc_stmt_lvl_usr_rules:\t(address) 0x%p\n",
		descr->psc_stmt_lvl_usr_rules);

	    /* statement-level after system-generated rules */
	    TRdisplay("\t\t\tpsc_stmt_lvl_sys_rules:\t(address) 0x%p\n",
		descr->psc_stmt_lvl_sys_rules);

	    /* Row-level before user-defined rules */
	    TRdisplay("\t\t\tpsc_row_lvl_usr_before_rules:\t(address) 0x%p\n",
		descr->psc_row_lvl_usr_before_rules);

	    /* Row-level before system-generated rules */
	    TRdisplay("\t\t\tpsc_row_lvl_sys_before_rules:\t(address) 0x%p\n",
		descr->psc_row_lvl_sys_before_rules);

	    /* statement-level before user-defined rules */
	    TRdisplay("\t\t\tpsc_stmt_lvl_usr_before_rules:\t(address) 0x%p\n",
		descr->psc_stmt_lvl_usr_before_rules);

	    /* statement-level before system-generated rules */
	    TRdisplay("\t\t\tpsc_stmt_lvl_sys_before_rules:\t(address) 0x%p\n",
		descr->psc_stmt_lvl_sys_before_rules);

	    /* psc_flags */
	    TRdisplay("\t\t\tpsc_flags:\t");

	    TRdisplay("\t\t\t\tPSC_RULES_CHECKED:\t");
	    if ((status =
		psq_booldmp(descr->psc_flags & PSC_RULES_CHECKED)) != E_DB_OK)
	    {
		psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
		return (status);
	    }
	} while (descr != last_descr);
    }

    TRdisplay("\n");

    /* Now do the set of columns for update.  psc_updmap is a bit map. */
    TRdisplay("\tpsc_updmap:\t");
    thisline = 0;
    for (i = 0; i < (i4)sizeof(cursor->psc_updmap) * BITSPERBYTE; i++)
    {
	if (BTtest(i, (char *) &cursor->psc_updmap))
	{
	    TRdisplay("%d ", i);
	    thisline++;
	    /* Limit to 10 numbers per row */
	    if (thisline >= 10)
	    {
		TRdisplay("\n\t\t");
		thisline = 0;
	    }
	}
    }
    TRdisplay("\n");

    /* Now do the column set */
    TRdisplay("\tpsc_restab:\n");
    TRdisplay("\t\tpsc_tabsize: %d\n", cursor->psc_restab.psc_tabsize);
    TRdisplay("\t\tpsc_coltab:\n");
    for (i = 0; i < cursor->psc_restab.psc_tabsize; i++)
    {
	for (column = cursor->psc_restab.psc_coltab[i];
	    column != (PSC_RESCOL *) NULL;
	    column = column->psc_colnext)
	{
	    TRdisplay("\n");
	    TRdisplay("\t\t\tpsc_attname:\t%#s\n", 
		sizeof (column->psc_attname), &column->psc_attname);
	    TRdisplay("\t\t\tpsc_type:\t");
	    if ((status = psq_dtdump(column->psc_type)) != E_DB_OK)
	    {
		psq_cb->psq_error.err_code = E_PS0002_INTERNAL_ERROR;
		return (status);
	    }
	    TRdisplay("\n\t\t\tpsc_len:\t%d\n", column->psc_len);
	    TRdisplay("\t\t\tpsc_prec:\t%d\n", column->psc_prec);
	    TRdisplay("\t\t\tpsc_attid:\t%d\n", column->psc_attid.db_att_id);
	    TRdisplay("\n");
	}
    }
	
    /* Now do the internal set of columns for update,
    ** psc_iupdmap is a bit map.
    */
    TRdisplay("\tpsc_iupdmap:\t");
    thisline = 0;
    for (i = 0; i < (i4)sizeof(cursor->psc_iupdmap) * BITSPERBYTE; i++)
    {
	if (BTtest(i, (char *) &cursor->psc_iupdmap))
	{
	    TRdisplay("%d ", i);
	    thisline++;
	    /* Limit to 10 numbers per row */
	    if (thisline >= 10)
	    {
		TRdisplay("\n\t\t");
		thisline = 0;
	    }
	}
    }
    TRdisplay("\n");

    /*
    ** Now dump a map of attributes of a view on which a cursor is declared
    ** which (attributes that is) are based on an expression
    ** psc_expmap is a bit map.
    */
    TRdisplay("\tpsc_expmap:\t");
    thisline = 0;
    for (i = 0; i < (i4)sizeof(cursor->psc_expmap) * BITSPERBYTE; i++)
    {
	if (BTtest(i, (char *) &cursor->psc_expmap))
	{
	    TRdisplay("%d ", i);
	    thisline++;
	    /* Limit to 10 numbers per row */
	    if (thisline >= 10)
	    {
		TRdisplay("\n\t\t");
		thisline = 0;
	    }
	}
    }
    TRdisplay("\n");

    return    (E_DB_OK);
}
