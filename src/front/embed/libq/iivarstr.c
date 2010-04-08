/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<adf.h>
# include	<gca.h>
# include	<erlq.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<adh.h>

/**
+*  Name: iivarstr.c - Send variables to the back end as strings.
**
**  Description:
**	IIvarstrio is used to write the contents of a host language variable
**	to the Quel parser as a string. The variable must represent a constant
**	in a Quel expression.
**
**  Defines:
**	IIvarstrio
**
**  Notes:
**	None.
-*
**  History:
**	30-dec-1986 - Written. (mrw)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIvarstrio - Write a variable to the back end as a string.
**
**  Description:
**	IIvarstrio is used to write the contents of a host language variable
**	to the Quel parser as a string. The variable must represent a constant
**	in a Quel expression.
**
**  Inputs:
**	indnull		- The address of null indicator variable (always null)
**	isvar		- TRUE/FALSE: is a variable/is a constant
**	type		- The EQUEL datatype  + DB_DBV_TYPE from 4GL.
**	length		- The length of the variable (0 for strings)
**	addr		- The address of the variable (value for constants)
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	Sends data to the backend parser.
**	
**  Notes:
**   1.	Strings are delimited with single-quotes (for SQL) or double-quotes
**	(for QUEL).
**   2. Escape is single-quote (for SQL) and backslash (for QUEL).
**   3. If we ever get an i1 by value we're in deep trouble.  The code
**	assumes that the first byte of the value is the i1 itself, which
**	simply isn't true on some machines.  On the other hand, the
**	preprocessor will never generate code that does this.
**
**  Example:
**		# define I 12
**		i4   i;
**		   
**		IIvarstrio( (short *)0, 1, T_INT, sizeof(i4), &i );
**		IIvarstrio( (short *)0, 0, T_INT, sizeof(i4), I );
**  History:
**	30-dec-1986 - Written. (mrw)
**	10-mar-1987 - Added null indicator support. (ncg)
**	26-aug-1987 - Modified to use GCA. (ncg)
**	07-sep-1988 - Modified to convert ' to '' within strings (SQL)
**                    or to convert " to \" and \ to \\ (QUEL) (emerson)
**	04-nov-1988 - Allows DB_DBV_TYPE from 4GL. (ncg)
**	12-dec-1988 - Generic error processing. (ncg)
**	05-apr-1991 - Bug fix 36341 - eliminate trailing blanks from
**		      input string if CHA type.  This fixes problem in 
**		      UNIX where the COPY filename got excessive
**		      trailing blanks. (teresal) 
**	18-nov-1993 - Bug fix 55915. Check for EOS on fixed length
**		      character buffers (C only) for FIPS. (teresal)
**	15-Jul-2004 (schka24)
**	    Allow i8's.
*/

void
IIvarstrio( indnull, isvar, type, length, addr )
i2		*indnull;
i4		isvar;
i4		type;
i4		length;
char		*addr;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    char		buf[32];
    DB_DATA_VALUE	dbv;
    DB_TEXT_STRING	*txt;		/* For type TEXT */
    char                *cp;		/* Current position in string  */
    char                *end_cp;	/* Last position in string  */
    char		*last_cp;	/* Last non-blank pos in string */
    char                delim;          /* String delimiter (' or ")   */
    char                escape;         /* String escape char (' or \) */
    DB_DATA_VALUE       dbv_delim;      /* For the string delimiter    */
    DB_DATA_VALUE       dbv_escape;     /* For the string escape char  */
    DB_DATA_VALUE	*dbp;		/* Input DBV */

    /*
    ** Dump the variable. Only canonical and DBV types should come through
    ** here. If DBV type then extract the canonical type out.
    */
    if (type == DB_DBV_TYPE)
    {
	dbp = (DB_DATA_VALUE *)addr;
	if (ADH_ISNULL_MACRO(dbp))	/* Validate not null */
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ0018_DBQRY_NULL, II_ERR, 0,(char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return;
	}
	isvar  = TRUE;			/* Get canonical info */
	type   = dbp->db_datatype;
	length = dbp->db_length;
	addr   = (char *)dbp->db_data;
	if (dbp->db_datatype < 0)
	{
	    length--;
	    type = -type;
	}
    }
    switch (type)
    {
      case DB_INT_TYPE:
	switch (length)
	{
	  case sizeof(i1):
	    if (isvar)
		STprintf( buf, ERx("%d"), *(i1 *)addr );
	    else
		STprintf( buf, ERx("%d"), *(i1 *)&addr );
	    break;
	  case sizeof(i2):
	    if (isvar)
		STprintf( buf, ERx("%d"), *(i2 *)addr );
	    else
		STprintf( buf, ERx("%d"), *(i2 *)&addr );
	    break;
	  case sizeof(i4):
	    if (isvar)
		STprintf( buf, ERx("%ld"), *(i4 *)addr );
	    else
		STprintf( buf, ERx("%ld"), *(i4 *)&addr );
	    break;
	  case sizeof(i8):
	    if (isvar)
		STprintf( buf, ERx("%lld"), *(i8 *)addr );
	    else
		STprintf( buf, ERx("%lld"), *(i8 *)&addr );
	    break;
	}
	II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(buf), buf);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	return;
      case DB_CHR_TYPE:
     	/* Check for EOS if FIPS "-check_eos flag was used (ESQLC only) */ 
	if (IIlbqcb->ii_lq_flags & II_L_CHREOS)
	{
	    if (adh_chkeos(addr, (i4)length) != OK) 
	    {
	    	IIlocerr(GE_DATA_EXCEPTION, E_LQ00EC_UNTERM_C_STRING, II_ERR, 
							0, (char *)0);
	    	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    	return;
	    }
	    if (length < 0)
		length = 0;
	}
	/* Fall through here */
      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	if (type == DB_VCH_TYPE || type == DB_TXT_TYPE || type == DB_LTXT_TYPE)
	{
	    txt = (DB_TEXT_STRING *)addr;
	    length = txt->db_t_count;
	    addr = (char *)txt->db_t_text;
	}
	else if (length == 0 && type == DB_CHR_TYPE)
	{
	    length = STlength(addr);
	}
	else if (type == DB_CHA_TYPE)
	{
	    /* Strip off trailing blanks for "non-C" strings */	
	    /* Bug fix 36341 */
	    cp = addr;
	    end_cp = addr + length;
	    last_cp = cp;
	    while (cp < end_cp)
	    {
		if (*cp != ' ')
                {
                    CMnext(cp);
                    last_cp = cp;
                }
                else
                {
                    CMnext(cp);
                }	
	    }
	    length = last_cp - addr;	/* Might be zero! */
	}
        /* The DML language may change dynamically! */
	if (IIlbqcb->ii_lq_dml == DB_SQL)
	{
	    delim  = '\'';
	    escape = '\'';
	}
	else
	{
	    delim  = '\"';
	    escape = '\\';
	}
	II_DB_SCAL_MACRO(dbv_delim,  DB_QTXT_TYPE, 1, &delim);
	II_DB_SCAL_MACRO(dbv_escape, DB_QTXT_TYPE, 1, &escape);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv_delim);
	/* Esacpe all nested quotes and escapes */
	end_cp = addr + length;
	for (cp = addr; cp < end_cp; CMnext(cp))
	{
	    if ((*cp == delim) || (*cp == escape))
	    {
		if (cp > addr)
		{
		    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, cp - addr, addr);
		    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
		    addr = cp;
		}
		IIcgc_put_msg(IIlbqcb->ii_lq_gca,FALSE, IICGC_VQRY,&dbv_escape);
	    }
	}
	II_DB_SCAL_MACRO( dbv, DB_QTXT_TYPE, cp - addr, addr );
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv_delim);
	return;
      case DB_FLT_TYPE:
      default:
	IIputdomio( indnull, isvar, type, length, addr );
	return;
    }
}
