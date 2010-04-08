/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** MO utilities for UNIX CS
*/

# include <compat.h>
# include <gl.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>

# include "csmgmt.h"

/**
** History:
**	27-Sep-2001 (inifa01) bug 103257 ingsrv 1324
**		IMA ima_server_sessions contains weird values in session_id (\0001 
**		at end of each session_id) and ima_server_sessions_extra contains 
**		no rows.
**		Resultant integer on the str_to_uns() conversion is out of the bounds
**		of an 'int' on 64 bit platforms, SPlookup() gets passed a bogus key
**		and returns MO_NO_INSTANCE as a result.
**		Changed all u_longnat to u_long.
**/
static void
str_to_uns(a, u)
register char	*a;
u_long		*u;
{
    register u_long	x; /* holds the integer beeing formed */
    
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
    u_long lval;
    
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
    u_long lval;
    
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

