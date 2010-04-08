/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ilmenu.h -	OSL Interpreted Frame Object Intermediate Language
**				Activation Generation Module Definitions File.
** Description:
**	Contains definitions of the interface for the activation generation
**	module of the OSL interpreted frame object intermediate langague
**	generator.
**
** History:
**	Revision 6.4
**	04/22/91 (emerson)
**		Modifications for alerters.
**
**	Revision 6.0  87/03/19  wong
**	Added activation option type enumerations.
**
**	Revision 5.1  86/09/13  01:19:30  wong
**	Moved from 5.0 "ol.h".
**/

/*
** Name:    ACT_TYPE -	Activation Type Enumeration.
*/

#define	    OLMENU	1	/* menu item */
#define	    OLFIELD	2	/* field activation */
#define	    OLCOLUMN	3	/* column activation */
#define	    OLKEY	4	/* FRS key activation */
#define	    OLFLD_ENTRY	5	/* field entry activation */
#define	    OLCOL_ENTRY	6	/* column entry activation */
#define	    OLTIMEOUT	7	/* timeout activation */
#define	    OLALERTER	8	/* alerter activation */

/*
** Name:    ACT_OPT -	Activation Option Type Enumeration.
*/

#define	    OPT_EXPL	0	/* menu item explanation */
#define	    OPT_VAL	1	/* validate switch */
#define	    OPT_ACTIV	2	/* activate switch */

#define	    OPT_MAX	3

/* Function Interface Declarations */

VOID	IGbgnMenu();
VOID	IGactivate();
VOID	IGendMenu();
