/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>

/**
** Name:	iiifrsvar.c
**
** Description:
**
**	Contains global definitions of forms runtime system variables.
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**	16-feb-83  -  Extracted from original runtime.c (jen)
**	7/9/86 (bab/dkh) Changed all ctrlZ's to ctrlQ's in
**		IIvalcomms, with the exception of #9.  Fix
**		for bug 9837 (changed to ctrlQ because ^Q
**		can't be activated on.)
**	12/23/86 (dkh) - Added support for new activations.
**	11/09/89 (dkh) - Added definition of IIFRIOIntOpt.
**      04/23/96 (chech02)
**          changed IIfrscb to a FAR pointer for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**/

GLOBALDEF RUNFRM	*IIrootfrm = NULL;	/* Top of list of available frames */
GLOBALDEF RUNFRM	*IIstkfrm = NULL;	/* Top of list of active frames */
GLOBALDEF RUNFRM	*IIfrmio = NULL;	/* Current frame for GET/PUTFORM */
GLOBALDEF FRAME	*IIprev = NULL;		/* Previous displayed frame */
GLOBALDEF i4	IIresult = 0;		/* Return status of frame driver */
GLOBALDEF i4	IImenuptr = 0;		/* Index into menu command structure */

GLOBALDEF i4	IIfromfnames = 0;	/* Flag indicating if we were running from FORMDATA loop */

# ifdef	ATTCURSES
GLOBALDEF i2	IIvalcomms[] =		/* List of valid commands */
# else
GLOBALDEF u_char	IIvalcomms[] =		/* List of valid commands */
# endif
{
	ctrlY,			/* 0 - c_CTRLY */
	ctrlC,			/* 1 - c_CTRLC */
	ctrlQ,			/* 2 - c_CTRLQ */
	ctrlQ,			/* 3 - c_CTRLESC */
	ctrlT,			/* 4 - c_CTRLT */
	ctrlF,			/* 5 - c_CTRLF */
	ctrlG,			/* 6 - c_CTRLG */
	ctrlB,			/* 7 - c_CTRLB */
	ctrlQ,			/* 8 - c_CTRLZ  ctrl_V now reserved for edit */
	ctrlZ,			/* 9 - c_CTRLZ */
# ifndef	ATTCURSES
	ctrlQ,			/* 10 - c_MENUCHAR */
# else	/* isATTCURSES */
	ctrlESC,		/* 10 - c_MENUCHAR */
	KEY_F(1),		/* 11 - c_F1 */		/* function key 1 */
	KEY_F(2),		/* 12 - c_F2 */
	KEY_F(3),		/* 13 - c_F3 */
	KEY_F(4),		/* 14 - c_F4 */
	KEY_F(5),		/* 15 - c_F5 */
	KEY_F(6),		/* 16 - c_F6 */
	KEY_F(7),		/* 17 - c_F7 */
	KEY_F(8),		/* 18 - c_F8 */
# endif
};

GLOBALDEF TBSTRUCT	*IIcurtbl = NULL;


/*
**  New runtime control block global.
*/
#ifdef WIN16
GLOBALDEF	FRS_CB	*FAR IIfrscb = NULL;
#else
GLOBALDEF	FRS_CB	*IIfrscb = NULL;
#endif


/*
**  Internal use only flag for the runtime layer.
*/
GLOBALDEF	i4	IIFRIOIntOpt = 0;
