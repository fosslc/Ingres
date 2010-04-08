/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	help.h	-	Defines for FEhelp
**
** Description:
**
** History:	$Log - for RCS$
**	<automatically entered by code control>
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**/

# define	H_EQ	0	/*  EQUEL program */
# define	H_VFRBF	1	/*  Vifred/RBF */
# define	H_IQUEL	2	/*  ISQL/IQUEL */
# define	H_VIG	3	/*  VIGRAPH layout (graphics on screen ) */
# define	H_QBFSRT 4	/*  QBF Sort form (suppresses field help) */
# define	H_GREQ	 5	/*  EQUEL with display-only graphics */
# define	H_FE	 6	/*  Standard ingres written front-end */

/*
**	External declarations.
*/

FUNC_EXTERN void	FEhkeys(char *, MENU, char **, char **, i4);
FUNC_EXTERN bool	FEhframe(char *, char *, bool, MENU, char *[], char *[], i4);
FUNC_EXTERN bool	IIRTdohelp(char *, char *, MENU, bool, i4, VOID(*)());
