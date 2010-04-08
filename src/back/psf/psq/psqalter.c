/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <dudbms.h>

/**
**
**  Name: PSQALTER.C - Functions used to alter characteristics of a parser
**		    session.
**
**  Description:
**      This file contains the functions necessary to alter the characteristics 
**      of a parser session.
**
**          psq_alter - Alter the characteristics of a parser session.
**
**
**  History:    
**      02-oct-85 (jeff)    
**          written
**	13-sep-90 (teresa)
**	    make the following booleans become bitflags instead:psq_catupd,
**	    pss_catupd
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.  Also validate distributed specification.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	11-feb-1993 (ralph)
**	    DELIM_IDENT: Set pss_dbxlate from psq_dbxlate to establish
**	    case translation semantics for the session.
**	    Copy effective userid into session cb.
**	05-apr-1993 (ralph)
**	    DELIM_IDENT: Don't set pss_dbxlate after all; not needed.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	2-aug-93 (robf)
**	    allow ustat to be change via PSQ_ALTER request
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
[@history_template@]...
**/

/*{
** Name: psq_alter	- Alter the characteristics of a parser session.
**
**  INTERNAL PSF call format: status = psq_alter(&psq_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psq_call(PSQ_ALTER, &psq_cb, &sess_cb);
**
** Description:
**      The psq_alter function alters the characteristics of a parser session. 
**      It can change the query language, the acceptibility of distributed 
**      statements and constructs, and/or the character to be used as a decimal
**      marker.
**
** Inputs:
**      psq_cb
**          .psq_qlang                  Query language identifier (PSF_NOLANG if
**					this is not to be changed).
**	    .psq_distrib		Indicator for whether distributed
**					constructs are to be accepted.  Use
**					PSF_DST_DONTCARE if this is not to be
**					changed.
**	    .psq_decimal
**		.psf_decspec		TRUE iff a new decimal marker is
**					specified.
**		.psf_decimal		The new decimal marker character, if
**					specified.
**	    .psq_flag			work containing bitflags:
**	        .psq_catupd		   TRUE iff now allowing catalog update.
**					   FALSE iff now disallowing catalog 
**					   update.
**	    .psq_ustat			User status mask
**
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psq_cb
**	    .psq_error			Detailed error status
**		E_PS0000_OK		    Success
**		E_PS0002_INTERNAL_ERROR	    Internal PSF problem
**		E_PS0201_BAD_QLANG	    Unknown query language specifier
**		E_PS0202_QLANG_NOT_ALLOWED  Query language not allowed in
**					    server
**		E_PS0204_BAD_DISTRIB	    Bad distributed specifier
**		E_PS0205_SRV_NOT_INIT	    Server not initialized
**		E_PS035E_BAD_DECIMAL	    Illegal decimal mareker specified
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
**	09-apr-87 (daved)
**	    written
**	11-feb-1993 (ralph)
**	    DELIM_IDENT: Set pss_dbxlate from psq_dbxlate to establish
**	    case translation semantics for the session.
**	    Copy effective userid into session cb.
**	05-apr-1993 (ralph)
**	    DELIM_IDENT: Don't set pss_dbxlate after all; not needed.
**	2-aug-93 (robf)
**	    allow ustat to be change via PSQ_ALTER request 
**	30-mar-94 (robf)
**          Allow session role to be altered.
**	23-may-94 (robf)
**          When changing subject privileges also update the
**	    update_syscat flag since thats still represented as a 
**	    privilege.
*/
DB_STATUS
psq_alter(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    extern PSF_SERVBLK  *Psf_srvblk;

    /* Validate everything first, before taking any action */

    /* Make sure server is initialized */
    if (!Psf_srvblk->psf_srvinit)
    {
	psq_cb->psq_error.err_code = E_PS0205_SRV_NOT_INIT;
	return (E_DB_ERROR);
    }

    /* Validate the query language */
    if (psq_cb->psq_qlang != DB_NOLANG && psq_cb->psq_qlang != DB_QUEL &&
	psq_cb->psq_qlang != DB_SQL)
    {
	psq_cb->psq_error.err_code = E_PS0201_BAD_QLANG;
	return (E_DB_ERROR);
    }

    /* Make sure query language is allowed in this session */
    if ((psq_cb->psq_qlang & Psf_srvblk->psf_lang_allowed) == 0)
    {
	psq_cb->psq_error.err_code = E_PS0202_QLANG_NOT_ALLOWED;
	return (E_DB_ERROR);
    }

    /* Check distributed specification
    **
    ** a=local_server, b=distrib_server, c=distrib_session
    **
    **   a,b
    **
    **     00  01  11  10
    **    -----------------
    ** c  |   |   |   |   |
    **  0 | 1 | 1 | 0 | 0 |
    **    |   |   |   |   |
    **    -----------------  ==> ERROR
    **    |   |   |   |   |
    **  1 | 1 | 0 | 0 | 1 |
    **    |   |   |   |   |
    **    -----------------
    */
    if (   !(psq_cb->psq_distrib & (DB_1_LOCAL_SVR | DB_3_DDB_SESS))
	|| (~psq_cb->psq_distrib & DB_2_DISTRIB_SVR)
	&& (psq_cb->psq_distrib & DB_3_DDB_SESS)
	)
    {
	psq_cb->psq_error.err_code = E_PS0204_BAD_DISTRIB;
	return (E_DB_ERROR);
    }

    /* Check decimal specification */
    if (psq_cb->psq_decimal.db_decspec &&
	(psq_cb->psq_decimal.db_decimal != '.' &&
	 psq_cb->psq_decimal.db_decimal != ','))
    {
	psq_cb->psq_error.err_code = E_PS035E_BAD_DECIMAL;
	return (E_DB_ERROR);
    }

    /* All validation has been done.  Now do the real work */

    /* 
    ** Copy over user status/privileges if requested 
    ** This is the only operation allowed in the call
    */
    if(psq_cb->psq_alter_op & PSQ_CHANGE_PRIVS)
    {
	    /* Update any update_syscat flag */
	    if (psq_cb->psq_ustat & DU_UUPSYSCAT)
		sess_cb->pss_ses_flag |= PSS_CATUPD;	/* pss_catupd = TRUE */
	    else
		sess_cb->pss_ses_flag &= ~PSS_CATUPD;	/* pss_catupd = FALSE */

	    /* Update privileges */
	    sess_cb->pss_ustat = psq_cb->psq_ustat;

	    return E_DB_OK;
    }

    if(psq_cb->psq_alter_op & PSQ_CHANGE_ROLE)
    {
    	STRUCT_ASSIGN_MACRO(psq_cb->psq_aplid, sess_cb->pss_aplid);
    }

    /* The query language */
    if (psq_cb->psq_qlang != DB_NOLANG)
	sess_cb->pss_lang = psq_cb->psq_qlang;

    /* The distributed specifier */
    sess_cb->pss_distrib = psq_cb->psq_distrib;

    /* The decimal marker */
    if (psq_cb->psq_decimal.db_decspec)
	sess_cb->pss_decimal = psq_cb->psq_decimal.db_decimal;
    /* the catalog update flag */
    if (psq_cb->psq_flag & PSQ_CATUPD)
	sess_cb->pss_ses_flag |= PSS_CATUPD;	/* pss_catupd = TRUE */
    else
	sess_cb->pss_ses_flag &= ~PSS_CATUPD;	/* pss_catupd = FALSE */

    /* Copy effective userid */

    STRUCT_ASSIGN_MACRO(psq_cb->psq_user.db_tab_own, sess_cb->pss_user);

    return    (E_DB_OK);
}
