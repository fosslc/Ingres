/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
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
+*  Name: iiwrite.c - Write a query to the database parser.
**
**  Defines:
**	IIwritio	- Write a query (post 6.0)
**	IIwritedb	- Write a query (pre 6.0)
**	IIputctrl	- Pass a control character.
** 
**  Example:
**	## range of e is emp
**  Generates:
** 	Pre 6.0:
**	  IIwritedb("range of e is emp");
**	  IIsyncup((char *)0, 0);
**	
**	Post 6.0:
**	  IIwritio(FALSE, NULL, ISREF, DB_CHR_TYPE, 0, "range of e is emp");
**	  IIsyncup((char *)0, 0);
-*
**  History:
**	08-oct-1986	- Modified from old file. (ncg)
**	21-apr-1987	- Modified for 6.0. (ncg)
**	02-may-1988	- Added IIputctrl for DB procedures. (ncg)
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	10-jan-1994 (teresal)
**	    Add support for checking for non-null terminated C string
**	    (FIPS). Bug 55915.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
+*  Name: IIwritio - Write a query string literal or variable to DBMS.
**
**  Description:
**	IIwritio is used to write a string literal or variable to the DBMS
**	as a piece of a query.  The call has the standard I/O arguments, and
**	has a flag to indicate whether the string should be trimmed or not.
**	If we're not yet in a query then start the query.  If that succeeded
**	then write the query string argument (in as large pieces as possible).
**
**	The trimming is done up to the second last blank. (Note that we 
**	previously trimmed all but the last one, but because there may be
**	Kanji chars with spaces as a second byte, we may leave two following
**	blanks).  In the usual case, string constants will not be trimmed, while
**	variables will be trimmed.
**
**  Inputs:
**	trim		- TRUE/FALSE - trim or not.
**	ind_unused	- The address of null indicator variable (always null)
**	isref_unused	- Should always be TRUE.
**	type		- The embedded datatype
**				DB_CHR_TYPE, DB_CHA_TYPE, DB_VCH_TYPE or
**				DB_DBV_TYPE pointing
**				at a textual type (maybe from OSL).
**	length		- The length of the variable (if 0 the use STlength).
**	qry		- The address of the qry string.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
**  Example:
**	## destroy name_var
**  Generates:
**	IIwritio(FALSE, NULL, ISREF, DB_CHR_TYPE, 0, "destroy ");
**	IIwritio(TRUE, NULL, ISREF, DB_CHR_TYPE, 0, name_var);
-*
**  Side Effects:
**	Sends data to the backend parser.
**	
**  History:
**	21-apr-1987 - Written. (ncg)
**	18-may-1988 - Modified to check for nesting in procedure as well as
**		      data retrieval. (ncg)
**	04-nov-1988 - Process nullable (trap NULL) DB values from 4GL. (ncg)
**	12-dec-1988 - Generic error processing. (ncg)
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
*/

void
IIwritio(trim, ind_unused, isref_unused, type, length, qry)
i4		trim;
i2		*ind_unused;
i4		isref_unused;
i4		type;
i4		length;
char		*qry;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    register char 	*cp;
    DB_DATA_VALUE	dbv;
    DB_DATA_VALUE	*dbp;

# ifdef II_DEBUG
    if (type == DB_CHR_TYPE)
	SIprintf(ERx("IIwritio: '%s'\n"), qry);
# endif

    /* If a LIBQ error occurred during the query then ignore till end */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;

    /* Build query text */
    if (qry == (char *)0)
    {
	qry = ERx(" ");
	length = 1;
    }

    dbv.db_datatype = DB_QTXT_TYPE;	/* Final data type to use */
    dbv.db_prec = 0;
    switch (type)
    {
      case DB_CHR_TYPE:
	/* Check for EOS if ESQLC flag "-check_eos" was used (FIPS) */
	if (IIlbqcb->ii_lq_flags & II_L_CHREOS)
	{
	    if (adh_chkeos(qry, (i4)length) != OK)	
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
	if (length == 0)
	    dbv.db_length = STlength(qry);
	else
	    dbv.db_length = length;
	dbv.db_data = (PTR)qry;
	break;
      case DB_VCH_TYPE:
	dbv.db_length = ((DB_TEXT_STRING *)qry)->db_t_count;
	dbv.db_data = (PTR)((DB_TEXT_STRING *)qry)->db_t_text;
	break;
      case DB_DBV_TYPE:			/* Internal from another front-end */
	dbp = (DB_DATA_VALUE *)qry;
	if (ADH_ISNULL_MACRO(dbp))	/* Validate not null */
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ0018_DBQRY_NULL, II_ERR, 0,(char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return;
	}
	switch (abs(dbp->db_datatype))
	{
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	    dbv.db_length = (dbp->db_datatype < 0) ?
				(dbp->db_length - 1) : (dbp->db_length);
	    dbv.db_data = dbp->db_data;
	    break;
	  case DB_TXT_TYPE:
	  case DB_LTXT_TYPE:
	  case DB_VCH_TYPE:
	    dbv.db_length = ((DB_TEXT_STRING *)dbp->db_data)->db_t_count;
	    dbv.db_data = (PTR)((DB_TEXT_STRING *)dbp->db_data)->db_t_text;
	    break;
	  default:
	    dbv.db_length = 1;
	    dbv.db_data = (PTR)ERx(" ");
	    break;
	}
	break;
      default:
	dbv.db_length = 1;
	dbv.db_data = (PTR)ERx(" ");
	break;
    }

    /* 
    ** Trim ? Don't count trailing blanks (except for last two); just
    ** decrement the length so they are not included.  Trailing blanks
    ** may have been put into user variables.  Note that we should NOT
    ** trim them all, but remove most of them.  Also note that because
    ** we check for three sequential blanks we won't get mixed up with
    ** Kanji blanks.
    */
    if (trim && dbv.db_length > 2)
    {
	cp = (char *)dbv.db_data + dbv.db_length - 1;
	while (   (*cp == ' ')
	       && (*(cp - 1) == ' ')
	       && (*(cp - 2) == ' ')
	       && (cp > (char *)dbv.db_data))
	{
	    cp--;
	    dbv.db_length--;
	}
    }

    /*
    ** If no query started yet try to start.  If in a RETRIEVE or DB procedure
    ** then go ahead and call IIqry_start.  It will return FAIL after setting 
    ** a "nested error" condition.
    */
    if ((IIlbqcb->ii_lq_curqry == II_Q_DEFAULT) ||
	(IIlbqcb->ii_lq_flags & (II_L_RETRIEVE|II_L_DBPROC)))
    {
	if (IIqry_start(GCA_QUERY, 0, (i4)dbv.db_length, dbv.db_data) == FAIL)
	    return;
    }

    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
}

/*{
+*  Name: IIwritedb - Do the actual query writing.
**
**  Description:
**	If we're not yet in a query then start the query.  If that succeeded
**	then write the query string argument (in as large pieces as possible).
**
**  Inputs:
**	qry	- char * - Query string.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	22-mar-1985	- Rewritten to allow looooong strings (B5299). (ncg)
**	09-may-1985 	- Added hook IIsqQry for SQL open cursors. (ncg)
**	08-oct-1986	- Modified to use ULC. (ncg)
*/

void
IIwritedb( qry )
char	*qry;
{
    IIwritio(TRUE, (i2 *)0, TRUE, DB_CHR_TYPE, 0, qry);
}


/*{
+*  Name: IIputctrl - Add a control character to the DBMS query string.
**
**  Description:
**	This routine was written to retain input format for database
**	procedures, without insert control characters in host strings.
**	For each new line detected in the CREATE PROCEDURE statement
**	this routine is called to insert a new line in the query text.
**	Characters currently processed:
**
**		II_cNEWLINE - "\n"
**		
**  Inputs:
**	ctrl	- Control character code.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	02-may-1988 	- Written for DB procedures. (ncg)
*/

VOID
IIputctrl(ctrl)
i4	ctrl;
{
    /*
    ** As this table grows, the strings should be added in the order of
    ** the definition.
    */
    static char *controls[] = { ERx("\n") };

    IIwritio(FALSE, (i2 *)0, TRUE, DB_CHR_TYPE, 0, controls[ctrl]);
}
