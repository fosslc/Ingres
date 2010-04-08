/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	MF.h, originally "envset.h", "envdefs.h", "shdefs.h", "shstruc.h",
**	"sherrdef.h" from the pclink directory.  (linda)
**
** History:
**	12-apr-89 (russ)
**		Collapsed the GENATTUNIX, GENBSDUNIX, and GENXENIX
**		cases into one case, GENUNIX.  Also, getting the value
**		of FNAMELEN from MAXNAMLEN on UNIX.  MAXNAMLEN is set in
**		diracc.h if not set in a system header file.
**	14-apr-89 (russ)
**		Change FNAMELEN to 256, its max possible value, because
**		MAXNAMLEN is defined in diracc.h, which can't be included
**		in mainline code, though mf.h is.  Also add OSID for UNIX.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Changed "#include fab" to "#include <fab.h>" as compiler
**		complaining about the format.
*/

# define	RTI	1

/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  ENVSET.H -- ENVIRONMENT DEFINITIONS FOR CONFIGURATION DIFFERENCES     **/
/**                                                                        **/
/**                        Copyright (c) 1985, 1986                        **/
/**                    Network Innovations Corporation                     **/
/**                          All Rights Reserved                           **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*  OPERATING SYSTEM DEFINITION -- Set one and only one define to 1         */
/****************************************************************************/

#define GENMSDOS		0
#define GENUNIX			1
#define GENVMS			0


/****************************************************************************/
/*                           CLIENT DEFINES                                 */
/****************************************************************************/

/****************************************************************************/
/*  CUSTOM VERSION DEFINITION -- Set define to 1 for particular version     */
/****************************************************************************/

#define MPXVERSION	1
#define RTIVERSION	1
#define ALTVERSION	0
#define PCOVERSION	0


/****************************************************************************/
/*  SUPPORTED DATABASES DEFINITION -- Set supported DBMS defines to 1       */
/****************************************************************************/

#define GENINFORMIX	0
#define GENUNIFY	0
#define GENINGRES	1
#define GENORACLE	0
#define GENHPSQL	0
#define GENDATATRIEVE	0
#define GENRDB		0


/****************************************************************************/
/*  SUPPORTED FORMATS DEFINITION -- Set supported formats defines to 1      */
/****************************************************************************/

#define GENINGOUT	1


/****************************************************************************/
/*  SUPPORTED NETWORKS DEFINITION -- Set supported network defines to 1     */
/****************************************************************************/

#define GENASYNC	1
#define GENALTASYNC	0	/* Altos alternate async */
#define GENFUSETHER	1	/* This is necessary for menus */
#define GENWORKNET	0
#define GENOSMERGE	0
#define GENSUNNFS	0
#define GENDECNET	0
#define GENNULNET	0
#define GENBANYAN	0


/****************************************************************************/
/*                           SERVER DEFINES                                 */
/****************************************************************************/

/****************************************************************************/
/*  SUPPORTED SERVER DATABASES -- Set supported DBMS defines to 1           */
/****************************************************************************/

#define GENH11INFORMIX	0		/* Informix-SQL 1.1 */
#define GENH20INFORMIX	0		/* Informix-SQL 2.0 */
#define GENHUNIFY	0		/* Unify (all) */
#define GENHCSINGRES	0		/* Ingres/CS */
#define GENHRTINGRES	1		/* Full Ingres */
#define GENHORACLE	0		/* Oracle */
#define GENHDATATRIEVE	0		/* Datatrieve */
#define GENHRDB		0		/* Rdb */


/****************************************************************************/
/*  SUPPORTED SERVER NETWORKS -- Set supported network defines to 1         */
/****************************************************************************/

#define GENHRAWASYNC	1
#define GENHPKTASYNC	1
#define GENHFUSETHER	0
#define GENHWORKNET	0
#define GENHOSMERGE	0
#define GENHDECNET	0
#define GENHBANYAN	0


/****************************************************************************/
/*  DEMO SERVER VERSION -- Set define to 1 for demo version                 */
/****************************************************************************/

#define DEMOVERSION	0

/*  NI MEMORY LOGGING HOOK -- affects pcmemory.c and doslib2.c */
/*
#define LOGMEM	1
*/ 

/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  ENVDEFS.H -- ENVIRONMENT DEFINITIONS FOR OS DIFFERENCES               **/
/**                                                                        **/
/**                        Copyright (c) 1985, 1986                        **/
/**                    Network Innovations Corporation                     **/
/**                          All Rights Reserved                           **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/*
**	(linda) -- Comment out '#include "envset.h"' statement, as it is
**	already included above as part of MF.h.
*/

/*	#include "envset.h"	*/


/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  OPERATING SYSTEM-SPECIFIC DEFINES -- Do not change or set             **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

#if (GENMSDOS)
#define OSNAME		"MS-DOS"
#ifndef MSDOS
#define MSDOS		1
#endif
#endif
#if (GENUNIX)
#define OSNAME		"UNIX"
#ifndef	UNIX
#define UNIX		1
#endif
#endif
#if (GENVMS)
#define OSNAME		"VMS"
#ifndef	VMS
#define VMS		1
#endif
#endif


#ifdef MSDOS
#define GLOBALDEF
#define UCHAR		char
#define USHORT		unsigned
#define FILCASE		FILUPPER
#define SUCEXIT		0
#define OSID		OSMSDOS
#define DRIVELEN	2
#define FNAMELEN	12
#define SFNAMELEN	8
#define PNAMELEN	66
#define TEMPDIR		"."
#define SPOOLER		"PRINT"
#define CURDIRNAME	"."
#define MFOFFSET	long		/* DOS file position is a long */
#endif

#ifdef UNIX					/* WECO or BSD */
#define GLOBALDEF
#define UCHAR		unsigned char
#define USHORT		unsigned short
#define FILCASE		FILMIXED
#define SUCEXIT		0
#define OSID		OSUNIX
#define DRIVELEN	0
#define FNAMELEN	256
#define PNAMELEN	256
#define TEMPDIR		"/tmp"		/* default temporary directory */
#define SPOOLER		"lp -c"		/* default spooler name */
#define CURDIRNAME	"."		/* symbol for current directory */
#define MFOFFSET	long		/* UNIX file position is a long */
#endif

#ifdef VMS
#define GLOBALDEF	globaldef
#define UCHAR		unsigned char
#define USHORT		unsigned short
#define FILCASE		FILUPPER
#define extern		globalref
#define SUCEXIT		1
#define OSID		OSVMS
#define DRIVELEN	16
#define FNAMELEN	85
#define SFNAMELEN	39
#define PNAMELEN	255
#define TEMPDIR		"[]"
#define SPOOLER		"PRINT"
#define CURDIRNAME	"[]"
#define MFOFFSET	struct {USHORT rfa1; USHORT rfa2; USHORT rfa3;}
#include <fab.h>
#include <rab.h>
#endif


typedef struct {
   long larec;				/* record nr for this entry */
   MFOFFSET laoffset;			/* file position for this entry */
} MFLABENTRY ;
typedef struct {
   FILE *mffp;
   int mfstruct;
   int mfaccmode;
   int mfreclen;
   long mfeofrec;
   long mfcurrec;
   MFLABENTRY *mflab;
#ifdef MSDOS
   char mfpath[1];
#endif
#ifdef VMS
   struct FAB mffab;			/* RMS FAB block for file */
   struct RAB mfrab;			/* RMS RAB block for file */
   long mfcurblk;			/* current block# in buffer */
   char *mfbufptr;			/* current position in buffer */
   USHORT mfbufcnt;			/* count of chars in buffer */
   int mfeof;				/* at end-of-file flag */
   char mfrwmode;			/* read/write mode */
#endif
} MFFILE ;

MFFILE *MFopen();
long MFtell();

/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  SHDEFS.H -- COMMON DEFINITIONS FOR PC/MULTIPLEX & HOST/MULTIPLEX      **/
/**                                                                        **/
/**  Definitions for logical values, ASCII characters, and the transac-    **/
/**  tion structure, which are identical on both sides of the network.     **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

#ifndef min
#define min(a,b)	((a)<=(b)?(a):(b))
#endif
#ifndef max
#define max(a,b)	((a)>=(b)?(a):(b))
#endif


/**********************************************************************/
/*  LOGICAL DEFINITIONS                                               */
/**********************************************************************/

#define OFF		0
#define ON		1

#define NO		0
#define YES		1

#ifndef	FALSE
#define FALSE		0		
#endif
#define TRUE		1

/*************************************************************************/
/*  SUCCESS/FAILURE RETURN VALUES FROM CALLED ROUTINES                   */
/*************************************************************************/

#define SUCCESS		0
#define FAILURE		-1
#define MFEOF		-2


/*************************************************************************/
/*  NULL, EMPTY OR BLANK THINGS...                                       */
/*************************************************************************/

#define NONE		0
#define NULLSTR		""
#define SPACES		" "
#define WHITESTR	" \011"
#define MAXLONG		2147483647L
#define SYNCSTR		"SYNC"


/****************************************************************************/
/*  DIRECTORY SEARCHING CALL CODES                                          */
/****************************************************************************/

#define FNEXT           0       /* find next file matching pattern */
#define FEND            1       /* close directory */


/****************************************************************************/
/*  FILE ORGANIZATION & RECORD TYPES -- USED FOR FILE CREATION              */
/****************************************************************************/

#define MF_FILORG	0x3F00		/* six bit mask for file org */
#define MF_SEQ		0x0100		/* Sequential organization */
#define MF_REL		0x0200		/* Relative file organization */
#define MF_IDX		0x0400		/* Indexed file organization */

#define MF_RECTYPE	0x00FF		/* eight bit mast for record type */
#define MF_FIX		0x0001		/* Fixed-length records */
#define MF_VAR		0x0002		/* Variable-length records */
#define MF_UDF		0x0004		/* Undefined records */

#define MF_TMP		0x4000		/* Temporary file flag */


/****************************************************************************/
/*  FILE ACCESS MODES -- USED FOR FILE OPEN/READ/WRITE                      */
/****************************************************************************/

#define MF_ACCMOD	0x00FF		/* Access mode mask */
#define MF_ACCMTH	0x3F00		/* Access method mask */

#define MF_BIN		0x0001		/* Binary mode access */
#define MF_TXT		0x0002		/* Text mode access */
#define MF_BLK		0x0010		/* Block mode access */
#define MF_REC		0x0020		/* Record mode access */

/*************************************************************************/
/*  FILE TYPES FOR SEARCHES & FILE TYPE RETURN VALUES                    */
/*************************************************************************/

#define FINDFILES	0		/* find files only */
#define FINDDIRS	1		/* find directories only */
#define FINDALL		16		/* find files & directories */


#define NOFILES		0		/* No files found */
#define FILTYPE		1		/* File is an ordinary file */
#define DIRTYPE		2		/* File is a directory */


/*************************************************************************/
/*  WINDOW IDENTIFICATIONS                                               */
/*************************************************************************/

#define MNWNDNR		1		/* Main Window id number */
#define LKWNDNR		2		/* Lookup Window id number */


/***************************************************************************/
/* LENGTHS AND MAXIMUMS                                                    */
/***************************************************************************/

#define SUFFIXLEN	4
#define MAXFNAMELEN	max(FNAMELEN,17)/* max of actual and display max */
/* XXX actual value - changed for multi-step login kludge
#define USRNAMELEN	9
*/
#define USRNAMELEN	64		/* maximum Host user name */
#define PASSWDLEN	20		/* maximum Host password length */

/* XXX actual value - changed for CDD dictionary name kludge
#define DBNAMELEN	14
*/
#define DBNAMELEN	64		/* maximum database name len */
#define DBUNAMELEN	20		/* maximum database user name len */
#define DBPASSWDLEN	20		/* maximum database password len */

#define TBLNAMELEN	(DBUNAMELEN+1+30) /* user.tablename */
#define COLNAMELEN	30		/* maximum database column name */
#define ECOLNAMELEN	(TBLNAMELEN+COLNAMELEN+1) /* table.columnname */

#define CRITTXTLEN	40		/* max. row selection crit. text */
#define PVWRECCNT	18		/* nr. records read for PREVIEW */
#define VHRECLEN	16

#define MODDATELEN	8
#define MODTIMELEN	6
#define ACCPERMLEN	12


/***************************************************************************/
/*  UNIVERSAL IDS -- HOST OPERATING SYSTEMS                                */
/***************************************************************************/

#define OSMSDOS		1
#define OSUNIX		2
#define OSBSD		3
#define OSVMS		4

/***************************************************************************/
/*  UNIVERSAL IDS -- FILENAME CASE SENSITIVITY                             */
/***************************************************************************/

#define FILMIXED	0
#define FILUPPER	1
#define FILLOWER	2


/***************************************************************************/
/*  UNIVERSAL IDS -- COMMUNICATIONS PROTOCOLS                              */
/***************************************************************************/

#define RAWASYNC	1
#define PKTASYNC	2
#define FUSETHER	3
#define WORKNET		4
#define OSMERGE		5
#define TCPETHER	6
#define SUNNFS		7
#define DECNET		8
#define NULNET		9
#define BANYAN		10


/***************************************************************************/
/*  UNIVERSAL IDS -- HOST DBMS SYSTEMS                                     */
/***************************************************************************/

#define DBINFORMIX	1
#define DBUNIFY		2
#define DBINGRES	3
#define DBORACLE	4
#define DBMISTRESS	5
#define DBHPSQL		6
#define DBDATATRIEVE	7
#define DBRDB		8


/****************************************************************************/
/* SERVER ENVIRONMENT VARIABLE NAMES                                        */
/****************************************************************************/

#define MPXTEMPDIR	"MPXTEMPDIR"	/* dir for temp file allocation */
#define MPXSPOOLER	"MPXSPOOLER"	/* name of spooler program */
#define MPXFLOAT	"MPXFLOATFMT"	/* format for floating point fields */
#define MPXMONEY	"MPXMONEYFMT"	/* format for currency fields */
#define MPXSHELL	"MPXSHELL"	/* program for [F8/Host] subshell */


/***************************************************************************/
/* CHARACTER CODES & CONSTANT STRINGS                                      */
/***************************************************************************/

#define NULLCHR		'\000'
#define BACKSPACE	'\010'
#define TAB		'\011'
#define LF		'\012'
#define FF		'\014'
#define CR		'\015'
#define ESCAPE		'\033'

#define BLANK		' '
#define PERIOD		'.'
#define DELIMITER	'|'
#define UNDERSCORE	'_'
#define COLON		':'
#define SEMICOLON	';'
#define STUFFCHAR	'~'
#define BACKSLASH	'\\'
#define COMMA		','
#define DOLLARSIGN	'$'
#define PERCENTSIGN	'%'
#define QMARK		'?'
#define ASTERISK	'*'
#define POUND		'#'
#define TILDECHR	'~'
#define AMPERSAND	'&'
#define DEL		'\177'
#define SLASH		'/'
#define LPAREN		'('
#define RPAREN		')'
#define LBRACE		'{'
#define RBRACE		'}'
#define LBRACKET	'['
#define RBRACKET	']'
#define HYPHEN		'-'
#define DQUOTE		'\042'
#define SQUOTE		'\047'

#define CTRL_A		'\001'
#define CTRL_C		'\003'
#define CTRL_L		'\014'
#define CTRL_Q		'\021'
#define CTRL_S		'\023'
#define CTRL_X		'\030'
#define CTRL_Z		'\032'


/***************************************************************************/
/* PACK AND UNPACK CONSTANTS                                               */
/***************************************************************************/

#define PCKSEPCHR	';'
#define PCKQUOTE	'`'
#define PCKINTTYPE	'd'
#define PCKLNGTYPE	'l'
#define PCKSTRTYPE	's'
#define PCKCHRTYPE	'c'


/***************************************************************************/
/* DATABASE COLUMN DATA TYPES                                              */
/***************************************************************************/

#define DTINTEGER	'I'		/* Integer data - no decimal places */
#define DTONEBYTE	'O'		/* i1 integer data type */
#define DTSMALLINT	'N'		/* Short Integer i2	*/
#define DTSCIENTIFIC	'S'		/* Scientific (floating point) data */
#define DTTEXT		'T'		/* Text data */
#define DTDATE		'D'		/* Date data */
#define DTTIME		'H'		/* Time data */
#define DTCURRENCY	'C'		/* Currency amount data */
#define DTFIXED		'F'		/* Fixed data */
#define DTBLANK		'B'		/* Blank column */


/***************************************************************************/
/* DATABASE ROW SELECTION CRITERION OPERATORS                              */
/***************************************************************************/

#define EQ_OP		1		/* A equal to B */
#define NEQ_OP		2		/* A not equal to B */
#define LT_OP		3		/* A less than B */
#define LTEQ_OP		4		/* A less than or equal to B */
#define GT_OP		5		/* A greater than B */
#define GTEQ_OP		6		/* A greater than or equal to B */
#define RANGE_OP	7		/* A in inclusive range B thru C */
#define SET_OP		8		/* A in the set B,C,D,... */
#define LOGICAL_OP	100		/* starting code for logical ops */
#define AND_OP		101		/* A and B and C... */
#define OR_OP		102		/* A or B or C... */

/***************************************************************************/
/* DATABASE ROW SELECTION CRITERION COMPARISON VALUE TYPES                 */
/***************************************************************************/

#define CRITXT_TYPE	1		/* compare to literal text */
#define CRICOL_TYPE	2		/* compare to another column */


/***************************************************************************/
/* DATABASE ROW SELECTION CRITERION HIERARCHY                              */
/***************************************************************************/

#define TOP_LEVEL	1		/* top-level crit. have level = 1 */
#define MAX_LEVELS	15		/* max. depth = 15 levels */


/***************************************************************************/
/*  FILE MANAGER BLOCK & RECORD SIZES                                      */
/***************************************************************************/

#define FILBLKSIZE	512	/* block size for file transfer */
#define VIEBLKSIZE	512	/* block size for file viewing */
#define HEXVIESIZE	16	/* record size for hex-mode file view */


/***************************************************************************/
/*  TRANSACTION SIZES                                                      */
/***************************************************************************/

#define TXNHDRSIZE	5	/* txn. header size (id+type+status+window) */
#define MAXTXNSIZE	2000	/* max. txn size */

/***************************************************************************/
/* TRANSACTION TYPES COMMON TO ALL SERVERS (Sender in parens)              */
/***************************************************************************/

#define GEN_ACK		1	/* OK acknowlegement (Host) */
#define ERR_ACK		2	/* ERROR acknowledgement (Host) */
#define SET_TRACE	3	/* turn trace mode on/off */
#define GET_DIRECTORY	5	/* get name of current directory */
#define STRT_SHELL	7	/* start host subshell (PC) */
#define GEN_RESET       8	/* general reset (PC) */
#define END_SERVER	9	/* terminate current server (PC) */
#define NEXT_ITEM	20	/* send next item of a list (PC) */


/***************************************************************************/
/* CONNECTION SERVER TRANSACTION ID & TYPES                                */
/***************************************************************************/

#define GEN_TXNID	'G'	/* connection server txn-id */
#define TE_TXNID	'T'	/* terminal emulator server id */
#define WLD_TXNID	'*'	/* matches any txn-id */

#define STRT_SESSION	0	/* start a Host session (PC) */
#define END_SESSION	9	/* terminate Host session (PC) */
#define STRT_DBSERVER   11	/* start database server (PC) */
#define STRT_FMSERVER	12	/* start file management server (PC) */


/***************************************************************************/
/* DATABASE SERVER TRANSACTION ID & TYPES                                  */
/***************************************************************************/

#define DB_TXNID	'D'	/* database server txn-id */

#define SET_DBBRAND	11	/* set database brand (PC) */
#define SET_DIRECTORY	12	/* change current Host directory (PC) */
#define SET_DBNAME	13	/* set database name (PC) */

#define GET_DBINFO	21	/* send list of databases (PC) */
#define GET_TBLINFO	22	/* send list of tables (PC) */
#define GET_COLINFO	23	/* send list of columns (PC) */

#define QRY_START	31	/* begin accepting query specs. (PC)*/
#define QRY_SELECT	32	/* add a select specification (PC) */
#define QRY_UNIQUE	33	/* add a unique specification (PC) */
#define QRY_FROM	34	/* add a table specification (PC) */
#define QRY_JOIN	35	/* add a join specification (PC) */
#define QRY_CRIT	36	/* add a row selection specification (PC) */
#define QRY_SORT	37	/* add a sort specification (PC) */
#define QRY_ABORT	38	/* abort current query spec sequence (PC) */

#define QRY_PREVIEW     41	/* execute preview inquiry (PC) */
#define QRY_INQUIRY	42	/* execute full inquiry (PC) */
#define GET_RECORD      43	/* send next record of query results (PC) */

#define INFO_DB		51	/* database name (Host) */
#define INFO_TBL	52	/* table name (Host) */
#define INFO_COL	53	/* column name & info (Host) */

#define QRY_HEADER	61	/* query completion status (Host) */
#define QRY_RECORD      62	/* query results record (Host) */


/***************************************************************************/
/* FILE MANAGEMENT SERVER TRANSACTION ID & TYPES                           */
/***************************************************************************/

#define FM_TXNID	'F'	/* file manager txn-id */

#define GET_FILEINFO	21	/* send list of file info. (PC) */
#define GET_SFILEINFO	22	/* send list of files. (PC) */

#define FIL_PSEND	31	/* initiate PC->Host print (PC) */
#define FIL_SPOOL	32	/* spool Host file on Host (PC) */
#define VIEW_OPEN	33	/* open Host view file (PC) */
#define VIEW_CLOSE	34	/* close Host view file (PC) */
#define VIEW_GETREC	35	/* request Host view file record (PC) */
#define VIEW_RECORD	36	/* view file record (Host) */
#define VIEW_EOFGET	37	/* get record number of last record (PC) */

#define FIL_SEND	41	/* initiate PC->Host transfer (PC) */
#define FIL_RECV	42	/* initiate Host->PC transfer (PC) */
#define GET_FILREC	43	/* request xfer file record (PC) */
#define FIL_ERASE	44	/* erase Host file (PC) */
#define FIL_COPY	45	/* copy Host file (PC) */
#define FIL_MOVE	46	/* move Host file (PC) */
#define FIL_RENAME	47	/* rename Host file (PC) */
#define FIL_ABORT	48	/* abort file transfer (PC) */
#define FIL_TYPE	49	/* request Host file type (PC) */

#define INFO_FIL	54	/* file information (Host) */
#define INFO_TYPE	55	/* file type (Host) */

#define FIL_RECORD	62	/* file xfer record (PC & Host) */
#define FIL_EOF		63	/* end-of-file on file transfer (PC & Host) */


/***************************************************************************/
/* TRANSACTION STATUS CODES COMMON TO ALL SERVERS                          */
/***************************************************************************/

#define OKSTATUS	0		/* transaction completed ok */
#define TXNIO_ERR	70		/* transaction failure */
#define TXNID_ERR	71		/* bad transaction id */
#define TXNTYP_ERR	72		/* bad transaction type */
#define TXNFMT_ERR	73		/* bad transaction body format */
#define NOMEM_ERR	74		/* Host memory exhausted */


/***************************************************************************/
/* TRANSACTION STATUS CODES FOR DATABASE SERVER  (returned from Host)      */
/***************************************************************************/

#define NOBRD_ERR	80		/* Database brand not set */
#define NODB_ERR	81		/* Database not set/open */
#define DBOPEN_ERR	82		/* Cannot open database */
#define SETBRD_ERR	83		/* Cannot set selected brand */
#define SETDIR_ERR	84		/* Cannot set selected directory */
#define TXNINV_ERR	85		/* Invalid transaction (context) */
#define NOITEM_ERR	86		/* No items left in list */
#define INTFAIL_ERR	87		/* Internal error/inconsistency */
#define WNDNR_ERR	88		/* Bad window number */
#define RECNR_ERR	89		/* Bad record number */
#define QRYEXE_ERR	90		/* Error during query execution */
#define DBRST_ERR	91		/* Error resetting d/b interface */
#define DBFIL_ERR	92		/* Error processing temporary file */
#define DBMS_ERR	93		/* DBMS error number - see errbuf */
#define QRYSRT_ERR	94		/* cannot sort on nondisplayed col */
#define QRYUNI_ERR	95		/* cannot sort with unique row sel */
#define QRYSQL_ERR	96		/* sql process terminated */
#define QRYRPT_ERR	97		/* cannot set unique with repeating */
#define QRYWLD_ERR	98		/* unsupported crit wildcards (DTR) */


/***************************************************************************/
/* TRANSACTION STATUS CODES FOR FILE MGMT. SERVER  (returned from Host)    */
/***************************************************************************/

#define FILOPNERR	80		/* Cannot open file */
#define FILWRIERR	81		/* Cannot write to file */
#define FILREAERR	82		/* Cannot read from file */
#define FILCLSERR	83		/* Cannot close file */
#define FILERAERR	84		/* Cannot erase file */
#define FILRENERR	85		/* Cannot rename file */
#define FILCPYERR	86		/* Cannot copy file */
#define FILMOVERR	87		/* Cannot move file */
#define FILSPLERR	88		/* Cannot spool file */
#define FILEOFERR	89		/* Cannot seek to req. position */


/***************************************************************************/
/* TRANSACTION BODY FORMATS  (PRINTF/SCANF)                                */
/***************************************************************************/

#define TXNHDRFMT	"%c%02d%02d"		/* transaction header */
#define COLNAMFMT	"%s.%s"			/* column name */


/***************************************************************************/
/* TRANSACTION BODY FORMATS  (STRPACK/STRUNPACK)                           */
/***************************************************************************/

#define TXNINTFMT	"d"			/* single integer */
#define TXNLNGFMT	"l"			/* single long */
#define TXNSTRFMT	"s"			/* single string */
#define TXNCHRFMT	"c"			/* single char */

#define TXNINIFMT	"dddddd"		/* INIT txn */
#define TXNCOLFMT	"scddd"			/* INFO_COL reply */
#define TXNSELFMT	"scdd"			/* QRY_SELECT txn */
#define TXNJOIFMT	"ss"			/* QRY_JOIN txn */
#define TXNCRIFMT	"dddscddsds"		/* QRY_CRIT */
#define TXNSRTFMT	"sd"			/* QRY_SORT txn */

#define QRYHDRFMT	"lds"			/* QRY_HEADER txn */
#define QRYRECFMT	"dl"			/* GET_RECORD txn */
#define DBOPENFMT	"sss"			/* name, username, password */

#define VIEOPNFMT	"sd"			/* VIEW_OPEN txn */
#define FILTYPFMT	"ss"			/* FIL_TYPE txn */
#define FILINFFMT	"sd"			/* FIL_INFO list reply */
#define FILDETFMT	"sdlssss"		/* FIL_INFO detail reply */
#define FILXFRFMT	"sd"			/* FIL_SEND/FIL_RECV txns */
#define FILCPYFMT	"sss"			/* FIL_COPY txns */
#define FILMOVFMT	"sss"			/* FIL_MOVE txn */
#define FILRENFMT	"ss"			/* FIL_RENAME txn */

/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  SHSTRUC.H  -- COMMON PC/MULTIPLEX & HOST/MULTIPLEX STRUCTURES         **/
/**                                                                        **/
/**                        Copyright (c) 1985, 1986                        **/
/**                    Network Innovations Corporation                     **/
/**                          All Rights Reserved                           **/
/**                                                                        **/
/**  These structures define the network transactions and chains used by   **/
/**  both PC and Host software.                                            **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/* TRANSACTION STRUCTURE, used inbound & outbound:                          */
/****************************************************************************/

typedef struct {
   char txid;				/* Transaction id */
   int txtype;				/* Transaction type */
   int txstatus;			/* Transaction completion status */
   int txtime;				/* Transaction timeout value */
   int txlength;			/* Transaction body length */
   char *txtext;			/* Transaction body pointer */
   char txbuf[MAXTXNSIZE+TXNHDRSIZE+1]; /* Transaction buffer (in/out) */
} TXN ;


/****************************************************************************/
/* CHAIN ELEMENT STRUCTURE:                                                 */
/****************************************************************************/

typedef struct ch_ele {
   struct ch_ele *prev;		/* pointer to preceding chain element */
   struct ch_ele *next;		/* pointer to succeeding chain element */
   int selected;		/* selection flag (T/F/positive integer) */
   char text[1];		/* prototype chain element body (NULLCHR) */
} CHAIN_ELE ;


/****************************************************************************/
/*  FILE INFO CHAIN BLOCK -- FULL & SHORT -- store data about system files: */
/****************************************************************************/

typedef struct mfistruc {
   struct mfistruc *mfiprev;		/* ptr to previous element */
   struct mfistruc *mfinext;		/* ptr to next element */	
   int mfiselected;			/* selected flag */
   char mfiname[MAXFNAMELEN+1];		/* file name */	
   int mfitype;				/* file type */
   long mfisize;			/* file size in bytes */
   char mfimoddate[MODDATELEN+1];	/* modification date (mm/dd/yy) */
   char mfimodtime[MODTIMELEN+1];	/* modification time (hh:mmp) */
   char mfiowner[USRNAMELEN+1];		/* name of file owner */
   char mfiperms[ACCPERMLEN+1];		/* permissions (rwxdrwxdrwxd) */
} MFINFO ;

typedef struct smfistruc {
   struct smfistruc *smfiprev;		/* ptr to previous element */
   struct smfistruc *smfinext;		/* ptr to next element */	
   int smfiselected;			/* selected flag */
   char smfiname[MAXFNAMELEN+1];	/* file name */	
   int smfitype;			/* file type */
} SMFINFO ;


#define ECTRLBRK	1
#define EOPNFIL		2
#define EFILIO		3
#define EMOVFIL		4
#define EPRTIO		5

