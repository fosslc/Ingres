/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
/**
** Name:	mqerrs.h -	QBF Error Numbers.
**
*/

/*
** ERROR NUMBERS
**
*/
# define NODB		E_QF0100_11000	/* No database name given */
# define NOFFORM	E_QF0103_11003	/* No form for -F flag */
# define NOCFORM	E_QF0108_11008	/* No compiled form for -C flag */
# define MODEWONAME	E_QF0109_11009	/* -mmode flag specified without table, qdef or qbfname */
# define ERRALLOC	E_QF010A_11010	/* error allocating space */
# define BADRECTYPE	E_QF010B_11011  /* Illegal record type found in mqbf cats */
# define BADADDFRM	E_QF010C_11012	/* error initializing internal mqbf forms */
# define BADFORMAT	E_QF010D_11013	/* bad format type found */
# define BADRANGE	E_QF010E_11014	/* bad range variable set up */
# define MQTBLCREATE	E_QF010F_11015	/* problem creating default tbl field */
# define NOMATCH	E_QF0110_11016	/* join column doesn't have fld match*/
# define MORETBLS	E_QF0111_11017	/* more than 1 tblfld on VIFRED form */
# define NOTBLFIELD	E_QF0112_11018	/* no table field on VIFRED form */
# define TYPEMATCH	E_QF0113_11019	/* column and field types don't match*/
# define BADQRYOPER	E_QF0114_11020	/* bad operatore for query */
# define BADQRYTYPE	E_QF0115_11021	/* bad type for query */
# define BADWRITE	E_QF0116_11022	/* bad write to temp file */
# define NOTABLE	E_QF0117_11023	/* table doesn't exist */
# define BADFILEOPEN	E_QF0118_11024	/* couldn't open temp file */
# define BADBREAK	E_QF0119_11025	/* error on breaking out of ret loop */
# define BADTBLNM	E_QF011A_11026	/* bad table name */
# define BADQDEF	E_QF011B_11027	/* bad qdef name */
# define NONAME		E_QF011C_11028	/* no name specified on command line */
# define NOQUERYD	E_QF011D_11029	/* qdef with given name doesn't exist */
# define NOQBFNM	E_QF011E_11030	/* qbfname with given names doesn't exist*/
# define NOFRMNM	E_QF011F_11031	/* frame with given name doesn't exist*/
# define NAMENOTFOUND	E_QF0120_11032	/* no name under qbfname,qdef or table */
# define DUMPQDEFBAD	E_QF0121_11033	/* can't replace rec 0 in catalog to save qdef */
# define COPYQDEFBAD	E_QF0122_11034	/* couldn't do copy to save qdef */
# define CANTRESTRT	E_QF0123_11035	/* couldn't restart xact after abort in update */
# define BADSTARTUP	E_QF0124_11036	/* couldn't start up ingres */
# define MODEINVALID	E_QF0125_11037	/* invalid mode with -m flag */
# define NOHELPFRMS	E_QF0126_11038	/* couldn't initialize compiled forms for help frames */
# define NOQDFRMS	E_QF0127_11039	/* couldn't initialize compiled forms for query defn. */
# define MDJINTBL	E_QF0129_11041	/* M/D join field in tblfield */
# define USRCODEBAD	E_QF012A_11042	/* user codes couldn't be initialized */
# define ILLEGALMDJ	E_QF012B_11043	/* M/D joins not all between one master and one detail */
# define REL_OVERFLOW	E_QF012C_11044	/* too many relations */
# define ATT_OVERFLOW	E_QF012D_11045	/* too many attributes */
# define JOIN_OVERFLOW	E_QF012E_11046	/* too many joins */
# define BADRECOVER	E_QF012F_11047	/* can't open recover file */
# define BOTHFLAGS	E_QF0130_11048	/* can't specify both -f and -F */
# define DDB_NOCONNECT	E_QF0131_11049	/* failure of FEconnect() */
# define PADDING_ERROR	E_QF0132_11050	/* failure return from ade_pad */
# define OUT_OF_SPACE	E_QF0133_11051	/* out of space in retrieve buffer */
# define COERCE_CHK_ERR	E_QF0134_11052	/* failure in ade_cancoerce */
