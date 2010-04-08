/*
**	iisetparm.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
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
# include	<cm.h> 
# include	<er.h> 
# include	<rtvars.h>

/**
** Name:	iisetparm.c	-	Paramaterized set support
**
** Description:
**
**	Public (extern) routines defined:
**		IIsetparam()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	02/19/89 (dkh) - Put in code to handle formats bigger than
**			 MAXFRSPARAM.
**	03/23/89 (dkh) - Put in change to pass in correct length to IIstrconv().
**	09-mar-1992 (mgw) - Bug #41149
**		Don't return from IIsetparam() on error processing any one
**		parameter until all parameters have been processed.
**	01-apr-1993 (swm)
**		Removed forward declaration of MEreqmem(), this is now
**		inherited from <me.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	i4	IIParProc();

/*{
** Name:	IIsetparam	-	Paramaterized set stmt support
**
** Description:
**	This routine provides the user with a param interface to IIsetfld
**	or IIsetcol similar to that found in an Param EQUEL statement.  
**	The caller passes the specific routine,a target list and an argument 
**	vector.  This routine parses the target list and gives the argument 
**	vector value for each format.
**
**	The form of the target list is:
**
**		<objectname> = %<format>  [, <objectname> = %<format> ]
**
**	Where:
**		<format> is one of	"%i1", "%i2", "%nat", "%i4",
**					"%f4", "%f8", "%c", "%cn",
**					"%vn", "%d"
**		and allows an optional NULL indicator.
**
** Inputs:
**	setfunc		Address of setting function
**	infmt		The target list format
**	inargv		The argument vector
**	transfunc	Entry point for non C params
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
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	17-may-1983  -	Modified to generalize over all "setting"
**			functions. (ncg)
**	20-dec-1983  -	Fixed bug 1870 (tabs included). (ncg)
**	24-sep-1984  -	Added query option for param statements. (ncg)
**	27-aug-1985  -  Added code for %d option to skip items. (joe)
**	01/09/87 (dkh) - ADT stubbing changes.
**	25-feb-1987  -  Modified to call IIparProc to parse the format
**			string, and to pass the NULL indicator to
**			the 'setting' routine. (drh)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**
*/

IIsetparam(setfunc, infmt, inargv, transfunc)
i4	(*setfunc)();			/* address of setting function */
char	*infmt;				/* the target list format */
char	**inargv;			/* the argument vector */
char	*(*transfunc)();		/* entry point for non-C param */
{
	i4	type;			/* datatype as parsed from format */
	i4	length;			/* length as parsed from format */
	char	objnm[MAXFRSNAME+1];	/* buffer for object name */
	char	*object;		/* pointer to object name */
	char	bfmtstr[MAXFRSPARAM +1];	/* buffer for format string */
	char	*fmtstr;
	char	*format;		/* ptr to fmtstr */
	char	**argv;			/* pointer for argument vector */
	char	*arg;
	char	*qrycp;			/* check for putoper(%s) format */
	i4	qry;
	i4	advance;
	i4	has_nullind;
	i2	*nullind;
	i4	parflag;
	char	**targv;
	i4	retval = TRUE;
	

	/*
	**	If there is not target list or argument vector, return
	*/
	if(infmt == NULL || inargv == NULL)
		return (TRUE);

	/*
	**	Copy argument vector pointer.  Other wise in FORTRAN, the
	**	pointer to this variable gets changed.
	*/
	argv = inargv;

	/*
	**	Copy the target list to chop up the string, get rid of
	**	any upper-case characters.
	*/

	fmtstr = bfmtstr;
	if ((length = STlength(infmt)) > MAXFRSPARAM)
	{
		if ((fmtstr = (char *) MEreqmem((u_i4) 0, (u_i4) length + 1,
			FALSE, (STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIsetparam"));
		}
	}

	format = IIstrconv(II_CONV, infmt, fmtstr, (i4)length);

	/*
	**	Parse the target list until there is not more string.
	*/
	while (*format != EOS)
	{
		/*
		**	Eat up white space.
		*/
		while (CMwhite(format) || *format == ',')
			CMnext(format);
		if (*format == '\0')
			break;
		/*
		**	Check for PUTOPER(%x) format.  If found skip it, and
		**	position on % name.
		*/
		qry = FALSE;
		object = objnm;
		qrycp = format;
		while (CMnmstart(qrycp))
			CMcpyinc(qrycp, object);
		*object = '\0';
		if (STcompare(objnm, ERx("putoper")) == 0)
		{
			qry = TRUE;
			/*
			**  Skip to next % name
			*/
			while (CMwhite(qrycp) || *qrycp == '(')
				CMnext(qrycp);
			format = qrycp;
		}
		/*
		**	Assign all alphanumeric characters to the object
		**	name buffer.  This is the object portion.
		**	Added _ for names - (jen) 6-20-83
		*/
		object = objnm;
		while (CMnmchar(format))
			CMcpyinc(format, object);
		*object = '\0';
		/*
		**	Eat up any white space and ')' for PUTOPER.
		*/
		while (CMwhite(format) || *format == ')')
			CMnext(format);
		/*
		**	There should be an equal sign here, else there is
		**	an error in the target list string.
		*/
		if (*format++ != '=')
		{
			IIFDerror(RTSFEQ, 1, IIfrmio->fdfrmnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return (FALSE);
		}
		/*
		**	Eat up any white space
		*/
		while (CMwhite(format))
			CMnext(format);
		/*
		**	Parse the format portion of the expression.
		*/

		if ( *format != '%' )
		{
			IIFDerror(RTSFPC, 1, IIfrmio->fdfrmnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return (FALSE);
		}
		else
			format++;	/* skip the % */

		type = 0;
		length = 0;
		has_nullind = FALSE;
		parflag = 0;
		parflag |= II_PARFRS;
		parflag |= II_PARSET;

		advance = ( IIParProc( format, &type, &length, &has_nullind,
			(PTR) NULL, parflag ));

		if ( advance < 0 )	/* error in format string */
		{
			IIFDerror(RTSFTF, 1, IIfrmio->fdfrmnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return( FALSE );
		}
		format += advance;	/* num chars read by IIparProc */

		/*
		** Check to see if we should skip this item.
		*/

		if (type == DB_DMY_TYPE)
		{
			argv++;
			if (has_nullind && *argv != NULL)
			{
				argv++;
			}
			continue;
		}
		/*
		**	Take the current pointer in the argument vector.
		**	If there is no pointer there is an error.
		*/
		if (*argv == NULL)
		{
			IIFDerror(RTSFFP, 2, IIfrmio->fdfrmnm, objnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return (FALSE);
		}

		/*
		**  Set up null indicator to pass to setting function.
		*/

		nullind = 0;
		if (has_nullind)
		{
			targv = argv;

			if (*++targv != NULL)
			{
				if ( transfunc )
				{
					nullind = (i2 *) (*transfunc)
					  (DB_INT_TYPE, sizeof(i2), *targv);
				}
				else
				{
					nullind = (i2 *) *targv;
				}
			}
			else
			{
				if (fmtstr != bfmtstr)
				{
					MEfree(fmtstr);
				}
				return( FALSE );
			}
		}

		/*
		**	Run IIsetXXX()	with the information gathered.
		*/
		if (qry)
			IIputoper((i4)1);
		if ( transfunc )
			arg = (*transfunc)( type, length, *argv );
		else
			arg = *argv;
		if (!(*setfunc)(objnm, nullind, 1, type, length, arg))
		{
			retval = FALSE;
		}
		/*
		**	Set to next argument
		*/
		argv++;

		if (has_nullind)
		{
			argv++;
		}
	}
	if (fmtstr != bfmtstr)
	{
		MEfree(fmtstr);
	}
	return (retval);
}
