/*
**	NMgtAt - Get Attribute.
**
**	Return the value of an attribute which may be systemwide
**	or per-user.  The per-user attributes are searched first.
**	A Null is returned if the name is undefined.
**
**	History:
**		3-1-83   - Written (jen)
**		9-29-83	 VMS CL (dd)
**		2-90	 Make NMgtAT a VOID, to agree with spec.
**
**	Copyright (c) 1983 - Ingres Corporation
*/

# include	 <compat.h>  
#include    <gl.h>
# include	 <nm.h>

FUNC_EXTERN	char *NMgetenv();

VOID
NMgtAt(name, value)
char	*name;
char	**value;
{
	*value = NMgetenv(name);
}
