/*
** Copyright (c) 2004 Ingres Corporation
*/

# include 	<compat.h>
# include 	<st.h>
# include 	<si.h>
# include 	<cm.h>
# include 	<er.h>
# include	<gl.h>
# include 	<iicommon.h>
# include 	<equel.h>
# include 	<equtils.h>
# include 	<eqscan.h>
# include 	<eqstmt.h>
# include 	<ereq.h>

/*
** Name: eqckfips.c - Check identifier to see if it is FIPS compliant.
**
** Description:
**	FIPS requires the ESQL preprocessor to flag all non Entry ANSI SQL2
**	compliant syntax. The following routines are called from the ESQL 
**	grammar to check an SQL	identifier to see if it is FIPS compliant. 
**	These routines are only called if the preprocessor is run with the 
**	FIPS flagger '-wfips_entry' on. If the identifier is not FIPS 
**	compliant a warning message is generated.
**
** Defines:
**      eqck_regid()      - Check if a regular identifier is FIPS compliant.
**      eqck_delimid()    - Check if a delimited identifier is FIPS complaint.
**
** Locals:
**	eqck_idlen()	  - Check if length of identifier is FIPS compliant.
**	eqck_idschar()	  - Check if identifier contains any INGRES special 
**			    characters (like: '#@$_').
**	eqck_idkeywd()	  - Check if identifier matches an ANSI SQL2 keyword.
**	eqck_idstrip()	  - Strip double quotes from a delimited identifier. 
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
**	26-apr-1993	(lan)
**	    Added eqck_tgt.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	FIPS_MAX_IDLEN	18

/* Local Routines - Function Prototypes */
static 	VOID	eqck_idlen(char *idname);
static 	VOID	eqck_idschar(char *idname);
static 	VOID	eqck_idkeywd(char *idname);
static 	VOID	eqck_idstrip(char *idname, char *stripid);

/* {
** Name: eqck_regid
**
** Description:     
**	Check if a regular identifier is FIPS compliant. This routine
**	is called from ESQL's grammar (esqlgram.my) to determine
**	if the identifier needs to be flagged by the FIPS flagger.
**	
**	A regular identifier must be flagged if:
**		o It's length is too long.
**		o It contains any special Ingres characters.
**		o It matches a ANSI SQL2 keyword that Ingres
**		  has not yet reserved.
**
** Inputs:
**      idname     char *	       - Regular Identifier name. 
**
** Outputs:
**      None.
**
** Returns:
**	None.
**
** Side Effects:
**      None.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
*/

VOID
eqck_regid(char *idname)
{
    eqck_idlen(idname);		/* Check that length is ok */
    eqck_idschar(idname);	/* Check for special Ingres characters */
    eqck_idkeywd(idname);	/* Check for ANSI SQL2 keyword */
}

/* {
** Name: eqck_delimid
**
** Description:     
**	Check if a delimited identifier is FIPS compliant. This routine
**	is called from ESQL's grammar (esqlgram.my) to determine
**	if the identifier needs to be flagged by the FIPS flagger.
**	
**	A delimited identifier must be flagged if:
**		o It's length is too long.
**
** Inputs:
**      idname     char *	       - Delimited Identifier name. 
**
** Outputs:
**      None.
**
** Returns:
**	None.
**
** Side Effects:
**      None.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
*/

VOID
eqck_delimid(char *idname )
{
    char	saveid[ID_MAXLEN + 1];	/* identifier stripped of quotes */
    i4		idlen;

    eqck_idstrip(idname, saveid);	/* Strip off quotes */	
    eqck_idlen(saveid);			/* Check if length is FIPS compliant */
}

/* {
** Name: eqck_idlen
**
** Description:     
**	Check if the identifier length is FIPS compatible. ANSI SQL2 Entry
**	level states that both regular and delimited identifiers (not
**	counting the quotes) are limited to 18 characters. Allowing an
**	identifier to be greater than 18 characters is a INGRES vendor 
**	extension and must be flagged as such by the ESQL FIPS flagger.
**
**	For example:
**
**		A23456789012345678    is a valid regular identifier
**		"A23456789012345678"  is a valid delimited identifier
**
**		A234567890123456789   is not a valid regular identifier
**		"A234567890123456789" is not a valid delimited identifier
**
**	Note: this routines assumes that double quotes have already been
**	stripped from a delimited identifier by first calling the 
**	eqck_idstrip() routine.
**
** Inputs:
**      idname     char *	       - Identifier name (if delimited, then
**					 double quotes have already
**				   	 been stripped off) 
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**      Will generate an ESQL warning message if the identifier length is not
**	FIPS compliant.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
*/

static VOID
eqck_idlen(char *idname )
{
    i4	idlen;

    /* Get length of id */	
    idlen = STlength(idname);

    /*	
    ** If identifier is too long, then this is not a valid FIPS id so give
    ** a FIPS warning.
    */
    if (idlen > FIPS_MAX_IDLEN)
	er_write( E_EQ0514_FIPS_IDLEN, EQ_WARN, 1, idname );
}
	
/* {
** Name: eqck_idschar
**
** Description:     
**      Check if the identifier contains any non-FIPS compliant 
**	characters. Ingres allows the following special	characters: 
**
**			# @ $ _
**
**	as part of a regular identifier. Allowing such characters is a vendor
**	extension to SQL2 and must be flagged by the FIPS flagger.
**
**	Note: SQL2 permits an underscore only if it is not the first or last
**	character in a regular identifier.
**
** Inputs:
**      idname     char *	       - Regular identifier name
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**      Will generate an ESQL warning message if the identifier contains 
**	any non-FIPS compliant characters.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
*/

static VOID
eqck_idschar(char *idname )
{
    register char	*cp = idname;

    if (*cp == '_')		/* Check if first char is an underscore */ 
    {
	er_write (E_EQ0515_FIPS_IDCHAR, EQ_WARN, 1, idname);
	return;
    }

    /* Check for special characters anywhere in the identifier */
    while (*cp)
    {
	/*
	** Check for special characters anywhere or for an underscore as
	** the last character.
	*/
	if (*cp == '$' || *cp == '@' || *cp == '#'
	    || (*cp == '_' && *(cp+1) == '\0'))
	{
	    er_write (E_EQ0515_FIPS_IDCHAR, EQ_WARN, 1, idname);
	    break;
	}
	CMnext(cp);
    }		
}

/* {
** Name: eqck_idkeywd
**
** Description:     
**      Check if the identifier matches an ANSI SQL2 keyword that
**	Ingres has not yet reserved. For FIPS, Ingres must either
**	reserved all ANSI SQL2 keywords (which is a large number of words)
**	or flag their usage with the FIPS flagger. We have decided to
**	flag their usage for now, and thus, we only reserve a subset of
**	ANSI SQL2 keywords. If in the future, we do reserve all 
**	SQL2 keywords this routine will become obsolete.
**
** Inputs:
**      idname     char *	       - Regular identifier name
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**      Will generate an ESQL warning message if the identifier matches 
**	an SQL2 keyword that has not already been reserved by ESQL.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
*/

static VOID
eqck_idkeywd(char *idname )
{
    char	*cp = idname;

    /* Look at ANSI SQL2 table to see if identifier is a keyword */
    if (sc_ansikey(tok_sql2key, tok_sql2num, idname) == OK)	
	er_write (E_EQ0516_FIPS_IDSQL2, EQ_WARN, 1, idname);
}

/* {
** Name: eqck_idstrip	(we don't use the UI routine IIUGdlm_ChkdlmBEobject()
**			 here because the preprocessor doesn't read 
**			 the IIdbconstants catalog which the UI routine
**			 needs for casing info.  All we need to do is strip
**			 the identifier anyway which is no big deal)
**
** Description:     
**	Strip off double quotes from a delimited identifier. Take off the
**	beginning and ending quotes as well as any escaped embedded quotes.
**	For example:
**			"My ""Identifier"""
**	would return:
**			My "Identifier" 
**		
**
** Inputs:
**      idname     char *	       - Delimited Identifier name. 
**
** Outputs:
**      stripid	  char *	       - Identifier stripped of quotes. 
**
** Returns:
**	None.
**
** Side Effects:
**      None.
**
** History:
**      31-jul-1992     (teresal)
**          Written for FIPS project.
**      23-feb-1994     (geri)
**	    Bug 58170: make sure that the resulting string is null-
**	    terminated
*/

static VOID
eqck_idstrip(char *idname, char *stripid )
{
    register char	*cp = idname;

    if (*cp == '"')		/* Get rid of starting double quote */ 
	CMnext(cp);

    /* Get rid of embedded double quotes */
    while (*cp)
    {
	if (*cp == '"')
	{
	    if (*(cp+1) == '"')		/* Skip escaped embedded quotes */
		CMnext(cp);
	    if (*(cp+1) == '\0')	/* Skip last quote */
	    {
	    	*stripid = '\0';
	    	break;
	    }	
	}
	CMcpyinc(cp, stripid);
    }
    if (*stripid != '\0')
	*stripid = '\0';
}

/* {
** Name: eqck_tgt	- handles the clearing and checking of the select
**			  target lists.
**
** Description:     
**	This routine is called from the ESQL grammar at the end of a statement
**	to clear the array that keeps limited information about the elements
**	of a select target list.  When a SELECT is encountered, this routine
**	is called to fill the array.  When a UNIONed SELECT is encountered,
**	this routine is called to check the array and if an element in the
**	UNIONed SELECT list is not 'compatible' with the corresponding element
**	in the SELECT list, a warning is issued.
**
** Inputs:
**	action		short		- 0 - clear the list and reset index
**					  1 - reset index
**					  2 - fill the list
**					  3 - check and flag
**	value		short		- 1 - a literal
**					  2 - an expression
**					  3 - a column specification
**
** Outputs:
**	None.
**
** Returns:
**	bool	- TRUE if the list is empty, else FALSE.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-1993 (lan)
**          Written.
**	20-may-1993 (kathryn)
**	    Change message number E_EQ0500 to E_EQ051D. The E_EQ0500 was
**	    previously used. 
**	20-mar-2002 (toumi01)
**	    Changed hardcoded 300 to DB_MAX_COLS for change from 300 to 1024.
*/

bool
eqck_tgt(short action, short value)
{
    static short	tgt_list[DB_MAX_COLS];
    static short	tgt_num = 0;

    switch (action)
    {
	case 0:				/* clear */
	    for (tgt_num = 0; tgt_num < DB_MAX_COLS; tgt_num++)
		tgt_list[tgt_num] = 0;
	    tgt_num = 0;
	    return FALSE;
	case 1:				/* reset */
	    tgt_num = 0;
	    if (tgt_list[0] == 0)	/* list is empty */
		return TRUE;
	    else
		return FALSE;
	case 2:				/* fill */
	    tgt_list[tgt_num++] = value;
	    return FALSE;
	case 3:				/* check and flag */
	    if ((tgt_list[0] != 0) && (tgt_list[tgt_num++] != value))
		er_write( E_EQ051D_FIPS_UNIONSEL, EQ_WARN, 0 );
	    return FALSE;
    }
}
