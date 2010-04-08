/*
** static char	Sccsid[] = "%W%	%G%";
*/

/*
** UTetable
**
** Scan an in-memory data structure as the moduleDictionary
**
**  History:
**	10/13/92 (dkh) - Removed routine UTetable() which no one uses.
**	11-aug-93 (ed)
**	    added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
*/

# include	<compat.h>
# include	<gl.h>
# include	<array.h>
# include	<st.h>
# include	<ut.h>

STATUS
UTemodtype(s)
char *s;
{
	switch (*s) {
	  case 'C': case 'c':	return(UT_CALL);
	  case 'P': case 'p':	return(UT_PROCESS);
	  case 'O': case 'o':	return(UT_OVERLAY);
	  default:	return(NULL);
	}
}
