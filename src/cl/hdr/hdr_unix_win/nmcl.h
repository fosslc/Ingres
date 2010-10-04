/*
** Copyright (c) 1987, 2010 Ingres Corporation
*/

/**
** Name: NM.H - Global definitions for the NM compatibility library.
**
** Description:
**      The file contains the type used by NM and the definition of the
**      NM functions.  These are used for handling INGRES names.
**
** History:
**	3-30-83   - Written (jen)
**      88/01/07  16:22:28  mikem
**	    made default location names II_CHECKPOINT, II_DATABASE, ...
**      88/07/01  10:53:57  mikem
**	    added ING_SORTDIR
**	11-may-89 (daveb)
**	    Remove RCS log and useless old history.
**	09-June-89 (dds)
**	    add define for ING_DMPDIR according to teg.
**	16-feb-90 (greg)
**	    NMgtAt() is a VOID.
**	28-Mar-90 (seiwald)
**	    New ADMIN location.
**      07-nov-92 (jrb)
**	    Changed ING_SORTDIR to ING_WORKDIR for ML Sorts project.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	10-oct-94 (nanpr01)
**	    moved the nmerr.h definition from the local cl/nm directory
**	    to here. Additionally added the masks instead of constants.
**          Bug # 63157
**	09-dec-1996 (donc)
**	    Add LOG as a symbol used by NMloc. II_CONFIG and II_SYSTEM for
**	    NMgtAt
**	23-may-97 (mcgem01)
**	    Add a prototype for NMstIngAt
**	29-sep-2000 (devjo01)
**	    Add NM_TOOLONG.
**      13-jan-2004 (somsa01)
**	    Added NM_CORRUPT, and fixed up some botched error codes.
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
**	06-apr-2004 (somsa01)
**	    Added NM_STCORRUPT, and fixed up some botched error codes.
**      02-feb-2005 (raodi01)
**	   Added II_UUID_MAC for mac based uuid creation 
**      28-apr-2005 (raodi01)
**         Above change was removed as it was not necessary
**	10-Mar-2010 (hanje04)
**	   SIR 123296
**	   Add UBIN as new location, used by LSB builds.
**	08-Sep-2010 (rajus01) SD issue 146492, Bug 124381
**	    Add LIB32, LIB64.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN char *  NMgetenv();

FUNC_EXTERN STATUS NMstIngAt(
#ifdef CL_PROTOTYPED
	char   *name,
	char   *value
#endif
);


/* 
** Defined Constants
*/

/* NM return status codes. */

#define  NM_OK           0
#define NM_PWDOPN	 (E_CL_MASK + E_NM_MASK + 0x01)
                                /*  Unable to open passwd file. */
#define NM_PWDFMT	 (E_CL_MASK + E_NM_MASK + 0x02)
                                /*  Bad passwd file format. */
#define NM_INGUSR        (E_CL_MASK + E_NM_MASK + 0x03)	
                                /*  There is no INGRES user in the passwd file*/
#define NM_INGPTH        (E_CL_MASK + E_NM_MASK + 0x04)	
                                /*  No path returned for INGRES user. */
#define NM_STOPN	 (E_CL_MASK + E_NM_MASK + 0x05)
                                /*  Unable to open symbol table. */
#define NM_STPLC	 (E_CL_MASK + E_NM_MASK + 0x06)
                                /*  Unable to position to record in symbol tbl*/
#define NM_STREP	 (E_CL_MASK + E_NM_MASK + 0x07)
                                /*  Unable to replace record in symbol table. */
#define NM_STAPP	 (E_CL_MASK + E_NM_MASK + 0x08)
                                /*  Unable to append symbol to symbol table. */
#define NM_LOC		 (E_CL_MASK + E_NM_MASK + 0x09)
                                /*  first argument must be 'f' or 't' */
#define NM_BAD_PWD	 (E_CL_MASK + E_NM_MASK + 0x0A)
                                /*  Bad home directory for INGRES superuser */
#define NM_STCORRUPT	 (E_CL_MASK + E_NM_MASK + 0x0B)
				/*  Corrupted symbol.tbl file */
#define NM_NO_II_SYSTEM	 (E_CL_MASK + E_NM_MASK + 0x0C)
                                /*  II_SYSTEM must be defined */
#define NM_PRTLNG	 (E_CL_MASK + E_NM_MASK + 0x0D)
                                /*  Part name too long */
#define NM_PRTFMT	 (E_CL_MASK + E_NM_MASK + 0x0E)
                                /*  Illegal part name format */
#define NM_PRTNUL	 (E_CL_MASK + E_NM_MASK + 0x0F)
                                /*  Part name is a null string */
#define NM_PRTALRDY	 (E_CL_MASK + E_NM_MASK + 0x10)
                                /*  Part name already set */
#define NM_TOOLONG	 (E_CL_MASK + E_NM_MASK + 0x11)
                                /*  Name value exceeds supported length */

/*
	Symbols used by NMgtAt()
*/

# define	ING_BE 		"ING_BE"
# define	ING_TM 		"ING_TM"
# define	ING_CKDIR	"II_CHECKPOINT"
# define	ING_DBDIR	"II_DATABASE"
# define	ING_JNLDIR 	"II_JOURNAL"
# define	ING_WORKDIR	"II_WORK"
# define	ING_BINDIR	"ING_BINDIR"
# define	ING_DBTMPLT	"ING_DBTMPLT"
# define	ING_DBDBTMP	"ING_DBDBTMP"
# define	ING_DMPDIR	"II_DUMP"
# define	ING_FILES	"ING_FILES"
# define	ING_LIB		"ING_LIB"
# define	ING_TEMP	"ING_TEMP"
# define	ING_HOME 	"ING_HOME"
# define	ING_CLERR 	"ING_CLERR"
# define	ING_EDIT	"ING_EDIT"
# define	ING_PRINT	"ING_PRINT"
# define	ING_SHELL	"ING_SHELL"
# define	ING_RECOVER	"ING_RECOVER"
# define	USER_NAME	"USER"
# define	USER_PORT	""
# define	TERM_TYPE	"TERM"
# define	II_CONFIG	"II_CONFIG"
# define	II_SYSTEM	"II_SYSTEM"

/*
	Symbols used by NMloc()
*/

# define	BIN		0
# define	DBTMPLT		1
# define	DBDBTMPLT	2
# define	FILES		3
# define	LIB		4
# define	SUBDIR		5
# define	TEMP		6
# define	RECOVER		7
# define	UTILITY		8
# define	ADMIN		9
# define	LOG		10
# define	UBIN		11
# define	LIB32		12
# define	LIB64		13
