/*
**Copyright (c) 2010 Ingres Corporation
**
**	History:
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Created
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

GLOBALDEF  ST_FUNCTIONS    ST_fvp = { STbcompare_DB, STgetwords_DB, STindex_DB, STrindex_DB, STrstrindex_DB,  \
STscompare_DB, STstrindex_DB, STtrmnwhite_DB, STtrmwhite_DB, STzapblank_DB };

