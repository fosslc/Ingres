/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<frscnsts.h>
#include	<ilops.h>
#include	<iltypes.h>
#include	<ilmenu.h>
#include	<lt.h>

/**
** Name:    igmenu.c -	OSL Interpreted Frame Object
**				Activation Generation Module.
** Description:
**	Contains the routines that maintain a stack of activation lists
**	and generate the intermediate language code statements used to
**	setup the activations (including the menus, hence the colloquial
**	name of this module) for a display loop or a sub-menu loop.
**
**	The activation list is a circular list whose elements are allocated
**	using the front-end list pool module.  The last element in the list is
**	the stack entry; the stack is maintained using the list module whose
**	interface is defined in "lt.h".
**
**	This file defines:
**
**	IGbgnMenu()	start a menu list.
**	IGactivate()	add activate element to list.
**	IGendMenu()	end a menu list and generate IL statements.
**
** History:
**	Revision 6.4
**	04/22/91 (emerson)
**		Modifications for alerters.
**	08/06/91 (emerson)
**		Modified menu activation options to be ILREFs.
**		(So that menu explanation can be a global constant - bug 35460).
**
**	Revision 6.3/03/00  90/02  wong
**	Changed menu names and FRS keys to be ILREFs to support constants as
**	menu items.
**
**	Revision 6.0  87/03/19  wong
**	Modified to support a variable number of activate (menu item,
**	function key) options.  Also, made part of ILG module.
**
**	Revision 5.1  86/10/14  20:26:48  wong
**	Initial revision.  86/07  wong
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*}
** Name:    ACTIVATE -	Activation List Element.
**
** Description:
**	A structure describing an element of an activation list.
**
** History:
**	02/90 (jhw) -- Modified menu names and FRS keys to be ILREFs.
**	08/06/91 (emerson)
**		Modified menu activation options to be ILREFs.
**		(So that menu explanation can be a global constant - bug 35460).
*/

typedef struct _a {
    i4	    act_type;	/* activation type, OLMENU, OLFIELD, ... */
    i4	    act_no;		/* activation number in list */
    IGSID   *act_sid;	/* operation SID for activation */
    union {
	struct {
	    ILREF	name;
	    ILREF	opt[OPT_MAX];
	}	menu;
#	define	act_menu act_u.menu
	char	*field;
#	define	act_fld act_u
	struct {
	    char	*field;
	    char	*column;
	}	 column;
#	define	act_col act_u.column
	char	*timeout;
#	define	act_timeout act_u.timeout
    } act_u;
    struct _a *act_next;
} ACTIVATE;

/*
** Activation List Element Pool
**	and Activation Stack Data Structures.
*/

static FREELIST	*listp = NULL;		/* list pool */
static LIST	*actlstk = NULL;	/* activation list stack */

/*{
** Name:    IGbgnMenu() -	Start an Activation List.
**
** Description:
**	Starts an activation list by pushing it on the stack.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
IGbgnMenu ()
{
    LTpush(&actlstk, (PTR)NULL, 0);
}

/*{
** Name:    IGactivate() -	Add Activation Element to List.
**
** Description:
**	Adds an activation element to the list.
**
** Input:
**	type	{nat} Activation type (e.g., OLMENU.)
**	op1	{PTR} Activation specifier.
**	op2	{PTR} Activation specifier or modifier.
**	sid	{IGSID *} IL statement reference.
**
** History:
**	07/86 (jhw) -- Written.
**	02/90 (jhw) -- Modified menu names to be ILREFs.
**	04/22/91 (emerson)
**		Modifications for alerters.
**	08/06/91 (emerson)
**		Modified menu activation options to be ILREFs.
**		(So that menu explanation can be a global constant - bug 35460)
**      30-nov-93 (smc)
**		Bug #58882
**      	Commented lint pointer truncation warning.
*/

VOID
IGactivate ( type, op1, op2, sid )
i4	type;
PTR	op1;
PTR	op2;
IGSID	*sid;
{
	register ACTIVATE	*actp;

	if (actlstk == NULL)
    		EXsignal(EXFEBUG, 1, ERx("IGactivate()"));

	if ( ( listp == NULL && (listp = FElpcreate(sizeof(*actp))) == NULL ) 
	  || (actp = (ACTIVATE *)FElpget(listp)) == NULL )
	{
		EXsignal(EXFEMEM, 1, ERx("IGactivate(mem)"));
	}

	actp->act_type = type;
	actp->act_sid = sid;
	switch (type)
	{
	  case OLMENU:
	        /* lint truncation warning if size of ptr > ILRE, 	
		   but code valid as calls of this type have op1
		   cast from ILREF to PTR to match the prototype */
		actp->act_menu.name = (ILREF)op1;
		goto act_opt;
	  case OLKEY:
		actp->act_menu.name = IGsetConst(DB_INT_TYPE, (char *)op1);
act_opt:
		if ( op2 == NULL )
		{
		    actp->act_menu.opt[OPT_EXPL]  = 0;
		    actp->act_menu.opt[OPT_VAL]   = FRS_UF;
		    actp->act_menu.opt[OPT_ACTIV] = FRS_UF;
		}
		else
		{
		    actp->act_menu.opt[OPT_EXPL]  = ((ILREF *)op2)[OPT_EXPL];
		    actp->act_menu.opt[OPT_VAL]   = ((ILREF *)op2)[OPT_VAL];
		    actp->act_menu.opt[OPT_ACTIV] = ((ILREF *)op2)[OPT_ACTIV];
		}
		break;

	  case OLFLD_ENTRY:
	  case OLFIELD:
		actp->act_fld.field = (char *)op1;
		break;

	  case OLCOL_ENTRY:
	  case OLCOLUMN:
		actp->act_col.field = (char *)op1;
		actp->act_col.column = (char *)op2;
		break;

	  case OLTIMEOUT:
		actp->act_timeout = (char *)op1;
		break;

	  case OLALERTER:
		break;

	  default:
		EXsignal(EXFEBUG, 1, ERx("IGactivate"));
		break;
	} /* end switch */

	/* add to top stack list as last element in circular list */
	if (actlstk->lt_rec == NULL)
	{
		actp->act_no = 1;
		actp->act_next = actp;
	}
	else
	{
		register ACTIVATE	*last = (ACTIVATE *)actlstk->lt_rec;

		actp->act_no = last->act_no + 1;
		actp->act_next = last->act_next;
		last->act_next = actp;
	}
	actlstk->lt_rec = (PTR)actp;
}

/*{
** Name:    IGendMenu() -	End Activation List and Generate IL Statements.
**
** Description:
**	Pops the top activation list off the stack and generates IL
**	statements for the activation list from first to last.
**
** Input:
**	submenu	{ILOP}  IL_BEGMENU or IL_BEGSUBMU.
**	sid	{IGSID *} Reference to display IL SID.
**
** History:
**	07/86 (jhw) -- Written.
**	04/87 (jhw) -- Changed to output display SID (initialize operation
**			for display loops or display submenus) as operand
**			for IL_ENDMENU operator.
**	02/90 (jhw) -- Menu and key names are now ILREFs.
**	04/22/91 (emerson)
**		Modifications for alerters.
**	08/06/91 (emerson)
**		Modified menu activation options to be ILREFs.
**		(So that menu explanation can be a global constant - bug 35460).
*/

VOID
IGendMenu (submenu, sid)
ILOP	submenu;
IGSID	*sid;
{
    register ACTIVATE	*lastp;

    if (actlstk == NULL)
	EXsignal(EXFEBUG, 1, ERx("IGendMenu()"));

    /* Note:  The activation list can be empty because of syntax errors
    ** in the source code.  So, it cannot be a bug if it is empty.
    */
    if ((lastp = (ACTIVATE *)LTpop(&actlstk)) == NULL)
	IGgenStmt(submenu, (IGSID *) NULL, 1, 0);
    else
    {
	register ACTIVATE	*actp;

	actp = lastp->act_next;	/* get 1st from circular list */
	lastp->act_next = NULL;	/* mark last */
    
	IGgenStmt(submenu, (IGSID *) NULL, 1, lastp->act_no);
	do
	{
	    switch (actp->act_type)
	    {
	      case OLMENU:
	      case OLKEY:
	      {
		ILREF	*opt;
		ILREF	expl  = 0;
		ILREF	valid = FRS_UF;
		ILREF	activ = FRS_UF;
    
		if ((opt = actp->act_menu.opt) != NULL)
		{
			expl  = opt[OPT_EXPL];
			valid = opt[OPT_VAL];
			activ = opt[OPT_ACTIV];
		}
    
		IGgenStmt(
			( actp->act_type == OLMENU ) ? IL_MENUITEM : IL_KEYACT,
			actp->act_sid,
			4, actp->act_menu.name, expl, valid, activ
		);
		break;
	      }
    
	      case OLFLD_ENTRY:
		IGgenStmt(IL_FLDENT, actp->act_sid,
		    1, IGsetConst(DB_CHR_TYPE, actp->act_fld.field)
		);
		break;

	      case OLFIELD:
		IGgenStmt(IL_FLDACT, actp->act_sid,
		    1, IGsetConst(DB_CHR_TYPE, actp->act_fld.field)
		);
		break;
    
	      case OLCOL_ENTRY:
		IGgenStmt(IL_COLENT, actp->act_sid, 2,
		    IGsetConst(DB_CHR_TYPE, actp->act_col.field),
		    IGsetConst(DB_CHR_TYPE, actp->act_col.column)
		);
		break;

	      case OLCOLUMN:
		IGgenStmt(IL_COLACT, actp->act_sid, 2,
		    IGsetConst(DB_CHR_TYPE, actp->act_col.field),
		    IGsetConst(DB_CHR_TYPE, actp->act_col.column)
		);
		break;
    
	      case OLTIMEOUT:
	      {
		ILREF	secs;
		i4	n;

		secs = ( actp->act_timeout != NULL &&
				CVan(actp->act_timeout, &n) == OK ) ? n : 0;
		IGgenStmt( IL_ACTIMOUT, actp->act_sid, 1, secs );
		break;
	      }
    
	      case OLALERTER:
		IGgenStmt(IL_ACTALERT, actp->act_sid, 0);
		break;

	      default:
		EXsignal(EXFEBUG, 1, ERx("IGendMenu()"));
		break;
	    } /* end switch */
    
	    lastp = actp->act_next;
	    FElpret(listp, actp);
	} while ((actp = lastp) != NULL);
    }
    IGgenStmt(IL_ENDMENU, sid, 0);
}
