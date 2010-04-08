/*
**	iisetinq.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<st.h>		 
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
# include	<er.h> 

/**
** Name:	iisetinq.c
**
** Description:
**	Support routines for  pre 4.0-style set and inquire stmts
**
**	These routines set and inquire various information maintained
**	by the Forms Runtime System (FRS).  The syntax for these routines
**	in EQUEL is:
**
**	    inq_frs "obj_type" "obj_name1" "obj_name2" ... 
**		(var1 = "attr1", var2 = "attr", ... )
**	    set_frs "obj_type" "obj_name1" "obj_name2" ... 
**		("attr1" = var1, "attr" = var2, ... )
**
**	The code generated is:
**
**	    if (IIinqset("obj_type", "obj_name1", ... , 0) != 0)
**	    {
**		IIfrsinq(1, DB_INT_TYPE, 4, &var1, "attr1");
**		IIfrsinq(1, DB_INT_TYPE, 4, &var2, "attr2");
**	    }
**
**	    if (IIinqset("obj_type", "obj_name1", ... , 0) != 0)
**	    {
**		IIfrsset("attr1", 1, DB_INT_TYPE, 4, &var1);
**		IIfrsset("attr2", 1, DB_INT_TYPE, 4, &var2);
**	    }
**
**	The arguments to the IIfrsset and IIfrsinq routines are
**	identical to those in the putform and getform routines
**	except that instead of a field name, an attribute name
**	is passed concerned with the object you have identified
**	to IIinqset.
**
**	The attributes are described in special files identified
**	with the objects the user is looking up.  The object table
**	is located below and named IIfrsobjs.  This table points to
**	the table of attributes for that object and the file name
**	with the lookup routines is in comments.
**
**	Public (extern) routines defined:
**		IIinqset()
**		IIfrsset()	
**		IIfrsinq()
**		IIfsinqio()
**		IIfssetio()
**
**	Private (static) routines defined:
**		get_ratt()
**
** History:
**	07-26-83  -  Written (jen).
**	27-feb-1987	Modified for ADTs and NULLs (drh)
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	19-feb-92 (leighb) DeskTop Porting Change:
**		adh_evcvtdb() has only 3 args, bogus 4th one deleted.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# ifndef	PCINGRES

# define	fdsiFRAME	1
# define	fdsiFIELD	2
# define	fdsiTABLE	3
# define	fdsiCOL		4
# define	fdsiFRS		5

GLOBALREF	RTATTLIST	IIattfld[];
GLOBALREF	RTATTLIST	IIattform[];
GLOBALREF	RTATTLIST	IIatttbl[];
GLOBALREF	RTATTLIST	IIattfrs[];
GLOBALREF	RTATTLIST	IIattcol[];

GLOBALREF RTOBJLIST	IIfrsobjs[];

/*
**	IIinqset set this pointer to the appropriate object
*/
GLOBALREF RTOBJLIST	*IIcurobj;

/*
**	These are the pointers that get passed to the routines
**	specified in the attribute tables.  They are set by
**	IIinqset and set according to the type of object being
**	inquired or set.
*/
static	i4	*IIarg1 = NULL;
static	i4	*IIarg2 = NULL;

FUNC_EXTERN	STATUS		adh_dbcvtev();
FUNC_EXTERN	STATUS		adh_evcvtdb();
FUNC_EXTERN	RUNFRM		*RTgetfrm();
FUNC_EXTERN	TBSTRUCT	*RTgettbl();
FUNC_EXTERN	FIELD		*RTgetfld();
FUNC_EXTERN	FLDCOL		*RTgetcol();
static	RTATTLIST	*get_ratt(char *, bool);



/*{
** Name:	 IIinqset	-	Look up object
**
** Description:
**	This routine looks up the object being set or inquired.
**	The first argument is the object name.  After this there
**	may be zero or more arguments identifying a particular
**	object and terminated by a zero.
**
**	IIinqset then uses this information to look up the appropriate
**	object in the above object table.  It then looks to see
**	if this is a valid object (i.e. does the frame exist).  If
**	not, the entire statement is aborted.  It then sets up
**	arguments for IIfrsinq and IIfrsset to lookup.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	object
**	p0
**	p1
**	p2
**	p3
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
**	
*/

IIinqset(object, p0, p1, p2, p3 )
char	*object;
char	*p0, *p1, *p2, *p3;
{
	char		*obj;			/* Object pointer and bufs */
	char		objbuf[MAXFRSNAME+1];
	char		*nmlist[3];		/* Name pointers and bufs */
	char		nmbufs[3][MAXFRSNAME+1];
	char		**nml;			/* Walks array of names */
	register i4	argcnt;
	RTOBJLIST	*rtobj;
	RUNFRM		*stkf;
	TBSTRUCT	*tbl;
	TBLFLD		*tblfld;
	FIELD		*fld;

	/*
	**	Get the object type and find all the object names.
	**	The names are terminated by a NULL.  Figure out the
	**	number of arguments - currently at most 3 can be relevant.
	*/
	obj = IIstrconv(II_CONV, object, objbuf, (i4)MAXFRSNAME);
	for (nml = nmlist, argcnt = 0; argcnt < 3; nml++, argcnt++)
	{
		*nml = ERx("");
	}
	/*
	** Flattened out argument pickoff. Currently there at at most 3
	** arguments after the 'object' - formname, tablename, columnname -
	** so do not go beyond that. (ncg)
	*/
	argcnt = 0;
	if ( p0 != (char *)0 )
	{
	   argcnt++;
	   nmlist[0] = IIstrconv(II_CONV, p0, nmbufs[0], (i4)MAXFRSNAME);
	   if ( p1 != (char *)0 )	
	   {
	      argcnt++;
	      nmlist[1] = IIstrconv(II_CONV, p1, nmbufs[1], (i4)MAXFRSNAME);
	      if ( p2 != (char *)0 )	
	      {
		  argcnt++;
	          nmlist[2] = IIstrconv(II_CONV, p2, nmbufs[2], (i4)MAXFRSNAME);
	      }
	   }
	}

	/*
	**  Look up the object type in the object table
	*/
	IIcurobj = NULL;
	for (rtobj = IIfrsobjs; rtobj->ro_name != NULL; rtobj++)
	{
		if (STcompare(obj, rtobj->ro_name) == 0)
		{
			IIcurobj = rtobj;
			break;
		}
	}
	if (IIcurobj == NULL)
	{
		/* ERROR */
		IIFDerror(SIOBJNF, 1, obj);
		return (FALSE);
	}

	/*
	**	Now look up the object by name.  Each object
	**	has its own way of looking up objects.
	*/
	nml = nmlist;
	switch (rtobj->ro_ptr)
	{
	  case	fdsiFIELD:
		if ((stkf = IIget_stkf(argcnt, (i4)2, obj, nml)) == NULL)
			return (FALSE);
		if ((fld = RTgetfld(stkf, *++nml)) == NULL)
		{
			IIFDerror(SIFLD, 1, *nml);
			return (FALSE);
		}
		IIarg1 = (i4 *)fld;
		IIarg2 = (i4 *)fld->fld_var.flregfld;
		break;

	  case	fdsiFRAME:
		if ((stkf = IIget_stkf(argcnt, (i4)1, obj, nml)) == NULL)
			return (FALSE);
		IIarg1 = (i4 *)stkf;
		IIarg2 = (i4 *)stkf->fdrunfrm;
		break;

	  case	fdsiTABLE:
		if ((stkf = IIget_stkf(argcnt, (i4)2, obj, nml)) == NULL)
			return (FALSE);

		if ((tbl = RTgettbl(stkf, *++nml)) == NULL)
		{
			IIFDerror(SITBL, 1, *nml);
			return (FALSE);
		}
		tblfld = tbl->tb_fld;

		IIarg1 = (i4 *)tbl;
		IIarg2 = (i4 *)tblfld;
		break;

	  case	fdsiCOL:
		if ((stkf = IIget_stkf(argcnt, (i4)3, obj, nml)) == NULL)
			return (FALSE);

		if ((tbl = RTgettbl(stkf, *++nml)) == NULL)
		{
			IIFDerror(SITBL, 1, *nml);
			return (FALSE);
		}
		tblfld = tbl->tb_fld;

		if ((fld = (FIELD *) RTgetcol(tblfld, *++nml)) == NULL)
		{
			IIFDerror(SICOL, 1, *nml);
			return (FALSE);
		}

		IIarg1 = (i4 *)tblfld;
		IIarg2 = (i4 *)fld;
		break;

	  case	fdsiFRS:
		break;

	  default:
		break;
	}

	return (TRUE);
}


/*{
** Name:	IIfssetio	-	Pre 4.0 set stmt support
**
** Description:
**	Set a forms runtime system variable.  This routine provides
**	support for the pre 4.0 style set/inquire.
**	
**	Build an embedded data value from the caller's parameters.
**	Determine the data type expected by the set routine, and
**	convert the data value to that type.  Call the appropriate
**	set routine, passing the data area of the db_data_value.
**
**	Note: this looks a little strange - old routines used 
**	DB_CHR_TYPE to mean a null terminated, lower-case
**	'c' string, so, while we convert to a DB_CHR_TYPE, we must take
**	extra steps to null terminate, trim trailing white space, and
**	convert to lower case.  Really, these routines want fdSTRING
**	parameters like IIstfsio and IIiqfsio (the post 4.0 style support
**	routines). 
**
**	This routine is part of RUNTIME's external interface.
**	
**
** Inputs:
**	name		field name as string
**	nullind		Ptr to the null indicator
**	isvar		flag indicating whether variable
**	type		embedded data type
**	len		embedded data length
**	tdata		embedded data variable
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
**	27-feb-1987	Created with code extracted from IIfrsset, modified
**			for ADTs and NULLs (drh).
**	
*/

i4
IIfssetio(char *name, i2 *nullind, i4 isvar, i4 type, i4 len, PTR tdata)
{
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		dbv;
	char			*nm;
	char			nmbuf[MAXFRSNAME+1];
	FUNC_EXTERN char	*IIstrconv();
	reg RTATTLIST		*ratt;
	char			cval[ MAXIQSET+1 ];
	i4			natval;
	char			*cptr;

	nm = IIstrconv(II_CONV, name, nmbuf, (i4)MAXFRSNAME);
	if ((ratt = get_ratt(nm, (bool)TRUE)) == NULL)
		return (FALSE);

	/*
	**  Build an embedded data value from the caller's parameters
	*/

	edv.ed_type = type;
	edv.ed_length = len;
	edv.ed_data = isvar ? tdata : (PTR) &tdata;
	edv.ed_null = nullind;

	/*
	**  Build a db data value to hold the converted embedded data
	**  value.  The type of this value is determined by the attribute
	**  being set.
	*/

	switch (ratt->ra_type)		/* embedded data type expected */
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
		dbv.db_datatype = DB_INT_TYPE;
		dbv.db_length = sizeof( i4  );
		dbv.db_prec = 0;
		dbv.db_data = (PTR) &natval;
		break;

	  case DB_CHR_TYPE:
		dbv.db_datatype = DB_CHR_TYPE;
		dbv.db_length = MAXIQSET;
		dbv.db_prec = 0;
		dbv.db_data = cval;
		break;

	  default:
		return( FALSE );
		break;
	}

	/*
	**  Convert the user's data value into the datatype expected
	**  by the set routine.
	*/

	if ( adh_evcvtdb( FEadfcb(), &edv, &dbv ) != OK )     
	{
		IIFDerror(RTSIERR, 0);
		return( FALSE );
	}

	/*
	**  Null terminate, trim trailing whitespace, and convert to
	**  lower case if a character string.
	*/

	if ( ratt->ra_type == DB_CHR_TYPE )
	{
		cval[ MAXIQSET ] = '\0';
		cptr = cval;
		STtrmwhite( cptr );
		CVlower( cptr );
	}

	/*
	**	The current object pointers have all the information needed
	**	for the right call to get information.  Call the appropriate
	**	routine with the appropriate pointers.
	*/

	(*ratt->ra_proc)(IIarg1, IIarg2, ratt->ra_arg, dbv.db_data );

	return (TRUE);
}

/*{
** Name:	IIfrsset	-	Pre 4.0 set stmt support
**
** Description:
**	Set a forms runtime system variable.  This routine provides
**	support for the pre 4.0 style set/inquire.
**	
**	This routine is part of RUNTIME's external interface as a
**	cover routine for pre 6.0 compatability (EQUEL still allowed
**	this style of set/inquire at 6.0, but issued a warning ).
**	
**
** Inputs:
**	name		field name as string
**	variable	flag indicating whether variable
**	type		embedded data type
**	len		embedded data length
**	tdata		embedded data variable
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
**	
*/

i4
IIfrsset(char *name, i4 variable, i4 type, i4 len, PTR tdata)
{
	return( IIfssetio( name, (i2 *) NULL, variable, type, len, tdata ) );
}

/*{
** Name:	IIfsinqio	-	Pre 4.0 inquire support
**
** Description:
**	Inquire about a forms runtime system variable.  This routine
**	provides compatability for the pre 4.0 style set/inquire.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	nullind		Ptr to a null indicator
**	variable	Flag - is variable
**	type		Embedded data type
**	len		Embedded data length
**	data		Embedded data value
**	name		Name of field to return
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
**	27-feb-1987	Created with code extracted from IIfrsinq, modified
**			to support ADTs and NULLs (drh)
**	24-mar-1987	Fixed bug in test of DB_CHR_TYPE attributes (drh)
**	
*/

/* VARARGS4 */

i4
IIfsinqio(i2 *nullind, i4 variable, i4 type, i4 len, PTR data, char *name)
{
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		dbv;
	i4			natval;
	char			cval[ MAXIQSET + 1];
	char			*cptr;
	char			nmbuf[MAXFRSNAME+1];
	FUNC_EXTERN char	*IIstrconv();
	reg char		*nm;
	reg RTATTLIST		*ratt;

	nm = IIstrconv(II_CONV, name, nmbuf, (i4)MAXFRSNAME);
	if ((ratt = get_ratt(nm, (bool)FALSE)) == NULL)
		return (FALSE);

	/*
	**  Build an embedded data value from the caller's parameters
	*/

	edv.ed_type = type;
	edv.ed_length = len;
	edv.ed_data = data;
	edv.ed_null = nullind;

	/*
	**  Build a db data value to reflect the type of data that
	**  will be returned from the inquire routine.
	*/

	switch ( ratt->ra_type )
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
		dbv.db_datatype = DB_INT_TYPE;
		dbv.db_length = sizeof( i4  );
		dbv.db_prec = 0;
		dbv.db_data = (PTR) &natval;
		break;

	  case DB_CHR_TYPE:
		dbv.db_datatype = DB_CHR_TYPE;
		dbv.db_length = MAXIQSET;
		dbv.db_prec = 0;
		dbv.db_data = cval;
		cptr = cval;
		break;

	  default:
		return( FALSE );
		break;
	}

	/*
	**	The current object pointers have all the information needed
	**	for the right call to get information.  Call the appropriate
	**	routine with the appropriate pointers.
	*/

	(*ratt->ra_proc)(IIarg1, IIarg2, ratt->ra_arg, dbv.db_data );

	/*
	**  If the data is 'c' type, the attribute-getting routine will
	**  have null terminated the data value.  Find the end of the
	**  string, and adjust the length appropriately so the conversion
	**  will not choke on the EOS byte.
	*/

	if ( ratt->ra_type == DB_CHR_TYPE )
	{
		dbv.db_length = STnlength( (i2) MAXIQSET+1, cptr );
	}

	/*
	**  Convert the data value into the user's embedded value
	*/

	if ( adh_dbcvtev( FEadfcb(), &dbv, &edv ) != OK )
	{
		IIFDerror(RTSIERR, 0);
		return( FALSE );
	}

	return (TRUE);
}


/*{
** Name:	IIfrsinq	-	Pre 4.0 inquire support
**
** Description:
**	Inquire about a forms runtime system variable.  This routine
**	provides compatability for the pre 4.0 style set/inquire.
**
**	This routine is part of RUNTIME's external interface as a
**	compatability cover for pre-6.0 EQUEL programs.  EQUEL still
**	accepted this style of set/inquire at 6.0.
**	
** Inputs:
**	variable	Flag - is variable
**	type		Embedded data type
**	len		Embedded data length
**	data		Embedded data value
**	name		Name of field to return
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
**	
*/

i4
IIfrsinq(i4 variable, i4 type, i4 len, PTR data, char *name)
{
	return( IIfsinqio( (i2 *) NULL, variable, type, len, data, name ) );
}

static	RTATTLIST	*
get_ratt(char *nm, bool set_condition)
{
		RTOBJLIST	*robj;
	reg	RTATTLIST	*ratt;

	/*
	**	Get attribute name to inquire and the pointer to the
	**	current FRS object defined in IIsetinq.c.
	*/
	if ((robj = IIcurobj) == NULL)
		return ((RTATTLIST *) NULL);

	/*
	**	Get name of attribute and look in the appropriate
	**	object table for that attribute
	*/
	for (ratt = robj->ro_att; ratt->ra_name != NULL; ratt++)
	{
		if (STcompare(ratt->ra_name, nm) == 0 &&
			ratt->ra_set == set_condition)
		{
			break;
		}
	}
	if (ratt->ra_name == NULL)
	{
		/* ERROR */
		IIFDerror(SIATTR, 1, nm);
		return ((RTATTLIST *) NULL);
	}
	return (ratt);
}
# else		/* PCINGRES */
static	int	junk = 0;
# endif		/* PCINGRES */
