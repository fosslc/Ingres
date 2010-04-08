/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
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
# include	<rtvars.h>

/**
** Name:	rtfndfrm.c  -  Find a frame descriptor
**		       assumes name is valid.
**
**	Finds a frame descriptor.
**
*/

RUNFRM	*
RTfindfrm(char *nm)
{
	reg	RUNFRM	*runf;

	/*
	**	Search the list of initialized forms.
	*/
	for (runf = IIrootfrm; runf != NULL; runf = runf->fdlisnxt)
	{
		if ( STequal(nm, runf->fdfrmnm) )
			break;
	}

	return (runf);
}
