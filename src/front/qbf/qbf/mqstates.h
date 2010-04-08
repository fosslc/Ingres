
/* static char	Sccsid[] = "@(#)mqstates.h	30.1	11/14/84"; */

/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
**	MQSTATES.H - header file containing various 
**		     constant definitions 
**
**	History:
**		26-may-89 (Mike S) Move ATTRIBINFO flags into mqtypes.qsh
*/

/* States of rows in a table field */
# define	UNDEF		0
# define	NEW		1
# define	UNCHANGED	2
# define	CHANGED		3
# define	DELETED		4

/* GETOPER return codes */
# define	NOP		0
# define	EQ		1
# define	NE		2
# define	LT		3
# define	GT		4
# define	LE		5
# define	GE		6

/* DEADLOCK ERROR NUMBER */
# define	DLCKERR		4700

/* status of record written to temp file */
# define	mqNOPREC	00	/* no change made to record */
# define	mqUPDREC	01	/* change record */
# define	mqDELREC	02	/* delete record */
# define	mqADDREC	03	/* append detail record */

/* status codes for relstat field of relation relation */
# define	s_CATALOG	0000001
# define	s_EXTCATALOG	0040000
# define	s_VIEW		0000040

/* QBF command line function flag */
# define	mqNOFUNC	00		/* no function specified on command line */
# define	mqAPPEND	01		/* APPEND only */
# define	mqBROWSE	02		/* BROWSE only */
# define	mqUPDATE	03		/* UPDATE only */
# define	mqALL		04		/* APPEND, BROWSE, UPDATE */

/* field/column modes */
# define	mqQRYONLY	QUERY
# define	mqDISPONLY	READ

/* State to return to from mq_tables */
# define	mqDEFINITION	0
# define	mqCATALOGS	1
# define	mqHELP		2
# define	mqQBFNAMES	3

# define	mqISNOTKEY	0
# define	mqISKEY		1

/* indicates what type of key to use for SQL UPDATEs/DELETEs */
# define	mqNOAPPEND	-3
# define	mqUSEROW	-2
# define	mqUSETID	-1
# define	mqNOUPDATE	0
# define	mqUSEKEYS	+1

