/*
** MO utilities for UNIX CS
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>

# include "csmgmt.h"

/*
** History:
**	??-???-??? Created
**
**	16-jan-1996 (duursma)
**	    Cross-integrated part of fix for bug 62320 from UNIX CL.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**
*/

static void
str_to_uns(a, u)
register char	*a;
u_i4		*u;
{
    register u_i4	x; /* holds the integer being formed */
    
    /* skip leading blanks */
    while ( *a == ' ' )
	a++;
    
    /* at this point everything had better be numeric */
    for (x = 0; *a <= '9' && *a >= '0'; a++)
    {
	x = x * 10 + (*a - '0');
    }
    
    *u = x;
}

/*
**  Given address of block, and a tree, return pointer to the
**	actual block if found.
*/
STATUS
CS_get_block( char *instance, SPTREE *tree, PTR *block )
{
    SPBLK spblk;
    SPBLK *sp;
    STATUS ret_val = OK;
    u_i4 lval;
    
    str_to_uns( instance, &lval );
    spblk.key = (PTR)lval;

    if( tree == NULL || (sp = SPlookup( &spblk, tree ) ) == NULL )
	ret_val = MO_NO_INSTANCE;
    else
	*block = (PTR)sp->key;
    return( ret_val );
}

/* ---------------------------------------------------------------- */

/* utilities for index methods */

STATUS
CS_nxt_block( char *instance, SPTREE *tree, PTR *block )
{
    SPBLK spblk;
    SPBLK *sp;
    STATUS ret_val = OK;
    u_i4 lval;
    
    if( *instance == EOS )	/* no instance, take the first one */
    {
	if( tree == NULL || (sp = SPfhead( tree )) == NULL )
	    ret_val = MO_NO_NEXT;
	else
	    *block = (PTR)sp->key;
    }
    else			/* given an instance */
    {
	str_to_uns( instance, &lval );
	spblk.key = (PTR)lval;
	if( tree == NULL || (sp = SPlookup( &spblk, tree )) == NULL )
	    ret_val = MO_NO_INSTANCE;
	else if( (sp = SPfnext( sp ) ) == NULL )
	    ret_val = MO_NO_NEXT;
	else
	    *block = (PTR)sp->key;
    }
    return( ret_val );
}

