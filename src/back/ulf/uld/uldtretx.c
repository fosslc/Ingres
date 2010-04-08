/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <cs.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <ex.h>
#include    <st.h>
#include    <bt.h>
#include    <cv.h>
#include    <me.h>
#include    <ulm.h>
#include    <uld.h>
#include    <ulx.h>


/*}
** Name: ULDTRETX.C - state of a building a linked list of text blocks
**
** Description:
**      A linked list of text blocks is created when converting a query
**      tree into text.  This structure describes the state of the linked
**      list and can be used to save the state of partially built linked
**      list.
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**	20-aug-92 (rog)
**	    Removed uld_signal by changing all calls to uld_msignal.
**	31-aug-92 (rog)
**	    Prototype ULF.  Moved structure definitions into uld.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-jun-2001 (somsa01)
**	    In uld_prconst(), to save stack space, dynamically allocate
**	    space for 'convbuf'.
[@history_template@]...
*/


/*
** Forward static definitions
*/
static STATUS	uld_handler	( EX_ARGS *args );
static VOID	uld_printf	( ULD_TTCB *global, char *stringp );
static VOID	uld_flush	( ULD_TTCB *global, ULD_TSTRING *tstring );
static VOID	uld_intprintf	( ULD_TTCB *global, i4 intvalue );
static VOID	uld_prvar	( ULD_TTCB *global, PST_QNODE *varp );
static VOID	uld_prconst	( ULD_TTCB *global, PST_QNODE *constp );
static VOID	uld_prexpr	( ULD_TTCB *global, PST_QNODE *qnode );
static VOID	uld_savef	( ULD_TTCB *global, ULD_TSTRING **targetpp );
static VOID	uld_restoref	( ULD_TTCB *global );
static VOID	uld_target	( ULD_TTCB *global, ULD_TSTRING **targetpp,
				  char *delimiter, char *equals );
static VOID	uld_where	( ULD_TTCB *global );
static VOID	uld_quel	( ULD_TTCB *global );
static VOID	uld_from	( ULD_TTCB *global );
static VOID	uld_sql		( ULD_TTCB *global );


/*{
** Name: uld_handler	- exception handler for ULF tree to text
**
** Description:
**      This exception handler was declared to exit out of utility routine
**      in case of an error.
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-88 (seputis)
**          initial creation
**	13-aug-92 (rog)
**	    Modified to use the ulx_exception generic exception handler.
[@history_line@]...
*/
static STATUS
uld_handler( EX_ARGS *args )
{
    if (EXmatch(args->exarg_num, 1, EX_JNP_LJMP) != 1)
    {
	/* We did not get an internally signaled exception */
	(VOID) ulx_exception(args, DB_ULF_ID, E_UL0307_EXCEPTION, TRUE);
    }
    return (EXDECLARE);
}

/*{
** Name: uld_msignal	- this routine will report another facility's error
**
** Description:
**      This routine will report an error from another facility.
**      It will check the status and see the error
**      is recoverable.  The routine will not return if the status is fatal
**      but instead generate an internal exception and
**      exit via the exception handling mechanism.  
**
** Inputs:
**      global				ptr to state variable
**      error                           error code to report
**      facility                        facility's error code
**      status                          facility's return status
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**
** History:
**	08-apr-88 (seputis)
**          initial creation
**	20-aug-1992 (rog)
**	    Removed test for a successful status because we never call
**	    this function with successful statuses.
**	10-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
[@history_line@]...
*/
VOID
uld_msignal( ULD_TTCB *global, DB_ERRTYPE error, DB_STATUS status,
	     DB_ERRTYPE facility )
{
    global->uld_retstatus = status;	/* save status code */
    global->error->err_code = error;	/* error code */
    global->error->err_data = facility;	/* error code */
    ulx_rverror("Tree to Text", status, error, facility, ULE_LOG, 
	(DB_FACILITY)DB_ULF_ID); 

    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error); /* signal a long
                                                ** jump with error code */
}

/*{
** Name: uld_printf	- place string into buffer
**
** Description:
**      This routine will move a null terminated string into the buffer
**
** Inputs:
**      global                          ptr to global state variable
**      stringp                         ptr to string to copy
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_printf( ULD_TTCB *global, char *stringp )
{
    i4		len;
    DB_STATUS	status;

    if (stringp[0] && !stringp[1] && global->uld_primary.uld_remaining)
    {	/* special case single character write */
	global->uld_primary.uld_tempbuf[global->uld_primary.uld_offset++]=stringp[0];
	global->uld_primary.uld_remaining--;
	return;
    }
    len = STlength(stringp);
    while (len > 0)
    {
	if (global->uld_primary.uld_remaining >= len)
	{
	    MEcopy((PTR)stringp, len, 
		(PTR)&global->uld_primary.uld_tempbuf[global->uld_primary.uld_offset]);
	    global->uld_primary.uld_offset += len;
	    global->uld_primary.uld_remaining -= len;
	    break;
	}
	else
	{   /* get a new buffer */
	    i4		length_to_move;
	    char	*source;
	    if (global->uld_primary.uld_offset <= 0)
	    {	/* need to copy directly to allocated buffer since size 
                ** of string is larger than temporary buffer */
		length_to_move = len;
		source = stringp;
		len = 0;
	    }
	    else
	    {	/* move from temporary buffer */
		length_to_move = global->uld_primary.uld_offset;
		source = &global->uld_primary.uld_tempbuf[0];
	    }
	    global->ulmcb->ulm_psize = length_to_move + sizeof(ULD_TSTRING)
		- ULD_TSIZE + 1; /* get size of memory block
				    ** required */
	    status = ulm_palloc( global->ulmcb );
	    if (DB_FAILURE_MACRO(status))
		uld_msignal(global, E_UL0305_NO_MEMORY, status,
		    global->ulmcb->ulm_error.err_code);
	    *global->uld_primary.uld_tstring = (ULD_TSTRING *)global->ulmcb->ulm_pptr;
	    (*global->uld_primary.uld_tstring)->uld_next = NULL;
	    (*global->uld_primary.uld_tstring)->uld_length = length_to_move;
	    MEcopy((PTR)source, length_to_move,
		(PTR)&(*global->uld_primary.uld_tstring)->uld_text[0]);
	    (*global->uld_primary.uld_tstring)->uld_text[length_to_move ] = 0; /* null terminate the
				    ** string */
	    global->uld_primary.uld_tstring = &(*global->uld_primary.uld_tstring)->uld_next;
	    global->uld_primary.uld_offset = 0;	    /* reset buffer to be empty */
	    global->uld_primary.uld_remaining = global->uld_linelength;
	}
    }
}

/*{
** Name: uld_flush	- flush characters out of temporary buffer
**
** Description:
**      Move any remaining characters out of the temp buffer and into
**      heap memory.  This is not done if the "pretty" flag is selected
**      since then the text is used for human display.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_flush( ULD_TTCB *global, ULD_TSTRING *tstring )
{
    if (tstring && global->pretty)
    {	/* ignore flag and simply add the tstring data to the existing
        ** text */
	while (tstring)
	{
	    uld_printf(global, &tstring->uld_text[0]);
	    tstring = tstring->uld_next;
	}
	return;
    }
    if (global->uld_primary.uld_offset > 0)
    {   /* flush the remainder of the buffer */
	DB_STATUS	    status;
	global->ulmcb->ulm_psize = global->uld_primary.uld_offset 
	    + sizeof(ULD_TSTRING) - ULD_TSIZE + 1; /* get size of memory block
				** required */
	status = ulm_palloc( global->ulmcb );
	if (DB_FAILURE_MACRO(status))
	    uld_msignal(global, E_UL0305_NO_MEMORY, status,
		global->ulmcb->ulm_error.err_code);
	*global->uld_primary.uld_tstring = (ULD_TSTRING *)global->ulmcb->ulm_pptr;
	(*global->uld_primary.uld_tstring)->uld_next = NULL;
	(*global->uld_primary.uld_tstring)->uld_length = global->ulmcb->ulm_psize-1;
	MEcopy((PTR)&global->uld_primary.uld_tempbuf[0], global->uld_primary.uld_offset,
	    (PTR)&(*global->uld_primary.uld_tstring)->uld_text[0]);
	(*global->uld_primary.uld_tstring)->uld_text[global->uld_primary.uld_offset] = 0; /* null terminate the
				** string */
	global->uld_primary.uld_tstring = &(*global->uld_primary.uld_tstring)->uld_next;
	global->uld_primary.uld_offset = 0;	    /* reset buffer to be empty */
	global->uld_primary.uld_remaining = global->uld_linelength;
    }
    if (tstring)
    {
	*global->uld_primary.uld_tstring = tstring;    /* link into list of text 
					** fragments */
	(*global->uld_primary.uld_tstring)->uld_next = NULL;
	global->uld_primary.uld_tstring = &(*global->uld_primary.uld_tstring)->uld_next;
    }
}

/*{
** Name: uld_intprintf	- place integer into text stream
**
** Description:
**      This routine will place an integer into the text stream.
**
** Inputs:
**      global                          ptr to state variable
**      intvalue                        integer to place into text buffer
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
uld_intprintf( ULD_TTCB *global, i4 intvalue )
{
    char                outstring[20];

    CVla((i4)intvalue, &outstring[0]);
    uld_printf(global, &outstring[0]);
}

/*{
** Name: ULD_PRVAR  - add variable name to string
**
** Description:
**      Insert description of variable into text string
**
** Inputs:
**      global                          ptr to state variable
**      varp                            ptr to var node
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-mar-88 (seputis)
**          initial creation
*/
static VOID
uld_prvar( ULD_TTCB *global, PST_QNODE *varp )
{
    char	*temp_name;

    (*global->varnode)(global->handle,
	varp->pst_sym.pst_value.pst_s_var.pst_vno,
	varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
	&temp_name);		    /* call user supplied routine to get
				    ** identification information */
    uld_printf(global, temp_name);
}

/*{
** Name: uld_prconst	- convert constant to text
**
** Description:
**      This routine will convert the internal representation of a
**      constant to a text form and insert it into the query text
**      string
**
** Inputs:
**      global                          global state variable
**      constp                          ptr to constant node
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    places constant text form into query text string
**
** History:
**      2-apr-88 (seputis)
**          initial creation
**	11-jun-2001 (somsa01)
**	    To save stack space, dynamically allocate space for 'convbuf'.
**	16-jan-06 (hayke02)
**	    Compare worst_width to DB_MAXSTRING rather than sizeof(convbuf),
**	    now that convbuf is a pointer. This  change fixes bug 113941.
[@history_template@]...
*/
static VOID
uld_prconst( ULD_TTCB *global, PST_QNODE *constp )
{
    if (constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
    {	/* a repeat parameter exist */
	uld_printf(global, "{REPEAT ");
	uld_intprintf(global, (i4)constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no);
	uld_printf(global,"}");
    }
    else
    {
	i2                  default_width;
	i2                  worst_width;
	{
	    DB_STATUS           tmlen_status;
	    tmlen_status = adc_tmlen( global->adfcb, &constp->pst_sym.pst_dataval, 
		    &default_width, &worst_width);
	    if (DB_FAILURE_MACRO(tmlen_status))
		uld_msignal(global, E_UL0300_BAD_ADC_TMLEN, tmlen_status,
		    global->adfcb->adf_errcb.ad_errcode);
	}

	{
	    DB_STATUS           tmcvt_status;
	    i4			outlen;
	    DB_DATA_VALUE       dest_dt;
	    i4			datatype;
	    char		*convbuf;

	    convbuf = (char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);

	    if ((worst_width-1) > DB_MAXSTRING)
	    {
		MEfree((PTR)convbuf);
		uld_msignal(global, E_UL0302_BUFFER_OVERFLOW, E_DB_ERROR, 0);
	    }

	    dest_dt.db_length = default_width;
	    dest_dt.db_data = (PTR)&convbuf[0];
	    tmcvt_status= adc_tmcvt( global->adfcb, 
		&constp->pst_sym.pst_dataval, &dest_dt, &outlen); /* use the
					** terminal monitor conversion routines
					** to display the results */
	    if (DB_FAILURE_MACRO(tmcvt_status))
	    {
		MEfree((PTR)convbuf);
		uld_msignal(global, E_UL0301_BAD_ADC_TMCVT, tmcvt_status,
		    global->adfcb->adf_errcb.ad_errcode);
	    }

	    convbuf[outlen]=0;		/* null terminate the buffer before
					** printing */
	    /* FIXME - do we need to look for SQL NULLs */
	    if ((datatype = constp->pst_sym.pst_dataval.db_datatype) < 0 )
		datatype = -datatype;	/* nullable datatypes are negative*/
	    switch (datatype)
	    {	/* FIXME - need to check for language in order to construct
		** the correct string constant delimiters */
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		case DB_TXT_TYPE:
		case DB_VCH_TYPE:
		{
		    char	*quoted; /* quoted string delimiter */
		    switch (global->language)
		    {	/* find quoted string delimiter */
		    case DB_SQL:
		    case DB_NOLANG:
			quoted = "\'";
			break;
		    case DB_QUEL:
			quoted = "\"";
			break;
		    default:
			MEfree((PTR)convbuf);
			uld_msignal(global, E_UL030A_BAD_LANGUAGE, E_DB_ERROR,
				   0);
		    }
		    uld_printf(global, quoted);
		    uld_printf(global, &convbuf[0]);
		    uld_printf(global, quoted);
		    break;
		}
		default:
		    uld_printf(global, &convbuf[0]);
	    }

	    MEfree((PTR)convbuf);
	}
    }
}

/*{
** Name: uld_prexpr	- convert a query tree expression
**
** Description:
**      Accepts the following query tree nodes 
**
**	expr :		PST_VAR
**		|	expr BOP expr
**		|	expr Uop
**		|	AOP AGHEAD qual
**			  \
**			   expr
**		|	BYHEAD AGHEAD qual
**			/    \
**		tl_clause     AOP
**				\
**				 expr
**		|	PST_CONST
**
**
** Inputs:
**      global                          state variable
**      qnode                           query tree node to convert
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
*/
static VOID
uld_prexpr( ULD_TTCB *global, PST_QNODE *qnode )
{
    DB_STATUS	    status;
    ADI_OP_NAME	    adi_oname;

    switch (qnode->pst_sym.pst_type)
    {
    case PST_VAR:
	uld_prvar(global, qnode);
	break;
    case PST_OR:
    {
	uld_printf(global, "(");
	uld_prexpr(global, qnode->pst_left);
	uld_printf(global, ") OR (");
	uld_prexpr(global, qnode->pst_right);
	uld_printf(global, ")");
	break;
    }
    case PST_AND:
    {
	if (!qnode->pst_right
	    ||
	    (qnode->pst_right->pst_sym.pst_type == PST_QLEND)
	   )
	    uld_prexpr(global, qnode->pst_left); /* degenerate AND can occur
						** on the right side */
	else
	{
	    uld_printf(global, "(");
	    uld_prexpr(global, qnode->pst_left);
	    uld_printf(global, ") AND (");
	    uld_prexpr(global, qnode->pst_right);
	    uld_printf(global, ")");
	}
	break;
    }
    case PST_BOP:
    {
	status = adi_opname(global->adfcb, 
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_oname);
	if (DB_FAILURE_MACRO(status))
	    uld_msignal(global, E_UL0304_ADI_OPNAME, status, 
		global->adfcb->adf_errcb.ad_errcode);
	switch(qnode->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype)
        {   /* FIXME - need to deal with ADI_COERCION */
	case ADI_COMPARISON:
	case ADI_OPERATOR:
	{	/* infix operator found */
	    uld_printf(global, "(");
	    uld_prexpr(global, qnode->pst_left);
	    uld_printf(global, " ");	    /* add blanks for operators
                                            ** such as "like" */
	    uld_printf(global, (char *)&adi_oname);
	    uld_printf(global, " ");
	    uld_prexpr(global, qnode->pst_right);
	    uld_printf(global, ")");
	    break;
	}
	case ADI_NORM_FUNC:
	{	/* prefix operator */
	    uld_printf(global, (char *)&adi_oname);
	    uld_printf(global, "(");
	    uld_prexpr(global, qnode->pst_left);
	    uld_printf(global, ",");
	    uld_prexpr(global, qnode->pst_right);
	    uld_printf(global, ")");
	    break;
	}
	default:
	    uld_msignal(global, E_UL0303_BQN_BAD_QTREE_NODE, E_DB_ERROR, 0);
	}
	break;
    }
    case PST_UOP:
    {
	status = adi_opname(global->adfcb, 
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_oname);
	if (DB_FAILURE_MACRO(status))
	    uld_msignal(global, E_UL0304_ADI_OPNAME, status, 
		global->adfcb->adf_errcb.ad_errcode);
	uld_printf(global, (char *)&adi_oname);
	uld_printf(global, "(");
	uld_prexpr(global, qnode->pst_left);
	uld_printf(global, ")");
	break;
    }

    case PST_COP:
    {
	status = adi_opname(global->adfcb, 
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_oname);
	if (DB_FAILURE_MACRO(status))
	    uld_msignal(global, E_UL0304_ADI_OPNAME, status, 
		global->adfcb->adf_errcb.ad_errcode);
	uld_printf(global, (char *)&adi_oname);
	break;
    }

    case PST_CONST:
	uld_prconst(global, qnode);
	break;

    default:
	uld_msignal(global, E_UL0303_BQN_BAD_QTREE_NODE, E_DB_ERROR, 0);
    }
}

/*{
** Name: uld_target	- produce a target list
**
** Description:
**      create a target list text string from the query tree
[@comment_line@]...
**
** Inputs:
**      global                          state variable
**      delimiter                       pointer to string which is the delimiter
**                                      between target list elements
**      equals                          ptr to string which is the delimiter between
**					the element name and its expression
**
** Outputs:
**      targetpp                        NULL if expression list is not required
**                                      to be separated from the element names
**                                      - non-NULL if the expression list is
**                                      required to be separated from the
**                                      respective element names
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_savef( ULD_TTCB *global, ULD_TSTRING **targetpp )
{
    MEcopy((PTR)&global->uld_primary, sizeof(global->uld_primary),
	(PTR)&global->uld_secondary);	    /*save the previous text state */
    global->uld_primary.uld_offset = 0;
    global->uld_primary.uld_tstring = targetpp;
    *targetpp = NULL;
    global->uld_primary.uld_remaining = global->uld_linelength;
}

/*{
** Name: uld_target	- produce a target list
**
** Description:
**      create a target list text string from the query tree
[@comment_line@]...
**
** Inputs:
**      global                          state variable
**      delimiter                       pointer to string which is the delimiter
**                                      between target list elements
**      equals                          ptr to string which is the delimiter between
**					the element name and its expression
**
** Outputs:
**      targetpp                        NULL if expression list is not required
**                                      to be separated from the element names
**                                      - non-NULL if the expression list is
**                                      required to be separated from the
**                                      respective element names
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_restoref( ULD_TTCB *global )
{
    MEcopy((PTR)&global->uld_secondary, sizeof(global->uld_primary),
	(PTR)&global->uld_primary);	    /*save the previous text state */
}

/*{
** Name: uld_target	- produce a target list
**
** Description:
**      create a target list text string from the query tree
[@comment_line@]...
**
** Inputs:
**      global                          state variable
**      delimiter                       pointer to string which is the delimiter
**                                      between target list elements
**      equals                          ptr to string which is the delimiter between
**					the element name and its expression
**
** Outputs:
**      targetpp                        NULL if expression list is not required
**                                      to be separated from the element names
**                                      - non-NULL if the expression list is
**                                      required to be separated from the
**                                      respective element names
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_target( ULD_TTCB *global, ULD_TSTRING **targetpp, char *delimiter,
	    char *equals )
{
    PST_QNODE	    *resdomp;
    char	    *namep;
    bool	    first_time;
    bool	    use_list;

    if (targetpp)
	*targetpp = NULL;	    /* expression list is required to be separated, so
                                    ** initialize the beginning of the list */
    first_time = TRUE;
    if ((resdomp = global->uld_qroot->pst_left) && (resdomp->pst_sym.pst_type == PST_RESDOM))
    {
	use_list = TRUE;	    /* a non-empty target list exists */
    }
    else
    {	/* no target list exists but call resdom routine to get target
        ** lists dynamically built from equivalence class bit maps */
	use_list = FALSE;

	/* in order to dynamically construct target lists from equivalence
	** class bit maps, call this routine with a NULL resdom and get
	** a resdom and target name to describe the equivalence class */
	resdomp = NULL;
    }

    while ( (resdomp = (*global->resdom)(global->handle, &namep, resdomp))
	    && 
	    (resdomp->pst_sym.pst_type == PST_RESDOM)
          )
    {	/* target list exists so traverse down the list */
	bool	    print_delimiter;
	print_delimiter = first_time;
	if (first_time)
	    first_time = FALSE;	    /* do not print the delimiter the first time thru
                                    ** but all subsequent times so that we get - 
                                    ** element, element, element */
	else
	    uld_printf(global, delimiter);
	if (targetpp)
	{   /* the column names are required to be separate from the
	    ** expressions so create two parallel text streams to
            ** collect this information */
	    uld_printf(global, namep);	/* add name of target list element */
	    uld_savef(global, targetpp); /* save the context for the ULD text building
                                        ** routines so that the expression list can be
                                        ** build out of line */
	    if (print_delimiter)
		uld_printf(global, delimiter);
	    uld_prexpr(global, resdomp->pst_right); /* print expression associated with
					** resdom node */
	    uld_restoref(global);	/* restore the state for the next element
                                        ** name */
	}
	else
	{   /* column names are either not needed, or are adjacent to the expression
            ** which is the right hand side of the assignment */
	    if (equals)
	    {   /* "element_name = " required for the target list element */
		uld_printf(global, namep);	/* get name of target list element */
		uld_printf(global, equals);	/* an equals string */
	    }
	    uld_prexpr(global, resdomp->pst_right); /* print expression associated with
					    ** resdom node */
	}

	if (!use_list			/* if already looking at bitmaps */
	    ||
	    (	!(resdomp = resdomp->pst_left) /* or the end of the list is reached */
		|| 
		(resdomp->pst_sym.pst_type != PST_RESDOM)
            )
           )
	{
	    use_list = FALSE;		/* indicates the end of the resdom list */
	    resdomp = NULL;		/* this causes the resdom routine to look at the
                                        ** equivalence class bit maps if any exist */
	}
    }
}

/*{
** Name: uld_where	- generate a where clause if necessary
**
** Description:
**      [@comment_text@] 
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_where( ULD_TTCB *global )
{
    if (global->uld_qroot->pst_right
	&&
	(global->uld_qroot->pst_right->pst_sym.pst_type != PST_QLEND))
    {	/* non null qualification exists so generate where clause */
	uld_printf(global, " where ");
	uld_prexpr(global, global->uld_qroot->pst_right);
    }
}

/*{
** Name: uld_quel	- produce quel text
**
** Description:
**      This is main entry point for producing quel text 
**
** Inputs:
**      global                          ptr to state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
uld_quel( ULD_TTCB *global )
{
    char	    *comma;
    char	    *equals;

    comma = ",";
    equals = "=";
    switch (global->uld_mode)
    {
    case PSQ_DEFCURS:	/* define cursor statement - new */
    case PSQ_QRYDEFED:	/* define cursor statement */
    case PSQ_RETRIEVE:	/* retrieve statement */
    {
	
	uld_printf(global, "retrieve(");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_printf(global, ")");
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_RETINTO:	/* retrieve into statement */
    {
	uld_printf(global, "retrieve into ");
	uld_flush(global, global->uld_rnamep);
	uld_printf(global, "(");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_printf(global, ")");
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_APPEND:	/* append statement */
    {
	uld_printf(global, "append ");
	uld_flush(global, global->uld_rnamep);
	uld_printf(global, "(");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_printf(global, ")");
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_REPLACE:	/* replace statement */
    {
	uld_printf(global, "replace ");
	uld_flush(global, global->uld_rnamep);
	uld_printf(global, "(");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_printf(global, ")");
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_DELETE:	/* delete statement */
    {
	uld_printf(global, "delete ");
	uld_flush(global, global->uld_rnamep);
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_COPY:	/* copy statement */
    case PSQ_CREATE:	/* create statement */
    case PSQ_DESTROY:	/* destroy statement */
    case PSQ_HELP:	/* help statement (on relations) */
    case PSQ_INDEX:	/* index statement */
    case PSQ_MODIFY:	/* modify statement */
    case PSQ_PRINT:	/* print statement */
    case PSQ_RANGE:	/* range statement */
    case PSQ_SAVE:	/* save statement */

    default:
	uld_msignal(global, E_UL030B_BAD_QUERYMODE, E_DB_ERROR, 0);
    }
}

/*{
** Name: uld_from	- generate the from list
**
** Description:
**      Generate the from list of the query for SQL.  The table name
**	is isolated so that eventually it can be possibly replaced by
**      a unique temp table name.  This would be useful for shared 
**      query plans.
**
** Inputs:
**      global                          query translation state variable
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
[@history_line@]...
[@history_template@]...
*/
static VOID
uld_from( ULD_TTCB *global )
{
    bool	    first_time;
    ULD_TSTRING	    *uldtextp;	/* return a text block descriptor
				** which will be linked in directly
				** so that the caller can keep a
				** ptr to it */
    char	    *aliasp;	/* ptr to alais name if necessary
				** or NULL otherwise */

    first_time = TRUE;

    while (!(*global->uld_range)(global->handle, &uldtextp, &aliasp))
    {
	if (first_time)
	{   /* print from list only if one table exists */
	    uld_printf(global, " from ");
	    first_time = FALSE;
	}
	else
	    uld_printf(global, ",");

	uld_flush(global, uldtextp);    /* flush so that uldtextp can
					    ** be linked in */
	if (aliasp)
	    uld_printf(global, aliasp);
    }
}

/*{
** Name: uld_sql	- produce sql text
**
** Description:
**      This is main entry point for producing sql text 
**
** Inputs:
**      handle                          ptr to state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
**	28-feb-03 (inkdo01)
**	    Change C native bit operation to BT function for range table expansion.
[@history_template@]...
*/
static VOID
uld_sql( ULD_TTCB *global )
{
    char	*comma;
    char	*equals;

    comma = ",";
    equals = "=";
    switch (global->uld_mode)
    {
    case PSQ_RETRIEVE:	/* retrieve statement */
    {
	uld_printf(global, "select ");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_where (global);	    /* generate where clause if it exists */
	uld_from (global);	    /* generate from list for SQL */
	break;
    }
    case PSQ_APPEND:	/* append statement */
    {   /* insert into tablename 
        **         [(column {,column})] values (expression{,expression})]
	**	|  [subquery]
        */
	ULD_TSTRING	*expression_list;
	uld_printf(global, "insert into ");
	uld_flush(global, global->uld_rnamep);
	uld_printf(global, "(");
	uld_target(global, &expression_list, comma, NULL); /* generate target list */
	uld_printf(global, ")");
	if (BTcount((char *)&global->uld_qroot->pst_sym.pst_value.pst_s_root.pst_tvrm,
		sizeof(PST_J_MASK)) == 0)
	{   /* if no relations in the from list then only expressions need to be
            ** evaluated so a VALUE clause can be used */
            uld_printf(global, "VALUES (");
	    uld_flush(global, expression_list);
	    uld_printf(global, ")");
	}
	else
	{   /* generate the subquery since variables are referenced */
            uld_printf(global, " select ");
	    uld_flush(global, expression_list);
	    uld_where (global);	    /* generate where clause if it exists */
	}
	break;
    }
    case PSQ_REPLACE:	/* replace statement */
    {
	uld_printf(global, "update ");
	uld_flush(global, global->uld_rnamep);
	uld_printf(global, " set ");
	uld_target(global, (ULD_TSTRING **)NULL, comma, equals); /* generate target list */
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_DELETE:	/* delete statement */
    {
	uld_printf(global, "delete from ");
	uld_flush(global, global->uld_rnamep);
	uld_where (global);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_RETINTO:	/* retrieve into statement */
    case PSQ_COPY:	/* copy statement */
    case PSQ_CREATE:	/* create statement */
    case PSQ_DESTROY:	/* destroy statement */
    case PSQ_HELP:	/* help statement (on relations) */
    case PSQ_INDEX:	/* index statement */
    case PSQ_MODIFY:	/* modify statement */
    case PSQ_PRINT:	/* print statement */
    case PSQ_RANGE:	/* range statement */
    case PSQ_SAVE:	/* save statement */
    case PSQ_DEFCURS:	/* define cursor statement - new */
    case PSQ_QRYDEFED:	/* define cursor statement */

    default:
	uld_msignal(global, E_UL030B_BAD_QUERYMODE, E_DB_ERROR, 0);
    }
}

/*{
** Name: ULD_TREE_TO_TEXT	- routine to convert query tree to text
**
** Description:
**      This routine will convert a query tree to a text string
**      and is written to be a utility routine which can be called by 
**      any facility i.e. (PSF or OPF) 
[@comment_line@]...
**
** Inputs:
**      handle                          handle passed to all caller defined
**                                      defined routines
**      ulmcb				ptr to ULM control block used to do
**                                      ulm_palloc's for any internally defined
**                                      memory.
**      linelength                      max length of one line to be returned
**                                      0 if max length of ULD_TSTRING is to be
**                                      used.
**      pretty                          TRUE - if non-machine use anticipated
**                                      so that indentation etc. should be
**                                      used if possible
**      qroot				ptr to a query tree to be converted
**                                      - if a root node then an entire statement
**                                      is generated, otherwise an expression is
**                                      generated 
**      varnode                         ptr to a routine which accepts a varnode
**                                      and handle, and returns the variable name
**                                      and attribute name.
**                                      (*varnode)(handle, varno, atno, namepp)
**                                          PTR   handle;
**                                          i4    varno;
**                                          i4    atno;
**                                          char  **namepp;
**                                      handle - is the user supplied handle
**                                      
**      conjunct                        routine which returns the next conjunct to
**                                      add to the qualification, otherwise return
**                                      EOF, used to traverse bitmaps of boolean
**                                      factors
**                                      bool (*conjunct)(handle, qtreepp)
**                                           PTR	handle;
**                                           PST_QNODE  **qtreepp;
**                                      bool must return TRUE if no more 
**                                      conjuncts exist
**                                      handle - is the caller's supplied handle
**                                      on startup
**                                      qtreepp - is a location in which the
**                                      root of the next conjunct is returned
**                                      to the text building routine
**      resdom                          routine to add the next resdom to the
**                                      target list, used to traverse bitmaps
**                                      of equivalence classes at intermediate
**                                      nodes
**
** Outputs:
**      tstring                         ptr to text string list null terminated
**                                      strings
**	Returns:
**	    DB_STATUS - result of conversion
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
DB_STATUS
uld_tree_to_text( PTR handle, i4  qmode, ULD_TSTRING *rnamep, ADF_CB *adfcb,
		  ULM_RCB *ulmcb, ULD_LENGTH linelength, bool pretty,
		  PST_QNODE *qroot, VOID (*varnode)(), bool (*tablename)(),
		  bool (*conjunct)(), PST_QNODE *(*resdom)(),
		  ULD_TSTRING **tstring, DB_ERROR *error, DB_LANG language )
{
    ULD_TTCB	    global;		/* state variable for tree to text
                                        ** conversion */
    EX_CONTEXT     excontext;	    /* context block for exception handler*/

    global.error = error;
    error->err_code = E_UL0000_OK;
    error->err_data = 0;
    global.uld_retstatus = E_DB_OK;

    if ( EXdeclare(uld_handler, &excontext) != OK)
    {
	/* - this exception handler was established to catch any access
	** violations from invalid trees, if the error code has been
	** filled in then a normal error exit is assumed, otherwise
	** this point should only have been reached if an
        ** unexpected exception such as an access violation
        ** occurred
        */
	EXdelete();
	if (error->err_code == E_UL0000_OK)
	    global.uld_retstatus = E_DB_SEVERE; /* an exception
				** was processed and it was not generated by
                                ** an internal error signal */
	return(global.uld_retstatus);
    }
    global.handle = handle;
    global.uld_mode = qmode;
    global.uld_rnamep = rnamep;
    global.adfcb = adfcb;
    global.ulmcb = ulmcb;

    if ((linelength > ULD_TSIZE) || (linelength <= 0))
	global.uld_linelength = ULD_TSIZE;
    else
	global.uld_linelength = linelength;

    global.pretty = pretty;

    global.uld_qroot = qroot;
    global.varnode = varnode;
    global.uld_range = tablename;
    global.conjunct = conjunct;
    global.resdom = resdom;
    global.language = language;
    switch(language)
    {
    case DB_QUEL:
	global.is_quel = TRUE;
	global.is_sql = FALSE;
	break;
    case DB_SQL:
	global.is_quel = FALSE;
	global.is_sql = TRUE;
	break;
    default:
	uld_msignal(&global, E_UL030A_BAD_LANGUAGE, E_DB_ERROR, 0);
    }

    global.uld_primary.uld_tstring = tstring;
    *tstring = NULL;

    /* initialize temp buffer state */
    global.uld_primary.uld_remaining = global.uld_linelength;
    global.uld_primary.uld_offset = 0;    

    if (qroot->pst_sym.pst_type == PST_ROOT)
    {	/* query tree root was supplied so translate entire query */
	switch(language)
	{
	case DB_QUEL:
	    uld_quel(&global);		/* return QUEL text */
	    break;
	case DB_SQL:
	    uld_sql(&global);		/* return SQL text */
	    break;
	default:
	    uld_msignal(&global, E_UL030A_BAD_LANGUAGE, E_DB_ERROR, 0);
	}
    }
    else
    {	/* only an expression was given */
	bool		end_of_list;

	uld_printf(&global, "(");
	do
	{
	    uld_prexpr(&global, qroot);
	    if (conjunct)
		end_of_list = (*conjunct)(handle, &qroot); /* while 
					    ** end of list is not
					    ** read continue adding conjuncts from
					    ** caller */
	    else
		end_of_list = TRUE;
	    if (!end_of_list)
		uld_printf(&global, ") AND (");
	    
	}
	while (!end_of_list);
	uld_printf(&global, ")");
    }

    uld_flush(&global, (ULD_TSTRING *)NULL); /* move all data out of temporary buffer and
					    ** place into linked list */
    EXdelete();                             /* delete exception handler */
    return(global.uld_retstatus);			    
}
