
/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <cm.h>
# include <gn.h>
# include "gnspace.h"
# include "gnint.h"

/**
** Name:    gnlegal.c - legalize name.
**			GN internal use only
**    
** Defines
**		legalize()
**
** History:
**	Somewhere in time ...
**		Created.
**	20-Apr-93 (fredb) hp9_mpe
**		Porting changes:  MPE/iX filenames cannot contain special
**		characters, not even underbars.  Added code to 'arule
**		GNA_XUNDER' case to remove special characters instead of
**		replacing them with underbars.
**	14-Mar-1995 (peeje01)
**	    Cross integration from 6500db_su4_us42.
**          original comments follow:
** **      9-sep-93 (dkhor/poh)
** **              GNA_XUNDER : The assignment *name = '_' before the for-loop
** **              would cause CMbytecnt(name) to return one.  Move this
** **              assignment to AFTER the for-loop so that CMbytecnt(name) has
** **              a chance to process the 2nd byte of a DBL char.
** **      11-oct-93 (dkhor/poh)
** **              CMalpha cnd CMdigit cannot handle certain traditional chinese
** **              char thus producing illegal DBL CHR frame name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** table used for non-leading bytes of multibyte characters in the
** XUNDER treatment.  This assures that pure KANJI names don't get
** turned identically into "__________"
*/
static char Xch[36] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z'
};

/*{
** Name:   legalize - legalize a name.
**
** Description:
**	Uses the case rules, and non-alphanumeric treatment for a space
**	to either legalize a name, or return GNE_NAME if it can't be
**	legalized.
**
** Inputs:
**
**    sp - SPACE structure for space name goes into.
**    name - name.
**
** Outputs:
**
**    return:
**		OK
**		GNE_NAME
*/

legalize(sp,name)
SPACE *sp;
char *name;
{
	i4 i;
#ifdef hp9_mpe
	char *oname;
	char *cp;
	char *cp1;

	oname = name;	/* save pointer to start of name for CMprev */
#endif

	for (; *name != EOS; name = CMnext(name))
	{
		switch(sp->crule)
		{
		case GN_UPPER:
			if (CMlower(name))
				CMtoupper(name,name);
			break;
		case GN_LOWER:
			if (CMupper(name))
				CMtolower(name,name);
			break;
		default:
			break;
		}
		if (CMdbl1st(name) || (!CMalpha(name) && !CMdigit(name)))
		{
			switch(sp->arule)
			{
			case GNA_REJECT:
				return (GNE_NAME);
			case GNA_XPUNDER:
				if (*name == '.')
					break;
				/* fall through */
			case GNA_XUNDER:
#ifdef hp9_mpe
				/*
				** Assume the name is \0 terminated and
				** that KANJI will never be a worry ...
				*/
				cp = name;
				cp1 = name+1;
				do CMcpyinc(cp1, cp);
					while (*cp != EOS);

				CMprev(name, oname);
#else
				for (i = CMbytecnt(name) - 1; i > 0; i--)
				{
					name[i] = Xch[(name[i])%36];
					if (sp->crule == GN_UPPER)
						CMtoupper(name+i,name+i);
				}
				*name = '_';
#endif
				break;
			default:
				break;
			}
		}
	}

	return (OK);
}
