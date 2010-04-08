/*
**	Copyright (c) 2004 Ingres Corporation
*/

/* # include's */

/**
** Name:	tu.h -	Defines for Tables Utility.
**
** Description:
**	This file contains the typedef for TU_MENUSTRUCT, the structure
**	for the optional menu item in the Tables Utility. It also contains
**	the definitions for the special bitmask flags, indicating which
**	of the 'special' Tables Utility menu items should be active.
**	This file defines:
**
**	TU_MENUSTRUCT	typedef for the optional menu item in the 
**			tables utility.
**
** History:
**	12-mar-1987 (daver)	First Written.
**	24-apr-1987 (daver)	Added end_not_quit field; constants 
**				TU_QUIT_ENABLED and TU_ONLYQUIT_ENABLED.
**	23-Aug-2009 (kschendel) 121804
**	    Add some prototypes to keep gcc 4.3 happier.
**/

/* # define's */

# define	TU_NONE			0
# define	TU_QBF_ENABLED		1
# define	TU_REPORT_ENABLED	2
# define	TU_QUIT_ENABLED		4
# define	TU_ONLYQUIT_ENABLED	8
# define	TU_EMPTYCATS		16

/* extern's */
/* static's */

/*}
** Name:	TU_MENUSTRUCT - Structure for Table Utility's optional menuitem.
**
** Description:
**	This is the structure used to create an optional menuitem in the
**	Tables Utility. This should only be used if the Tables Utility's
**	main procedure is called as a subroutine. If the main procedure is
**	called as a standalone (via utexe.def), the menuitem will not appear.
**
**	The structure holds the optional menuitem's name (to be displayed
**	in the top frame of the Tables Utility), the address of the routine
**	to be executed, a PTR some memory containing the routine's parameters,
**	and an explanation string for Lotus Menus (not currently used).
**
** History:
**	12-mar-1987 (daver)	First Written.
*/
typedef struct
{
	char		*name;		/* menuitem name to be displayed */
	VOID		(*rout)();	/* caller's routine to be executed */
	PTR		mem;		/* memory for routine's parameters */
	char		*expl;		/* explanation string for lotus menus */
	bool		end_not_quit;	/* TRUE if end was hit; FALSE if quit */

} TU_MENUSTRUCT;

/* Function prototypes */

FUNC_EXTERN bool FEecats(void);

/* No full prototype on these thanks to eqc limitations */
FUNC_EXTERN bool ltcheck();
