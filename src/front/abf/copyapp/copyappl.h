/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**	copyappl.h
**
**
**	Defines
**		Header file for copyappl utility
**
**	Called by:
**		All routines in copyappl utility
**
**	History:
**		Written 11/15/83 (agh)
**		17-sep-86 (marian) bug 9740
**			Add form_ext to make fix portable (stolen from
**			abfconsts.h)
**
*/
# include	<er.h>
# include	"caerror.h"
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	<st.h>

/*
** global flags for copyappl
*/
extern bool	caPromptflag;	/* prompt when duplicate names found? */
extern bool	caReplaceflag;	/* replace old object when duplicate name? */
extern bool	caQuitflag;	/* back out if duplicate name found? */

extern char	*caUname;	/* name of user for -u flag */
extern char	*caXpipe;	/* to be used with -X flag */

# define	CAMAXOBJS	100
		/* maximum number of objects of any one type in application */
# define	CAMAXLEN	2048
		/* maximum length of a line in an intermediate file */

/*
** VMS gets .mar forms.	 UNIX and CMS get C.
*/

# ifdef VMS
#	define	FORM_EXT	ERx("mar")
# else
#	define	FORM_EXT	ERx("c")
# endif

