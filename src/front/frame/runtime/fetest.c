/*
**  FEtest.qc
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	12/18/87 (dkh) - Fixed jup bug 1648.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**      03-Feb-96 (fanra01)
**          Extracted data for DLLs on Windows NT.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	<uigdata.h>


GLOBALREF       char    *FEtstin;
GLOBALREF       char    *FEtstout;

FUNC_EXTERN	char	*STalloc();


STATUS
FEtest(iostring)
char	*iostring;
{
	char	*comma;
	char	*in;
	char	*out;
	bool	noout = FALSE;
 	STATUS	retval;


# ifndef DGC_AOS
	if ((comma = STindex(iostring, ERx(","), 0)) == NULL)
# else
	if ((comma = STindex(iostring, ERx("~"), 0)) == NULL)
# endif
	{
		noout = TRUE;
	}
	if (!noout && *(comma + 1) == '\0')
	{
		noout = TRUE;
		*comma = '\0';
	}
	in = iostring;
	if (noout)
	{
		out = NULL;
	}
	else
	{
		out = comma;
		out++;
		*comma = '\0';
	}
 	if ((retval = FTtest(in, out)) == OK)
 	{
 		IIUIfedata()->testing = TRUE;
 	}

	if (IIUIfedata()->testing)
	{
		if (in != NULL)
		{
			FEtstin = STalloc(in);
		}
		if (out != NULL)
		{
			FEtstout = STalloc(out);
		}
	}

 	return(retval);
}
