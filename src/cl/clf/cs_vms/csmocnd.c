/*
** Copyright (C) 1987, 1992, 2000 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <exhdef.h>
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
**	2-Nov-1992 (daveb)
**	    make CS_cnd_classes GLOBALCONSTDEF for VMS sharability.
**	9-Dec-1992 (daveb)
**	    change .cl.clf to .clf.
**	13-Jan-1993 (daveb)
**	    Rename exp.clf.cs to exp.clf.vms.cs in all MO objects.
**      16-jul-93 (ed)
**	    added gl.h
**	18-jan-1996 (duursma)
**	    Cross-integration of UNIX change 412675: use MOpvget instead
**	    of MOuivget and MOptrget instead of MOuintget
**      06-feb-1998 (kinte01)
**         Cross-integrated changes from VAX VMS CL to AXP VMS CL for OI 2.0
**         16-Sep-93 (daveb)
**           Rename cnd.cnd_index to cnd_index, to match all else.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**/

MO_INDEX_METHOD CS_cnd_index;

static char index_name[] =  "exp.clf.vms.cs.cnd_index";

GLOBALCONSTDEF MO_CLASS_DEF CS_cnd_classes[] =
{
  { 0, index_name, 0, MO_READ, index_name, 0,
	MOpvget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.vms.cs.cnd_next",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_next ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_next ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.vms.cs.cnd_prev",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_prev ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_prev ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.vms.cs.cnd_waiter",
	MO_SIZEOF_MEMBER( CS_CONDITION, cnd_waiter ),
	MO_READ, index_name,
	CL_OFFSETOF( CS_CONDITION, cnd_waiter ),
	MOptrget, MOnoset, NULL, CS_cnd_index },

  { 0, "exp.clf.vms.cs.cnd_name",
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
	     i4 linstance,
	     char *instance, 
	     PTR *instdata )
{
    STATUS stat = OK;
    PTR ptr;

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
	    stat = MOulongout( MO_INSTANCE_TRUNCATED,
			      (u_i8)ptr,
			      linstance,
			      instance );
	}
	break;

    default:

	stat = MO_BAD_MSG;
	break;
    }
    return( stat );
}

/* end of csmocnd.c */
