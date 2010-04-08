/*
**	iiretparm.c
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
** Name:	iiretparm.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIretparam()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	02/19/89 (dkh) - Put in code to handle formats bigger than
**			 MAXFRSPARAM.
**	03/22/89 (dkh) - Put in change to pass in correct length to IIstrconv().
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


/*{
** Name:	IIretparam 	-	Paramaterized IIretfld call
**
** Description:
**	This routine provides the user with a param interface to IIretfld()
**	and IItretcol() similar to that found in an Param EQUEL statement.  
**	The caller passes the specific function, a target list and an argument
**	vector.  This routine parses the target list and gives the argument 
**	vector value for each format and calls the passed routine.
**
**	The parsing of the formats is done by IIParProc, a shared routine
**	from LIBQ.
**
**	The form of the target list is:
**
**		%<format> = <objectname> [, %<format> = <objectname>]
**
**	Where:
**		<format> is one of	"%i1", "%i2", "%nat", "%i4"
**					"%f4", "%f8", "%c", "%cn",
**					"%vn", and "%d"
**		and allows an optional null indicator.
**
**
** Inputs:
**	retfunc		Address of the returning function
**	infmt		Ptr to the target list
**	inargv		Argument vector
**	transfunc	Entry point for non-C param
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
**	17-may-1983  -	Modified to generalize over all "returning"
**			functions. (ncg)
**	20-dec-1983  -	Fixed bug 1870 (tabs included). (ncg)
**	24-feb-1984  -	Added query option for param statements. (ncg)
**	27-aug-1985  -  Added code for %d to skip items. (Joe)
**	01/09/87 (dkh) - ADT stubbing changes.
**	25-feb-1987  -  Added call to IIParProc, and made change to pass
**			null indicator directly to retfunc (drh)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
*/

IIretparam(retfunc, infmt, inargv, transfunc)
i4	(*retfunc)();			/* address of "returning" function */
char	*infmt;				/* pointer to the target list */
char	**inargv;			/* argument vector */
i4	(*transfunc)();			/* entry point for non-C param */
{
	i4	type;			/* datatype of the format found */
	i4	length;			/* datatype length of format */
	i4	data[ DB_MAXSTRING/sizeof(i4) +1];
	char	objnm[MAXFRSNAME+1];	/* buffer to store object name */
	char	*object;		/* ptr to object name */
	char	bfmtstr[MAXFRSPARAM +1];	/* storage for format string */
	char	*fmtstr;
	reg	char	*format;	/* pointer to fmtstr */
	char	**argv;			/* pointer to argument vector */
	char	*arg;
	char	*qrycp;			/* check for getoper(%s) format */
	i4	qry;
	i4	advance;
	i4	has_nullind;
	i2	*nullind;
	i2	nuldata;
	i4	parflag;
	char	**targv;
	
	
	/*
	**	If no target list or argument vector, return
	*/
	if (infmt == NULL || inargv == NULL)
		return (TRUE);

	fmtstr = bfmtstr;
	if ((length = STlength(infmt)) > MAXFRSPARAM)
	{
		if ((fmtstr = (char *) MEreqmem((u_i4) 0, (u_i4) length + 1,
			FALSE, (STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIretparam"));
		}
	}

	format = IIstrconv(II_CONV, infmt, fmtstr, (i4)length);

	/*
	**	Reassign argument vector.  If you use inargv, the FORTRAN
	**	variable this represents keeps getting incremented.
	*/
	argv = inargv;

	/*
	**	Parse the target list until there is no more string
	*/
	while (*format != EOS)
	{
		/*
		**	Eat up white space, commas, and ')' for getoper.
		*/
		while (CMwhite(format) || *format == ',' || *format == ')')
			CMnext(format);
		if (*format == '\0')
			break;

		/*
		**	Get the format string.  We share this routine with LIBQ
		*/
		
		if ( *format != '%')
		{
			IIFDerror(RTSFPC, 1, IIfrmio->fdfrmnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return( FALSE );
		}
		else
			format++;

		type = 0;
		length = 0;
		has_nullind = FALSE;
		parflag = 0;
		parflag |= II_PARFRS;
		parflag |= II_PARRET;

		advance = ( IIParProc( format, &type, &length, &has_nullind,
			(char *) NULL, parflag ) );
		if ( advance < 0 )	/* error in format string */
		{
			IIFDerror(RTSFTF, 1, IIfrmio->fdfrmnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return( FALSE );
		}
		format += advance;	/* num chars processed in IIpdatype */

		/*
		**	Eat up white space
		*/
		while (CMwhite(format))
			CMnext(format);
		/*
		**	There should now be an equal sign
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
		**	Eat up white space
		*/
		while (CMwhite(format))
			CMnext(format);
		/*
		**	Check for GETOPER(object) format.  If found skip it, and
		**	position on object name.
		*/

		qry = FALSE;
		object = objnm;
		qrycp = format;
		while (CMnmstart(qrycp))
			CMcpyinc(qrycp, object);
		*object = '\0';
		if (STcompare(objnm, ERx("getoper")) == 0)
		{
			qry = TRUE;
			/*
			**  Skip to next name
			*/
			while (CMwhite(qrycp) || *qrycp == '(')
				CMnext(qrycp);
			format = qrycp;
		}

		/*
		**	Reposition the object name buffer and assign
		**	all alphanumeric characters to get the object name.
		**	Added '_' format.	20-jun-83 (jen)
		*/

		object = objnm;
		while (CMnmchar(format))
			CMcpyinc(format, object);
		*object = '\0';

		/*
		** If type is DB_DMY_TYPE then don't do anything with
		** this part of the param string, but skip pass the
		** argv.
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
		**	If there is no variable pointer, this is an error
		*/

		if (*argv == NULL)
		{
			IIFDerror(RTSFFP, 2, IIfrmio->fdfrmnm, objnm);
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return(FALSE);
		}

		/*
		**  Set up null indicator interface so the RETFUNC
		**  call below will properly handle things.
		*/
		nullind = 0;
		if (has_nullind)
		{
			targv = argv;
			if (*++targv != NULL)
			{
				if ( transfunc )
					nullind = &nuldata;
				else
					nullind =  (i2 *) *targv;
			}
			else
			{
				if (fmtstr != bfmtstr)
				{
					MEfree(fmtstr);
				}
				return(FALSE);
			}
		}

		if (qry)
			IIgetoper( (i4)1 );
		if ( transfunc )
			arg = (char *)data;
		else
			arg = *argv;
		if (!(*retfunc)(nullind, 1, type, length, arg, objnm))
		{
			if (fmtstr != bfmtstr)
			{
				MEfree(fmtstr);
			}
			return (FALSE);
		}
		if ( transfunc )	/* non-C translation */
		{
			(*transfunc)( type, length, (char *)data, *argv);

			if ( has_nullind )
			{
				(*transfunc)( DB_INT_TYPE, sizeof(i2), &nuldata,
					*targv );
			}
		}

		/*
		**	Get the next pointer
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
	return (TRUE);
}
