/*
**	Copyright (c) 1983, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	"nmerr.h"


/*
**	NMpathIng - Get Ingres home directory path.
**
**	Return the value of an attribute which may be systemwide
**	or per-user.  The per-user attributes are searched first.
**	A Null is returned if the name is undefined.
**
**	History:
**		3-15-83   - Written (jen)
**		3-20-83   - Changed to return status. (jen)
**		8-31-83	  - Changed to VMS compat. (dd)
**	03-aug-1987 (greg)
**	    Use II_SYSTEM not SYS_INGRES
**	    STpolycat returns pointer to buffer, use it.
**
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	26-jun-2001 (loera01)
**	   Update for multiple product builds
**	26-mar-2003 (abbjo03)
**	    Adjust result for rooted logicals.
*/

FUNC_EXTERN char * NMgetenv();

STATUS
NMpathIng(
char	**name)
{
	static	char	t_Pathname[255] = "";
		char	*t_PathPtr;
		STATUS	status = OK;

	if ((t_PathPtr = NMgetenv(SystemLocationVariable)) == NULL)
	{
		status = NM_INGUSR;
	}
	else
	{
	    /* If the path is a rooted logical, overwrite the last ']' */
	    size_t l = STlength(t_PathPtr);
	    if (t_PathPtr[l - 2] == '.' && t_PathPtr[l - 1] == ']')
	    {
		t_PathPtr[l - 1] = EOS;
		STpolycat(3, t_PathPtr, SystemLocationSubdirectory, "]",
		    t_Pathname);
	    }
	    else
	    {
		t_PathPtr  = STpolycat(4, t_PathPtr, "[", 
				SystemLocationSubdirectory, "]",  t_Pathname);
	    }
	}

	*name = t_Pathname;

	return(status);
}
