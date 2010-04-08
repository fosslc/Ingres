/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)pv.h	1.1	1/24/85";	*/

/*
**  PV.H -- definitions for parameter vectors
*/

# ifndef PV_MAXPC


/* setable constants */
# define	PV_MAXPC	384	/* maximum number of parameters */

/* the parameter vector type */
typedef struct
{
	i2	pv_type;	/* the type, see below */
	i2	pv_len;		/* the length of the value */
	union
	{
		i2		pv_int;		/* PV_INT */
		struct QTREE	*pv_qtree;	/* PV_QTREE */
		char		*pv_str;	/* PV_STR */
		char		*pv_tuple;	/* PV_TUPLE */
	} pv_val;
}  PARM;
# define	pv_t		PARM

/* pv_type values */
# define	PV_EOF		0	/* end of list */
# define	PV_INT		1	/* integer */
# define	PV_STR		2	/* string */
# define	PV_QTREE	3	/* query tree */
# define	PV_TUPLE	4	/* tuple */


# endif /* PV_MAXPC */
