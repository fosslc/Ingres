/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include   <mu.h>
# include   <si.h>

/**
** Name:	erloc.h	- Internal ER header file
**
** Description:
**	This file defines data types and file names to be internally used in ER
**	library.
**	ER_FASTFILE and ER_SLOWFILE are the message file names to use at
**	run-time.
**	ERCONTROL is the structure in the message, file to control message file
**	operation.
**	ERMULTI is the structure to control internally message system.
**
** History:
**	01-Oct-1986 (kobayashi) - first written
**	20-jul-1988 (roger)
**	     Added error messages known only within ER.
**	12-jun-89 (jrb)
**	    Added generic error field to MESSAGE_ENTRY struct.
**	    Removed #defs for MNX files since these are now in ER.H
**	Sept-90 (bobm)
**	    Slow message file format change to remove size limitation
**	    (version _v3).  Add magic number / version to file.
**	21-oct-92 (andre)
**	    changed ER_MAX_NAME to 32 (maximum length of a #define name) +
**	    removed MESSAGE_ENTRY.me_generic_code +
**	    added MESSAGE_ENTRY.me_sqlstate +
**	    upped ER_VERSION to 4
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
**      28-sep-1998 (canor01)
**          Move some of the more useful definitions to cscl.h.
**	12-jan-1999 (canor01)
**	    Move ER_UNIXERROR and ER_INTERR_TESTMSG to ercl.h where they will
**	    be visible to the world and not re-used.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-sep-2001 (somsa01)
**	    Added an MU_SEMAPHORE member to ER_SEMFUNCS.
**      17-jan-2002 (stial01)
**          Added WARNING to ER_SEMFUNCS, there is a copy in cl/clf/cm/cmfile.c
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**/

#define		ER_HEADSUFF	ERx("h")

#define		SUCCESS		0x01
#define		ABORT		0x2c

#define		MAX_MSG_LINE	1500
#define		RW_MAXLINE	1500

#define		GIDSIZE		8
#define		CLASS_SIZE	341


/*}
** Name:	ERCONTROL	- each class's control part
**
** Description:
**	This 'ERCONTROL' is set the information about message number table
**	size, message area size and message data offset in runtime fast
**	message file.  
**
** History:
**	01-Oct-1986 (kobayashi) - first written
*/
typedef struct control_r
{
	i4	tblsize;
	i4 	areasize;
	i4	offset;
} ERCONTROL;

/*}
** Name: INDEX_PAGE - Describes the format of the index page.
**
** Description:
**	This structure describes the format of the index page that is 
**	used to locate the bucket that should contain the error message
**	that we are looking for.
**
**	NOTE - there is an assumption that this structure is 2048 bytes, the
**		record size for the message file.  Rather than push our luck,
**		we better ONLY add i4 items to it, and adjust the
**		ER_SMAX_KEY value accordingly.  Thus, we combine a magic
**		number and version into a single i4, rather than two
**		shorts.  The macros defined below control how they are
**		combined.
**
**
**	History:
**	01-oct-1986
**	Create new for 5.0.
*/

#define	ER_SMAX_KEY	510

#define ER_MAGIC	666
#define ER_VERSION	4	/* matches suffix on file name */

#define ER_SANITY(M,V)	((i4) M << 16 | V)
#define ER_GETMAG(S)	(S >> 16)
#define ER_GETVER(S)	(S & 0xffffL)

typedef	struct 	_INDEX_PAGE
{
	i4		sanity;		   /* magic number and version */
	i4		index_count;	   /* Number of index entries. */
	i4		index_key[ER_SMAX_KEY];
					   /* Highest message in corresponding
					   ** bucket. */
}	INDEX_PAGE;

/*}
**	Name: MESSAGE_ENTRY - Format of a message entry.
**
** Description:
**	Each message on a page is described in the following format.
**	Messages are always a multiple of 4 bytes long, and messages do not
**	span across pages.
**
** History:
**	01-oct-1986
**		Create new for 5.0
**	12-jun-1989 (jrb)
**	    Added generic error field.
**	21-oct-92 (andre)
**	    changed ER_MAX_NAME to 32 (maximum length of a #define name) +
**	    removed me_generic_code +
**	    added me_sqlstate
*/

#define	ER_MAX_NAME		32

typedef	struct	_MESSAGE_ENTRY
{
    i2		me_length;		/* Length of the message entry. */
    i2		me_text_length;		/* Length of the test portion. */
    i4		me_msg_number;		/* The message number for thismessage */
    i2		me_name_length;		/* Length of the name field. */
    char	me_sqlstate[5];		/* SQLSTATE status code for this msg */
    char	me_name_text[ER_MAX_NAME + 1 + ER_MAX_LEN];
					/* Area for message name and message text. */
}   MESSAGE_ENTRY;

/*}
**	Name: ER_CLASS_TABLE - Format of a class table.
**
** Description:
**	Each class table in allocated memory is described in the following
**	format.
**	In this table, it includes the pointer of message table header and
**	the number of message on each class.
**
** History:
**	01-oct-1986
**		Create new for 5.0 KANJI
**	17-mar-1989 (Mike S)
**	     Added "data_pointer" field to ER_CLASS
*/
typedef	struct	_ER_CLASS_TABLE
{
	u_char **message_pointer;	/* the pointer of message table header */
	u_char *data_pointer;	/* pointer to message text area */
	u_i4 number_message;	/* the number of message */
}	ER_CLASS_TABLE;

/*}
** Name:	ERFILE - System dependent file control part.
**
** Description:
**	If system is VMS, this structure is stored internal file id and 
**	internal stream id for both slow and fast.
**	If system is UNIX, this structure is stored file stream to be
**	returned (f)open function.
**
** History:
**	15-Oct-1986 (kobayashi) - first written
**
*/
typedef FILE	*ERFILE;

/*}
** Name:	ERMULTI	- Internal table to be used multi language
**
** Description:
**	This 'ERMULTI' is set ER_CLASS_TABLE pointer,number of class,
**	language id,flag of default language,internal file id. and  internal
**	stream id. of fast and slow message file .
**	And this is used internal message system.
**
** History:
**	01-Oct-1986 (kobayashi) - first written
*/
typedef struct _ERMULTI
{
	ER_CLASS_TABLE *class;	/* the base pointer of class table */
	u_i4 number_class;	/* the size of class table */
	i4 language;		/* language id */
	bool deflang;		/* the flag of default language */
	ERFILE	ERfile[2];	/* machine dependent file descriper */
} ERMULTI;


/*}
** Name: ER_LANGT_DEF - 	definition of ER_langt table.
**
** Description:
**	This defines data type of ER_langt table for multiple language.
**  This table is used to get directory name and internal code.
**  (in ERlangcode and ERlangstr) In fact, these are defined in ERinternal.c
**
** History:
**	07-Oct-1986 (kobayashi)
**		Created new for 5.0 KANJI.
*/
typedef struct _ER_LANGT_DEF
{
	char *language;
	u_i4 code;
}	ER_LANGT_DEF;

/*}
** Name: ER_SEMFUNCS - Holds semaphoring information for IO to the message file.
**
** Description:
**	This structure will be set up at ERinit() time if semaphoring routines
**  are supplied.  It will be used by cer_getdata() when IO to the message file
**  occurs.
**
** History:
**	29-sep-1988 (thurston)
**	    Created.
**	21-sep-2001 (somsa01)
**	    Added an MU_SEMAPHORE member to ER_SEMFUNCS.
*/

typedef struct _ER_SEMFUNCS
{
/* 
** WARNING !!!
** This structure must be kept in sync with the one in cl/clf/cm/cmfile.c
*/
    STATUS	    (*er_p_semaphore)();
    STATUS	    (*er_v_semaphore)();
    u_i2	    sem_type;
#define		    CS_SEM	0x01	/* CS semaphore routines */
#define		    MU_SEM	0x02	/* MU semaphore routines */

    CS_SEMAPHORE    er_sem;
    MU_SEMAPHORE    er_mu_sem;
}	ER_SEMFUNCS;

/*
** Define a structure that describes a variable length item.
*/

typedef struct _DESCRIPTOR
{
    int             desc_length;        /* Length of the item. */
    char            *desc_value;        /* Pointer to item. */
}   DESCRIPTOR;


/* Max number of languages in one server */

#define ER_MAXLANGUAGE	(i4)5

/* Max # of characters in a language name */

#define	ER_MAXLNAME	(i4)25

/*  This definition is used cer_sstr function */

#define	ER_GET		1
#define	ER_LOOKUP	2

/* definitions used by alternate message file processing */
# define ER_SETFNAME	0
# define ER_GETFNAME    1
# define ER_SETFD       2
# define ER_GETFD       4
# define ER_RESET       8

/*  The following are globalrefs within the ER library */

GLOBALREF	ERMULTI		ERmulti[];
GLOBALREF	ER_LANGT_DEF	ER_langt[];

/*  following functions are global func. in only ER library */

FUNC_EXTERN STATUS cer_close(ERFILE *);
FUNC_EXTERN STATUS cer_open(LOCATION *, ERFILE *, CL_ERR_DESC *);
FUNC_EXTERN STATUS cer_getdata(char *, i4, i4, ERFILE *, CL_ERR_DESC *);
FUNC_EXTERN STATUS cer_dataset(i4, i4, char *, ERFILE *);
FUNC_EXTERN STATUS cer_sysgetmsg(CL_ERR_DESC *, i4 *,
			 DESCRIPTOR *, CL_ERR_DESC *);

FUNC_EXTERN VOID cer_excep(STATUS, char **);

FUNC_EXTERN bool cer_isasync(void);
FUNC_EXTERN bool cer_istest(void);
FUNC_EXTERN bool cer_issem(ER_SEMFUNCS **);

FUNC_EXTERN STATUS cer_get(ER_MSGID, char **, i4, i4);
FUNC_EXTERN i4  cer_fndindex(i4);
FUNC_EXTERN STATUS cer_nxtindex(i4, i4 *);
FUNC_EXTERN STATUS cer_finit(i4, ER_MSGID, i4, CL_ERR_DESC *);
FUNC_EXTERN STATUS cer_sinit(i4, ER_MSGID, i4, CL_ERR_DESC *);
FUNC_EXTERN STATUS cer_fopen(ER_MSGID, i4, LOCATION *, i4, CL_ERR_DESC *);
FUNC_EXTERN STATUS cer_init(i4, i4, LOCATION *, char *, i4);
FUNC_EXTERN char *cer_fstr(ER_CLASS, i4, i4);
FUNC_EXTERN STATUS cer_read(ER_CLASS, i4);
FUNC_EXTERN bool cer_isopen(i4, i4);
FUNC_EXTERN STATUS cer_sstr(ER_MSGID, char *, char *, i4, i4, CL_ERR_DESC *, i4);
FUNC_EXTERN VOID cer_tclose(ERFILE *);
FUNC_EXTERN VOID cer_tfile(ER_MSGID, i4, char *, bool);
