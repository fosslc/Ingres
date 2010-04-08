/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    osloop.h -	OSL Parser Loop Block Definitions.
**
** Description:
**	This file defines the loop block types recognized by the loop block
**	module of the OSL parser.
**
** History:
**	Revision 6.4/02
**	10/01/91 (emerson)
**		Changes for bugs 34788, 36427, 40195, 40196:
**		Split LP_ENDMENU into LP_DISPMENU and LP_RUNMENU;
**		Changed defined "LP_" constants to be powers of 2
**		so they can be or'ed together into bit masks;
**		added defines for LP_NONDISPLAY and LP_LABEL.
**		Note that LP_NONE now has a different meaning.
**
**	Revision 6.2  89/05  wong
**	Added LP_ENDMENU.
**
**	Revision 6.0  87/02/07  wong
**	Added LP_NONE, and LP_QWHILE for query while loops.
**
**	Revision 5.1  86/10/17  16:38:42  wong
**	Initial revision (copied from 5.0.)
*/

/*
** Constants
*/
#define		LP_DISPLAY	0x0001	/* A display/submenu loop (top) */
#define		LP_WHILE	0x0002	/* A while loop */
#define		LP_UNLOADTBL	0x0004	/* An unloadtable loop */
#define		LP_QUERY	0x0008	/* A query with a submenu */
#define		LP_QWHILE	0x0010	/* A query while loop */
#define		LP_IF		0x0020	/* An if or elseif block */
#define		LP_DISPMENU	0x0040	/* An unattached display submenu */
#define		LP_RUNMENU	0x0080	/* An unattached run submenu */

#define		LP_NONDISPLAY	0x00fe	/* Any of the above, except LP_DISPLAY*/

#define		LP_LABEL	0x4000	/* An artificial value, used to indicate
					** that a labelled loop is desired */

#define		LP_NONE		0
