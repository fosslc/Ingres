/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** rbfdata.c
** variables used when VIFRED is running in RBF mode,
** included here for compiling VIFRED without RBF
**
** History:
**	11/20/89 (dkh) - Corrected declaration of "Sections".
**	09-jul-90 (sandyd)
**		RbfAggSeq was added on 21-jun-90 to globs.h, but it also must be
**		added here, so we don't get undefined symbol complaints on VMS.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

# ifndef FORRBF
GLOBALDEF CS	*Cs_top = {0};		/* start of array of CS structs */
GLOBALDEF CS	Other = {0};
GLOBALDEF Sec_List Sections = {0};	/* struct of section breaks */
GLOBALDEF i2	Cs_length = {0};	/* number of attributes */
GLOBALDEF i2	Agg_count = {0};	/* number of aggregates */
GLOBALDEF FRAME	*Rbf_frame = {0};	/* ptr to RBF frame */
GLOBALDEF bool	Rbfchanges = FALSE;
GLOBALDEF i2	RbfAggSeq = {0};	/* unique seq # for FDnewfld */

bool	rFcoptions()
{
}
# endif
