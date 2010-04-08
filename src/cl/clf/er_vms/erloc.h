/*
**    Copyright (c) 1986, 2000 Ingres Corporation
*/

/**
** Name:  ERLOC.H - Internal ER header file
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
**	======================================================================
**	NOTE!!!  If this file is #included, the program including it must also
**		 #include <cs.h>, and <si.h> if VMS.
**	======================================================================
**
** History:
**	01-Oct-1986 (kobayashi) - first written
**	29-sep-88 (thurston)
**	    Added the ER_SEMFUNCS typedef.
**	21-oct-88 (thurston)
**	    Got rid of nested #includes.
**	06-mar-89 (Mike S)
**	    Add fields to indicate mapped fast message file
**	    Add field in ER_CLASS_TABLE to point to messages
**	30-apr-89 (jrb)
**	    Added generic error field to MESSAGE_ENTRY struct.
**	08-may-89 (jrb)
**	    Changed name of "slow.mnx" and "fast.mnx" to "slow_v2.mnx" and
**	    "fast_v2.mnx" respectively.  This was because these versions are no
**	    longer compatible with the older mnx files.
**	23-may-89 (jrb)
**	    Removed #defs for MNX files since these are now in ER.H
**	02-aug-89 (Mike S)
**	    Change section name for 7.0 (similar to 08-may-89 change).
**	27-jun-89 (sandyd)
**	    Changed section name for 6.4 (*_v2.mnx now becomes *_v3.mnx).
**	    Also moved ER_FASTSECT next to ER_VERSION, as a reminder that
**	    they must be changed in synch.
**	21-oct-92 (andre)
**	    changed ER_MAX_NAME to 32 (maximum length of a #define name) +
**	    removed MESSAGE_ENTRY.me_generic_code +
**	    added MESSAGE_ENTRY.me_sqlstate +
**	    upped ER_VERSION to 4
**	14-dec-92 (sandyd)
**	    Updated fast-message global section name (ER_FASTSECT) to
**	    correspond to the new _v4.mnx format.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
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
**	History:
**	01-oct-1986
**	Create new for 5.0.
**
**	NOTE - there is an assumption that this structure is 2048 bytes, the
**		record size for the message file.  Rather than push our luck,
**		we better ONLY add i4 items to it, and adjust the
**		ER_SMAX_KEY value accordingly.  Thus, we combine a magic
**		number and version into a single i4, rather than two
**		shorts.  The macros defined below control how they are
**		combined.
*/

#define	ER_SMAX_KEY	510
#define ER_MAGIC	666
#define ER_VERSION	4	/* matches suffix on file name */
/*
**	ER_FASTSECT is used to construct the global section name for
**	the fast messages.  Before changing it, read the discussion in
**	front!frontcl!fmsginst!iifstmsg.c.
*/
#define	ER_FASTSECT	ERx("II_FST4MSGS_")


#define ER_SANITY(M,V)	((i4) M << 16 | V)
#define ER_GETMAG(S)	(S >> 16)
#define ER_GETVER(S)	(S & 0xffffL)

typedef	struct 	_INDEX_PAGE
{
	i4		index_count;	   /* Number of index entries. */
	i4		sanity;		   /* magic number and version */
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
**	30-apr-1989 (jrb)
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
					/* Area for message name and text. */
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
*/
typedef	struct	_ER_CLASS_TABLE
{
 	u_char **message_pointer;/* the pointer of message table header */
 	u_char *data_pointer;    /* the pointer to the message data */
	u_i4 number_message;	 /* the number of message */
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
    PTR		map_addr;
    bool	mapped;
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

/* Max number of languages in one server */

#define ER_MAXLANGUAGE	(i4)5

/* Max # of characters in a language name */

#define	ER_MAXLNAME	(i4)25

/*  This definition is used cer_sstr function */

#define	ER_GET		1
#define	ER_LOOKUP	2

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

/*  The following are globalrefs within the ER library */

GLOBALREF	ERMULTI		ERmulti[];
GLOBALREF	ER_LANGT_DEF	ER_langt[];

/*  following functions are global func. in only ER library */

FUNC_EXTERN STATUS cer_get();
FUNC_EXTERN STATUS cer_init();
FUNC_EXTERN STATUS cer_finit();
FUNC_EXTERN STATUS cer_sinit();
FUNC_EXTERN i4 cer_fndindex();
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
FUNC_EXTERN char    *cer_tfile();
