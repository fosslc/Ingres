/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	global.h -- local header file for ingcntrl
**
** History:
**	? - (?) created.
**	4/87 (peterk) - commented.
**	11-may-89 (mgw)
**		Added LOC_DMPS for the II_DUMP location.
**      19-oct-1990 (pete)
**              Merge in changes done by Jennifer Lyerla in 9/89 for B1 proj.
**		Added DB_ACCESS definition.
**	8-dec-1992 (jonb)
**		Added definitions for ML sort support.
**	20-sep-93 (robf)
**	        Added definitions for enhanced security privs
**/

/* Warning: redefinition */
# define	DB_PRIVATE	0000000		/* indicates if database is
						** public or private. If first
						** bit in iidatabase.ACCESS is
						** clear then database=private
						*/
# define	DB_GLOBAL	0000001		/* If first bit in
						** iidatabase.ACCESS is set
						** then public ("GLOBAL")
						*/
# define	DB_ACCESS	0004000		/* bit DBPR_ACCESS in
						** iidatabase.ACCESS tells if
						** ACCESS or NOACCESS was
						** GRANTed.
						*/
# define	DB_DISTRIBUTED	0000001		/* if first bit in
						** iidatabase.DBSERVICE is set
						** then database=distributed.
						*/
# define	DB_STAR_CDB	0000002		/* if second bit in
						** iidatabase.DBSERVICE is set
						** then database=Star-CDB.
						*/
/*# define	DB_OPERATIVE	0000020		** Database operative flag */

# define	U_CREATDB	0000001		/* can create data bases */
# define	U_DRCTUPDT	0000002		/* can specify direct update */
# define	U_UPSYSCAT	0000004		/* can update system catalogs */
# define	U_TRACE		0000020		/* can use trace flags */
# define	U_APROCTAB	0000100		/* can use arbitrary proctab */
# define	U_EPROCTAB	0000200		/* can use =proctab form */
# define        U_OPERATOR      0001000         /* Operator privilege */
# define        U_AUDIT         0002000         /* Audit user activity */
# define	U_SYSADMIN	0004000		/* Can perform system admin */
# define	U_SUPER		0100000		/* ingres superuser */
# define	U_SECURITY      0100000		/* ingres security*/
# define U_AUDITOR             0x00002000      /* Audit */
# define U_ALTER_AUDIT         0x00004000      /* Can alter audit system */
# define U_MAINTAIN_USER       0x00010000      /* Can maintain users */
# define U_WRITEDOWN           0x00020000      /* WRITE_DOWN privilege */
# define U_INSERTDOWN          0x00040000      /* INSERT_DOWN privilege */
# define U_WRITEUP             0x00080000      /* WRITE_UP privilege */
# define U_INSERTUP            0x00100000      /* INSERT_UP privilege */
# define U_MONITOR             0x00200000      /* DBMS MONITOR privilege */
# define U_SETLABEL            0x00400000      /* SET Session Label Priv */
# define U_WRITEFIXED          0x00800000      /* WRITE_FIXED privilege*/
# define U_AUDIT_QRYTEXT       0x01000000      /* Audit query text */

# define	IC_INVALID	-99     /* STATUS = invalid value */

/* Redefinitions of bits in the iiextend status field... */
 
# define        DU_EXT_OPERATIVE        0x01  /* This extension is operative. */
# define        DU_EXT_DATA             0x02  /* This is a DATA extension */
# define        DU_EXT_WORK             0x04  /* This is a WORK extension */
# define        DU_EXT_AWORK            0x08  /* an auxiliary WORK extension */

/* Location types; matches corresponding entries in dudbms.qsh... */

#define        DU_GLOB_LOCS    1       /* 000001 - General purpose location.*/
#define        DU_DMPS_LOC     2       /* 000002 - Dumping location. */
#define        DU_DBS_LOC      8       /* 000010 - Database location. */
#define        DU_WORK_LOC     16      /* 000020 - Work location. */
#define        DU_AWORK_LOC    32      /* 000040 - Auxiliary Work Location */
#define        DU_JNLS_LOC     64      /* 000100 - Journaling location. */
#define        DU_CKPS_LOC     512     /* 001000 - Checkpoint location. */

/* 
** States for data set entries in the table field
**	(these can be dropped if #include <runtime.h>; there were errors
**	in runtime.h the day I tried to change it. pe)
*/
# define	stUNDEF		0	/* empty or bad row 		*/
# define	stNEW		1	/* appended at runtime on frame	*/
# define	stUNCHANGED	2	/* loaded through EQUEL program	*/
# define	stCHANGE	3	/* changed by runtime user	*/
# define	stDELETE	4	/* deleted by runtime user 	*/

/*
** valid values for iiprotect.progtype (type of Authorization Identifier: User,
**	Group, Role, Public.
*/
# define	TYPE_USER	0
# define	TYPE_GROUP	1
# define	TYPE_ROLE	2
# define	TYPE_PUBLIC	3

FUNC_EXTERN char	*ynb();
FUNC_EXTERN bool	rows();

/* Handy macro */

#define CONFIRM(n, t)	IIUFccConfirmChoice(CONF_END, (n), (t), NULL, NULL)

GLOBALREF	char	*No;
GLOBALREF	char	*Yes;
GLOBALREF	char    *Request;

