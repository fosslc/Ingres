/*
** Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)rbftype.h	1.1	1/24/85";	*/

# include	 <stype.h> 
# include	 <afe.h> 
# include	 "rbfcons.h" 
# include	 <ft.h>
# include	 <frame.h> 
# include	 <list.h> 
# include	 <vifrbft.h> 
# include	 <vifrbf.h> 

/*
**	Types for RBF.
**      HISTORY:
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**
**	12/05/92 (dkh) - Added the RFCOLDESC structure to support
**			 choose columns in RBF.
*/

/*
**	The AGG_TYPE structure contains the information associated with
**		an all of the allowable aggregate functions for one datatype.
**		agnames points to an array of structures, each element of
**		which contains the various names for an aggregate as well
**		a flag indicating whether it is a unique (prime) function.
*/

typedef struct
{
	DB_DATA_VALUE	dbdv;		/* used for db_datatype */
	ADF_AGNAMES	*agnames;	/* array of structs, one el per agg */
	i4		num_ags;	/* number of aggs for datatype */
} AGG_TYPE;


typedef struct _rfcoldesc
{
	char		name[FE_MAXNAME + 1];	/* name of a db column */
	bool		deleted;		/* was column deleted */
	DB_DT_ID	datatype;		/* datatype of column */
	i4		length;			/* data length of column */
	i2		prec;			/* data prec of column */
} RFCOLDESC;
