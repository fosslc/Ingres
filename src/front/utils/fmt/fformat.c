/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h> 
# include	<multi.h>
# include	<cm.h>
# include	<cv.h>
# include	<st.h>
# include	"format.h"

/*
**   F_FORMAT - format a numeric value acording to the supplied format
**		structure.  The numeric value may be an integer, a decimal,
**		or a float.
**
**	Parameters:
**		cb	- Pointer to an ADF_CB control block for ADF
**			  routine call(s).
**		value	- Pointer to a DB_DATA_VALUE describing the numeric
**			  value to be formatted.
**		fmt	- Pointer to an FMT structure detailing the formatting
**			  requirements.
**		display	- Pointer to a DB_DATA_VALUE to contain the result
**			  formatted value (DB_TEXT_STRING).
**
**	Returns:
**		OK	if all went well.
**
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Calls:
**		CVfa, f_nt.
**
**	History:
**		6/18/81 (ps) - written.
**		11/5/82 (ps) - add reference to fmt->fmt_sign.
**		3/22/83 (mmm) - did compat lib conversion
**				-changed strcpy calls to STcopy
**				-changed strcat calls to STcat
**				-changed ftoa call to CVfa
**				-changed zero_mem call to MEfill
**		7/2/85 (jrc) -  Changes for international.  Call CVfa
**				correctly.
**	18-dec-86 (grant)	changed parameters to DB_DATA_VALUEs,
**				display is now text string instead of
**				null-terminated one. Added cb parameter
**				for call to adc_cvinto.
**	12/23/87 (dkh) - Performance changes.
**	16-may-88 (bruceb)
**		Deal with OK/non-OK returns from CVfa rather than
**		with a specific bad status.
**	11-oct-1993 (rdrane)
**		Extensive modifications to preserve the precision and value
**		of the input numeric value.  We no longer unconditionally
**		convert to float prior to formatting, but rather use the
**		appropriate CV routine for integer, decimal, and float.
**		Note that this results in changed parameterization for
**		f_nt().  Obsolete local variables were removed, "B"
**		formatting was prioritized, and copy loops were optimized.
**	7-dec-1993 (rdrane)
**		Extract code related to conversion to ASCII into a separate
**		routine, since this is also needed by fnt_modify() for input
**		masking.
**	7-mar-1994 (rdrane)
**		Remove useless code relating to format_char - it's now
**		handled entirely in f_cvta().
**	22-mar-94 (ricka)
**		Removed "include <stdio.h>", left in by accident.
**	1-apr-1994 (rdrane)
**		Add is_fract to indicate a value which has no integer portion
**		(b61977)
**      30-Mar-1995 (liibi01)
**              Fix bug 67753, qbf fails when try to append to a decimal field
**              with precision equal to scale. QBF quits the os and the xterm
**              freezes. 
**	18-dec-1996 (angusm)
**		Bug 76711: display of decimal data field with default
**		formatting fails if II_DECIMAL=','
**	22-dec-1998 (crido01) SIR 94437
**		Modify F_CVTA to roundup instead of truncate if asked.
**      5-feb-1999 (rodjo04)
**		Modified above fix to check to see if the pointer is a 
**		valid pointer address before checking for roundup.
**              It might be a char value instead of an address.
**	24-mar-1999 (crido01)
**		Fix to last fix.  Look at fmt_ccsize, it carries the number
**		of elements in the fmt_ccarray, which is 1 if the tool does
**		not use format strings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-nov-2002 (rigka01) bug# 108058
**          For CR format, do not strip rightmost 2 blanks, otherwise decimal
**	    points don't align 
**      30-Jan-2003 (drivi01)
**          Increased size stored in field_wid field being passed into 
**          CVpka to account for decimal point and a sign.  Replaced 
**          fmt->fmt_width with dec_prec+fmt->fmt_sign+(dec_scale>0).
**      03-Feb-2003 (hanch04)
**          Back out last change, breaks other part of the front end.
**	06-Oct-2009 (maspa05) bug 122690
**	    Allocate ff_dec_dbv based on DB_MAX_DECPREC not hard-coded 16
**	    decimals can be bigger than 16 now
*/

STATUS
f_format(cb, value, fmt, display)
ADF_CB		*cb;
DB_DATA_VALUE	*value;
FMT		*fmt;
DB_DATA_VALUE	*display;
{
	i4		i;
	i4		width;
	i4		len;
	STATUS		status;
	DB_TEXT_STRING	*disp;
	u_char		*text;
	u_char		*b_ptr;
	u_char		*d_ptr;
	bool		is_zero;
	bool		is_neg;
	bool		is_fract;
	u_char		buffer[(MAX_FMTSTR + 1)];


	/*
	** Set-up our working output string and international
	** decimal symbol.  Set-up "quick" reference variables
	** so we don't have to resolve into the data structures
	** all the time.
	*/
	width = fmt->fmt_width;
	disp = (DB_TEXT_STRING *)display->db_data;
	disp->db_t_count = width;
	text = disp->db_t_text;

	/*
	** Screen BLANKING format here, so we don't have to go through
	** any to-ASCII conversions, since it doesn't matter what type
	** or value we're dealing with.
	*/
	if  (fmt->fmt_type == F_B)
	{
		MEfill((u_i2) width, FC_BLANK, text);
		return(OK);
	}

	/*
	** Determine the type of numeric data, and convert it to the
	** corresponding type of ASCII string representation.  Establish
	** the format character, and modify as required.  Note that only
	** FLOAT types which are not using a numeric template will be other
	** than format 'f'.
	*/
	buffer[0] = EOS;
	status = f_cvta(cb,value,fmt,&buffer[0],&is_zero,&is_neg,&is_fract);
	len = STlength((char *)&buffer[0]);

	switch(fmt->fmt_type)
	{
	case(F_NT):
		if  (status != OK)
		{
			break;
		}
		status = f_nt(cb, buffer, fmt, disp, is_zero, is_neg, is_fract);
		if (status == OK)
		{
			switch(fmt->fmt_sign)
			{
			case(FS_MINUS):
				f_strlft(text, width, text, width);
				break;

			case(FS_PLUS):
			{
				char *t_ptr;
			        /* need to replace with doublebyte-compat */	
				t_ptr=&fmt->fmt_var.fmt_template[fmt->fmt_width];
				CMprev(t_ptr,fmt->fmt_var.fmt_template);
			        if ((*t_ptr=='r') ||  (*t_ptr =='R'))
				{
				    CMprev(t_ptr,fmt->fmt_var.fmt_template);
			            if ((*t_ptr=='c') ||  (*t_ptr =='C'))
			              /* last two characters in template is
				      ** cr or CR so leave two blanks for 
				      ** decimal place alignment
				      */ 
				      f_strrgt(text, width-2, text, width-2);
				}
				else    
				  f_strrgt(text, width, text, width);
			        
				break;
			}

			case(FS_STAR):
				f_strctr(text, width, text, width);
				break;

			case(FS_NONE):
				break;
			}
		}
		break;

	case(F_I):
	case(F_F):
	case(F_E):
	case(F_G):
	case(F_N):
		if  (status != OK)
		{
			break;
		}
		switch(fmt->fmt_sign)
		{
		case(FS_MINUS):
			f_strlft(&buffer[0], len, text, width);
			break;

		case(FS_PLUS):
			f_strrgt(&buffer[0], len, text, width);
			break;

		case(FS_STAR):
			f_strctr(&buffer[0], len, text, width);
			break;

		case(FS_NONE):
			if (fmt->fmt_type != F_G) 
			{
			    f_strrgt(&buffer[0], len, text, width);
			}
			else
			{
				MEfill((u_i2)(width - len), FC_FILL, text);
				d_ptr = text + (width - len);
				b_ptr = &buffer[0];
				while (--len >= 0)
				{
					CMcpyinc(b_ptr, d_ptr);
				}
			}
			break;
		}
	}

	/*
	** If error found, fill result field with asterisks
	*/
	if (status != OK)
	{
		MEfill((u_i2) width, FC_ERROR, text);
		return(afe_error(cb, status, 0));
	}

	return(OK);
}


/*
**   F_CVTA - Convert a numeric DB_DATA_VALUE to an ASCII string.
**
**	Parameters:
**		cb	- Pointer to an ADF_CB to use for ADF calls.
**			  Used for DECIMAL conversions only.
**		value	- Pointer to a DB_DATA_VALUE describing the numeric
**			  value to be converted.
**		fmt	- Pointer to the associated FMT structure.
**			  Not used for INTEGER conversions.
**		display	- Pointer to a character buffer to contain the result
**			  string (should be at least MAX_FMTSTR in size).
**		is_zero	- Pointer to a bool to indicate if value is exactly
**			  zero.
**		is_neg	- Pointer to a bool to indicate if value is negative.
**		is_fract- Pointer to a bool to indicate if value is has no
**			  decimal portion, i.e., -1 < x < 1.
**
**	Returns:
**		OK	if all went well.
**		FAIL	if something went wrong.
**
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	History:
**	7-dec-1993 (rdrane)
**		Extracted from f_format() code.
**	10-feb-1994 (rdrane)
**		Handle MONEY types separately - they're not exactly
**		like FLOATs (b59425).
**	7-mar-1994 (rdrane)
**		Revert to converting DB_INT_TYPE to DB_FLOAT_TYPE so we
**		don't break integer value/exponential format combinations,
**		e.g.,
**			 int_val (e21)
**		(bug 59921).  Also, convert all floats to f8 - we were losing
**		precision.  This lets us eliminate d_len and f_val.
**	1-apr-1994 (rdrane)
**		Added is_fract to parameter list and logic to set it.  Note
**		the permissible resultant combinations:
**
**			      x = 0   x < -1  -1 < x < 1; x != 0
**		 is_zero	T	F		F
**		 is_neg		F	T		T
**		 is_fract	F	F		T
**
**		(b61977)
**	9-apr-1994 (rdrane)
**		Increase size of ff_flt_dbv and ff_dec_dbv buffers to prevent
**		memory corruptrion due to overflow when NULLable and holding
**		the maximum value value.
static  char
*/

STATUS
f_cvta(cb,value,fmt,display,is_zero,is_neg,is_fract)
	ADF_CB		*cb;
	DB_DATA_VALUE	*value;
	FMT		*fmt;
	char		*display;
	bool		*is_zero;
	bool		*is_neg;
	bool		*is_fract;
{
	i4		i;
	i4		dec_prec;
	i4		dec_scale;
	i4		len;
	STATUS		status;
	i2		f_len;
	i4		i_val;
	char		format_char;
	i2		dec_rounded = 0;
				/*
				** Allow for slop-over, since ADF assumes that
				** the NULL indicator byte immediately follows
				** the data.  We use an array since the coding
				** standards disallow UNIONs, we need to
				** guarantee alignment, and it's cheaper than
				** a dynamic allocation.
				*/
static	f8		ff_flt_dbv[2] = {0.00,0.00};
static	char		ff_dec_dbv[(DB_MAX_DECLEN + 1)];
static	DB_DATA_VALUE	ff_flt_dbdv =
			{
               			(PTR)&ff_flt_dbv[0],
               			8,
				DB_FLT_TYPE,
               			0
			};
static	DB_DATA_VALUE	ff_dec_dbdv =
			{
               			(PTR)&ff_dec_dbv[0],
               			DB_PREC_TO_LEN_MACRO(1),
				DB_DEC_TYPE,
               			DB_PS_ENCODE_MACRO(1,0)
			};
static	AFE_DCL_TXT_MACRO(8) buf_ff_txt_dbv;
static	DB_TEXT_STRING	*ff_txt_dbv;
static	DB_DATA_VALUE	ff_txt_dbdv =
			{
               			(PTR)NULL,
               			8,	/* hold string "-1.0" */
				DB_LTXT_TYPE,
               			0
			};


	/*
	** Determine the type of numeric data, and convert it to the
	** corresponding type of ASCII string representation.
	*/
	*display = EOS;
	*is_zero = FALSE;
	*is_neg = FALSE;
	*is_fract = FALSE;
	switch(abs(value->db_datatype))
	{
	case DB_INT_TYPE:
	case DB_MNY_TYPE:
	case DB_FLT_TYPE:
		ff_flt_dbdv.db_length = 8;
		ff_flt_dbdv.db_datatype = DB_FLT_TYPE;
		ff_flt_dbdv.db_prec = 0;
		if  (adc_getempty(cb,&ff_flt_dbdv) != OK)
		{
			return(FAIL);
		}
		if  (AFE_NULLABLE_MACRO(value->db_datatype))
		{
			AFE_MKNULL(&ff_flt_dbdv);
		}
		status = adc_cvinto(cb,value,&ff_flt_dbdv);
		if  (status != OK)
		{
			return(FAIL);
		}
		/*
		** Set up for common float processing
		*/
		value = &ff_flt_dbdv;
		if  (*(double *)value->db_data == 0)
		{
			*is_zero = TRUE;
		}
		else if  (*(double *)value->db_data < 0)
		{
			*is_neg = TRUE;
		}
		if  ((!(*is_zero)) &&
		     (*(double *)value->db_data < (double)1.00) &&
		     (*(double *)value->db_data > (double)-1.00))
		{
			*is_fract = TRUE;
		}
		format_char = ERx('f');
		if  (fmt->fmt_type != F_NT)
		{
			format_char = fmt->fmt_var.fmt_char;
			if (format_char == 'i')
			{
				format_char = 'f';
			}
			else if (format_char == 'I')
			{
				format_char = 'F';
			}
		}
		/*
		** CVfa() wants an i2 for it's returned length;
		** CVpka() wants a nat.  Mixing these up can
		** cause bus errors due to alignment differences.
		*/
		status = CVfa(*(double *)value->db_data,fmt->fmt_width,
			      fmt->fmt_prec,
			      format_char,
			      cb->adf_decimal.db_decimal,
			      display,&f_len);
		len = f_len;
		break;
	case DB_DEC_TYPE:
                dec_prec = DB_P_DECODE_MACRO(value->db_prec);
                dec_scale = DB_S_DECODE_MACRO(value->db_prec);
		ff_dec_dbdv.db_length = value->db_length;
		ff_dec_dbdv.db_datatype = DB_DEC_TYPE;
		ff_dec_dbdv.db_prec = value->db_prec;
		if  (adc_getempty(cb,&ff_dec_dbdv) != OK)
		{
			return(FAIL);
		}
		if  (AFE_NULLABLE_MACRO(value->db_datatype))
		{
			AFE_MKNULL(&ff_dec_dbdv);
		}
		if  (adc_compare(cb,value,&ff_dec_dbdv,&i) != OK)
		{
			return(FAIL);
		}
		if  (i == 0)
		{
			*is_zero = TRUE;
		}
		else if  (i < 0)
		{
			*is_neg = TRUE;
		}
		if  (!(*is_zero))
		{
			ff_dec_dbdv.db_length = value->db_length;
			ff_dec_dbdv.db_prec = value->db_prec;
                        if ( dec_prec == dec_scale )
                             ff_dec_dbdv.db_prec += 0x100; 
			ff_txt_dbv = (DB_TEXT_STRING *)&buf_ff_txt_dbv;
			ff_txt_dbdv.db_data = (PTR)ff_txt_dbv;
/* bug 76711 */
			if (cb->adf_decimal.db_decimal == FC_DECIMAL)
			{
			    STcopy(ERx("1.00"),
			      (char *)&ff_txt_dbv->db_t_text[0]);
						sizeof(ff_txt_dbv->db_t_count);
			}	
			else
			{
			    STcopy(ERx("1,00"),
			      (char *)&ff_txt_dbv->db_t_text[0]);
						sizeof(ff_txt_dbv->db_t_count);
			}
/* bug 76711 */
			ff_txt_dbv->db_t_count = STlength(ERx("1.00"));
			ff_txt_dbdv.db_length = STlength(ERx("1.00")) +
						sizeof(ff_txt_dbv->db_t_count);
			if  (adc_cvinto(cb,&ff_txt_dbdv,&ff_dec_dbdv) != OK)
			{
				return(FAIL);
			}
			if  (AFE_NULLABLE_MACRO(value->db_datatype))
			{
				AFE_MKNULL(&ff_dec_dbdv);
			}
			if  (adc_compare(cb,value,&ff_dec_dbdv,&i) != OK)
			{
				return(FAIL);
			}
			if  (i < 0)
			{
/* bug 76711 */
			    	if (cb->adf_decimal.db_decimal == FC_DECIMAL)
			    	{
				    STcopy(ERx("-1.00"),
				       (char *)&ff_txt_dbv->db_t_text[0]);
				}
				else
				{
				    STcopy(ERx("-1,00"),
				       (char *)&ff_txt_dbv->db_t_text[0]);
				}
				ff_txt_dbv->db_t_count = STlength(ERx("-1.00"));
/* bug 76711 */
				ff_txt_dbdv.db_length = STlength(ERx("-1.00")) +
						sizeof(ff_txt_dbv->db_t_count);
				if  (adc_cvinto(cb,&ff_txt_dbdv,
						&ff_dec_dbdv) != OK)
				{
					return(FAIL);
				}
				if  (AFE_NULLABLE_MACRO(value->db_datatype))
				{
					AFE_MKNULL(&ff_dec_dbdv);
				}
				if  (adc_compare(cb,value,
						 &ff_dec_dbdv,&i) != OK)
				{
					return(FAIL);
				}
				if  (i > 0)
				{
					*is_fract = TRUE;
				}
			}
		}
		dec_prec = DB_P_DECODE_MACRO(value->db_prec);
		dec_scale = DB_S_DECODE_MACRO(value->db_prec);
		if ( fmt->fmt_var.fmt_template != NULL &&
		     fmt->fmt_ccsize > 1 &&
		     fmt->fmt_var.fmt_template[fmt->fmt_width-1] == FC_ROUNDUP)
		{
			status = CVpka(value->db_data,
			       dec_prec,dec_scale,
			       cb->adf_decimal.db_decimal,
			       fmt->fmt_width, fmt->fmt_prec,
			       (i4)CV_PKLEFTJUST|(i4)CV_PKROUND,
			       display,&len);
		}
		else
		{
			status = CVpka(value->db_data,
			       dec_prec,dec_scale,
			       cb->adf_decimal.db_decimal,
			       fmt->fmt_width, fmt->fmt_prec,
			       (i4)CV_PKLEFTJUST,
			       display,&len);
		}
		break;
	default:
		/*
		** We really shouldn't be in this routine if we have
		** a non-numeric type ...
		*/
		return(FAIL);
		break;
	}

	return(OK);
}

