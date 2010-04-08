# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 
# include	<ds.h>
# include	<feds.h> 
# include	<si.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
**  decls.h -- Compile Frame Global Declarations
**
** History:
**	03/04/87 (dkh) - Added support for ADTs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern FILE	*fcoutfp;		/* output file pointer */
extern char	*fcoutfile;		/* output file name */
extern char	**fcntable;		/* list of form names */
extern char	*fcnames[];		/* list of form names */
extern char	*fcdbname;		/* database name */
extern char	*fcname;
extern i4	fclang;			/* language to use (C or Macro) */
extern i4	fceqlcall;
extern FRAME	*fcfrm;			/* frame pointer address */
extern bool	fcrti;			/* Internal RT conventions */
extern bool	fcstdalone;		/* Is this a standalone? */

/* standalone, allow maximum of 2 forms per file */
# define	NUMFORMS	2

extern char	*valstr;	/* will contain "_v%s" */
