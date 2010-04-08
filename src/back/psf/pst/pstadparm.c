/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <adfops.h>

/**
**
**  Name: PSTADPARM.C - Functions for adding parameter nodes
**
**  Description:
**	This file contains the functions for adding a parameter node to a
**	query tree.
**
**          pst_adparm - Add a parameter node to a query tree - param type
**			is text string.
**	    pst_2adparm - Add a parameter node to query tree. param type
**			is in db data value.
**
**
**  History:
**      01-may-86 (jeff)    
**          written
**	18-feb-88 (stec)
**	    Change initialization of pst_pmspec (must not be PST_PMMAYBE).
**	07-jun-88 (stec)
**	    Modify for DB procs.
**	03-nov-88 (stec)
**	    Correct a bug found by lint.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	04-mar-93 (rblumer)
**	    change parameter to psf_adf_error to be psq_error, not psq_cb.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      15-Jul-1993 (fred)
**          Add DB_VBYTE_TYPE as a VLUP type.  This mechanism needs to
**          be improved for FE support of OME.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      10-sep-2008 (gupsh01,stial01)
**          Add unorm() to vlups that need to be normalized 
[@history_template@]...
**/

static bool add_unorm (PST_QNODE *tree, PSS_SESBLK *sess_cb);

/*{
** Name: pst_adparm	- Add a parameter node to a query tree.
**
** Description:
**      This function adds a parameterized constant node to a query tree.
**	It takes a parameter number and a format, translates the format to
**	a datatype and length, and creates a constant node containing the
**	datatype, length, and parameter number.
**
**	Valid formats are:
**	    c
**	    i1
**	    i2
**	    i4
**	    f4
**	    f8
**
** Inputs:
**	sess_cb				Session control block
**	psq_cb				Query control block
**      stream                          The memory stream for allocating the
**					node
**	parmno				The parameter number
**	format				The format string
**	newnode				Place to put pointer to new node
**	highparm			Pointer to the highest parm number found
**					so far.
**
** Outputs:
**      newnode                         Filled in with pointer to new node
**	highparm			Filled in with new parm number, if
**					higher than previous value.
**	psq_cb				Query control block
**	    .psq_error			filled in, if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	01-may-86 (jeff)
**          written
**	29-jul-87 (daved)
**	    set VLUP (strings) to have length of ADE_LEN_UNKNOWN
**	18-feb-88 (stec)
**	    Change initialization of pst_pmspec (must not be PST_PMMAYBE).
**	07-jun-88 (stec)
**	    Initialize pst_tparmtype in the constant node.
**	03-nov-88 (stec)
**	    Correct a bug found by lint.
**	30-mar-89 (jrb)
**	    Changed datalen to an i4 (to match pst_node's input type) and added
**	    dataprec for decimal project.
**	12-jan-93 (andre)
**	    pass 0 as precision to pst_node()
**	10-aug-93 (andre)
**	    fixed cause of compiler warning
*/
DB_STATUS
pst_adparm(
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PSF_MSTREAM        *stream,
	i2		   parmno,
	char		   *format,
	PST_QNODE	   **newnode,
	i4		   *highparm)
{
    DB_STATUS		status;
    register i4         first_char = format[0];
    register i4	second_char = format[1];
    register i4	third_char = format[2];
    DB_DT_ID		datatype;
    i4			datalen;
    PST_CNST_NODE	const_node;
    bool		legal_format = TRUE;
    i4		err_code;

    if (first_char == 'c')
    {
        if (!second_char)
        {
            const_node.pst_pmspec = PST_PMMAYBE;
            datatype = DB_TXT_TYPE;
            datalen = ADE_LEN_UNKNOWN;
	}
	else
	{
	    legal_format = FALSE;
        }
    }
    else if (first_char == 'i' && third_char == 0)
    {
        const_node.pst_pmspec  = PST_PMNOTUSED;
        datatype = DB_INT_TYPE;
        if (second_char == '1')
	    datalen = 1;
        else if (second_char == '2')
	    datalen = 2;
        else if (second_char == '4')
	    datalen = 4;
        else
	    legal_format = FALSE;
    }
    else if (first_char == 'f' && third_char == 0)
    {
        const_node.pst_pmspec  = PST_PMNOTUSED;
        datatype = DB_FLT_TYPE;
        if (second_char == '4')
	    datalen = 4;
        else if (second_char == '8')
            datalen = 8;
        else
	    legal_format = FALSE;
    }
    else
    {
        legal_format = FALSE;
    }

    if (legal_format)
    {
	/* Allocate the constant node */
	const_node.pst_tparmtype = PST_RQPARAMNO;
	const_node.pst_parm_no = parmno;
	const_node.pst_cqlang = sess_cb->pss_lang;
	const_node.pst_origtxt = (char *) NULL;

	status = pst_node(sess_cb, stream, (PST_QNODE *) NULL,
	    (PST_QNODE *) NULL, PST_CONST, (PTR) &const_node,
	    sizeof(const_node), datatype, (i2) 0, datalen,
	    (DB_ANYTYPE *) NULL, newnode, &psq_cb->psq_error, (i4) 0);

	/* Remember the highest parameter number */
	if (*highparm < parmno)
	    *highparm = parmno;
	return (status);
    }
    else
    {
        (VOID) psf_error(200L, 0L, PSF_USERERR,
	    &err_code, &psq_cb->psq_error, 1, STtrmwhite(format), format);
        return (E_DB_ERROR);
    }
}

/*{
** Name: pst_2adparm	- Add a parameter node to a query tree.
**
** Description:
**      This function adds a parameterized constant node to a query tree.
**
** Inputs:
**	sess_cb				session control block
**	psq_cb				query control block
**      stream                          The memory stream for allocating the
**					node
**	parmno				The parameter number
**	format				The format db data value 
**	newnode				Place to put pointer to new node
**	highparm			Pointer to the highest parm number found
**					so far.
**
** Outputs:
**      newnode                         Filled in with pointer to new node
**	highparm			Filled in with new parm number, if
**					higher than previous value.
**	psq_cb				query control block
**	    .psq_error			filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	25-nov-1986 (daved)
**          written
**	18-feb-88 (stec)
**	    Change initialization of pst_pmspec (must not be PST_PMMAYBE).
**	07-jun-88 (stec)
**	    Initialize pst_tparmtype in the constant node.
**	30-mar-89 (jrb)
**	    Changed declaration of datalen to an i4 (to match pst_node); added
**	    dataprec to pst_node parms for decimal project.
**	19-sep-89 (andre)
**	    Do NOT call adi_pm_encode() for SQL queries.
**	15-jun-92 (barbara)
**	    Sybil merge.  Added Star comments:
**	    28-feb-89 (andre)
**		Do not set db_length of VLUPs to ADE_LEN_UNKNOWN in STAR.
**	28-aug-92 (barbara)
**	    parmno is now the actual number that appears after the '$'
**	    ($#=...); it used to be #+1.  Consequently, pst_parm_no will
**	    be set to parmno+1.  'highparm' is still set to parmno, which is
**	    correct.  In pst_header, header->pst_numparm will be set to
**	    highparm+1.  Therefore if there are 3 repeat query parameters,
**	    highparm=2 (0 based), pst_numparm=3.
**      15-Jul-1993 (fred)
**          Add DB_VBYTE_TYPE as a VLUP type.  This mechanism needs to
**          be improved for FE support of OME.
**	04-Nov-2004 (gupsh01)
**	    Added support for nvarchar types.
*/
DB_STATUS
pst_2adparm(
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PSF_MSTREAM        *stream,
	i2		   parmno,
	DB_DATA_VALUE	   *format,
	PST_QNODE	   **newnode,
	i4		   *highparm)
{
    DB_STATUS		status;
    DB_DT_ID		datatype;
    i2			dataprec;
    i4			datalen;
    PST_CNST_NODE	const_node;
    i4		err_code;

    datatype = format->db_datatype;
    dataprec = format->db_prec;
    datalen  = format->db_length;

    /* Allocate the constant node */
    const_node.pst_tparmtype = PST_RQPARAMNO;
    const_node.pst_parm_no = parmno +1;

    /* will not look for QUEL pattern-matching characters for SQL queries */
    if (sess_cb->pss_lang == DB_SQL)
    {
        const_node.pst_pmspec = PST_PMNOTUSED;
    }
    else
    {
        switch (abs(datatype))
        {
	    case DB_CHR_TYPE:
	    case DB_TXT_TYPE:
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    {
	        i4	reqs = 0;
	        i4	lines = 0;
	        bool	pmchars;
	        ADF_CB	*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;

	        reqs |= (ADI_DO_BACKSLASH | ADI_DO_MOD_LEN);
	        if (sess_cb->pss_qualdepth)
	        {
		    reqs |= ADI_DO_PM;
	        }

	        status = adi_pm_encode(adf_scb, reqs, format, &lines, &pmchars);
	        sess_cb->pss_lineno += lines;

	        if (DB_FAILURE_MACRO(status))
	        {
		    if (   adf_scb->adf_errcb.ad_errcode == E_AD1015_BAD_RANGE
			|| adf_scb->adf_errcb.ad_errcode == 
			       E_AD3050_BAD_CHAR_IN_STRING
		       )
		    {
			psf_adf_error(&adf_scb->adf_errcb, 
				      &psq_cb->psq_error, sess_cb);
		    }
		    else
		    {
			(VOID) psf_error(E_PS0377_ADI_PM_ERR, 
			    adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
			    &err_code, &psq_cb->psq_error, 0);
		    }

		    return (status);
	        }

		const_node.pst_pmspec = (pmchars == TRUE) ? PST_PMUSED
							  : PST_PMNOTUSED;

	        break;
	    }
	    default:
	    {
	        const_node.pst_pmspec = PST_PMNOTUSED;
	        break;
	    }		
        }
    }
	
    const_node.pst_cqlang = sess_cb->pss_lang;
    const_node.pst_origtxt = (char *) NULL;

    status = pst_node(sess_cb, stream, (PST_QNODE *) NULL,
        (PST_QNODE *) NULL, PST_CONST, (PTR) &const_node,
        sizeof(const_node), datatype, dataprec, datalen,
        (DB_ANYTYPE *) format->db_data, newnode, &psq_cb->psq_error,
        (i4) 0);
	    
    if (DB_FAILURE_MACRO(status))
        return (status);

    /* Remember the highest parameter number */
    if (*highparm < parmno)
        *highparm = parmno;

    /*
    ** Catch VLUP (variable length user param). The value of
    ** db_length for these cases needs to be set as follows.
    */
    if ((~sess_cb->pss_distrib & DB_3_DDB_SESS) &&
        (abs(datatype) == DB_VCH_TYPE ||
	 abs(datatype) == DB_NVCHR_TYPE ||
	 abs(datatype) == DB_TXT_TYPE ||
	 abs(datatype) == DB_VBYTE_TYPE)
       )
    {
        (*newnode)->pst_sym.pst_dataval.db_length = ADE_LEN_UNKNOWN;
    }

    if (add_unorm(*newnode, sess_cb))
    {
	PST_OP_NODE opnode;

	opnode.pst_opno = ADI_UNORM_OP;
	opnode.pst_opmeta = PST_NOMETA;
	opnode.pst_pat_flags = AD_PAT_DOESNT_APPLY;
	opnode.pst_joinid = PST_NOJOIN;

	status = pst_node( sess_cb, &sess_cb->pss_ostream, *newnode, NULL,
	    PST_UOP, (char *)&opnode, sizeof(opnode), 
	    DB_NODT, (i2)0, (i4)0, (DB_ANYTYPE *)NULL, 
	    newnode, &psq_cb->psq_error, PSS_JOINID_PRESET);
    }

    return (status);
}

static bool
add_unorm (PST_QNODE *tree, PSS_SESBLK *sess_cb)
{
    i2		dt;
    bool	unorm;
    i4		utf8;
    ADF_CB	*adf_scb;
    DB_STATUS	status;
    DB_ERROR	error;

    adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
    utf8 = (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED);

    /*
    ** Note:
    ** When UNORM was being added in opc_cqual1()... we had to worry about
    ** fake  PST_CONST nodes constructed in opv_constnode to pass aggregate
    ** results...
    ** We don't have to worry about those fake constants here in PSF
    */

    dt = abs(tree->pst_sym.pst_dataval.db_datatype);

    /* always unorm NCHR, NVCHR */
    if (dt == DB_NCHR_TYPE || dt == DB_NVCHR_TYPE)
	unorm = TRUE;
    else if (!utf8)
	unorm = FALSE;
    else if (dt == DB_CHR_TYPE || dt == DB_CHA_TYPE || dt == DB_TXT_TYPE ||
		dt == DB_VCH_TYPE)
	unorm = TRUE;
    else
	unorm = FALSE;

    return (unorm);
}

