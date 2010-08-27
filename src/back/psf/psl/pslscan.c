/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <me.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <cui.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfindep.h>
#include    <psfparse.h>
#include    <pshparse.h>
#include    "pslgram.h"
#include    "pslscan.h"
#include    <yacc.h>
#include    <cui.h>

/**
**
**  Name: PSLSCAN.ROC - The scanner for the server's QUEL parser.
**
**  Description:
**      This file contains the scanner (lexical analyser) for the QUEL scanner
**	in the server.  It also contains all related tables.
**
**          psl_scan - The QUEL scanner.
[@func_list@]...
**
**
**  History:    
**      30-jan-86 (jeff)    
**          Adapted from the 3.0 version.  Clobbered set_scanbuf_size, since
**	    all buffers are now dynamically expandible.
**	19-jul-86 (jeff)
**	    Added copying of query text to output chain
**      29-aug-86 (seputis)
**          changed pss_cb->pss_memleft to &pss_cb->pss_memleft
**	09-sep-1986 (fred)
**	    Changed offsets in Key_index table to correspond
**	    correctly with the keywords in Key_string.  This should
**	    be figured out at startup rather than be a manual
**	    operation (please).  Also changed CL include files
**	    to be specified in lower case as is the standard.
**	24-nov-1986 (daved)
**	    added OPEN as a keyword. Add necessary keywords to support
**	    cursor commands.
**	8-jan-1987 (daved)
**	    handle HEXCONST. convert x'....' to a VARCHAR and return.
**	3-aug-87 (daved)
**	    remove HEXCONST from QUEL (for now).
**	13-sep-88 (andre)
**	    changed order of Setwords to put them in ascending sequence
**	    (as they were expected to be in the first place)
**	03-oct-88 (stec)
**	    Complete Kanji related changes.
**	05-oct-88 (stec)
**	    Fix conversion of names to lowercase problem.
**	18-oct-88 (stec)
**	    Fix recognition of a Kanji name too long bug.
**	03-nov-88 (stec)
**	    Initialize digct to zero.
**	22-nov-88 (stec)
**	    Fix set noio_trace command.
**	18-sep-89 (ralph)
**	    Added double reserved words: SETMAXIO, SETMXROW, SETMXCPU,
**	    SETMXPAGE, SETMXCOST.
**	27-jul-89 (jrb)
**	    Changed scanning of numeric literals to return decimal for a certain
**	    class of numbers.  Also, allowed for backward compatibility with old
**	    literal semantics by checking pss_num_literals flag in session cb.
**	12-sep-90 (teresa)
**	    the following booleans are now bitflags: pss_txtemit
**	2-nov-90 (andre)
**	    fix bug 33417.  If toksize > 0 AND the current symbol block doesn't
**	    have enough space for the piece pointer, we need to copy the data
**	    being returned into the new block.  yacc_cb->yylval contains an
**	    address of the data being returned, so we must copy toksize bytes
**	    starting with the address found in yacc_cb->yylval, and not starting
**	    with address of yacc_cb->yylval as was done until now.
**	5-nov-90 (andre)
**	    replaced pss-flag with pss_stmt_flags and pss_dbp_flags
**	26-dec-90 (andre)
**	    define SET [NO]OPTIMIZEONLY as a double keyword
**	24-may-91 (andre)
**	    Define double keywords SETROLE, SET_NOROLE, SETGROUP, SET_NOGROUP,
**	    and SETUSER
**	12-nov-91 (rblumer)
**	  merged from 6.4:   4-feb-91 (rogerk)
**	    Moved SET [NO]LOGGING and SET LOGTRACE support from SQL to QUEL.
**	  merged from 6.4:  25-feb-91 (rogerk)
**          Added SET SESSION as a double keyword.
**	  merged from 6.4:  22-jul-91 (rog)
**	    Specify correct number of arguments for errors 2700 and 2705. (9877)
**	20-jul-92 (andre)
**	    add a new keyword - EXCLUDING
**      8-nov-1992 (ed)
**          increased number of elements in key_info for DB_MAXNAME
**	01-dec-92 (andre)
**	    removed support for SET USER/GROUP/ROLE from QUEL
**	01-dec-92 (andre)
**	    removed support for SET USER/GROUP/ROLE from QUEL
**	12-mar-93 (ralph)
**	    DELIM_IDENT:
**	    Translate regular identifiers according to database semantics
**	06-mar-93 (andre)
**	    got rid of DELCURSOR double keyword.  DELETE CURSOR should not go
**	    through the grammar - this will make sure that if it does, an error
**	    will be produced 
**	06-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Change reference to pss_cb->pss_dbxlate to be ptr to u_i4.
**      06-may-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT statement. New keyword
**          CHANGED.
**	29-jun-93 (andre)
**	    #included CV.H
**	06-jul-93 (anitap)
**	    Undid the above change for "SET UPDATE_ROWCOUNT" statement.
**      14-jul-93 (ed)
**          replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	21-mar-95 (ramra01)
**	    Interpretation of II_DECIMAL during create table with decimal
**	    Bug 67519.
**	03-nov-95 (pchang)
**	    Change to use cui_f_idxlate() which is faster than cui_idxlate(),
**	    when translating regular identifiers.
**	19-mar-96 (pchang)
**	    Fixed bug 70204.  Incorrect test on the next symbol location for
**	    byte alignment prevented a new symbol block to be allocated when
**	    there are exactly 2 bytes left on the current symbol table, and
**	    subsequent alignment operation on the next symbol pointer caused
**	    it to advance beyond the current symbol block and thus, corrupting
**	    adjacent object when the next symbol was assigned.  SEGVIO followed
**	    when the trashed object was later referenced.
**	26-mar-96 (abowler)
**	    Bugs 69866 & 75493: If II_DECIMAL is ',', handle case of
**	    'select col,0' by checking to see if last token was a NAME.
**	27-Feb-1997 (jenjo02)
**	    Extended <set session> statement to support <session access mode>
**	    (READ ONLY | READ WRITE).
**	    Added new keywords, READ and WRITE.
**	21-jul-97 (inkdo01)
**	    Added SET [NO]FLATTEN to permit control of subselect flattening
**	    without trace point privileges (as needed for op132).
**	24-Nov-1997 (shero03)
**	    Bug 87127 - decimal constant over 15 digits is truncated to 15.
**	19-Jan-1999 (shero03)
**	    Add set random_seed
**      03-Dec-1998 (hanal04)
**          Added support for 'set transaction' statement to QUEL. b94351.
**      24-aug-1999 (gygro01)
**	    Backed change for b94351 / pb ingsrv 816, becaused it caused
**	    bug 98507 / pb ingsrv 995, found during QA testing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      23-Mar-2001 (toumi01)
**	    Add support for 'set transaction' statement to [E]QUEL. b94351.
**	    This will allow db utilities written in quel to issue e.g.
**	    SET TRANSACTION READ WRITE to override an installation level
**	    isolation level setting of READ UNCOMMITTED.
**	23-nov-04 (inkdo01)
**	    Added "set [no]hash", "set [no]ojflatten", "set [no]parallel"
**	    having forgotten about them for Quel.
**	24-May-2006 (kschendel)
**	    Remove obsolete (v5) SET keywords, J_FREESZn and suchlike.
**      10-sep-2008 (gupsh01,stial01)
**          Added psl_quel_unorm()
**      22-sep-2008 (stial01)
**          Added unorm for QDATA (b120938)
**      04-nov-2008 (stial01)   
**          Don't require Key_info to have DB_MAXNAME entries
**      20-nov-2008 (stial01)
**          Don't assume data value for PSQ_HVQ has db_length > DB_MAXNAME
**      10-mar-2009 (stial01)
**          Added unorm error handling (b121769)
**	18-Mar-2010 (kiria01) b123438
**	    Avoid editing the sorted token list directly. See
**	    pslscanprep.awk.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
**	21-Jul-2010 (kschendel) SIR 124104
**	    Add set [no]create_compression.
*/

/*
**  Defines of other constants.
*/

/*
**      KEY_INFO_SIZE is the number of entries in the Key_info table.
*/
#define                 KEY_INFO_SZ (sizeof(Key_info)/sizeof(KEYINFO)


/*}
** Name: AGGTOK - Table of aggregate token values.
**
** Description:
**      This structure is used to contain the set of aggregate token values
**      for aggregate tokens. The number in the tokval field for keywords
**	is used to find the text which is to be returned.
**
** History:
**	13-oct-86 (daved)
**	    written
*/
typedef struct _AGGTOK
{
    char            *stoken;		/* A token value */
}   AGGTOK;


/*}
** Name: SECONDARY - Table of second words for a keyword token.
**
** Description:
**      This structure is used to contain the set of second words for a
**      two-word keyword.  It is assumed that each such table is in
**	alphabetical order, to make searching faster.  The structure contains
**	a pointer to a second word, the token to return for that word, and
**	the value for the token.
**
** History:
**     28-jan-86 (jeff)
**          written
*/
typedef struct _SECONDARY
{
    char            *word2;		/* A second word */
    i4              token2;		/* The corresponding token */
    i4              val2;           /* The corresponding value */
}   SECONDARY;

/*}
** Name: KEYINFO - Information about keywords.
**
** Description:
**      This structure is used to define keywords in the QUEL grammar.  For
**      each keyword, it gives the token to return, the value associated with
**	the token (if any), and a pointer to another keyword table for those
**	cases of compund keywords (keywords composed of more than one word).
**
** History:
**     28-jan-86 (jeff)
**          Modified from old Key_info struct.
*/
typedef struct _KEYINFO
{
    i4              token;              /* The token to be returned */
    i4		    tokval;             /* Value of the token */
    i4              num2ary;            /* Number of possible 2nd words */
    SECONDARY       *secondary;         /* Table of second words */
}   KEYINFO;

/*
** Definition of all global variables owned by this file.
*/

/*
** static const u_char Key_string[]
**
** This structure contains all of the keywords.  It doesn't contain pointers
** to the keyword strings; rather, the strings are embedded directly in this
** structure.  The keywords are arranged by length order: all of the two-letter
** keywords are together, as are the three-letter keywords, etc.  Within each
** group they are in alphabetical order.  Each keyword is preceded by an index
** into the Key_info[] array, and is followed by a zero.  Each set of
** keywords is followed by a zero.
**
** static const u_char *Key_index[]
** 
** When searching for a keyword, first find the length n of the word, and get
** the n'th entry in Key_index[].  This will be a pointer to the set of
** keywords of length n in Key_string[].  Then you can look for the word,
** searching until you find word or the end of the set of words.  If you find
** the word, use the number immediately preceding it as an index into
** Key_info[].  The entry there will give the token and its corresponding
** value.
**
** static const u_char *Key_info[]
**
** Some tokens are composed of two words.  In cases like this, Key_info[]
** will contain a pointer to a list of possible second words, along with the
** number of possible second words.  The list of second words will be in
** alphabetical order.  You can do a binary search to look up the second word.
** If you find it, the list of second words will contain the token and its
** corrseponding value.  If you don't find the second word, then the token and
** its value will be in Key_info[], which you have already looked up.  If
** the token there is zero, this means that the first word should be treated
** as a name if the second word is not found.
*/

/*
** Key_string and Key_index are now generated from a data file to ease maintenance.
** See pslscanprep.awk for information.
*/
#include "pslscantoks.h"


/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "begin" to form a keyword.
*/
static const SECONDARY      Beginwords[] = {
		       { "transaction", BGNXACT, PSL_GOVAL }
					     };

#define             BEGINSIZE           (sizeof(Beginwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "CLOSE" to form a keyword.
*/
static const SECONDARY      Closewords[] = {
		       { "cursor", CLSCURSOR, PSL_GOVAL }
					     };

#define             CLOSESIZE           (sizeof(Closewords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "define" to form a keyword.
*/
static const SECONDARY      Definewords[] = {
			{ "cursor",    DEFCURSOR,PSL_GOVAL },	
			{ "integrity", DEFINTEG, PSL_GOVAL },
			{ "location",  DEFLOC,	 PSL_GOVAL },
			{ "permit",    DEFPERM,  PSL_GOVAL },
			/* 6.0 repeat query */
			{ "qry",     DEFQRY,	 PSL_GOVAL },
			/* 5.0 repeat query */
			{ "query",     DEFQRY,	 PSL_GOVAL },
			{ "view",      DEFVIEW,  PSL_GOVAL }
					      };

#define             DEFSIZE             (sizeof(Definewords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "destroy" to form a keyword.
*/
static const SECONDARY      Destroywords[] = {
			{ "integrity", DESTINTEG, PSL_GOVAL },
			{ "permit",    DESTPERM,  PSL_GOVAL },
					       };

#define             DESTSIZE            (sizeof(Destroywords)/sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "for" to form a keyword.
*/
static const SECONDARY      Forwords[] = {
		       { "deferred",	DEFERUPD,   0},
		       { "direct",	DIRECTUPD,  0},
		       { "readonly",	FORREADONLY,0},
		       { "retrieve",    RETRIEVE,   0},
		       { "update",	FORUPDATE,  0}
					     };

#define             FORSIZE           (sizeof(Forwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "is" to form a keyword.
*/
static const SECONDARY      Iswords[] = {
			{ "null", ISNULL,   0}
					   };

#define             ISSIZE               (sizeof(Iswords)/sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "not" to form a keyword.
*/
static const SECONDARY	Notwords[] = {
			{ "null", NOTNULL, 0 }
					    };
#define		    NOTSIZE		(sizeof(Notwords)/sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "only" to form a keyword.
*/
static const SECONDARY	Onlywords[] = {
			{ "where", ONLYWHERE, 0 }
					    };
#define		    ONLYSIZE		(sizeof(Onlywords)/sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "open" to form a keyword.
*/
static const SECONDARY      Openwords[] = {
		       { "cursor", OPENCURSOR, PSL_GOVAL }
					     };

#define             OPENSIZE           (sizeof(Openwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "order" to form a keyword.
*/
static const SECONDARY      Orderwords[] = {
		       { "by", ORDERBY, PSL_GOVAL }
					     };

#define             ORDERSIZE           (sizeof(Orderwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "replace" to form a keyword.
*/
static const SECONDARY      Repwords[] = {
		       { "cursor", REPCURSOR, PSL_GOVAL }
					     };

#define             REPSIZE           (sizeof(Repwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "retrieve" to form a keyword.
*/
static const SECONDARY      Retwords[] = {
		       { "cursor", RETCURSOR, PSL_GOVAL }
					     };

#define             RETSIZE           (sizeof(Retwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "set" to form a keyword.
*/
static const SECONDARY      Setwords[] = {
			{ "aggregate",    SETAGGR,     PSL_GOVAL  },
			{ "autocommit",	  SETAUTOCOMMIT, PSL_GOVAL},
		        { "cpufactor",    SETCPUFACT,  PSL_GOVAL  },
		        { "create_compression",  SETCREATECOMPRESSION,  PSL_ONSET  },
		        { "date_format",  SETDATEFMT,  PSL_GOVAL  },
			{ "decimal",      SETDECIMAL,  PSL_GOVAL  },
			{ "flatten",	  SETFLATTEN,  PSL_ONSET  },
			{ "hash",   	  SETHASH,     PSL_ONSET  },
			{ "io_trace",     SETIOTRACE,  PSL_ONSET  },
			{ "joinop",       SETJOINOP,   PSL_ONSET  },
			{ "journaling",   SETJOURNAL,  PSL_ONSET  },
			{ "lock_trace",   SETLTRACE,   PSL_ONSET  },
			{ "lockmode",     SETLOCKMODE, PSL_GOVAL  },
			{ "log_trace",    SETLOGTRACE, PSL_ONSET  },
			{ "logging",	  SETLOGGING,  PSL_ONSET  },
			{ "maxcost",	  SETMXCOST,   PSL_ONSET  },
			{ "maxcpu",	  SETMXCPU,    PSL_ONSET  },
			{ "maxio",	  SETMXIO,     PSL_ONSET  },
			{ "maxpage",	  SETMXPAGE,   PSL_ONSET  },
			{ "maxquery",	  SETMXIO,     PSL_ONSET  },
			{ "maxrow",	  SETMXROW,    PSL_ONSET  },
			{ "money_format", SETMNYFMT,   PSL_GOVAL  },
			{ "money_prec",   SETMNYPREC,  PSL_GOVAL  },
		        { "nocreate_compression",  SETCREATECOMPRESSION,  PSL_OFFSET  },
			{ "noflatten",    SETFLATTEN,  PSL_OFFSET },
			{ "nohash",       SETHASH,     PSL_OFFSET },
			{ "noio_trace",   SETIOTRACE,  PSL_OFFSET },
			{ "nojoinop",     SETJOINOP,   PSL_OFFSET },
			{ "nojournaling", SETJOURNAL,  PSL_OFFSET },
			{ "nolock_trace", SETLTRACE,   PSL_OFFSET },
			{ "nolog_trace",  SETLOGTRACE, PSL_OFFSET },
			{ "nologging",	  SETLOGGING,  PSL_OFFSET },
			{ "nomaxcost",	  SETMXCOST,   PSL_OFFSET },
			{ "nomaxcpu",	  SETMXCPU,    PSL_OFFSET },
			{ "nomaxio",	  SETMXIO,     PSL_OFFSET },
			{ "nomaxpage",	  SETMXPAGE,   PSL_OFFSET },
			{ "nomaxquery",	  SETMXIO,     PSL_OFFSET },
			{ "nomaxrow",	  SETMXROW,    PSL_OFFSET },
			{ "noojflatten",  SETOJFLATTEN, PSL_OFFSET },
			{ "nooptimizeonly", SETOPTIMIZEONLY, PSL_OFFSET },
			{ "noparallel",   SETPARALLEL, PSL_OFFSET },
			{ "noprintqry",	  SETPRINTQRY, PSL_OFFSET },
			{ "noqep",	  SETQEP,      PSL_OFFSET },
			{ "nosql",        SETSQL,      PSL_OFFSET },
			{ "nostatistics", SETSTATS,    PSL_OFFSET },
			{ "notrace",      SETTRACE,    PSL_OFFSET },
			{ "ojflatten",    SETOJFLATTEN, PSL_ONSET },
			{ "optimizeonly", SETOPTIMIZEONLY, PSL_ONSET },
			{ "parallel",     SETPARALLEL, PSL_ONSET  },
			{ "printqry",	  SETPRINTQRY, PSL_ONSET  },
			{ "qep",	  SETQEP,      PSL_ONSET  },
			{ "random_seed",  SETRANDOMSEED,PSL_OFFSET },
			{ "result_structure", SETRSTRUCT, PSL_GOVAL},
			{ "ret_into",     SETRETINTO,  PSL_GOVAL  },
                        { "session",      SETSESSION,  PSL_GOVAL  },
			{ "sql",          SETSQL,      PSL_ONSET  },
			{ "statistics",   SETSTATS,    PSL_ONSET  },
			{ "trace",        SETTRACE,    PSL_ONSET  },
                        { "transaction",  SETTRANSACTION,    PSL_GOVAL },
			{ "update_rowcount", SETUPDROWCNT, PSL_GOVAL },
					   };

#define             SETSIZE             (sizeof(Setwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "end" to form a keyword.
*/
static const SECONDARY      Endwords[] = {
			{ "transaction",    ENDXACT,     PSL_GOVAL  }
					    };
#define             ENDSIZE             (sizeof(Endwords) / sizeof(SECONDARY))

/*
** This table contains the words and the corresponding tokens and values that
** can follow the introductory word "with" to form a keyword.
*/
static const SECONDARY      Withwords[] = {
		       { "null", WITHNULL, 0}
					     };

#define             WITHSIZE          (sizeof(Withwords) / sizeof(SECONDARY))

/*
** This table contains the tokens and corresponding values for each keyword.
** For those keywords that are made up of two words, there will be a pointer
** to a secondary word table, and a count telling the length of the secondary
** table.  NOTE THAT THE ORDER OF THE ROWS IN THIS TABLE CORRESPONDS TO THE
** NUMBERS IN Key_string[].  DO NOT CHANGE THINGS AROUND IN HERE WITHOUT
** BRINGING Key_string[] UP TO DATE.
*/

static const KEYINFO                Key_info[] = {
		       /* Zero is not a legal index, this is a placeholder */
/* 0 */		       {    0,	    0,		0,	(SECONDARY *) NULL   },
/* 1 */		       {    AS,	    0,		0,	(SECONDARY *) NULL   },
/* 2 */		       {    AT,	    0,		0,	(SECONDARY *) NULL   },
/* 3 */		       {    BY,	    0,		0,	(SECONDARY *) NULL   },
/* 4 */		       {    IN,	    0,		0,	(SECONDARY *) NULL   },
/* 5 */		       {    IS,	   ADI_EQ_OP,	ISSIZE, (SECONDARY *) Iswords},
/* 6 */		       {    OF,	    0,		0,	(SECONDARY *) NULL   },
/* 7 */		       {    ON,	    0,		0,	(SECONDARY *) NULL   },
/* 8 */		       {    LBOP,   PST_OR,	0,	(SECONDARY *) NULL   },
/* 9 */		       {    TO,	    0,	        0,	(SECONDARY *) NULL   },
/* 10 */	       {    ALL,    0,		0,	(SECONDARY *) NULL   },
/* 11 */	       {    LBOP,   PST_AND,	0,	(SECONDARY *) NULL   },
/* 12 */	       {    AGOP,   1 /*any*/,	0,	(SECONDARY *) NULL   },
/* 13 */	       {    AGOP,   2 /*avg*/,	0,	(SECONDARY *) NULL   },
/* 14 */	       {    AGOP,   3 /*max*/,	0,	(SECONDARY *) NULL   },
/* 15 */	       {    AGOP,   4 /*min*/,	0,	(SECONDARY *) NULL   },
/* 16 */	       {    NOT,    0,		NOTSIZE,(SECONDARY *) Notwords},
/* 17 */	       {    SET,    PSL_GOVAL,	SETSIZE,(SECONDARY *) Setwords},
/* 18 */	       {    AGOP,   5 /*sum*/,	0,	(SECONDARY *) NULL   },
/* 19 */	       {    AGOP,   6 /*avgu*/,	0,	(SECONDARY *) NULL   },
/* 20 */	       {    COPY,   PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 21 */	       {    FROM,   0,		0,	(SECONDARY *) NULL   },
/* 22 */	       {    HELP,   0,		0,	(SECONDARY *) NULL   },
/* 23 */	       {    INTO,   0,		0,	(SECONDARY *) NULL   },
/* 24 */	       {    0/*move*/,	    0,	0,	(SECONDARY *) NULL   },
/* 25 */	       {    SAVE,   PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 26 */	       {    SORT,   0,		0,	(SECONDARY *) NULL   },
/* 27 */	       {    AGOP,   7 /*sumu*/,	0,	(SECONDARY *) NULL   },
/* 28 */	       {    WITH,   0,		WITHSIZE,(SECONDARY *) Withwords},
/* 29 */	       {    ABORT,  PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 30 */	       {    0,      0,		BEGINSIZE, (SECONDARY *) Beginwords	    },
/* 31 */	       {    AGOP,   8 /*count*/,0,	(SECONDARY *) NULL   },
/* 32 */	       {    PRINT,  PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 33 */	       {    RANGE,  PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 34 */	       {    UNTIL,  0,		0,	(SECONDARY *) NULL   },
/* 35 */	       {    WHERE,  0,		0,	(SECONDARY *) NULL   },
/* 36 */	       {    APPEND, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 37 */	       {    AGOP,   9 /*countu*/,0,	(SECONDARY *) NULL   },
/* 38 */	       {    CREATE, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 39 */	       {    0,	    0,		DEFSIZE, (SECONDARY *) Definewords	     },
/* 40 */	       {    DELETE, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 41 */	       {    MODIFY, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 42 */	       {    0,	    0,		0,	(SECONDARY *) NULL   },
/* 43 */	       {    UNIQUE, 0,		0,	(SECONDARY *) NULL   },
/* 44 */	       {   DESTROY, PSL_GOVAL,	DESTSIZE, (SECONDARY *) Destroywords	     },
/* 45 */	       {    0,	    0,		0,	(SECONDARY *) NULL },
/* 46 */	       {   EXECUTE, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 47 */	       {    0,	    0,		0,	(SECONDARY *) NULL },
/* 48 */	       {   LOGGING, 0,		0,	(SECONDARY *) NULL   },
/* 49 */	       {   REPLACE, PSL_GOVAL,	REPSIZE, (SECONDARY *) Repwords	     },
/* 50 */	       {  RELOCATE, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 51 */	       {  RETRIEVE, PSL_GOVAL,	RETSIZE, (SECONDARY *) Retwords	     },
/* 52 */	       {    0,	    0,		0,	(SECONDARY *) NULL   },
/* 53 */	       { SAVEPOINT, PSL_GOVAL,	0,	(SECONDARY *) NULL   },
/* 54 */	       {    0,	    0,		0,	(SECONDARY *) NULL   },
/* 55 */	       { VIEW,	    0,		0,	(SECONDARY *) NULL   },
/* 56 */	       { ONLY,      0,          ONLYSIZE, (SECONDARY *) Onlywords          },
/* 57 */	       {    0,	    0,		ENDSIZE,  (SECONDARY *) Endwords           },
/* 58 */	       {     INDEX, PSL_GOVAL,  0,	(SECONDARY *) NULL   },
/* 59 */	       {    0,	    0,		OPENSIZE, (SECONDARY *) Openwords	     },
/* 60 */	       {    0,	    0,		CLOSESIZE, (SECONDARY *) Closewords	     },
/* 61 */	       {    FOR,    0,	FORSIZE,  (SECONDARY *) Forwords },
/* 62 */	       {    0,      0,		0, (SECONDARY *) NULL},
/* 63 */	       {    0,	    0,	ORDERSIZE, (SECONDARY *) Orderwords },
/* 64 */	       { PERMIT,    0,		0, (SECONDARY *) NULL},
/* 65 */	       { INTEGRITY, 0,		0, (SECONDARY *) NULL},
/* 66 */	       { EXCLUDING, 0,          0, (SECONDARY *) NULL},
/* 67 */	       { READ,      0,          0, (SECONDARY *) NULL},
/* 68 */	       { WRITE,     0,          0, (SECONDARY *) NULL},
/* 69 */	       { LEVEL,             0,  0,      (SECONDARY *) NULL   },
/* 70 */	       { COMMITTED,         0,  0,      (SECONDARY *) NULL   },
/* 71 */	       { ISOLATION,         0,  0,      (SECONDARY *) NULL   },
/* 72 */	       { REPEATABLE,        0,  0,      (SECONDARY *) NULL   },
/* 73 */	       { UNCOMMITTED,       0,  0,      (SECONDARY *) NULL   },
/* 74 */	       { SERIALIZABLE,      0,  0,      (SECONDARY *) NULL   },
						   };

/*
** This table contains the token values for aggregates. The value from
** the token slot in the key_info structure is used to index this structure.
*/

static const AGGTOK	Agg_tok[] = {
/* 0 */	       { ""},
/* 1 */	       { "any"},
/* 2 */	       { "avg"},
/* 3 */	       { "max"},
/* 4 */	       { "min"},
/* 5 */	       { "sum"},
/* 6 */	       { "avgu"},
/* 7 */	       { "sumu"},
/* 8 */	       { "count"},
/* 9 */	       { "countu"},
			};

static i4 
psl_quel_unorm(
PSS_SESBLK *pss_cb,
PSQ_CB	    *psq_cb,
i4	    token);

static bool
psl_dtunorm(
ADF_CB          *adf_cb,
i2		idt);


/*{
** Name: psl_scan	- The scanner for the server's QUEL parser.
**
** Description:
**      This function is the scanner for the server's QUEL parser.  It takes 
**      a pointer to a parser session control block, and looks at the data in 
**      there to get the next token from the query buffer.  Each token can be 
**      returned with a corresponding value.  The tokens are defined by the
**	grammar. 
**
**  Notes:
**      The idea is to make this routine fast but small, to this end a couple
**      tricks have been used.  The next character pointer is load from the
**	control block at the start of the function, it then up to the code to
**	update the control block before exiting.  The name routine assumes that
**	the entry will go in the symbol_table, so it copies and lowercases names
**	as it goes.  The number routines assume that the extra ' ' is at the
**	end of the buffer, so that we can handle weird termination conditions
**	easily.
[@comment_line@]...
**
** Inputs:
**      pss_cb                          Pointer to a parser server control block
**        .pss_nxtchar                   Place where next token begins
**	  .pss_endbuf			  End of query buffer
**	  .pss_prvtok			  Place where last token began
**	  .pss_lineno			  Current line number in query buffer
**	  .pss_symnext			  Next available space in symbol table
**	  .pss_symblk			  Current symbol table block
**	    .pss_symdata		    Beginning of data area
**	    .pss_sbnext			    Pointer to next symbol table block
**	  .pss_symstr			  Memory stream for symbol table blocks
**	  .pss_yacc			  Yacc control block for this thread
**	    .pss_yylval			    Place to put value of next token
**	  .pss_decimal			  The decimal marker character to use
**	  .pss_qualdepth		  Current depth of where clauses (so
**					  we know whether to convert pattern-
**					  matching characters)
**	  .pss_qrydata			  data values for query text
**	  .pss_dval			  current data value number
**	psq_cb				Pointer to control block used to call
**					the parser
**	  .psq_error			  For holding error information
**
** Outputs:
**	pss_cb
**        .pss_nxtchar                   Set to spot after current token
**	  .pss_prvtok			  Set to spot before current token
**	  .pss_lineno			  Incremented for each newline found
**	  .pss_symnext			  Moved to just beyond space used by
**					  current token.
**	  .pss_symblk			  Set to point to new symbol table block
**					  if one is allocated
**	    .pss_sbnext			    Set to NULL if new block allocated
**	  .pss_yacc			  Yacc control block for this thread
**	    .pss_yylval			    Value associated with token put here
**	  .pss_dval			  current data value number
**	Returns:
**	    The token, or 0 if no more tokens, or -1 if error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate more symbol table space.
**	    Can send errors to user or log them.
**
** History:
**	30-jan-86 (jeff)
**          written
**	31-mar-88 (stec)
**	    Cleanup of 2703 error code handling.
**	03-oct-88 (stec)
**	    Complete Kanji related changes.
**	05-oct-88 (stec)
**	    Fix conversion of names to lowercase problem.
**	18-oct-88 (stec)
**	    Fix recognition of a Kanji name too long bug.
**	27-jul-89 (jrb)
**	    Changed scanning of numeric literals to return decimal for a certain
**	    class of numbers.  Also, allowed for backward compatibility with old
**	    literal semantics by checking pss_num_literals flag in session cb.
**	24-jan-91 (andre)
**	    As a part of adding support for QUEL shareable repeat queries, we
**	    need to store the text of the query in a standard format.
**	12-nov-91 (rblumer)
**	  merged from 6.4:  03-jul-91 (rog)
**	    Error 2701 expects an argument, but we weren't passing one. (37928)
**	  merged from 6.4:  25-jul-91 (andre)
**	    NLs inside string constants will be stripped only if the session is
**	    with a pre-64 FE (pss_cb->pss_ses_flag & PSS_STRIP_NL_IN_STRCONST).
**	    This will fix bug 38098
**	  merged from 6.4:  22-jul-91 (rog)
**	    Specify correct number of arguments for error 2700 and 2705. (9877)
**	12-mar-93 (ralph)
**	    DELIM_IDENT:
**	    Translate regular identifiers according to database semantics
**	06-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Change reference to pss_cb->pss_dbxlate to be ptr to u_i4
**	10-aug-93 (andre)
**	    fix causes of compiler warnings
**	17-aug-93 (andre)
**	    uppercase contents of NAME iff CUI_ID_REG_U is set in 
**	    *pss_cb->pss_dbxlate rather than iff 
**	    *pss_cb->pss_dbxlate != CUI_ID_INGRES.  I think it is better to use
**	    the indicator of case of regular identifier than to insist than
**	    unless case translation semantics are INGRES-like, regular 
**	    identifiers must be always uppercased
**	18-aug-93 (andre)
**	    added support for QUEL version of delimited identifiers.  It is my
**	    intent that they be allowed only inside <column list> in 
**	    DEFINE PERMIT statement.  We must allow delimited identifiers there
**          because QEF may need to generate a column list for a QUEL permit 
**	    affected by revocation of column-specific privilege, and some of 
**	    those column names may need to be expressed as delimited identifiers
**	    (as a matter of fact, for now they will all be expressed as 
** 	    delimited identifiers - there is hope, however, that at some point 
**	    we will get our hands on IIUGdlm_ChkdlmBEobjct (I cannot and will 
**	    not claim credit for this one) which will enable us to avoid 
**	    delimiting column names unless absolutely necessary)
**	21-mar-95 (ramra01)
**	    If the II_DECIMAL is set to ',' the query would fail with a
**	    CREATE table with DECIMAL(4,2). The fact that the stmt is a
**	    CREATE is now maintained in the pss_stmt_flags. Any bearing 
**          on the decimal will examine this flag for interpreting a
**	    comma or decimal point. (B67519)
**	03-nov-95 (pchang)
**	    Change to use cui_f_idxlate() which is faster than cui_idxlate(),
**	    when translating regular identifiers.
**	19-mar-96 (pchang)
**	    Fixed bug 70204.  Incorrect test on the next symbol location for
**	    byte alignment prevented a new symbol block to be allocated when
**	    there are exactly 2 bytes left on the current symbol table, and
**	    subsequent alignment operation on the next symbol pointer caused
**	    it to advance beyond the current symbol block and thus, corrupting
**	    adjacent object when the next symbol was assigned.  SEGVIO followed
**	    when the trashed object was later referenced.
**  08-May-97 (merja01)
**      Changed try to ii_try to prevent conflict with definition in
**      Digital Unix 4.0 c_excpt.h.
**	7-jul-99 (hayke02)
**	   Modified the call to psf_error() for errorno 2716 (E_US0A9C)
**	   to include the query line number (pss_cb->pss_lineno). The number
**	   of parameters (num_parms) has been upped from 0 to 1. This change
**	   fixes bug 94238.
**	11-sep-01 (inkdo01)
**	    Attempts to make reserved words context sensitive - also change 
**	    PSS_SYMBLKs to be 256 bytes (rest is wasted).
**	12-jun-02 (toumi01)
**	    Implement quel keyword retry so as to avoid making new keywords
**	    (added for the 'set transaction' statement) into reserved words.
**	3-Feb-2005 (schka24)
**	    Numeric-literal flag holder renamed to parser_compat, fix here.
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMspace macro calls to relaxed bool form
**	04-Feb-2009 (gupsh01)
**	    On UTF8 installations allow for the quel scripts
**	    to contain the leading Byte Order Mark, which is
**	    ordinarily placed on files saved as UTF-8 on
**	    some platforms like windows.
*/
i4
psl_scan(
	register PSS_SESBLK *pss_cb,
	register PSQ_CB	    *psq_cb)
{
    register u_char	*next_char;
    register u_char	*qry_end;
    ULM_RCB		ulm_rcb;
    i4		err_code;
    i4			ret_val = 0;
    u_char		*firstchar;
    PTR			piece;
    i4			toksize;
    i4			dval_num;
    char		*number;
    char		ch;
    YACC_CB		*yacc_cb;
    char		marker;
    DB_DATA_VALUE	*db_val;
    DB_CURSOR_ID	*db_cursid;
    i4		lineno;
    DB_STATUS		status;
    i4			left;
				/*
				** will point to the first non-white character
				** in a token (if any); will skip over any
				** leading comments
				*/
    register u_char	*non_white_char = (u_char *) NULL;
    bool		multi_reswd = FALSE;
    i4			utf8;

    utf8 = (((ADF_CB*) pss_cb->pss_adfcb)->adf_utf8_flag & AD_UTF8_ENABLED);
    next_char	    = pss_cb->pss_nxtchar;
    qry_end	    = pss_cb->pss_endbuf;
    firstchar	    = next_char;
    toksize	    = 0;
    dval_num	    = pss_cb->pss_dval;
    yacc_cb	    = (YACC_CB*) pss_cb->pss_yacc;
    lineno	    = pss_cb->pss_lineno;

#ifdef BYTE_ALIGN
    left = &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE] - pss_cb->pss_symnext;
    /* 
    ** If not aligned and it is not possible to align or
    ** it doesn't make sense to align, allocate symbol memory block
    */
    if ((((PSS_SBSIZE - DB_CNTSIZE - left) % DB_ALIGN_SZ) != 0) &&
	(left <= DB_ALIGN_SZ)
       )
    {
	status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
	if (DB_FAILURE_MACRO(status))
	{
	    return(-1);	/* error */
	}
    }
    /* always start with aligned values */
    pss_cb->pss_symnext =
	(u_char *) ME_ALIGN_MACRO(pss_cb->pss_symnext, DB_ALIGN_SZ);
#endif /* BYTE_ALIGN */

    /* Init reserved word control fields. */
    yacc_cb->yyrestok = 0;
    yacc_cb->yy_rwdstr = NULL;
    yacc_cb->yyreswd = FALSE;

    /*
    ** Remember starting point for previous token, for error reporting and so
    ** we can compensate for the parser's look-ahead when parsing two statements
    ** in succession.
    */
    pss_cb->pss_prvtok = next_char;

    while (next_char <= qry_end)
    {
	/* remember the first non-white character of the token */
	if (non_white_char == (u_char *) NULL && !CMwhite(next_char))
	{
	    non_white_char = next_char;
	}
	
        switch (*next_char)
        {
        case '\t': case '\r':
	    CMnext(next_char);
            continue;

        case '\n':
	    CMnext(next_char);
	    pss_cb->pss_lineno++;
            continue;

	case ',':
	    /*
	    ** Due to internationalization, we must determine whether either
	    ** of the characters is the decimal point.
	    */

	    if (pss_cb->pss_decimal != ',' || next_char >= qry_end ||
		!CMdigit(next_char + 1) || ret_val == NAME ||
               ( pss_cb->pss_decimal == ',' && 
                 pss_cb->pss_stmt_flags & PSS_CREATE_STMT ))
 
	    {
		CMnext(next_char);
		pss_cb->pss_nxtchar = next_char;
		yacc_cb->yylval.psl_tytype = 0;
		ret_val = COMMA;
		goto tokreturn;
	    }
	    /* This comma is part of a real number, fall through to that case */

        case '.':
            if (next_char >= qry_end || !CMdigit(next_char + 1))
            {
                /*  not a number of the form .ddd so must be a '.'  */
		CMnext(next_char);
                pss_cb->pss_nxtchar = next_char;
		yacc_cb->yylval.psl_tytype = 0;
		ret_val = PERIOD;
		goto tokreturn;
            }

        case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
            {
                register u_char		*number = next_char;
                register i4		type;
                i4			c = *number;
		register i4		decimal = pss_cb->pss_decimal;
                i4			int_number;
                f8			double_number;
		i4			tot_digs = 0;
		i4			frac_digs = 0;

		CMnext(next_char);

		/*
		** We're going to scan a numeric literal.  If the user wants
		** the old numeric literal semantics, we only return integer
		** and floating types.  If the user wants the new (INGRES 8.0)
		** numeric literal semantics, we return integer, packed decimal,
		** and floating types.  `type' keeps track of the result type
		** we expect to return (although we never return the type, just
		** the token).
		*/

                /*  pick off the leading digits */
		type = DB_INT_TYPE;

                if (c != decimal)
		{
		    tot_digs++;
		    
		    while (next_char <= qry_end)
		    {
			c = *next_char;
			CMnext(next_char);
			if (!CMdigit(&c))
			    break;
			tot_digs++;
		    }
		}

		/*
                ** See if we need to scan fractional part of floating point
		** number.  According to the user's manual, a floating point
		** number must have a digit after the decimal marker.  However,
		** there have been a couple of versions of INGRES in which this
		** was not enforced.  This causes problems when the decimal
		** marker is a comma: how should we interpret (x=10,y=20)?
		** However, it would probably cause problems for the users to
		** start enforcing this now.  The decision was to require a
		** digit after the decimal only if the marker is a comma.
		** This little section of code will leave next_char pointing
		** to the place after the fractional part of the number, if
		** any.  If there isn't any, it will leave next_char pointing
		** to the place after the integer part of the number.
		*/

                if ( !( (pss_cb->pss_stmt_flags & PSS_CREATE_STMT) &&
                        decimal == ',' ) &&
		    c == decimal &&
		    next_char <= qry_end &&
		    ( (!CMdigit(next_char) && decimal != ',') ||
		     CMdigit(next_char)))
		{
		    type = (pss_cb->pss_parser_compat & PSQ_FLOAT_LITS)  ?
			    DB_FLT_TYPE  :  DB_DEC_TYPE;
		    
                    while (next_char <= qry_end)
		    {
			c = *next_char;
			CMnext(next_char);
			if (!CMdigit(&c))
			    break;
			frac_digs++;
		    }
		    tot_digs += frac_digs;
		}

                /*  if there is an exponent, eat it up  */

                if ((c == 'e' || c == 'E') && next_char <= qry_end)
		{
		    type = DB_FLT_TYPE;
		    c = *next_char;
		    CMnext(next_char);
                    if (CMdigit(&c) || c == '-' || c == '+')
		    {
                        while (next_char <= qry_end)
			{
			    c = *next_char;
			    CMnext(next_char);
			    if (!CMdigit(&c))
				break;
			}
		    }
		}

                /*  zero terminate the number for conversion    */

		if (next_char <= qry_end + 1)
		{
		    CMprev(next_char,number);
		    *next_char = '\0';
		}

                pss_cb->pss_nxtchar = next_char;

                /*  make sure there is room in the symbol table;  actually
		**  max (sizeof(i2), sizeof(i4), sizeof(f8), DB_MAX_DECLEN)
		**  would be sufficient, but let's be on the safe side.
		*/

                if (pss_cb->pss_symnext + DB_MAXNAME + 1 >=
		    &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
                {
		    status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(-1);	/* error */
		    }
		}

                yacc_cb->yylval.psl_strtype = (char *) pss_cb->pss_symnext;

                if (	type == DB_INT_TYPE
		    && CVal((char *) number, &int_number) == 0
		   )
                {
                    /*  syntax says integer and there was no overflow   */

                    *next_char = c;
                    if (int_number >= MINI2 && int_number <= MAXI2)
                    {
                        *(i2 *)pss_cb->pss_symnext = (i2)int_number;
                        pss_cb->pss_symnext += sizeof(i2);
			ret_val = I2CONST;
			toksize = sizeof(i2);
			goto tokreturn;
                    }
                    *(i4 *) pss_cb->pss_symnext = int_number;
                    pss_cb->pss_symnext += sizeof(i4);
		    ret_val = I4CONST;
		    toksize = sizeof(i4);
		    goto tokreturn;
                }

		/* If decimal and not too many digits... */
		/* B87127 allow integer to become decimal, unless
		** II_NUMERIC_LITERAL FLOAT is set, or old non-decimal
		** client
		*/
	        if ((((type == DB_INT_TYPE) && 	
	   	      (pss_cb->pss_parser_compat & PSQ_FLOAT_LITS)==0) ||
	            (type == DB_DEC_TYPE))  &&  tot_digs <= DB_MAX_DECPREC)
		{
		    i2		    ps;

		    /* put precision/scale in first */
		    ps = DB_PS_ENCODE_MACRO(tot_digs, frac_digs);
		    I2ASSIGN_MACRO(ps, *pss_cb->pss_symnext);

		    /* followed by the number itself */
		    _VOID_ CVapk((char *)number, (char)decimal,
			    tot_digs, frac_digs,
			    (PTR)((u_char *)pss_cb->pss_symnext + sizeof(i2)));
		    
		    *next_char = c;
		    ret_val = DECCONST;
		    toksize = DB_PREC_TO_LEN_MACRO(tot_digs) + sizeof(i2);
		    pss_cb->pss_symnext += toksize;
		    goto tokreturn;
		}

		/*
		** Either a floating point number, or a decimal number that was
		** too large, or an integer that was too large
		*/

                if (CVaf((char *) number, decimal, &double_number) != OK)
		{
		    i4	lineno = pss_cb->pss_lineno;

		    _VOID_ psf_error(2707L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2, (i4) sizeof(lineno), &lineno,
			(i4) STlength((char *) number), number);
		    return (-1);
		}
                *next_char = c;
                *(f8 *) pss_cb->pss_symnext = (f8)double_number;
                pss_cb->pss_symnext += sizeof(f8);
		ret_val = F8CONST;
		toksize = sizeof(f8);
		goto tokreturn;
            }

        case '"':
            {
                register u_char	    *string;
                register i4	    length;
                register i4	    esc; 
                register i4	    left;
		u_char		    *start;
                i4		    digct = 0;
		bool		    escdig;

		CMnext(next_char);

                esc = 0;
                start = string =
		 (u_char *) ((DB_TEXT_STRING *) pss_cb->pss_symnext)->db_t_text;
                length =  DB_MAXSTRING;
		left = &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE] - start;

                for (;;)
                {
                    register i4      c;

                    if (next_char > qry_end)
		    {
			_VOID_ psf_error(2700L, 0L, PSF_USERERR,
			    &err_code, &psq_cb->psq_error, 1,
			    (i4) sizeof(pss_cb->pss_lineno),
			    &pss_cb->pss_lineno);
			return (-1);
		    }
                    
                    c = *next_char;

		    /*
		    **  Real conversions of the input string will be done
		    **  by adi_pm_encode() called from pslgram. Here we just
		    **  need to be aware of the rules to be able to detect
		    **  the end of the string (i.e., ending quote). All
		    **  chars between the beginning and ending quote should
		    **  be output.
		    */
                    if (esc)
                    {
                        switch (c)
                        {
			  /* detect '\' followed by up to 3 octal digits */
                          case '0':  case '1':  case '2':  
			  case '3':  case '4':  case '5':  
			  case '6':  case '7':
                            {
				escdig = TRUE;
				digct++;
                                break;
                            }
			  /* All other chars with preceded by a backslash */
			  default:
			    {
				escdig = FALSE;
				break;	
			    }
                        }
			if ((escdig == FALSE) || (digct == 3))
			    esc = 0;
                    }
                    else if (c == '\\')
                    {
                        esc++;
			digct = 0;
			escdig = FALSE;
                    }
                    else if (c == '\n' && 
                             pss_cb->pss_ses_flag & PSS_STRIP_NL_IN_STRCONST)
                    {
			/*
			** only strip the NL if connected to an older FE;
			** otherwise it is considered a part of the string
			*/
			pss_cb->pss_lineno++;
			CMnext(next_char);
                        continue;
                    }
                    else if (c == '"')
		    {
			CMnext(next_char);
                        break;
		    }
                    
		    CMbytedec(length, next_char);
                    if (length < 0)
		    {
			pss_cb->pss_lineno += lineno;
			_VOID_ psf_error(2701L, 0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 1,
			    (i4) sizeof (pss_cb->pss_lineno), 
			    &pss_cb->pss_lineno);
			return (-1);
		    }

		    CMbytedec(left, next_char);
                    if (left < 0)
                    {
			PTR	new_block;

			status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
			if (DB_FAILURE_MACRO(status))
			{
			    return(-1);	/* error */
			}

			/*
			** We need to check whether there is enough memory
			** in this new block to avoid memory corruption.
			*/
			if ((PSS_SBSIZE - DB_CNTSIZE) < (DB_MAXSTRING - length))
			{
			    _VOID_ psf_error(E_PS0376_INSUF_MEM, 0L, PSF_INTERR,
				&err_code, &psq_cb->psq_error, 0);
			    return (-1);
    			}

			new_block = 
			    (PTR) ((DB_TEXT_STRING *) pss_cb->pss_symnext)->
				db_t_text;

			/* Copy the string (read so far) into the new block */
			MEcopy((PTR) start, DB_MAXSTRING - length,
			    new_block);

			start = (u_char *) new_block;
			   
			string = start + (DB_MAXSTRING - length - 1);
			left = &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE]
			    - string;
		    }

		    CMcpyinc(next_char, string);
                }

                ((DB_TEXT_STRING *) pss_cb->pss_symnext)->db_t_count =
		    DB_MAXSTRING - length;
		pss_cb->pss_nxtchar = next_char;
                yacc_cb->yylval.psl_textype =
		    (DB_TEXT_STRING *) pss_cb->pss_symnext;
                pss_cb->pss_symnext = string;
		ret_val = SCONST;
		toksize = DB_MAXSTRING - length + DB_CNTSIZE;
		goto tokreturn;
            }

        case '\'':
            {
		register u_char *word = pss_cb->pss_symnext;
		u_i4	 	src_len = qry_end - next_char;
		u_i4     	dst_len = DB_MAXNAME;
		u_char		*lim;
		u_i4	quel_delim_id_semantics;

		/*
		** make sure there is room in the symbol table.
		** in case of delimited identifier body being too long we need 3
		** extra chars: 2 for Kanji char and 1 for terminating null.
		*/

		lim = word + DB_MAXNAME + 3 ;

		if (lim >= &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
		{
		    status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(-1);	/* error */
		    }

		    word = pss_cb->pss_symnext;
		}

		/*
		** cui_idxlate() treats "ed strings as delimited identifiers and
		** 'ed strings as 'quoted identifier' which makes perfect sense
		** in SQL but in QUEL, where the delimited identifier is 
		** surrounded with single quotes, we are forced to translate 
		** case translation semantics for delimited identiers into 
		** corresponding semantics for 'quoted identifiers'
		*/
		if (*pss_cb->pss_dbxlate & CUI_ID_DLM_M)
		    quel_delim_id_semantics = CUI_ID_QUO_M;
		else if (*pss_cb->pss_dbxlate & CUI_ID_DLM_U)
		    quel_delim_id_semantics = CUI_ID_QUO_U;
		else 
		    /* default behaviour - lowercase */
		    quel_delim_id_semantics = CUI_ID_QUO_L;
		
		quel_delim_id_semantics |= CUI_ID_STRIP;

		/*
		** Invoke cui_idxlate() to translate the identifier
		*/

		status = cui_idxlate(next_char, &src_len, word, &dst_len, 
		    quel_delim_id_semantics, (u_i4 *) NULL, 
		    &psq_cb->psq_error);
		if (DB_FAILURE_MACRO(status))
		{
		    psf_error(psq_cb->psq_error.err_code, 0L, PSF_USERERR,
				&err_code, &psq_cb->psq_error, 2,
				sizeof(ERx("Delimited identifier"))-1,
				ERx("Delimited identifier"),
				src_len, next_char);
		    return(-1);
		}
		
		next_char += src_len;
		word += dst_len;

		/* update next character pointer */
		pss_cb->pss_nxtchar = next_char;
		*(pss_cb->pss_symnext + dst_len) = EOS;
		yacc_cb->yylval.psl_strtype = (char *) pss_cb->pss_symnext;
		pss_cb->pss_symnext += dst_len + 1;
		ret_val = DELIM_IDENT;
		toksize = dst_len + 1;
		goto tokreturn;
            }

        case '(':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = 0;
	    ret_val = LPAREN;
	    goto tokreturn;

        case ')':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = 0;
	    ret_val = RPAREN;
	    goto tokreturn;

        case ':':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = 0;
	    ret_val = COLON;
	    goto tokreturn;

        case '$':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = 0;
	    ret_val = DOLLAR;
	    goto tokreturn;

        case '+':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = ADI_ADD_OP;
	    ret_val = UAOP;
	    goto tokreturn;

        case '-':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = ADI_SUB_OP;
	    ret_val = UAOP;
	    goto tokreturn;
        
        case '=':
	    CMnext(next_char);
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = ADI_EQ_OP;
	    ret_val = ISOP;
	    goto tokreturn;

        case '!':
	    {
		char	buf[3];
		i4	error = 0;

		buf[0] = *next_char;
		buf[1] = ' ';
		buf[2] = '\0';

		CMnext(next_char);

		if (next_char > qry_end)
		{
		    error++;
		}
		else if (*next_char != '=')
		{
		    buf[1] = *next_char;
		    error++;
		}

		if (error)
		{
		    pss_cb->pss_lineno += lineno;
		    _VOID_ psf_error(2702L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2, 
			(i4) sizeof (pss_cb->pss_lineno),
			&pss_cb->pss_lineno, (i4) STtrmwhite(buf), buf);
		    return (-1);
		}

		CMnext(next_char);
		pss_cb->pss_nxtchar = next_char;
		yacc_cb->yylval.psl_tytype = ADI_NE_OP;
		ret_val = EOP;
		goto tokreturn;
	    }        

        case '*':
	    CMnext(next_char);
            if (next_char <= qry_end && *next_char == '*')
            {
                pss_cb->pss_nxtchar = next_char + 1;
                yacc_cb->yylval.psl_tytype =
		    ADI_POW_OP;
		ret_val = BAOPH;
		goto tokreturn;
            }
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = ADI_MUL_OP;
	    ret_val = BAOP;
	    goto tokreturn;

        case '<':
	    CMnext(next_char);
	    
	    /* allow use of <> as a synonym for != */
	    if (next_char <= qry_end && !CMcmpcase(next_char, ">"))
	    {
		CMnext(next_char);
		yacc_cb->yylval.psl_tytype = ADI_NE_OP;
		ret_val = EOP;
	    }
            else if (next_char <= qry_end && !CMcmpcase(next_char, "="))
            {
                CMnext(next_char);
                yacc_cb->yylval.psl_tytype = ADI_LE_OP;
		ret_val = BDOP;
            }
            else
	    {
                yacc_cb->yylval.psl_tytype = ADI_LT_OP;
		ret_val = BDOP;
	    }
	    pss_cb->pss_nxtchar = next_char;
	    goto tokreturn;
        
        case '>':
	    CMnext(next_char);
            if (next_char <= qry_end && *next_char == '=')
            {
                CMnext(next_char);
                yacc_cb->yylval.psl_tytype = ADI_GE_OP;
            }
            else
	    {
                yacc_cb->yylval.psl_tytype = ADI_GT_OP;
	    }
            pss_cb->pss_nxtchar = next_char;
	    ret_val = BDOP;
	    goto tokreturn;

        case '/':
	    CMnext(next_char);
            if (next_char <= qry_end && *next_char == '*')
            {
                register i4 endcomment = 0;

		CMnext(next_char);
                while (next_char <= qry_end)
                {
                    if (*next_char == '*' &&
			(next_char + 1) <= qry_end && *(next_char + 1) == '/')
                    {
                        CMnext(next_char);
                        CMnext(next_char);
                        endcomment++;
                        break;
                    }
                    else if (*next_char == '\n')
                    {
                        pss_cb->pss_lineno++;
                    }
		    CMnext(next_char);
                }
                if (endcomment == 0)
		{
		    _VOID_ psf_error(2705L, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			(i4) sizeof(pss_cb->pss_lineno),
			&pss_cb->pss_lineno);
		    return (-1);
		}

		/*
		** non_white_char is used for storing text of repeat queries;
		** we don't have to store the comments
		*/
		non_white_char = (u_char *) NULL;

                continue;
            }
            pss_cb->pss_nxtchar = next_char;
            yacc_cb->yylval.psl_tytype = ADI_DIV_OP;
	    ret_val = BAOP;
	    goto tokreturn;

	case PSQ_HV:
    	    {
		/* this character must be preceded by a space and followed by
		** an optional number and then a space.
		*/
		char	buf[3];
		bool	leave_loop = TRUE;

		err_code = 0;
		buf[0] = *next_char;
		CMnext(next_char);
		buf[1] = *next_char;
		buf[2] = '\0';

		do
		{
		    /* check previous character */
		    if (*(next_char - 2) != ' ')
		    {
			err_code = 1;
			break;
		    }
		    /* get the special marker */
		    if (*next_char != PSQ_HVD && *next_char != PSQ_HVQ)
		    {
			err_code = 1;
			break;
		    }
		    marker = *next_char;
		    CMnext(next_char);

		    /* check for number */
		    if (CMdigit(next_char))
		    {
			number  = (char*) next_char;
			CMnext(next_char);
			while(next_char <= qry_end && CMdigit(next_char))
			{
			    CMnext(next_char);
			}

			/* Determine the number */
			ch = *next_char;
			*next_char = '\0';
			CVal((char*) number, &dval_num);
			*next_char	    = ch; 
		    }
		    if (next_char > qry_end)
		    {
			err_code = 1;
			break;
		    }
		    /* check for trailing blank */
		    if (!CMspace(next_char))
		    {
			err_code = 1;
			break;
		    }
		    CMnext(next_char);
		    pss_cb->pss_nxtchar	= next_char;
		    
		    /* leave_loop has already been set to TRUE */
		} while (!leave_loop);

		if (err_code)
		{
		    pss_cb->pss_lineno += lineno;
		    _VOID_ psf_error(2702L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2, 
			(i4) sizeof (pss_cb->pss_lineno),
			&pss_cb->pss_lineno, (i4) STtrmwhite(buf), buf);
		    return (-1);
		}
		else
		{
		    if 
		    (
			dval_num >= pss_cb->pss_dmax
			||
			(
			    marker == PSQ_HVQ
			    &&
			    dval_num + 2 >= pss_cb->pss_dmax
			)
		    )
		    {
			_VOID_ psf_error(2716L, 0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 1,
			    (i4) sizeof(pss_cb->pss_lineno),
			    &(pss_cb->pss_lineno));
			return (-1);
		    }

		    /* current data value */
		    db_val = pss_cb->pss_qrydata[dval_num];

		    toksize = (marker == PSQ_HVQ) ? sizeof (DB_CURSOR_ID)
						  : sizeof (DB_DATA_VALUE);

		    /*  make sure there is room in the symbol table */

		    if (pss_cb->pss_symnext + toksize >=
			&pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
		    {
			status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
			if (DB_FAILURE_MACRO(status))
			{
			    return(-1);	/* error */
			}
		    }


		    /* copy the value into the symbol table. when we
		    ** fix text emission, we might not need to do this and
		    ** could just pass a pointer to the value.
		    */
		    yacc_cb->yylval.psl_dbval =
			(DB_DATA_VALUE *) pss_cb->pss_symnext;

		    if (marker == PSQ_HVD)
		    {
			STRUCT_ASSIGN_MACRO(*db_val,
			    *(DB_DATA_VALUE*) pss_cb->pss_symnext);
			dval_num++;
			ret_val = QDATA;
		    }
		    else
		    {
			db_cursid = (DB_CURSOR_ID*) pss_cb->pss_symnext;
			STRUCT_ASSIGN_MACRO(*(i4*)db_val->db_data,
			    db_cursid->db_cursor_id[0]);
			db_val = pss_cb->pss_qrydata[++dval_num];
			STRUCT_ASSIGN_MACRO(*(i4*)db_val->db_data,
			    db_cursid->db_cursor_id[1]);
			db_val = pss_cb->pss_qrydata[++dval_num];
			MEmove(db_val->db_length, db_val->db_data, ' ',
			    DB_MAXNAME, (PTR) db_cursid->db_cur_name);
			dval_num++;
			ret_val = QUERYID;
		    }
		    pss_cb->pss_dval    = dval_num;
		    pss_cb->pss_symnext += toksize;
		    /* put value into symbol table and return token */
		    goto tokreturn;
		}
	    }			
        }   /* switch */

	if (CMspace(next_char))
	{
	    CMnext(next_char);
	    continue;
	}

	if (CMnmstart(next_char) || *next_char == '_')
	{
	    register u_char  *word = pss_cb->pss_symnext;
	    register i4      length;
	    register u_char  *key, *mark;
	    register i4      keylen;

	    /*  make sure there is room in the symbol table.
	    **  in case of name too long we need 3 extra chars
	    **  2 for Kanji char and 1 for terminating null.
	    */

	    if (word + DB_MAXNAME + 3 >=
		&pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
	    {
		status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
		if (DB_FAILURE_MACRO(status))
		{
		    return(-1);	/* error */
		}

		word = pss_cb->pss_symnext;
	    }

	    /*  start entering the name into the symbol table   */

	    (VOID)CMtolower(next_char, word);
	    length = CMbytecnt(next_char);
	    CMnext(next_char);
	    CMnext(word);
	    mark = word;
	    while (next_char <= qry_end)
	    {
		/* if a good character and not a '$' in a 5.0 repeat
		** query.
		*/
		if (CMnmchar(next_char) &&
		    !(pss_cb->pss_defqry == PSQ_50DEFQRY &&
		    CMcmpcase(next_char, "$") == 0 )
		)
		{
		    length += CMbytecnt(next_char);
		    CMtolower(next_char, word);
		    CMnext(next_char);
		    CMnext(word);
		}
		else
		    break;

		if (length > DB_MAXNAME)
		{
		    /* Name too long */
		    i4	    lineno = pss_cb->pss_lineno;

		    *word = '\0';
		    _VOID_ psf_error(2703L, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			(i4) sizeof(lineno), &lineno,
		    (i4) STlength((char *) pss_cb->pss_symnext),
			pss_cb->pss_symnext);
		    return (-1);
		}
		mark = word;
	    }
	    /* update next character pointer */
	    pss_cb->pss_nxtchar = next_char;

	    /*
	    **  special anchor value for search, at
	    **  last legal char in word
	    */
	    *mark = -1;

	    /* return to the beginning of the word */
	    word = pss_cb->pss_symnext;

	    /*
	    ** if we are storing away the text of a QUEL repeat query take 
	    ** advantage of the fact that word points at a lower-cased token
	    ** (or a first part of a double keyword) and store it now
	    */

	    /*
	    ** set non_white_char to NULL to prevent storing the same piece of
	    ** text twice
	    */
	    non_white_char = (u_char *) NULL;

	    if (pss_cb->pss_stmt_flags & PSS_QUEL_RPTQRY_TEXT
		&&
		psq_rptqry_text(pss_cb->pss_tchain, word, length, &piece,
				&psq_cb->psq_error) != E_DB_OK)
	    {
		return (-1);
	    }

	    /*  Check to see if this is a keyword   */
	    if (length < (sizeof(Key_index) / sizeof(Key_index[0])))
	    {
		/* find starting position by length */
		key = (u_char *)Key_index[length];
	    }
	    else
	    {
		/*  Note Key_index is not filled out to DB_MAXNAME entries */
	        key = "\0";
	    }

	    /*  While there are more keywords of the same length    */

	    while (*key++)
	    {
		for (keylen = 0; ; keylen++)
		    if (key[keylen] != word[keylen])
			if (keylen == length)
			{
			 register KEYINFO   *tret = (KEYINFO*)&Key_info[*--key];
			 register u_char    *this_char;
			 register u_char    *word2;
			 register i4	    ii_try;
			 register i4	    low;
			 register i4	    high;
			 u_char		    savechar;
			 i4		    lines = 0;
			 SECONDARY	    *second;
			 i4		    compval;

			 /* If there could be a 2nd word, look for it */
			 if ((second = tret->secondary) !=
			    (SECONDARY *) NULL)
			 {
			  for (this_char = next_char;
			    (*this_char == '\t' || *this_char == '\r' ||
				CMspace(this_char) || *this_char == '\n')
				&& this_char <= qry_end;
			    CMnext(this_char))
			  {
			   if (*this_char == '\n')
			    lines++;
			  }

			  /*
			  ** If there is no second word at all, it could
			  ** be a keyword or a name.  This depends on
			  ** whether the Key_info[] structure has a token
			  ** entry for this word.  If not, it can't be a
			  ** keyword all by itself; it depends on the
			  ** following word to make it part of a two-
			  ** word keyword.
			  */
			  if (this_char > qry_end ||
			    !CMalpha(this_char))
			  {
			   if (tret->token)
			    goto rettok;
			   else
			    goto retname;
			  }
			  word2 = this_char;
			  CMnext(this_char);

			  /*
			  ** Now find the end of the word.  This part
			  ** depends on there being a blank at the end
			  ** of the query buffer.  If there isn't, it
			  ** could run off the end.  This is an optimization
			  ** that should be documented externally.
			  */
			  for ( ; CMalpha(this_char) || *this_char == '_'
				|| CMdigit(this_char); CMnext(this_char))
			     ;  /* NULL BODY */

			  /* Null-terminate the second word. */
			  if ((savechar = *this_char) == '\n')
			   lines++;
			  *this_char = '\0';

			  /*
			  ** Binary search for the second word among the
			  ** set of possible second words.
			  */
			  low = 0;
			  high = tret->num2ary - 1;
			  while (high >= low)
			  {
			   ii_try = (high + low) / 2;
			   /*
			   ** Normally, we try not to use function calls
			   ** to do comparisons within the scanner.  This
			   ** particular comparison isn't critical, however,
			   ** so we'll take the easy way out.
			   */
			   if (!(compval = STcasecmp((char *) word2, 
			    second[ii_try].word2)))
			   {
			    break;
			   }
			   else if (compval > 0)
			   {
			    low = ii_try + 1;
			   }
			   else
			   {
			    high = ii_try - 1;
			   }
			  }

			  if (high >= low)	/* TRUE means found 2nd word */
			  {
			   multi_reswd = TRUE;
			   pss_cb->pss_nxtchar = this_char;
			   *this_char = savechar;
			   pss_cb->pss_lineno += lines;
			   yacc_cb->yylval.psl_tytype = second[ii_try].val2;
			   ret_val = second[ii_try].token2;

			   /*
			   ** if we are storing away the text of a QUEL repeat
			   ** query, lowercase the second part of a double
			   ** keyword, and add it to the text chain
			   */
			   if (pss_cb->pss_stmt_flags & PSS_QUEL_RPTQRY_TEXT)
			   {
			     u_char	buf[DB_MAXNAME], *bufp = buf,
					*src = word2;

			     for (; src < this_char; CMnext(src), CMnext(bufp))
			     {
				CMtolower(src, bufp);
			     }

			     if (psq_rptqry_text(pss_cb->pss_tchain, buf,
			         bufp - buf, &piece, &psq_cb->psq_error)
				 != E_DB_OK)
			     {
			       return (-1);
			     }
			   }

			   goto tokreturn;
			  }

			  /*
			  ** 2nd word not found, so the first word is
			  ** either a token or a name.  Which it is depends
			  ** on whether there is a token value in the
			  ** Key_info entry.  If so, it is a token.  If
			  ** not, it's a name.
			  */

			  *this_char = savechar;
			  if (tret->token)
			   goto rettok;
			  else
			   goto retname;
			 }

			if (tret->token == 0)
			{
			    goto retname;
			}

			rettok:
		    if (tret->token == AGOP)
		    {
			yacc_cb->yylval.psl_strtype = 
				Agg_tok[tret->tokval].stoken;
		    }
		    else
		    {
			yacc_cb->yylval.psl_tytype =
				tret->tokval;
		    }
			    ret_val = tret->token;
			/* If this is a simple token, enter dual keyword/ident
			** logic. Otherwise, treat it as a token only. */
			if ( tret->token == LBOP || tret->token == AGOP
				|| multi_reswd )
			    goto tokreturn;
			else goto reswdident;
			}
			else
			    break;
		key += length; key++;       /* try next keyword */
	    }

	/* If it drops off loops, must be IDENT - branch to NAME logic. */
	    goto retname;

	/* Potential dual keyword/ident. Flag it as such and save keyword as if
	** it's a name, to be recycled if necessary in parser. */
	reswdident:
	    yacc_cb->yyreswd = TRUE;
	    yacc_cb->yy_rwdstr = (char *)word;
	    /* Now drop into NAME processing for upper translation, etc. */

	/* Not a keyword, update symbol_table pointer and terminate the word */
	retname:
	    word[length] = EOS;

	    /*
	    ** If the regular identifiers must be uppercased, do so now
	    */
	    if (*pss_cb->pss_dbxlate & CUI_ID_REG_U)
	    {
		if ( cui_f_idxlate(
			(char *)word, length, *pss_cb->pss_dbxlate) == FALSE )
		{
		    status = cui_idxlate(word, (u_i4 *)NULL, (u_char *)NULL,
					(u_i4 *)NULL, *pss_cb->pss_dbxlate,
					(u_i4 *)NULL, &psq_cb->psq_error);
 
		    if (DB_FAILURE_MACRO(status))
		    {
			if (yacc_cb->yyreswd)
			{
			    /* Turn it back into simple keyword token. */
			    yacc_cb->yyreswd = FALSE;
			    goto tokreturn;
			}
			psf_error(psq_cb->psq_error.err_code, 0L, PSF_USERERR,
					&err_code, &psq_cb->psq_error, 2,
					sizeof(ERx("Identifier"))-1,
					ERx("Identifier"),
					length, word);
			return(-1);
		    }
		}
	    }

	    pss_cb->pss_symnext += length + 1;
	    yacc_cb->yylval.psl_strtype = (char *)word;
	    if (yacc_cb->yyreswd == FALSE)
		ret_val = NAME;
	    toksize = length + 1;
	    goto tokreturn;
	}

	/* For Unicode (UTF8 installations) The QUEL script is allowed to
        ** have the Unicode character 'Zero Width No-Break Space or U+FEFF'
        ** ignore it.
        */
        if (utf8 )
        {
          if ( ((next_char + 3) <= qry_end) &&
                (CM_ISUTF8_BOM(next_char)) )
          {
                CMnext(next_char);
                continue;
          }
        }

	pss_cb->pss_lineno += lineno;
	_VOID_ psf_error(2708L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 2,
	    (i4) sizeof(pss_cb->pss_lineno), &pss_cb->pss_lineno,
	    (i4) sizeof(char), next_char);
	return (-1);

    }	/* scan while */
    /* restore the lineno to that which existed on entry because
    ** we found no new tokens.
    */
    pss_cb->pss_lineno = lineno;
    yacc_cb->yylval.psl_tytype = PSL_GOVAL;
    ret_val = 0;

tokreturn:
    /*
    ** If we are emitting text, and the we are storing the token value in
    ** the symbol table, also put a pointer to the piece of text directly
    ** after the symbol value in the symbol table.  This will enable us to
    ** substitute for these pieces of text while in the parser.
    */
    if ( (pss_cb->pss_stmt_flags & PSS_TXTEMIT) && toksize != 0)
    {
	/* Make sure there's room for piece pointer */
	if (pss_cb->pss_symnext + sizeof(piece) >=
	    &pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
	{
	    status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(-1);	/* error */
	    }

	    pss_cb->pss_symnext += toksize;

	    /*
	    ** Note that when toksize is != 0, yacc_cb->yylval contains a
	    ** address of the value of the current token.  Therefore, in order
	    ** to copy value of a token from the old symbol block to the new one
	    ** we need to copy toksize bytes starting with the address found in
	    ** yacc_cb->yylval and not starting with address of yacc_cb->yylval.
	    **
	    ** This fixes bug 33417 and maybe some others.  (2-nov-90 (andre))
	    */

	    /* Copy token value to new block */
	    MEcopy((PTR) yacc_cb->yylval.psl_strtype, toksize,
	           (PTR) pss_cb->pss_symblk->pss_symdata);
	    yacc_cb->yylval.psl_strtype = 
		(char *) pss_cb->pss_symblk->pss_symdata;
	}
    }

    if (pss_cb->pss_stmt_flags & PSS_TXTEMIT)
    {
	/* Add the current token text to the chain */
	if (ret_val != QDATA)
	{
	    if (psq_tadd(pss_cb->pss_tchain, firstchar,
		pss_cb->pss_nxtchar - firstchar, &piece, &psq_cb->psq_error)
		    != E_DB_OK)
	    {
		return (-1);
	    }
	}
	else
	{
	    if (psq_tcnvt(pss_cb, pss_cb->pss_tchain, 
		(DB_DATA_VALUE*) yacc_cb->yylval.psl_dbval,
		&piece, &psq_cb->psq_error)
		    != E_DB_OK)
	    {
		return (-1);
	    }
	}

	/*
	** If the current token has a value in the symbol table, store the
	** piece pointer there, too.
	*/
	if (toksize != 0)
	{
#ifdef    BYTE_ALIGN
	    MEcopy((char *) &piece, sizeof(piece),
		(char *) pss_cb->pss_symnext);
#else
	    *((PTR *) pss_cb->pss_symnext) = piece;
#endif
	    pss_cb->pss_symnext += sizeof(piece);
	}
    }

    /*
    ** if we are storing away the text of a QUEL repeat query and if there was
    ** text to be stored (note that text of keywords and NAMEs has already been
    ** lowercased and stored, so non_white_char will be set to NULL), try to
    ** store it
    */
    if (non_white_char != (u_char *) NULL
        &&
	pss_cb->pss_stmt_flags & PSS_QUEL_RPTQRY_TEXT
	&&
	psq_rptqry_text(pss_cb->pss_tchain, non_white_char,
			pss_cb->pss_nxtchar - non_white_char, &piece,
			&psq_cb->psq_error) != E_DB_OK)
    {
	return (-1);
    }
    if (ret_val == CREATE)
       pss_cb->pss_stmt_flags |= PSS_CREATE_STMT;

    if (ret_val == QDATA || (utf8 && ret_val == SCONST))
    {
	/*
	** pss_yylval set to Value associated with token
	** pss_symnext set to Next available space in symbol table
	** Normalize the token (may change pss_yylval, pss_symnext
	*/ 
	ret_val = psl_quel_unorm(pss_cb, psq_cb, ret_val);
	if (ret_val == -1)
	    return (-1);
    }

    return (ret_val);
}


/*
** Name: psl_quel_unorm
**
** Description: Normalize token if necessary
**
** Inputs:
**	pss_cb		Parser session state control block
**	psq_cb		Query parsing state control block
**      token
**      len
**
** Outputs:
**	len             token len may change
** 
** Side Effects:
**	pss_yylval      updated to point to normalized token
**
** History:
**	10-sep-2008 (gupsh01,stial01)
**          Created from psl_unorm (minus UCONST which is not defined in quel.
**	28-May-2010 (gupsh01)
**	    For extra long strings, truncate to the maximum length
**	    length allowed.
*/
static i4 
psl_quel_unorm(
PSS_SESBLK *pss_cb,
PSQ_CB	    *psq_cb,
i4	    token)
{
    DB_NVCHR_STRING	*nvchr;
    DB_TEXT_STRING	*text;
    char		*name;
    DB_NVCHR_STRING	*norm_nvchr;
    DB_TEXT_STRING	*norm_text;
    char		*norm_name;
    DB_DATA_VALUE	dv1;
    DB_DATA_VALUE	rdv;
    DB_DATA_VALUE	*pop[2];
    ADI_LENSPEC		lenspec;
    DB_STATUS		status;
    YACC_CB		*yacc_cb;
    ADF_CB		*adf_cb;
    i4			reserve;
    DB_DATA_VALUE	*qdv;
    DB_DATA_VALUE	*n_qdv;
    i4			qnull;
    i4			input_str_len;
    char		*input_str;
    i4			norm_str_len;
    i4			norm_val_len;
    char		*tmp_symnext;
    char 		*save_symnext;
    i4			namelen;
    i4			size = 0;
    i4			val1, val2;
    i4			error;
    i4			maxlen;
    i4			utf8;

    save_symnext = pss_cb->pss_symnext;
    adf_cb = (ADF_CB*) pss_cb->pss_adfcb;
    yacc_cb = pss_cb->pss_yacc;
    MEfill(sizeof(ADI_LENSPEC), (u_char)0, (PTR)&lenspec);
    lenspec.adi_lncompute = ADI_O1UNORM;
    pop[0] = &dv1;

    if (token == SCONST)
    {
	text = yacc_cb->yylval.psl_textype;
	dv1.db_datatype = DB_VCH_TYPE;
	dv1.db_length = text->db_t_count + DB_CNTSIZE;
	dv1.db_data = (PTR)text;
	rdv.db_datatype = DB_VCH_TYPE;

	utf8 = (((ADF_CB*) pss_cb->pss_adfcb)->adf_utf8_flag & AD_UTF8_ENABLED);
	if (utf8)
	    maxlen = DB_UTF8_MAXSTRING; 
        else
	    maxlen = DB_MAXSTRING; 

        if (dv1.db_length > (maxlen + DB_CNTSIZE))
	{
	    if (adf_cb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
            {
               int lineno = pss_cb->pss_lineno;
               _VOID_ psf_error(9412L, 0L, PSF_USERERR, &error,
                               &psq_cb->psq_error, 2, (i4) sizeof(lineno), &lineno,
                               (i4) sizeof(maxlen),  &maxlen);

	       if (adf_cb->adf_strtrunc_opt == ADF_ERR_STRTRUNC)
                  return (-1);
            }
	    /* truncate the length to max allowed length */
	    dv1.db_length = maxlen;
	    ((DB_TEXT_STRING *)dv1.db_data)->db_t_count = maxlen;
  	}
    }
    else if (token == QDATA)
    {
	/* check if we need to unorm QDATA */
	qdv = yacc_cb->yylval.psl_dbval;
	STRUCT_ASSIGN_MACRO(*qdv, dv1);

	if (!psl_dtunorm(adf_cb, qdv->db_datatype))
	    return (token);
	
	/* don't normalize NULL QDATA */
	if (qdv->db_datatype < 0)
	{
	    status = adc_isnull(adf_cb, qdv, &qnull);
	    if (qnull)
		return (token);

	    /* Unorm routines don't take nullable datatypes */
	    dv1.db_datatype = abs(dv1.db_datatype);
	    dv1.db_length--;
	}

	/* Always unorm into VCH/NVCH so that we can trim unused expand bytes */
	if (dv1.db_datatype == DB_NVCHR_TYPE || dv1.db_datatype == DB_NCHR_TYPE)
	    rdv.db_datatype = DB_NVCHR_TYPE;
	else
	    rdv.db_datatype = DB_VCH_TYPE;
    }
    else if (token == NAME)
    {
        /* FIX ME tricky this is null terminated string */
        name = yacc_cb->yylval.psl_strtype;
	dv1.db_datatype = DB_CHR_TYPE;
	/* For NAME tokens, don't normalize the null, just add it back */
	namelen = STlength(name);
	dv1.db_length = namelen;
	dv1.db_data = (PTR)name;
	rdv.db_datatype = DB_VCH_TYPE;
    }
    /* FIX ME what about delim identifiers */

    if (dv1.db_datatype == DB_CHA_TYPE || dv1.db_datatype == DB_CHR_TYPE
		|| dv1.db_datatype == DB_NCHR_TYPE)
    {
	input_str_len = dv1.db_length;
	input_str = (char *)dv1.db_data;
    }
    else
    {
	input_str_len = dv1.db_length - DB_CNTSIZE;
	input_str = (char *)dv1.db_data + DB_CNTSIZE;
    }

    /* get maximum length for normalized token */
    status = adi_0calclen(adf_cb, &lenspec, 1, &pop[0], &rdv);
    if (status)
    {
	TRdisplay("psl_unorm adi_0calclen error \n");
	/* set result length to max we can handle anyway */
	rdv.db_length = DB_MAXSTRING + DB_CNTSIZE;
    }

#ifdef xDEBUG
TRdisplay("psl_unorm type %d len %d res len %d\n", dv1.db_datatype,
dv1.db_length, rdv.db_length);
#endif

    /* Make sure there is room in the symbol table for rdv.db_length */
    /* FIX ME note PSS_SBSIZE is DB_MAXSTRING + DB_CNTSIZE is this enough */
    /* Or should we use a temp buf maxstring *n , and then just make sure */
    /* there is space for actual normalized len */
    /* and if no expansion... just copy normalized over orig */
    /* IF it MEcmp finds dif */

    reserve = rdv.db_length;
    if (token == NAME || (token == QDATA && qdv->db_datatype < 0))
	reserve += 1; /* for the null or null indicator */

    if (token == QDATA)
	reserve += sizeof(DB_DATA_VALUE);

    /* ALWAYS align data value */
    reserve += sizeof(ALIGN_RESTRICT) - 1;

    if (pss_cb->pss_symnext + reserve >=
	&pss_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
    {
	status = psf_symall(pss_cb, psq_cb, PSS_SBSIZE);
	if (DB_FAILURE_MACRO(status))
	{
	    return(-1);	/* error */
	}
    }

    /* ALWAYS align data value */
    tmp_symnext = pss_cb->pss_symnext;
    tmp_symnext = (u_char *) ME_ALIGN_MACRO(tmp_symnext, DB_ALIGN_SZ);

    if (token == QDATA)
    {
	n_qdv = (DB_DATA_VALUE *)tmp_symnext;
	STRUCT_ASSIGN_MACRO(*qdv, *n_qdv);
	tmp_symnext += sizeof(DB_DATA_VALUE);
    }

    rdv.db_data = (PTR)tmp_symnext;

    /* Do the unorm */
    if (rdv.db_datatype == DB_NVCHR_TYPE)
	status = adu_unorm(adf_cb, &dv1, &rdv);
    else
	status = adu_utf8_unorm(adf_cb, &dv1, &rdv);
    if (status)
    {
	status = psl_unorm_error(pss_cb, psq_cb, &rdv, &dv1, status);
	if (status)
	    return (-1);
    }

    /*
    ** rdv.db_length assumes maximum unorm expansion, get actual bytes used
    ** and advance pss_symnext only for for spaced used by unorm
    */
    if (rdv.db_datatype == DB_NVCHR_TYPE)
    {
	norm_nvchr = (DB_NVCHR_STRING *)rdv.db_data;
	norm_str_len = norm_nvchr->count * sizeof(UCS2);
    }
    else /* result is alway VCH, which we may have to extract string */
    {
	norm_text = (DB_TEXT_STRING *)rdv.db_data;
	norm_str_len = norm_text->db_t_count;
    }

    /* Trim the unorm expansion bytes from normalized dv */
    if (dv1.db_datatype == DB_CHA_TYPE || dv1.db_datatype == DB_CHR_TYPE
		|| dv1.db_datatype == DB_NCHR_TYPE)
	norm_val_len = norm_str_len;
    
    else
	norm_val_len = norm_str_len + DB_CNTSIZE;

    /* Optimization... if unormalized-normalized match, don't advance 
    ** pss_symnext or reset yylval,len
    */
    if (norm_str_len == input_str_len &&
	!MEcmp(input_str, (char *)rdv.db_data + DB_CNTSIZE, norm_str_len))
    {
	/* Move back to input token, caller will move the pss_symnext */
	/* for now always advance so we can test this code
	pss_cb->pss_symnext = save_symnext;
	return (token);
	*/
    }
    else if (ult_check_macro(&pss_cb->pss_trace, 19, &val1, &val2))
    {
	i4	t1 = dv1.db_datatype;
	i4	t2 = rdv.db_datatype;

	/*
	** set trace point ps147
	** Normalization can make the string smaller..
	** But it it more likely to happen if host data had a null.
	** Don't print diagnostic if norm_str_len < input_str_len	
	** If you want to see the query with this msg... 
	** use set trace point sc924 to trace this diagnostic msg
	*/
	if (norm_str_len >= input_str_len)
	    ule_format(E_PS037C_UNORM_DIAG, (CL_SYS_ERR *)0, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &error, 4,
		sizeof(i4), &t1, sizeof(i4), &dv1.db_length,
		sizeof(i4), &t2, sizeof(i4), &rdv.db_length);
    }

    /* Update yylval to point to normalized token */
    if (token == SCONST)
    {
	yacc_cb->yylval.psl_textype = norm_text;
	size = norm_text->db_t_count + DB_CNTSIZE;
        pss_cb->pss_symnext = (char *)norm_text + size;
    }
    else if (token == QDATA)
    {
	yacc_cb->yylval.psl_dbval = n_qdv;

	if (n_qdv->db_datatype < 0)
	    norm_val_len++;
	n_qdv->db_length = norm_val_len;

	/* Change the db_data pointer to the rdv.db_data */
        if (dv1.db_datatype== DB_CHA_TYPE || 
		dv1.db_datatype == DB_CHR_TYPE ||
		dv1.db_datatype == DB_NCHR_TYPE)
	    n_qdv->db_data = rdv.db_data + DB_CNTSIZE;
	else
	    n_qdv->db_data = rdv.db_data;

	if (n_qdv->db_datatype < 0)
	    (VOID)adg_vlup_setnull(n_qdv);
	
	size = n_qdv->db_length;
        pss_cb->pss_symnext = (char *)n_qdv->db_data + size;
    }
    else if (token == NAME)
    {
	yacc_cb->yylval.psl_strtype = norm_text->db_t_text;
	/* add back the null */
	norm_text->db_t_text[norm_text->db_t_count] = '\0';
	size = norm_text->db_t_count + 1;
        pss_cb->pss_symnext = (char *)norm_text->db_t_text + size;
    }

    return (token);
}

static bool
psl_dtunorm(
ADF_CB          *adf_cb,
i2		idt)
{
    bool	utf8;
    DB_DT_ID	dt;

    utf8 = (adf_cb->adf_utf8_flag & AD_UTF8_ENABLED);
    dt = abs(idt);

    if (dt == DB_NCHR_TYPE || dt == DB_NVCHR_TYPE ||
	(utf8 && 
	    (dt == DB_CHR_TYPE || dt == DB_CHA_TYPE 
	    || dt == DB_TXT_TYPE || dt == DB_VCH_TYPE)))
	return (TRUE);
    else
	return (FALSE);
}
