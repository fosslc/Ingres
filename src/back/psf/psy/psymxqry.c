/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <opfcb.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <scf.h>
#include    <qefcb.h>
#include    <dudbms.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYMXQRY.C - Functions used to establish temporary query limits
**
**  Description:
**      This file contains functions for processing SET [NO]MAX...
**	statements:
**
**          psy_maxquery    - Validates and sets temporary limits
**
**  History:
**	22-jun-89 (ralph)
**	    written
**	08-aug-90 (ralph)
**	    force opf enumeration when query limit is in effect (bug 30809)
**	09-nov-90 (stec)
**	    Changed psy_maxquery to initialize opf_thandle in OPF_CB.
**	15-jan-91 (ralph)
**	    Use DBPR_OPFENUM to determine if OPF enumeration should be forced
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	10-aug-93 (andre)
**	    removed declaration of scf_call()
**      16-sep-93 (smc)
**          Removed (PTR) cast to session ids now they are CS_SID.
**          Added/moved <cs.h> for CS_SID.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	9-nov-93 (robf)
**          Add security auditing (resource controls)
**	10-Jan-2001 (jenjo02)
**	    Removed call to SCU_INFORMATION to return QEF_CB*;
**	    qef_call() takes care of this itself.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_maxquery(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_maxquery - Validate and set temporary query limits
**
**  INTERNAL PSF call format: status = psy_maxquery(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_MXQUERY,&psy_cb, sess_cb);
**
** Description:
**	This procedure is used to set temporary query limits.
**	When SET [NO]MAXxxxx is specified, a psy_cb is built
**	that contains the specified limit.  The format of
**	the control block is similar to one built by
**	GRANT QUERY_xxxx_LIMIT. 
**
**	This routine calls scu_information() to determine the
**	session's values for QUERY_xxxx_LIMIT.  The value
**	specified on the SET MAXxxxx statement must not exceed
**	the session's QUERY_xxxx_LIMIT value.
**
**	Once the request is verified,  qec_alter() is called to
**	set the query limit fields in the QEF_CB control block.
**
** Inputs:
**      psy_cb
**	    .psy_fl_dbpriv		control flags
**	    .psy_qdio_limit		query I/O  limit
**	    .psy_qrow_limit		query row  limit
**	    .psy_qcpu_limit		query CPU  limit
**	    .psy_qpage_limit		query page limit
**	    .psy_qcost_limit		query cost limit
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Alters query limit fields in the QEF_CB control block.
**	    Calls OPF_ALTER to force/reset enumeration if query limits are/not
**	    in effect.
**
** History:
**	22-jun-89 (ralph)
**          written
**	11-jul-90 (ralph)
**	    correct qef_alt array dimension
**	08-aug-90 (ralph)
**	    force opf enumeration when query limit is in effect (bug 30809)
**	09-nov-90 (stec)
**	    Initialize opf_thandle in OPF_CB.
**	15-jan-91 (ralph)
**	    Use DBPR_OPFENUM to determine if OPF enumeration should be forced
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	10-Jan-2001 (jenjo02)
**	    Removed call to SCU_INFORMATION to return QEF_CB*;
**	    qef_call() takes care of this itself.
*/

DB_STATUS
psy_maxquery(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tempstat;
    i4		err_code;
    extern PSF_SERVBLK	*Psf_srvblk;
    SXF_ACCESS	        access;
    char		*objtype="";
    DB_ERROR		err_blk;

    u_i4	    dbpriv_flags,
		    tempmax_flags;
    i4	    qdio_limit,
		    qrow_limit,
		    qcpu_limit,
		    qpage_limit,
		    qcost_limit;
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[7];
    QEF_RCB	    qef_rcb;
    QEF_ALT	    qef_alt[6];
    OPF_CB	    opf_cb;

    scf_cb.scf_length	= sizeof(SCF_CB);
    scf_cb.scf_type	= SCF_CB_TYPE;
    scf_cb.scf_facility	= DB_PSF_ID;
    scf_cb.scf_session	= (SCF_SESSION) sess_cb->pss_sessid;
    scf_cb.scf_ptr_union.scf_sci    = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 7;

    /* Set up to get permanent query limits */

    sci_list[0].sci_length  = sizeof(dbpriv_flags);
    sci_list[0].sci_code    = SCI_DBPRIV;
    sci_list[0].sci_aresult = (char *) &dbpriv_flags;
    sci_list[0].sci_rlength = NULL;
    sci_list[1].sci_length  = sizeof(qdio_limit);
    sci_list[1].sci_code    = SCI_QDIO_LIMIT;
    sci_list[1].sci_aresult = (char *) &qdio_limit;
    sci_list[1].sci_rlength = NULL;
    sci_list[2].sci_length  = sizeof(qrow_limit);
    sci_list[2].sci_code    = SCI_QROW_LIMIT;
    sci_list[2].sci_aresult = (char *) &qrow_limit;
    sci_list[2].sci_rlength = NULL;
    sci_list[3].sci_length  = sizeof(qcpu_limit);
    sci_list[3].sci_code    = SCI_QCPU_LIMIT;
    sci_list[3].sci_aresult = (char *) &qcpu_limit;
    sci_list[3].sci_rlength = NULL;
    sci_list[4].sci_length  = sizeof(qpage_limit);
    sci_list[4].sci_code    = SCI_QPAGE_LIMIT;
    sci_list[4].sci_aresult = (char *) &qpage_limit;
    sci_list[4].sci_rlength = NULL;
    sci_list[5].sci_length  = sizeof(qcost_limit);
    sci_list[5].sci_code    = SCI_QCOST_LIMIT;
    sci_list[5].sci_aresult = (char *) &qcost_limit;
    sci_list[5].sci_rlength = NULL;

    /* While we're at it, get the temporary query limit flags */

    sci_list[6].sci_length  = sizeof(tempmax_flags);
    sci_list[6].sci_code    = SCI_MXPRIV;
    sci_list[6].sci_aresult = (char *) &tempmax_flags;
    sci_list[6].sci_rlength = NULL;

    /* Get permanent query limits */

    if (scf_call(SCU_INFORMATION, &scf_cb) != E_DB_OK)
    {
	(VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L, PSF_INTERR,
			 &scf_cb.scf_error.err_code, &psy_cb->psy_error, 0);
	return(E_DB_ERROR);
    }

    /* Set up to write temporary query limits to QEF_CB */

    qef_rcb.qef_length	= sizeof(QEF_RCB);
    qef_rcb.qef_type	= QEFRCB_CB;
    qef_rcb.qef_ascii_id	= QEFRCB_ASCII_ID;
    qef_rcb.qef_sess_id	= sess_cb->pss_sessid;
    qef_rcb.qef_asize	= 6;
    qef_rcb.qef_alt	= qef_alt;
    qef_rcb.qef_eflag	= QEF_EXTERNAL;
    qef_alt[0].alt_code = QEC_DBPRIV;
    qef_alt[1].alt_code = QEC_QDIO_LIMIT;
    qef_alt[2].alt_code = QEC_QROW_LIMIT;
    qef_alt[3].alt_code = QEC_QCPU_LIMIT;
    qef_alt[4].alt_code = QEC_QPAGE_LIMIT;
    qef_alt[5].alt_code = QEC_QCOST_LIMIT;

    /* Process SET [NO]MAXIO or SET [NO]MAXQUERY */
    if (psy_cb->psy_ctl_dbpriv & DBPR_QDIO_LIMIT)
    {
	objtype="MAXIO";
	/* Make sure value (if specified) is within range */
	if ((psy_cb->psy_fl_dbpriv & DBPR_QDIO_LIMIT) &&
	    (dbpriv_flags & DBPR_QDIO_LIMIT) &&
	    (psy_cb->psy_qdio_limit > qdio_limit))
	{
	    (VOID)psf_error(E_US1862_EXCESSIVE_MAXIO, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, (i4)2,
		    sizeof(psy_cb->psy_qdio_limit),&psy_cb->psy_qdio_limit,
		    sizeof(qdio_limit), &qdio_limit);
	    qef_alt[1].alt_code	    = QEC_IGNORE;
	    status = E_DB_WARN;
	}
	/* If value specified, use it */
	else if (psy_cb->psy_fl_dbpriv & DBPR_QDIO_LIMIT)
	{
	    tempmax_flags |= DBPR_QDIO_LIMIT;
	    qdio_limit = psy_cb->psy_qdio_limit;
	}
	/* Else use the permanent value */
	else
	{
	    tempmax_flags &= ~DBPR_QDIO_LIMIT;
	    tempmax_flags |= (DBPR_QDIO_LIMIT & dbpriv_flags);
	}
    }
    else
    {
	qef_alt[1].alt_code	= QEC_IGNORE;
    }

    /* Process SET [NO]MAXROW */
    if (psy_cb->psy_ctl_dbpriv & DBPR_QROW_LIMIT)
    {
	objtype="MAXROW";
	/* Make sure value (if specified) is within range */
	if ((psy_cb->psy_fl_dbpriv & DBPR_QROW_LIMIT) &&
	    (dbpriv_flags & DBPR_QROW_LIMIT) &&
	    (psy_cb->psy_qrow_limit > qrow_limit))
	{
	    (VOID)psf_error(E_US1864_EXCESSIVE_MAXROW, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, (i4)2,
		    sizeof(psy_cb->psy_qrow_limit),&psy_cb->psy_qrow_limit,
		    sizeof(qrow_limit), &qrow_limit);
	    qef_alt[2].alt_code	    = QEC_IGNORE;
	    status = E_DB_WARN;
	}
	/* If value specified, use it */
	else if (psy_cb->psy_fl_dbpriv & DBPR_QROW_LIMIT)
	{
	    tempmax_flags |= DBPR_QROW_LIMIT;
	    qrow_limit = psy_cb->psy_qrow_limit;
	}
	/* Else use the permanent value */
	else
	{
	    tempmax_flags &= ~DBPR_QROW_LIMIT;
	    tempmax_flags |= (DBPR_QROW_LIMIT & dbpriv_flags);
	}
    }
    else
    {
	qef_alt[2].alt_code	= QEC_IGNORE;
    }

    /* Process SET [NO]MAXCPU */
    if (psy_cb->psy_ctl_dbpriv & DBPR_QCPU_LIMIT)
    {
	objtype="MAXCPU";
	/* Make sure value (if specified) is within range */
	if ((psy_cb->psy_fl_dbpriv & DBPR_QCPU_LIMIT) &&
	    (dbpriv_flags & DBPR_QCPU_LIMIT) &&
	    (psy_cb->psy_qcpu_limit > qcpu_limit))
	{
	    (VOID)psf_error(E_US1861_EXCESSIVE_MAXCPU, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, (i4)2,
		    sizeof(psy_cb->psy_qcpu_limit),&psy_cb->psy_qcpu_limit,
		    sizeof(qcpu_limit), &qcpu_limit);
	    qef_alt[3].alt_code	    = QEC_IGNORE;
	    status = E_DB_WARN;
	}
	/* If value specified, use it */
	else if (psy_cb->psy_fl_dbpriv & DBPR_QCPU_LIMIT)
	{
	    tempmax_flags |= DBPR_QCPU_LIMIT;
	    qcpu_limit = psy_cb->psy_qcpu_limit;
	}
	/* Else use the permanent value */
	else
	{
	    tempmax_flags &= ~DBPR_QCPU_LIMIT;
	    tempmax_flags |= (DBPR_QCPU_LIMIT & dbpriv_flags);
	}
    }
    else
    {
	qef_alt[3].alt_code	= QEC_IGNORE;
    }

    /* Process SET [NO]MAXPAGE */
    if (psy_cb->psy_ctl_dbpriv & DBPR_QPAGE_LIMIT)
    {
	objtype="MAXPAGE";
	/* Make sure value (if specified) is within range */
	if ((psy_cb->psy_fl_dbpriv & DBPR_QPAGE_LIMIT) &&
	    (dbpriv_flags & DBPR_QPAGE_LIMIT) &&
	    (psy_cb->psy_qpage_limit > qpage_limit))
	{
	    (VOID)psf_error(E_US1863_EXCESSIVE_MAXPAGE, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, (i4)2,
		    sizeof(psy_cb->psy_qpage_limit),&psy_cb->psy_qpage_limit,
		    sizeof(qpage_limit), &qpage_limit);
	    qef_alt[4].alt_code	    = QEC_IGNORE;
	    status = E_DB_WARN;
	}
	/* If value specified, use it */
	else if (psy_cb->psy_fl_dbpriv & DBPR_QPAGE_LIMIT)
	{
	    tempmax_flags |= DBPR_QPAGE_LIMIT;
	    qpage_limit = psy_cb->psy_qpage_limit;
	}
	/* Else use the permanent value */
	else
	{
	    tempmax_flags &= ~DBPR_QPAGE_LIMIT;
	    tempmax_flags |= (DBPR_QPAGE_LIMIT & dbpriv_flags);
	}
    }
    else
    {
	qef_alt[4].alt_code	= QEC_IGNORE;
    }

    /* Process SET [NO]MAXCOST */
    if (psy_cb->psy_ctl_dbpriv & DBPR_QCOST_LIMIT)
    {
	objtype="MAXCOST";
	/* Make sure value (if specified) is within range */
	if ((psy_cb->psy_fl_dbpriv & DBPR_QCOST_LIMIT) &&
	    (dbpriv_flags & DBPR_QCOST_LIMIT) &&
	    (psy_cb->psy_qcost_limit > qcost_limit))
	{
	    (VOID)psf_error(E_US1860_EXCESSIVE_MAXCOST, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, (i4)2,
		    sizeof(psy_cb->psy_qcost_limit),&psy_cb->psy_qcost_limit,
		    sizeof(qcost_limit), &qcost_limit);
	    qef_alt[5].alt_code	    = QEC_IGNORE;
	    status = E_DB_WARN;
	}
	/* If value specified, use it */
	else if (psy_cb->psy_fl_dbpriv & DBPR_QCOST_LIMIT)
	{
	    tempmax_flags |= DBPR_QCOST_LIMIT;
	    qcost_limit = psy_cb->psy_qcost_limit;
	}
	/* Else use the permanent value */
	else
	{
	    tempmax_flags &= ~DBPR_QCOST_LIMIT;
	    tempmax_flags |= (DBPR_QCOST_LIMIT & dbpriv_flags);
	}
    }
    else
    {
	qef_alt[5].alt_code	= QEC_IGNORE;
    }

    /* Set temporary query limits in QEF_ALT array */

    qef_alt[0].alt_value.alt_u_i4 = tempmax_flags;
    qef_alt[1].alt_value.alt_i4 = qdio_limit;
    qef_alt[2].alt_value.alt_i4 = qrow_limit;
    qef_alt[3].alt_value.alt_i4 = qcpu_limit;
    qef_alt[4].alt_value.alt_i4 = qpage_limit;
    qef_alt[5].alt_value.alt_i4 = qcost_limit;

    /* Write temporary query limits to QEF_CB */

    if (qef_call(QEC_ALTER, ( PTR ) &qef_rcb) != E_DB_OK)
    {
	(VOID) psf_error(E_QE0210_QEC_ALTER, 0L, PSF_INTERR,
			 &qef_rcb.error.err_code, &psy_cb->psy_error, 0);
	return(E_DB_ERROR);
    }

#ifdef  OPF_RESOURCE
    /* Tell OPF to force enumeration if any query limit in effect */

    opf_cb.opf_length   = sizeof(opf_cb);
    opf_cb.opf_type     = OPFCB_CB;
    opf_cb.opf_owner    = (PTR)DB_PSF_ID;
    opf_cb.opf_ascii_id = OPFCB_ASCII_ID;
    opf_cb.opf_scb      = NULL;      /* OPF must get its own session cb */
    opf_cb.opf_level    = OPF_SESSION;
    opf_cb.opf_thandle	= NULL;

    if (tempmax_flags & DBPR_OPFENUM)
    {
        opf_cb.opf_alter = OPF_ARESOURCE;       /* Force enumeration */
    }
    else
    {
        opf_cb.opf_alter = OPF_ANORESOURCE;     /* Don't force enumeration */
    }

    tempstat = opf_call(OPF_ALTER, &opf_cb);

    status = (status > tempstat) ?
              status : tempstat;
#endif

    /*
    ** Audit changing query resource limit
    */
    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
    {
	if(status==E_DB_OK)
	    access=SXF_A_SUCCESS|SXF_A_LIMIT;
	else
	    access=SXF_A_FAIL|SXF_A_LIMIT;
	tempstat=psy_secaudit(
	    FALSE,
	    sess_cb,
	    objtype,
	    (DB_OWN_NAME *)0,
	    STlength(objtype),
	    SXF_E_RESOURCE,
	    I_SX2740_SET_QUERY_LIMIT,
	    access,
	    &err_blk);
	if(tempstat>status)
	{
	    status=tempstat;
	}
    }
    return    (status);
}
