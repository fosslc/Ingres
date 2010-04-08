/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <clconfig.h>
# include <cs.h>
# include <mo.h>

# include <csinternal.h>
# include "cslocal.h"

# include "csmgmt.h"

/**
** Name:	csmocnd.c	- CS MO condition management
**
** Description:
**	Defines MO classes for conditions, and functions to
**	be used to attach and detach them.
**
**	CS_cnd_index	- MO index for CS_CONDITIONS
**
** History:
**	29-Oct-1992 (daveb)
**	    documented
**	18-Nov-1992 (daveb)
**	    correct "cs.cnd.cnd_index" to "cs.cnd_index"
**	13-Jan-1993 (daveb)
**	    Rename MO objects to include a .unix component.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      01-sep-93 (smc)
**          Changed MOulongout to MOptrout to be portable to axp_osf.
**      10-Mar-1994 (daveb)  60514
**          use MOpvget for other pointer fields.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**	    added OS_THREADS_USED and cnd_waiters
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Dec-2003 (jenjo02)
**	    Deleted cnd_waiters, no longer used.
**/

MO_INDEX_METHOD CS_cnd_index;

static char index_name[] =  "exp.clf.unix.cs.cnd_index";

GLOBALDEF MO_CLASS_DEF CS_cnd_classes[] =
{
  { 0, index_name, 0, MO_READ, index_name, 0,
	MOpvget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.unix.cs.cnd_next",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_next ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_next ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.unix.cs.cnd_prev",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_prev ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_prev ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.unix.cs.cnd_waiter",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_waiter ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_waiter ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.unix.cs.cnd_name",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_name ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_name ),
	MOstrpget, MOnoset, NULL, CS_cnd_index },

  { 0 }
};


/*{
** Name:	CS_cnd_index	- MO index for CS_CONDITIONS
**
** Description:
**	Standard MO index function for CS_CONDITIONS.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT
**	cdata		ignored
**	linstance	length of instance buffer
**	instance	the instance in question
**
** Outputs:
**	instance	on GETNEXT, stuffed with found instance
**	instdata	stuffed with a pointer to the CS_CONDITION.
**
** Returns:
**	OK or various MO index function returns.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/
STATUS
CS_cnd_index(i4 msg,
	     PTR cdata,
	     i4  linstance,
	     char *instance, 
	     PTR *instdata )
{
    STATUS stat = OK;
    PTR ptr;

# ifdef OS_THREADS_USED
    CS_synch_lock( &Cs_srv_block.cs_cnd_mutex );
# endif /* OS_THREADS_USED */
    switch( msg )
    {
    case MO_GET:

	if( OK == (stat = CS_get_block( instance,
				       Cs_srv_block.cs_cnd_ptree,
				       &ptr ) ) )
	    *instdata = ptr;
	break;

    case MO_GETNEXT:

	if( OK == ( stat = CS_nxt_block( instance,
					Cs_srv_block.cs_cnd_ptree,
					&ptr ) ) )
	{
	    *instdata = ptr;
	    stat = MOptrout( MO_INSTANCE_TRUNCATED,
			      ptr,
			      linstance,
			      instance );
	}
	break;

    default:

	stat = MO_BAD_MSG;
	break;
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock( &Cs_srv_block.cs_cnd_mutex );
# endif /* OS_THREADS_USED */
    return( stat );
}

/* end of csmocnd.c */
