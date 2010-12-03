/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
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
**  Name: PSQCLSCRS.C - Functions used to close cursors in a session
**
**  Description:
**      This file contains the functions necessary to close open cursors 
**      in a parser session.
**
**          psq_clscurs - Close open cursor(s) within a parser session
**
**
**  History:    
**      02-oct-85 (jeff)
**          written
**	13-sep-90 (teresa)
**	    changed psq_force and psq_clsall to bitflags in psq_flag.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_clscurs(
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psq_clscurs	- Close all open cursors within a parser session.
**
**  INTERNAL PSF call format: status = psq_clscurs(&psq_cb, &sess_cb);
**
**  EXTERNAL call format:    status = psq_call(PSQ_CLSCURS, &psq_cb, &sess_cb);
**
** Description:
**      The psq_clscurs function closes open cursors within a parser 
**      session and deallocates all resources associated with those cursors.
**	It should be used when a cursor is closed for any reason (e.g. there
**	was an error fetching the next row).  It should be used when an
**	"end transaction" statement executes to close all open cursors.
**
** Inputs:
**      psq_cb
**	    .psq_cursid			Id of the cursor to close
**	    .psq_flag			Map of bitflags:
**	      .psq_clsall		 TRUE means to close all cursors.  Above
**					 input will be ignored if this one is
**					 TRUE.
**            .psq_force                 TRUE means to close the cursor(s)
**					 regardless of error conditions.
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psq_cb
**	    .psq_error			Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
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
**	    none
**
** History:
**	02-oct-85 (jeff)
**          written
**	13-sep-90 (teresa)
**	    changed psq_force and psq_clsall to bitflags in psq_flag.
*/
DB_STATUS
psq_clscurs(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    PSC_CURBLK          *cursor;
    register i4	i;
    DB_STATUS		status;
    PSC_CURBLK		*next;

    if (psq_cb->psq_flag & PSQ_CLSALL)
    {
	/* Close all cursors */
	for (i = 0; i < PSS_CURTABSIZE; i++)
	{
	    cursor = sess_cb->pss_curstab.pss_curque[i];
	    while (cursor != (PSC_CURBLK *) NULL)
	    {
		/*
		** Get next pointer now because cursor control block
		** will probably be deallocated.
		*/
		next = cursor->psc_next;
		status = psq_crclose(cursor, &sess_cb->pss_curstab,
		    &sess_cb->pss_memleft, &psq_cb->psq_error);
		if (status != E_DB_OK && !(psq_cb->psq_flag & PSQ_FORCE) )
		    return (status);
		cursor = next;
	    }
	}
    }
    else
    {
	/* Just close one cursor */
	status = psq_crfind(sess_cb, &psq_cb->psq_cursid, &cursor,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);

	if (cursor == (PSC_CURBLK *) NULL)
	{
	    psq_cb->psq_error.err_code = E_PS0401_CUR_NOT_FOUND;
	    return (E_DB_ERROR);
	}

	status = psq_crclose(cursor, &sess_cb->pss_curstab,
	    &sess_cb->pss_memleft, &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);
    }

    return    (E_DB_OK);
}
