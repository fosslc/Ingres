/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)dst.h	1.1	1/24/85";	*/

/*
** some constants used in the data structure definition
** facility
*/

/*
** ERROR codes
*/
# define	RDSTNRECUR	-1	/* Can't be recursively called */
# define	RDSTNLANG	-2	/* A bad language parameter */
# define	RDSTNALIGN	-3	/* A bad Alignment parameter */
# define	RDSTNVIS	-4	/* Bad visiblity parameter */
# define	RDSTUNKNOWN	-5	/* Don't know how to do */

/*
** LANGUAGES
*/
# define	RDSTLLOW	1
# define	RDSTLMACRO	1		/* macro */
# define	RDSTLC		2		/* C */
# define	RDSTLHI		2

/*
** ALIGNMENT TYPES
*/
# define	RDSTALOW	1
# define	RDSTAUN		1		/* unaligned */
# define	RDSTAC		2		/* As C would */
# define	RDSTAHI		2

/*
** VISIBILITY
*/
# define	RDSTVLOW	1
# define	RDSTVGLOB	1		/* GLOBAL */
# define	RDSTVLOCAL	2		/* Local */
# define	RDSTVHI		2

/*
** DATA TYPES
*/
# define	RDSTDLOW	1
# define	RDSTDLONG	1		/* Long word, 4 bytes */
# define	RDSTDSHORT	2		/* a i2 */
# define	RDSTDBYTE	3		/* a byte */
# define	RDSTDFLOAT	4		/* DB_FLT_TYPE */
# define	RDSTDDOUBLE	5		/* a f8 */
# define	RDSTDCHAR	6		/* a character */
# define	RDSTDCSTR	7		/* A STRING, must have length*/
# define	RDSTDSTRP	8		/* A string pointer */
# define	RDSTDADDR	9		/* AN ADDRESS */
# define	RDSTDHI		9
