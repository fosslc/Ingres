/*
**	iihidecol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 
# include	<frserrno.h>
# include	<afe.h>
# include	<cm.h> 
# include	<me.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iihidecol.c
**
** Description:
**	Hidden column definition
**
**	Public (extern) routines defined:
**		IIthidecol()
**	Private (static) routines defined:
**		parse_hide()
**		typemap()
**
** History:
**	05/09/87 (dkh) - Fixed IIhidecol() so that hidden columns are
**			 created correctly.
**	19-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/11/87 (dkh) - Fixed parsing of "with null" and "not null".
**	22-apr-88 (bruceb)
**		Increased the buffer size from 256 to 2000 for lists
**		of hidden columns.
**	05/20/88 (dkh) - More ER changes.
**	11/08/88 (dkh) - Fixed jup bug 3414.
**	07/22/89 (dkh) - Fixed bug 6765.
**	11/29/90 (dkh) - Fixed a compiler complaint.
**	07/06/92 (dkh) - Added support for decimal datatype for 6.5.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

static	i4	parse_hide();		/* parse 1-string hidden list	*/
static STATUS	null_parse();

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	COLDESC	*IIcdget();
FUNC_EXTERN	STATUS	afe_pad();
FUNC_EXTERN	STATUS	afe_tychk();

# define	MAXTYPESZ	100	/* maximum size for a type spec */

# define	IS_WITH		1
# define	IS_NOT		0

/*{
** Name:	IIthidecol	-	Add hidden column to tblfld
**
** Description:
**	Adds a hidden column to a tablefield.
**
**	Search mapping table that maps strings of ingres types (such
**	as nat) to records that contain info of type. Once found fill
**	in the newly allocated descriptor and append it to the hidden 
**	column descriptor list. If the column type is NULL from the 
**	preprocessor then parse the column name as a complete target
**	list.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	colname		Name of hidden column, or target list if
**				coltype is NULL.
**	coltype		Ingres user type of hidden column
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Examples and Code Generation:
**	## inittable f1 t2 (col2 = c200)
**
**      if (IItbinit("f1", "t1", "fill") != 0) {
**	  IIthidecol("col2", "c200");
**	  IItfill();
**	}
**
** Side Effects:
**
** History:
**	14-mar-1983 	- Written (ncg)	
**	07-oct-1983	- Byte alignment by 4 (ncg)
**	14-feb-1984	- Added 1-variable target list option (ncg)
**	01/10/87 (dkh) - ADT stubbing changes.
**	19-feb-1987	- Changed for ADT's, NULLs, Kanji character
**			  manipulation. Eliminated typemap and MPDESC (drh)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	05/09/87 (dkh) - Initialize var withnull so that it has the proper
**			 value if null_parse() is not called.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**            Added kchin/dkhor's change for axp_osf
**            19-jan-93 (dkhor)
**            On axp_osf, pointer size is 8 bytes.  When creating hidden
**            column clptr, Ingres validate against Ingres datatypes which
**            excludes long (which is 8 bytes on axp_osf).
**            i4 is allowed but it is not big enough for
**            pointer assignment.  So we botch by assigning 8 to db_length.
**            01-feb-93 (kchin)
**            Piggyback dkhor's fix on axp_osf, but this time is for 'ptrfld'.
**            This is to fix a seg. fault in vision when called by FEfillst().
**            15-feb-93 (kchin)
**            Added idfld like what dkhor has added for clptr.  This is to
**            fix a seg. fault in vifred when called by IIVFfdFldDup().
**     24-mar-1999 (hweho01)
**            Extended the change for axp_osf to ris_u64 (AIX 64-bit platform).
**      21-Oct-2000 (hanje04)
**          Extended the change for axp_osf to axp_lnx (Alpha Linux)
**      06-Mar-2003 (hanje04)
**              Extend axp_osf changes to all 64bit platforms (LP64).
**	
*/

i4
IIthidecol(colname, coltype)
char	*colname;
char	*coltype;
{
	char			*colptr;	/* fetching column name */
	char			*typptr;	/* fetching type name */
	COLDESC			*cd;		/* new column descriptor */
	DB_USER_TYPE		dbt;		/* user column type */
	ADF_CB			*cb;		/* session control blk */
	register ROWDESC	*rd;		/* row descriptor of table */
	char			c_buf[MAXFRSNAME+1];
	char			t_buf[MAXTYPESZ+1];
	char			*cp;
	i4			adv;
	i4			withnull = 0;	/* initialize to zero */
	i4			pad;
	DB_DATA_VALUE		tdbv;
	
	if (!RTchkfrs(IIfrmio))
		return (FALSE);

	cb = FEadfcb();

	if (coltype == NULL)
	{
		return (parse_hide(colname));
	}

	/* prepare column name and type name for processing */

	if ((colptr=IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME)) ==
		NULL)
	{
		/* no column name given for table I/O statement */
		IIFDerror(TBNOCOL, 1, (char *) IIcurtbl->tb_name);
		return (FALSE);
	}

	typptr = IIstrconv(II_CONV, coltype, t_buf, (i4)MAXTYPESZ);

	/* does column already exist */

	if (STcompare(colptr, ERx("_state")) == 0 ||
	    STcompare(colptr, ERx("_record")) == 0 ||
	    IIcdget(IIcurtbl, colptr) != NULL)
	{
		/* column already exists--cannot hide */
		IIFDerror(TBHCEXIST, 2, (char *) colptr,
			IIcurtbl->tb_name);
		return (FALSE);
	}
	if (!IIcurtbl->dataset->ds_rowdesc->r_memtag)
	{
		IIcurtbl->dataset->ds_rowdesc->r_memtag = FEgettag();
	}

	/*
	**  Check for possible NULL qualifiers in
	**  the data type specification.  This is
	**  needed in case this routine is called
	**  directly from user code.
	**
	**  First find the real data type.
	*/
	cp = typptr;
	while (*cp != EOS && !(CMspace(cp)))
	{
		CMnext(cp);
	}

	/*
	**  If there is something left, check for possible
	**  NULL qualifiers.
	*/
	if (*cp != EOS)
	{
		if (null_parse(cp, &adv, &withnull) != OK)
		{
			/*
			**  Some sort of syntax error.
			*/
			IIFDerror(TBHCTYPE, 2,
				(char *)IIcurtbl->tb_name, coltype);
			return(FALSE);
		}

		/*
		**  Put EOS BEFORE the 'with null' or 'not null'
		**  clause - that info is stored in the DB_USER_TYPE
		**  structure as a flag.
		*/

		*cp = '\0';
	}

	/*
	**  Build a DB_USER_TYPE with the column type string,
	**  then call afe_tychk to check it for legality, and
	**  get the correct internal size and type
	*/

	if ( withnull != 0 )
		dbt.dbut_kind = (i4) DB_UT_NULL;
	else
		dbt.dbut_kind = (i4) DB_UT_DEF;

	STtrmwhite( typptr );
	STcopy( typptr, dbt.dbut_name );

	tdbv.db_prec = 0;
	tdbv.db_data = NULL;
	if ( afe_tychk( cb, &dbt, &tdbv ) != OK )
	{
		IIFDerror(TBHCTYPE, 2, (char *) IIcurtbl->tb_name,
			dbt.dbut_name);
		return( FAIL );
	}


	rd = IIcurtbl->dataset->ds_rowdesc;

	/*
	**  Compute the data offset for the hidden column.  Realsize is
	**  the size of the row's data so far.  Align from this point,
	**  and use that as the offset.
	*/

	if ( afe_pad( cb, rd->r_realsize, tdbv.db_datatype, &pad ) != OK )
		return( FAIL );

	/*
	**  Allocate column descriptor.
	*/
	if ((cd = (COLDESC *)FEreqmem(IIcurtbl->dataset->ds_rowdesc->r_memtag,
		sizeof(COLDESC), TRUE, (STATUS *) NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIthidecol"));
	}

	/* update all column descriptor information */

	MEcopy((PTR) &tdbv, (u_i2) sizeof(DB_DATA_VALUE), (PTR) &(cd->c_dbv));
#if defined(LP64)
    if (STcompare(colname, "clptr") == 0 || STcompare(colname, "ptrfld") == 0 || STcompare(colname,"idfld") == 0)
        cd->c_dbv.db_length = 8;
#endif /* LP64 */
	cd->c_name = FEtsalloc(IIcurtbl->dataset->ds_rowdesc->r_memtag,
		colptr);

	cd->c_qryoffset = 0;		/* no query offset for hidden cols */
	cd->c_hide = 1;

	cd->c_offset = rd->r_realsize + pad;
	rd->r_realsize = rd->r_realsize + pad + cd->c_dbv.db_length;

	/*
	**  Add the column to the hidden column list
	*/

	cd->c_nxt = rd->r_hidecd;		
	rd->r_hidecd = cd;

	return (TRUE);
}

/*{
** Name:	null_parse	- Parse null specification.
**
** Description:
**	Parse the optional [with | not] null qualifiers that can be
**	specified for a table field hidden column specification.
**
** Inputs:
**	format		Pointer to the current position in the format string.
**	adv		Pointer to number of characters to advance in string.
**	withnull	Pointer to a i4  to set to 1 if 'with null'
**
** Outputs:
**	adv		Updated with the number of bytes to advance in string
**	withnull	Set to 1 if 'with null', 0 otherwise
**
** Returns:
**		OK	If parse worked.
**		FAIL	If incorrect syntax found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01/10/87 (dkh) - Initial version.
**	02/19/87 (drh) - Extensively modified for Kanji.  Added withnull
**			 parameter.
*/
static STATUS
null_parse(format, adv, withnull )
char	*format;
i4	*adv;
i4	*withnull;
{
	char	*cp;
	char	buf1[256];
	i4	skip;
	i4	with_or_not = IS_WITH;

	*withnull = 0;
	*adv = 0;

	/*
	**  Skip white space or till end of string.
	*/
	for (skip = 0; *format != EOS && CMwhite(format); )
	{
		CMbyteinc( skip, format );
		CMnext( format );
	}

	/*
	**  Return if end of string.
	*/
	if (*format == '\0')
	{
		return(OK);
	}

	/*
	**  Grab next word.
	*/
	for (cp = buf1, *cp = EOS; (*format != EOS && CMnmstart(format)); )
	{
		CMtolower( format, cp );
		CMbyteinc( skip, format );
		CMnext( format );
		CMnext( cp );
	}

	/*
	**  Check if word is "WITH" or "NOT".
	*/

	*cp = EOS;  

	if (STcompare(buf1, ERx("with")) != 0 )
	{
		if (STcompare( buf1, ERx("not")) != 0 )
		{
			return( OK );
		}
		with_or_not = IS_NOT;
	}

	/*
	**  Skip more white space or till end of string.
	*/
	for (; *format != EOS && CMwhite(format); )
	{
		CMbyteinc( skip, format );
		CMnext( format );
	}

	/*
	**  If no more string, then syntax error.
	*/

	if ( *format == EOS )
	{
		return(FAIL);
	}

	/*
	**  Get next word.
	*/

	for (cp = buf1, *cp = EOS; (*format != EOS && CMnmstart(format)); )
	{
		CMtolower( format, cp );
		CMbyteinc( skip, format );
		CMnext( format );
		CMnext( cp );
	}

	/*
	**  This word had better be "NULL" or it is a syntax error.
	*/

	*cp = EOS;
	if (STcompare(buf1, ERx("null")) != 0)
	{
		return(FAIL);
	}

	if ( with_or_not == IS_WITH )		/* if not "not null" */
		*withnull = 1;		/* then it's "with null" */
	*adv = skip;
	return(OK);
}


/*{
** Name:	parse_hide	-	Parse target list for hidden cols
**
** Description:
**	This routine parses a list of hidden column specifications.
**	This is necessary because EQUEL allows the target list to
**	be specified in a single program string-variable.
**
**	The format of the hidden column list is:
**
**	colnm = type [with null | not null] {, colnm = type ....}
**                                ________
**
**	This routine parses the list, and calls IIthidecol for
**	each column specification it finds.
**
**	Note that variable can NEVER be an ADT - it must be a null
**	terminated string (drh).
**
** Inputs:
**	format		The hidden column list to parse
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	19-feb-1987	Modified for Kanji character conventions (drh)
*/

static i4
parse_hide(format)
char	*format;
{
	register char	*cp;
	char		hidebuf[DB_MAXSTRING + 1];
	char		namebuf[256];
	char		*hcol;		/* hidden column name	*/
	char		*htype;		/* hidden column type	*/
	i4		advance;
	i4		dummy;
	i4		lparen;

	if ((cp=IIstrconv(II_CONV, format, hidebuf, (i4)DB_MAXSTRING)) == NULL)
	{
		/* no column name given for table I/O statement */
		IIFDerror(TBNOCOL, 1, (char *) IIcurtbl->tb_name);
		return (FALSE);
	}

	while ( *cp != EOS )
	{
		/* eat white space to point at start of column name */

		while (CMwhite(cp) && *cp != EOS)
			CMnext(cp);
		if (*cp == EOS)
			break;

		/*  
		**  Find column name and put it in a new buffer.
		**  Can't do in-line hacking since there may be
		**  no white space where we can put an EOS.
		*/

		hcol = namebuf;
		if ( !CMnmstart(cp) )
		{
			break;
		}
		else
		{
			CMtolower(cp, hcol);
			CMnext(cp);
			CMnext(hcol);
		}
		while ( CMnmchar(cp) )
		{
			CMtolower(cp, hcol);
			CMnext(cp);
			CMnext(hcol);
		}
		*hcol = EOS;

		if (*cp == EOS)
		{
			break;
		}
		else
		{
			/*
			**  Find the '=' symbol by skipping over
			**  any white space then check for the '=' sign.
			*/
			while (*cp != EOS && CMwhite(cp))
			{
				CMnext(cp);
			}
			if (*cp != '=')
			{
				break;
			}
			else
			{
				/*
				**  Skip the '=' sign.
				*/
				CMnext(cp);
			}
		}

		/*
		** eat white space to point at start of column data type
		*/

		while (CMwhite(cp) && *cp != EOS)
			CMnext(cp);
		if (*cp == EOS)
			break;

		/*  
		** Point to the start of the column type, then find the
		** end of the type specification (including null qualifier,
		** if any) and put EOS there
		*/

		htype = cp;

		lparen = FALSE;

	/*	while (*cp != EOS  && (!CMspace(cp)) && *cp != ',')*/
	/*	while (*cp != EOS && (!CMspace(cp)))*/
		while (*cp != EOS)
		{
			if (*cp == '(')
			{
				lparen = TRUE;
			}
			else if (*cp == ')')
			{
				lparen = FALSE;
			}
			else if (*cp == ',')
			{
				if (!lparen)
				{
					break;
				}
			}
			CMnext(cp);
		}
		if ( *cp == EOS )
		{
			/*
			**  last pair in list
			**  and no null qualifiers
			*/

			IIthidecol(namebuf, htype);
			break;
		}

		if (*cp != ',')
		{
			/*
			**  Check for NULL qualifiers.
			*/
			if (null_parse(cp, &advance, &dummy ) != OK)
			{
				break;
			}

			cp +=advance;	/* point past null qualifier */
		}
		*cp = EOS;	/* null terminate column type */
		CMnext(cp);	/* point past the EOS we added */

		if (!IIthidecol(namebuf, htype))
			break;
	}
	return (TRUE);
}
