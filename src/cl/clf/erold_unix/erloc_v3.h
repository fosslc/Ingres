/*
**Copyright (c) 2004 Ingres Corporation
*/

# ifndef    VMS
# include   <si.h>
# endif

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
**	08-dec-1995 (sweeney)
**	    add as new code to build 6.4 (_v3) format .mnx files for 1.1+
**	11-apr-1999 (schte01)
**	    Remove definitions for ER_UNIXERROR and ER_INTERR_TESTMSG which
**       were added to ercl.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define		ER_HEADSUFF	ERx("h")

#define		ER_FASTSIDE	(i4)1
#define		ER_SLOWSIDE	(i4)0

#define		SUCCESS		0x01
#define		ABORT		0x2c

#define		MAX_MSG_LINE	1500
#define		RW_MAXLINE	1500

#define		GIDSIZE		8
#define		CLASS_SIZE	341

/*	Masks used in taking apart MSGID */

#define		ER_MSGIDMASK1	0x10000000L
#define		ER_MSGIDMASK2	0x0FFF0000L
#define		ER_MSGIDMASK3	0xFFFFL
#define		ER_MSGIDMASK4	0x01L
#define		ER_MSGIDMASK5	0xFFFL

#define		ER_MSGHISHIFT	28
#define		ER_MSGMIDSHIFT	16




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
#define ER_VERSION	3	/* matches suffix on file name */

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
*/

#define	ER_MAX_NAME		30

typedef	struct	_MESSAGE_ENTRY
{
    i2		me_length;		/* Length of the message entry. */
    i2		me_text_length;		/* Length of the test portion. */
    i4		me_msg_number;		/* The message number for thismessage */
    i4		me_generic_code;	/* The generic error for this entry */
    i2		me_name_length;		/* Length of the name field. */
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
#ifdef	VMS
typedef struct _ERFILE
{
    i4	w_ifi;
    i4	w_isi;
}   ERFILE;
#else
typedef FILE	*ERFILE;
#endif

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
*/

typedef struct _ER_SEMFUNCS
{
    STATUS	    (*er_p_semaphore)();
    STATUS	    (*er_v_semaphore)();
    CS_SEMAPHORE    er_sem;
}	ER_SEMFUNCS;

/* Max number of languages in one server */

#define ER_MAXLANGUAGE	(i4)5

/* Max # of characters in a language name */

#define	ER_MAXLNAME	(i4)25

/*  This definition is used cer_sstr function */

#define	ER_GET		1
#define	ER_LOOKUP	2

/*  The following are globalrefs within the ER library */

GLOBALREF	ERMULTI		ERmulti[];
GLOBALREF	ER_LANGT_DEF	ER_langt[];

/*  following functions are global func. in only ER library */

FUNC_EXTERN STATUS cer_get();
FUNC_EXTERN STATUS cer_init();
FUNC_EXTERN STATUS cer_finit();
FUNC_EXTERN STATUS cer_sinit();
FUNC_EXTERN i4  cer_fndindex();
FUNC_EXTERN STATUS cer_nxtindex();
FUNC_EXTERN char *cer_fstr();
FUNC_EXTERN STATUS cer_sstr();
FUNC_EXTERN STATUS  cer_open();
FUNC_EXTERN STATUS  cer_close();
FUNC_EXTERN STATUS  cer_get();
FUNC_EXTERN bool    cer_isopen();
FUNC_EXTERN STATUS  cer_read();
FUNC_EXTERN STATUS  cer_dataset();
FUNC_EXTERN STATUS  cer_sysgetmsg();
FUNC_EXTERN bool    cer_isasync();
FUNC_EXTERN bool    cer_istest();
FUNC_EXTERN bool    cer_issem();
FUNC_EXTERN VOID    cer_tclose();
