/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<ds.h>
# include	<ut.h>
# include	<me.h>
# include	"UTi.h"

		/* We shouldn't need this function anymore
		** 1-14-85
		** (peterk)
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
		*/

/*
** UTcommand
**
** UTcommand creates a command line. As comline points to a static buffer,
** it would be a good idea to copy it upon completion.
*/

UTcommand(dict, dsTab, program, arglist, comline, N, arg1)
	ModDictDesc		*dict;
	ArrayOf(DsTemplate *)	*dsTab;
	char			*program,
				*arglist;
	char			**comline;
	i4			N;
	PTR			arg1;
{
	SH_DESC		sh_desc;
	UT_COM		com;
	STATUS		stat;

	sh_desc.sh_type = 0;	/* initialize */
	stat = UTcomline(&sh_desc, 0, 0, (PTR)dict, (PTR)dsTab, program,
			    arglist, &com, (CL_ERR_DESC *) 0, N, &arg1);
	if (!dict || dict->modDicType == UT_LOCATION) {
		/* free the dynamic allocations (from UTeprog) */
		MEfree((PTR)com.module->modDesc);
		MEfree((PTR)com.module);
	}

	*comline = com.result.comline;
	return stat;
}
