
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"

/**
** Name:	fmmulti.c	This file contains the routines for doing
**				multi-line formatting.
**
** Description:
**	This file defines:
**
** 	fmt_workspace		Compute size of workspace needed.
**
** 	IIfmt_init		Initialize workspace prior to doing
**				multi-line formatting.
**
** 	fmt_multi		Format the next line of multi line output.
**
** History:
**	18-dec-86 (grant)	implemented.
**	28-aug-87 (rdesmond)	resets global F_inquote in fmt_init.
**	20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje' .
**	30-apr-90 (cmr) Made F_inquote a nat.
**      26-sep-91 (jillb/jroy--DGC) (from 6.3)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Nov-2004 (komve01)	Set the fmt_cont_line attribute if cont_lint is set.
**							For Bug 113498, for copy.in to work correctly for views.
**	30-Nov-2004 (komve01)
**            Undo change# 473469 for Bug#113498.
**       18-apr-2006 (stial01)
**          Init new fields in FM_WORKSPACE (b115704)
**/

/* # define's */

/* static's */

/*{
** Name:	fmt_workspace	- Compute size of workspace needed.
**
** Description:
**	This routine is used to compute the size of a workspace needed
**	for doing multi-line output with a multi-line format.  The workspace
**	is simply a temporary area for the formatting library to use
**	while it is doing multi-line output.
**
**	The workspace is passed to IIfmt_init and fmt_multi using a PTR.
**	It must point to an area of memory of the size returned by this routine.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The FMT to find the workspace size for.
**
**	type		A DB_DATA_VALUE containing the type that will be
**			displayed with the format.
**
**		.db_datatype	The ADF datatype id for the type.
**
**		.db_length	The length of the type.
**
**	length		Must point to a nat.
**
** Outputs:
**	length		Will be set to the length of the workspace needed.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM6009_NOT_MULTI_LINE_FMT	The given fmt parameter is not
**						a multi-line format.
**
** History:
**	18-dec-86 (grant)	implemented.
*/

STATUS
fmt_workspace(cb, fmt, type, length)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*type;
i4		*length;
{
    i4	    	     rows;
    i4	    	     cols;
    STATUS	     status;

    if (fmt == NULL || length == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    status = fmt_size(cb, fmt, type, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    if (rows == 1)
    {
	return afe_error(cb, E_FM6009_NOT_MULTI_LINE_FMT, 6,
			 sizeof(fmt->fmt_type), (PTR)&(fmt->fmt_type),
			 sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width),
			 sizeof(fmt->fmt_prec), (PTR)&(fmt->fmt_prec));
    }

    *length = sizeof(FM_WORKSPACE);

    return OK;
}

/*{
** Name:	IIfmt_init	- Initialize workspace prior to doing
**				  multi-line formatting.
**
** Description:
**	This routine initializes a workspace need for multi-line output
**	prior to the start of multi-line formatting.   The value passed
**	here must be the same value that will be used in the calls to
**	fmt_multi.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The FMT with which to format the value.
**
**	value		The value to format.
**
**	workspace	A workspace of the size given by fmt_workspace.
**
** Outputs:
**	workspace	Will be ready for calls to fmt_multi.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM6009_NOT_MULTI_LINE_FMT	The given fmt parameter is not
**						a multi-line format.
**
** History:
**	18-dec-86 (grant)	implemented.
**	28-aug-87 (rdesmond)	resets global F_inquote for use in f_colfmt.
**	9/26/89 (elein) B7152
**		Added support for multi line date format, ie dates
**		formatted with cn.w.  
**	16-mar-93 (DonC) Bug 45483
**		If form had a multi-line template and the db_datatype was
**		-3 (nullable date), not 3 (non-nullable date), a segvio 
**		would occur. By using abs(db_datatype), function now works
**		the same with both nullable and non-nullable dates. Before
**		a -3 would cause code used for an internal "unloaddb" data-
**		type to be invoked with tragic consequences.
**	11-sep-06 (gupsh01)
**		Added support for ANSI date/time types.
*/

STATUS
IIfmt_init(cb, fmt, value, workspace)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*value;
PTR		workspace;
{
    i4			rows;
    i4			cols;
    STATUS		status;
    FM_WORKSPACE	*ws;
    i4			length;
    i4			isnull;
static    DB_DATA_VALUE	datedbv;
static    AFE_DCL_TXT_MACRO(MAX_FMTSTR)       datebuf;
static    DB_DATA_VALUE	seclbldbv;
static    AFE_DCL_TXT_MACRO(SL_MAX_EXTERNAL)    seclblbuf;

    /* initialize datedbv, once only */
    if( datedbv.db_length != MAX_FMTSTR)
    {
    	datedbv.db_datatype = DB_LTXT_TYPE;
    	datedbv.db_length = sizeof(datebuf);
    	datedbv.db_data = (PTR) &datebuf;
    }
    /* initialize seclbldbv, once only */
    if( seclbldbv.db_length != SL_MAX_EXTERNAL)
    {
    	seclbldbv.db_datatype = DB_LTXT_TYPE;
    	seclbldbv.db_length = sizeof(seclblbuf);
    	seclbldbv.db_data = (PTR) &seclblbuf;
    }

    if (fmt == NULL || value == NULL || workspace == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    status = fmt_size(cb, fmt, value, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    if (rows == 1)
    {
	return afe_error(cb, E_FM6009_NOT_MULTI_LINE_FMT, 6,
			 sizeof(fmt->fmt_type), (PTR)&(fmt->fmt_type),
			 sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width),
			 sizeof(fmt->fmt_prec), (PTR)&(fmt->fmt_prec));
    }

    ws = (FM_WORKSPACE *) workspace;
    ws->fmws_left = fmt->fmt_width;
    ws->fmws_inquote = F_NOQ;
    ws->fmws_incomment = FALSE;
    ws->fmws_multi_buf = NULL;
    ws->fmws_multi_size = DB_MAXSTRING + FMT_OVFLOW;
    ws->fmws_cprev = NULL;

    status = adc_isnull(cb, value, &isnull);
    if (status != OK)
    {
	return status;
    }
    else if (isnull)
    {
	/* put null string in display */
	length = cb->adf_nullstr.nlst_length;

	if (fmt->fmt_width > 0 && length > fmt->fmt_width)
	{
	    ws->fmws_count = 0;
	}
	else
	{
	    ws->fmws_count = length;
	    ws->fmws_text = (u_char *)(cb->adf_nullstr.nlst_string);
	}

	return OK;
    }

    /* 
    ** DonC: Use abs() of db_datatype, to do otherwise causes 
    ** ws->fmws_text to be a null pointer causing a segvio when
    ** dates are forced through case F_CU below. (Bug 45483)
    */
    if(( abs(value->db_datatype) == DB_DTE_TYPE) ||
       ( abs(value->db_datatype) == DB_ADTE_TYPE) ||
       ( abs(value->db_datatype) == DB_TMWO_TYPE) ||
       ( abs(value->db_datatype) == DB_TMW_TYPE) ||
       ( abs(value->db_datatype) == DB_TME_TYPE) ||
       ( abs(value->db_datatype) == DB_TSWO_TYPE) ||
       ( abs(value->db_datatype) == DB_TSW_TYPE) ||
       ( abs(value->db_datatype) == DB_TSTMP_TYPE) ||
       ( abs(value->db_datatype) == DB_INDS_TYPE) ||
       ( abs(value->db_datatype) == DB_INYM_TYPE))
    {
	status = f_fmtdate(cb, value, fmt, &datedbv);
	if (status != OK)
        {
		return status;
	}
    	f_strlen(&datedbv, &(ws->fmws_text), &length);
    }
    else
    {
    	f_strlen(value, &(ws->fmws_text), &length);
    }

    switch(fmt->fmt_type)
    {
    case F_C:
    case F_CJ:
    case F_CJE:
    case F_CF:
    case F_CFE:
    /* internal format used by unloaddb() */
    case F_CU:
	/* get the length of the string without trailing blanks */
	ws->fmws_count = f_lentrmwhite(ws->fmws_text, length);
	break;

    case F_T:
	/* all chars count */
	ws->fmws_count = length;
	break;
    }


    return OK;
}

/*{
** Name:	fmt_multi	- Format the next line of multi line output.
**
** Description:
**	This routine takes a multi-line FMT, a value, and a workspace
**	and formats the next line of the value.  The FMT must
**	be a multi-line output in order for this routine to work.  Also
**	the workspace must have been initialized with the same value by
**	a call to IIfmt_init.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The FMT with which to format the value.
**
**	value		The value to format.
**
**	workspace	A workspace of the size given by fmt_workspace.
**
**	display		The place to put the formatted value.
**		.db_datatype	Must be DB_LTXT_TYPE.
**		.db_length	The maximum length of this display value.
**		.db_data	Must point to where to put the display value.
**
**	more		Points to a bool.
**
** Outputs:
**	display	
**		.db_data	Will point to the formatted value.
**
**	more			Contains TRUE if there was another line
**				left to format.
**				Contains FALSE if there are no more lines
**				to format.
**
**	mark_byte1		TRUE to insert a special marker when splitting
**				a 2-byte char.  FALSE for a space.
**
** Side Effects:
**	workspace will change.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM6003_DISPLAY_NOT_LONGTEXT	The datatype of the display
**						parameter is not LONGTEXT.
**		E_FM6004_DISPLAY_LEN_TOO_SHORT	The length of the display
**						parameter is too short for the
**						format width.
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM6009_NOT_MULTI_LINE_FMT	The given fmt parameter is not
**						a multi-line format.
**		E_FM6002_ZERO_WIDTH		Width of the format is zero.
**
** History:
**	18-dec-86 (grant)	implemented.
**	27-mar-87 (bab)		Reverse the formatted string as the last
**				step before returning.
**	27-jul-89 (cmr)		Add a flag to indicate whether a marker or a
**				space is desired when splitting a 2-byte char.
**      3/21/91 (elein)         (b35574) Add parameter for FRS, boxed messages
**                              which need to change control characters to
**                              spaces in order to display boxed messages
**                              correctly.  For internal use only.
**	16-Nov-2004 (komve01)	Set the fmt_cont_line attribute if cont_lint is set.
**							For Bug# 113498, for copy.in to work correctly for views.
**	30-Nov-2004 (komve01)
**            Undo change# 473469 for Bug#113498.
*/

STATUS
fmt_multi(cb, fmt, value, workspace, display, more, mark_byte1, frs_boxmsg)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*value;
PTR		workspace;
DB_DATA_VALUE	*display;
bool		*more;
bool		mark_byte1;
bool		frs_boxmsg;
{
    STATUS  status;
    i4	    rows;
    i4      cols;
    bool    reverse;
    DB_TEXT_STRING	*disp;
    FM_WORKSPACE	*ws = (FM_WORKSPACE *)workspace;

    if (fmt == NULL || value == NULL || workspace == NULL || display == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    if (display->db_datatype != DB_LTXT_TYPE)
    {
	return afe_error(cb, E_FM6003_DISPLAY_NOT_LONGTEXT, 2,
			 sizeof(display->db_datatype),
			 (PTR)&(display->db_datatype));
    }

    status = fmt_size(cb, fmt, value, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    if (rows == 1)
    {
	return afe_error(cb, E_FM6009_NOT_MULTI_LINE_FMT, 6,
			 sizeof(fmt->fmt_type), (PTR)&(fmt->fmt_type),
			 sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width),
			 sizeof(fmt->fmt_prec), (PTR)&(fmt->fmt_prec));
    }

    if (display->db_length-DB_CNTSIZE < cols)
    {
	return afe_error(cb, E_FM6004_DISPLAY_LEN_TOO_SHORT, 4,
			 sizeof(display->db_length), (PTR)&(display->db_length),
			 sizeof(cols), (PTR)&cols);
    }

    if (cols <= 0)
    {
	return afe_error(cb, E_FM6002_ZERO_WIDTH, 0);
    }

    disp = (DB_TEXT_STRING *)display->db_data;

    if ((fmt->fmt_width == 0 && ((FM_WORKSPACE *)workspace)->fmws_count > 0) ||
        (fmt->fmt_width > 0 && ((FM_WORKSPACE *)workspace)->fmws_left > 0))
    {   /* format the next line */
	*more = TRUE;
        f_colfmt((FM_WORKSPACE *)workspace, disp, cols, fmt->fmt_type,
		mark_byte1, frs_boxmsg);
    }
    else
    {
	*more = FALSE;

	/* Free fmws_multi_buf if allocated in fcolfmt */
	if (ws->fmws_multi_buf)
	{
	    MEfree((PTR)ws->fmws_multi_buf);
	    ws->fmws_multi_buf = NULL;
	}
    }

    switch(fmt->fmt_sign)
    {
    case FS_PLUS:
	f_strrgt(disp->db_t_text, cols, disp->db_t_text, cols);
	break;

    case FS_MINUS:
	f_strlft(disp->db_t_text, cols, disp->db_t_text, cols);
	break;

    case FS_STAR:
	f_strctr(disp->db_t_text, cols, disp->db_t_text, cols);
	break;
    }

    status = fmt_isreversed(cb, fmt, &reverse);
    if (status != OK)
    {
	return status;
    }

    if (reverse)
    {
	f_revrsbuf(cols, TRUE, ((DB_TEXT_STRING *)display->db_data)->db_t_text);
    }

    return OK;
}
