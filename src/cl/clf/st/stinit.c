/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

/*
** Copyright (c) 2010 Ingres Corporation
**
**      STinit - Initialize string function pointers based on character set
**
**	History:
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Created
*/

void
STinit()
{
	if (!(CMGETDBL))
	{
	    ST_fvp.IISTscompare = STscompare_SB;
	    ST_fvp.IISTbcompare = STbcompare_SB;
	    ST_fvp.IISTgetwords = STgetwords_SB;
	    ST_fvp.IISTindex = STindex_SB;
	    ST_fvp.IISTrindex = STrindex_SB;
	    ST_fvp.IISTrstrindex = STrstrindex_SB;
	    ST_fvp.IISTscompare = STscompare_SB;
	    ST_fvp.IISTstrindex = STstrindex_SB;
	    ST_fvp.IISTtrmnwhite = STtrmnwhite_SB;
	    ST_fvp.IISTtrmwhite = STtrmwhite_SB;
	    ST_fvp.IISTzapblank = STzapblank_SB;
	}
}

