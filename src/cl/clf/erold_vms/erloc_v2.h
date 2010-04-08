/*
**    Copyright (c) 1986, 2000 Ingres Corporation
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
**	29-nov-1990 (sandyd)
**	    This is an archived version of the old "v2" erloc.h taken from the 
**	    ingres6302p piccolo library (unix!cl!clf!er!erloc.h r1.1).
**	    We use this to build an old "v2" version of ercompile so that 
**	    we can recreate "{fast,slow}_v2.mnx" files from source in new 
**	    releases.  (See the README file for details.)
**	    To minimize the risk of future changes in <er.h> breaking
**	    this old ercompiler, the few definitions needed from <er.h>
**	    have been moved into this file, and the reference to <er.h>
**	    from ercomp_v2.c has been removed.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/*
** DEFINITIONS PULLED FROM <er.h>
*/
/*===========================*/
#define		ER_FASTFILE	ERx("fast_v2.mnx")
#define		ER_SLOWFILE	ERx("slow_v2.mnx")
#define		ERx(c)		c
#define		ER_MAX_LEN	(i4)1000
typedef		u_i4		ER_MSGID;	/* data type of message id */
typedef		i4		ER_CLASS;	/* data type of class number */
/*===========================*/

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

/* Error messages that needn't be publicized */

# define ER_UNIXERROR          0x00010910    /* used by ERlookup */
# define ER_INTERR_TESTMSG     0x00010911    /* can be used to test */



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
*/

#define	ER_SMAX_KEY	511

typedef	struct 	_INDEX_PAGE
{
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
FUNC_EXTERN VOID    cer_tclose();
