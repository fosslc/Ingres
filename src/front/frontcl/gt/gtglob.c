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
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"

/**
** Name:    gtglob.c -	 Graphics System Globals
**
** Description:
**	GT global variables.
**
** History:
**	86/02/27  10:10:22  wong
**		Added 'G_locate' and 'G_restrict'.
**		
**	86/03/18  18:00:37  bobm
**		remove G_reverse flag
**		
**	86/03/24  18:20:19  bobm
**		G_internal -> G_idraw
**		
**	86/04/18  14:49:43  bobm
**		non-fatal error message for drawing errors
**
**	8/86 (rlm)	add G_ttyloc, G_myterm
**	1/89 (Mike S.)	add G_holdrd
**	2/89 (Mike S.)	add G_tc_do, G_tc_bc, and G_mfws
**	3/23/89 (Mike S)  add G_homerow for CEO status line
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**/

GLOBALDEF GR_FRAME	*G_frame;	/* Graphics System Frame */

GLOBALDEF GT_DEVICE	G_dev;		/* Graphics System device description */

GLOBALDEF Gws		*G_ws;		/* GKS workstation pointer */
GLOBALDEF Gws		*G_mfws;	/* GKS metafile workstation pointer */
GLOBALDEF Gdefloc	*G_dloc;	/* GKS locator device */

GLOBALDEF bool		G_idraw;	/* GKS Internal call flag */
GLOBALDEF bool		G_gkserr;	/* GKS SysError Flag,
					**	flags syserr on GKS errors */
GLOBALDEF i4		G_homerow;	/* Number of Home row */

/* Graphics TERMCAP - names correspond to TERMCAP capability */
GLOBALDEF char *G_tc_up, *G_tc_cm, *G_tc_nd, *G_tc_ce, *G_tc_dc;
GLOBALDEF char *G_tc_GE, *G_tc_GL, *G_tc_rv, *G_tc_re, *G_tc_cl;
GLOBALDEF char *G_tc_bl, *G_tc_be, *G_tc_do, *G_tc_bc;

/* Graphics Screen Size */

GLOBALDEF i4	G_lines;		/* Graphics lines on device */
GLOBALDEF i4	G_cols;			/* Graphics columns on device */

GLOBALDEF i4	G_reflevel;	/* refresh level (for GTrestrict state) */

/* Graphics Edit Globals */
GLOBALDEF char G_last [80];	/* last warning message */
GLOBALDEF bool G_mref;		/* menu refresh flag */

GLOBALDEF bool G_plotref;	/* plot refresh flag */
GLOBALDEF bool G_interactive;	/* interactive use of GT flag */
GLOBALDEF bool G_holdrd;	/* Whether to delay redrawing */

GLOBALDEF bool G_locate;	/* use locator flag */
GLOBALDEF bool G_restrict;	/* restrict locator flag */

/* data truncation limit */
GLOBALDEF i4  G_datchop = MAX_POINTS;

GLOBALDEF i4  (*G_mfunc)() = NULL;	/* Mode switch function pointer */
GLOBALDEF i4  (*G_efunc)() = NULL;	/* Error message function pointer */

/* TERM && TTYLOC variables - set up only by GTinit, not GTninit */
GLOBALDEF char *G_myterm;
GLOBALDEF char *G_ttyloc;
