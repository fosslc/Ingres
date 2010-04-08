/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	Constants in INGMENU
*/

# define	EXITING		(i4)0
# define	CONTINUING	(i4)1

# define	H_ABF		(i4)01
# define	H_GBF		(i4)02
# define	H_GRAPH		(i4)04
# define	H_INGRES	(i4)010
# define	H_QBF		(i4)020
# define	H_RBF		(i4)040
# define	H_REPORT	(i4)0100
# define	H_SREPORT	(i4)0200
# define	H_VIFRED	(i4)0400
# define	H_MQBF		(i4)01000
# define	H_XMQBF		(i4)02000
# define	H_SQL		(i4)04000
# define	H_VIGRAPH	(i4)010000
# define	H_RGRAPH	(i4)020000
# define	H_EXPERT	(i4)0100000


# define	H_FORMNAME	(i4)01
# define	H_TBLNAME	(i4)02
# define	H_OUTFILE	(i4)04
# define	H_GRAPHNAME	(i4)010
# define	H_REPNAME	(i4)020
# define	H_APPLNAME	(i4)040
# define	H_FNAME		(i4)0100
# define	H_BLOCK		(i4)0200
# define	H_COLUMN	(i4)0400
# define	H_WRAP		(i4)01000
# define	H_DEFAULT	(i4)02000
# define	H_QDEF		(i4)04000
# define	H_NAME		(i4)010000
# define	H_ANY		(i4)020000

/*
**	These are used to define the style of QBF needed 
*/
# define	QBF_START	(i4)0
# define	QBF_JOIN	(i4)2

# define	starterr(cmd)
