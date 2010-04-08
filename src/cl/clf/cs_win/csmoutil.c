/*
**  Copyright (c) 1995, 2004 Ingres Corporation
*/

# include <compat.h>
/**# include <sys\types.h>**/
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>

# include "csmgmt.h"

/*
**  MO utilities for UNIX CS
**
**  History:
**	13-oct-2003 (somsa01)
**	    Use u_i8 instead of u_i4 on LP64 platforms.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
*/

static void
str_to_uns(a, u)
	register char  *a;
#ifdef LP64
	u_i8           *u;
#else
	u_i4           *u;
#endif
{
#ifdef LP64
	register u_i8      x;	/* holds the integer beeing formed */
#else
	register u_i4      x;	/* holds the integer beeing formed */
#endif

	/* skip leading blanks */
	while (*a == ' ')
		a++;

	/* at this point everything had better be numeric */
	for (x = 0; *a <= '9' && *a >= '0'; a++) {
		x = x * 10 + (*a - '0');
	}

	*u = x;
}

/*
**  Given address of block, and a tree, return pointer to the
**	actual block if found.
*/
STATUS
CS_get_block(char *instance, SPTREE * tree, PTR * block)
{
	SPBLK           spblk;
	SPBLK          *sp;
	STATUS          ret_val = OK;
#ifdef LP64
	u_i8            lval;
#else
	u_i4            lval;
#endif

	str_to_uns(instance, &lval);
	spblk.key = (PTR) lval;

	if ((sp = SPlookup(&spblk, tree)) == NULL)
		ret_val = MO_NO_INSTANCE;
	else
		*block = (PTR) sp->key;
	return (ret_val);
}

/* ---------------------------------------------------------------- */

/* utilities for index methods */

STATUS
CS_nxt_block(char *instance, SPTREE * tree, PTR * block)
{
	SPBLK           spblk;
	SPBLK          *sp;
	STATUS          ret_val = OK;
#ifdef LP64
	u_i8            lval;
#else
	u_i4            lval;
#endif


	if (*instance == EOS) {	/* no instance, take the first one */
		if ((sp = SPfhead(tree)) == NULL)
			ret_val = MO_NO_NEXT;
		else
			*block = (PTR) sp->key;
	} else {		/* given an instance */
		str_to_uns(instance, &lval);
		spblk.key = (PTR) lval;
		if ((sp = SPlookup(&spblk, tree)) == NULL)
			ret_val = MO_NO_INSTANCE;
		else if ((sp = SPfnext(sp)) == NULL)
			ret_val = MO_NO_NEXT;
		else
			*block = (PTR) sp->key;
	}
	return (ret_val);
}
