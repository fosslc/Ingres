/*
** History:
**	##-###-#### (eduardo)
**		Created.
**	16-may-1990 (rog)
**		Added SEP_VER and SEP_VER_SIZE to allow better control over
**		versions.
**	19-may-1990 (rog)
**		Added UNPUT_SIZE to specify the size of the unput() buffer
**		used by the lex routines.
**	27-aug-1990 (rog)
**		Changed SEPFILE to use uchar's instead of char's, and changed
**		the version to 3.1.
**	29-aug-1990 (rog)
**		Changed uchar's to u_char's for portability.
**	24-jun-1991 (donj)
**		Move *_CMMD and FILL_PROMPT from sepostcl.c to here so they
**		are available to other modules in order to implement condi-
**		tional execution. Add if_level[] and if_ptr.
**      19-jul-1991 (donj)
**              Changed i4  if_level and if_ptr to GLOBALREFs for UNIX cc 
**		which is pickier than VMS cc.
**	30-aug-91 (donj)
**		Add in Trace constants for implementing tracing. Add in
**		constants SKIP_VER_* to implement "skip, but verify" canons.
**	25-dec-91 (donj)
**		Changed sep version to "3.2".
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Added defines for sep_state and sep_state_old vars. Added
**	    if_level[] array and if_ptr for .if,.else,.endif functionality.
**	    Added CONTROL_PROMPT (for .if,.else,.endif) and ENDING_PROMPT (for
**	    recognising the ending banner of a sep test) string constants.
**	27-jan-1992 (donj)
**	    Add TRACE_PARM for sepparam debugging.
**	24-feb-1992 (donj)
**	    Add TRACE_DIALOG for debugging the interaction between SEP and
**	    the SEPSON subprocess.
**	02-apr-92 (donj)
**	    Added WORD_ARRAY_SIZE used by SEP_cmd_line() and the options
**	    arrays it uses. Added two bits to sep_state defines,
**	    OPEN_CANON_ARGS and CLOSE_CANON_ARGS to flag when the "<<" and
**	    ">>" had supplemental arguments present. This is for
**	    implementation of labelled canons. Also added OPTION_LIST
**	    structure for SEP_cmd_line(). Also modified SEP_VER.
**	15-apr-92 (donj)
**	    Added ABORT_TEST constant to be used to indicate that the sep
**	    test should be aborted.
**	05-jul-92 (donj)
**	    Added a static array constant for the double quote character in
**	    the form that is compatible with 8/16 bit char sets and the CM cl
**	    module. Added constants for handling cl ME memory tags.
**	10-jul-92 (donj)
**	    Moved ME memory tag constants from memory.c to here and included
**	    this file in memory.c. Also changed all constants to be cast as
**	    a u_i4.
**      14-jul-92 (donj)
**          Changed TOKEN_TAG to fit the other SEP_ME_TAG_xxxx constants.
**	21-jul-92 (donj)
**	    Version [] should be READONLY char, not just READONLY (int?).
**	 1-sep-1992 (donj)
**	    Add TRACE_GRAMMAR for grammar2 debugging.
**	30-nov-1992 (donj)
**	    Added grammar_methods constants, GRAMMAR_LEX, GRAMMAR_SED, etc.
**	    Also added some other ME tag constants.
**	04-feb-1993 (donj)
**	    Added definition of MPE if platform is hp9_mpe. Also moved typedef
**	    of structure for tracking fill files, FILLSTACK, from sepostcl.c.
**	    Also created a ME tag for use by the fill file routines. Take out
**	    definition for GRAMMAR_CM, obsolete.
**	15-feb-1993 (donj)
**	    Change casting of ME tags to match the tag arg casting.
**       5-mar-1993 (donj)
**	    Alpha patch, didn't like READONLY.
**	31-mar-1993 (donj)
**	    Bump Sep_ver to 3.3. Add a new field to OPTION_LIST structure,
**	    arg_required. Used for telling SEP_CMD_LINE to look ahead for an
**	    argument (i.e. "-w 5" rather than "-w5" or "-w=5").
**      23-apr-1993 (donj)
**	    Change typedef for SEPFILE so acc on Sun can deal with it.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 7-may-1993 (donj)
**	    Change ME tag datatypes to match ME modules.
**      19-may-1993 (donj)
**	    Change SEP_VER_SIZE to ansi C requirements. Added another ME tag.
**	    Added a trace flag for tracing the CL ME module tables.
**	 2-jun-1993 (donj)
**	    Added a definition for header, sepdefs_h. Added trace flag for an
**	    empty trace.
**	14-aug-1993 (donj)
**	    Re-arranged memory tags and changed OPTION_LIST for use with
**	    linked lists of structs rather than an array of structs.
**	 9-sep-1993 (donj)
**	    Add a constant for the new command unsetenv called UNSETENV_CMMD.
**	    Add a backward pointer to ENV_VARS so we could straighten out the
**	    problems of setting and unseting and restoring old values env_chain
**	    is now a proper lifo list (stack) of values.
**	23-sep-1993 (donj)
**	    Add a SEP_ME_TAG_MISC for memory allocations that don't fit a particular
**	    group but will need to be freed.
**	30-dec-1993 (donj)
**	    Added Translate Token Mask constants for Trans_Token().
**	 9-feb-1994 (donj)
**	    Added static declaration for SNG_QUO as well as DBL_QUO.
**	14-apr-1994 (donj)
**	    Added command constants for testcases and a TEST_CASE typedef.
**	20-apr-1994 (donj)
**	    Added two fields to the TEST_CASE typedef.
**	08-jun-1994 (donj)
**	    Added a special cmmd constant for ferun to eliminate the need
**	    for an ferun shell wrapper.
**	24-mar-1995 (holla02)
**	    Added static declaration for HYPHEN.  Used in sepspawn.c (UNIX)
**	    as part of criteria for stripping DBL_QUO
**	03-may-1995 (holla02)
**	    Added static declaration for PLUS for same purpose as HYPHEN above.
**	    Used in sepspawn.c to recognize and strip double quotes from "+.." 
**	    commandline options.
**
**	27-sep-1995 (somsa01)
**	    Default number of columns for an MS-DOS window (in NT) is 25.
**	    Therefore, I added ifdef NT_GENERIC for the definition of 
**	    STATUS_ROWS to 4.
**	09-feb-1998 (kinte01)
**	   Cross-integrated changes from AXP VMS CL to VAX VMS CL for OI 2.0
**	28-Jul-1998 (lamia01)
**	    Changed version identifier to 3.4 due to added diff masking tokens.
**

**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-May-2001 (hanje04)
**	    Updated sep version to 3.5
**	25-May-2001 (hanje04)
**	    Updated sep version to 3.6
**	21-Jan-2003 (xu$we01)
**	    Increased buffer size from 4096 to 65536 to handle the large
**	    page size used by user. 
**	15-may-2003 (abbjo03)
**	    Reduce the buffer size to 16384 since RACC files on VMS only
**	    support record sizes up to 32255.
*/

/*
**	SEP version support.  We use Version[], so SEP_VER should just contain
**	"Version" and the current version.
*/

#ifndef sepdefs_h

#define sepdefs_h       /* do this only once */

#define SEP_VER ERx("Version 3.6: ")
GLOBALREF    Version [] ;

#define SEP_VER_SIZE	(sizeof(SEP_VER) + STlength((char *)Version) + 1)

/* general values */

#ifdef hp9_mpe
#ifndef MPE
#define MPE
#endif
#endif

#define WORD_ARRAY_SIZE 30
#define TERM_LINE	256
#define	TEST_LINE	82
#define SCR_LINE	16384
#define	MAIN_COLS	80
#define	MAIN_ROWS	21
#define	STATUS_COLS	80
#ifdef NT_GENERIC
#define	STATUS_ROWS	4
#else
#define	STATUS_ROWS	3
#endif
#define	SEPOFF		0   /* no outstanding TC connection */
#define	SEPONBE		1   /* outstanding connection with BE */
#define	SEPONFE		2   /* outstanding connection with FE */

/*	SEP test commands	*/

#define UNKNOWN_CMMD		 0
#define OSTCL_CMMD		 1
#define FORMS_CMMD		 2
#define TM_CMMD			 3
#define TYPE_CMMD		 4
#define EXISTS_CMMD		 5
#define DELETE_CMMD		 6
#define FILL_CMMD		 7
#define CD_CMMD			 8
#define SETENV_CMMD		 9
#define SEPSET_CMMD		10
#define UNSEPSET_CMMD		11
#define IF_CMMD			12
#define ELSE_CMMD		13
#define ENDIF_CMMD		14
#define UNSETENV_CMMD		15
#define TEST_CASE_CMMD		16
#define TEST_CASE_END_CMMD	17
#define FERUN_CMMD              18
#define MAX_CMMD_VAL		18

/*  Values for (i4 sep_state, sep_state_old) var */

#define STARTUP_STATE	   0x0000
#define IN_OS_STATE        0x0001
#define IN_TM_STATE        0x0002
#define IN_FE_STATE        0x0004
#define IN_COMMENT_STATE   0x0008
#define IN_CANON_STATE     0x0010
#define BLANK_LINE_STATE   0x0020
#define IN_CONTROL_STATE   0x0040
#define IN_FILL_STATE      0x0080
#define IN_SKIP_STATE      0x0100
#define IN_SKIPV_STATE     0x0200
#define IN_ENDING_STATE    0x0400
#define OPEN_CANON_ARGS    0x0800
#define CLOSE_CANON_ARGS   0x1000
#define STATE_MASK         0x1fff

#define SKIP_CANON      20
#define SKIP_VER_CANON  21

#define NULLSTR			ERx("")
#define SEP_PROMPT		ERx("?")
#define	TM_PROMPT		ERx("*")
#define KFE_PROMPT		ERx("^^")
#define FILL_PROMPT		ERx("!!")
#define OPEN_CANON		ERx("<<")
#define CLOSE_CANON		ERx(">>")
#define OPEN_COMMENT		ERx("/*")
#define CLOSE_COMMENT		ERx("*/")
#define SKIP_SYMBOL		ERx("~")
#define SKIP_VER_SYMBOL		ERx("~~")
#define CONTROL_PROMPT		ERx(".")
#define ENDING_PROMPT		ERx("Ending")

/*	window positions	*/

#define FIRST_MAIN_COL	0
#define LAST_MAIN_COL	79
#define FIRST_MAIN_ROW	0
#define LAST_MAIN_ROW	20
#define	FIRST_STAT_ROW	21
#define PROMPT_ROW	1
#define	PROMPT_COL	1

/*	unput() buffer size for the lexical routines	*/

#define UNPUT_SIZE	512

/* Misc. other convenient constants. */

static char                DBL_QUO [3] = { '\042', 0, 0 } ;
static char                SNG_QUO [3] = { '\047', 0, 0 } ;
static char                HYPHEN  [3] = { '\055', 0, 0 } ;
static char                PLUS    [3] = { '\053', 0, 0 } ;

/* structures */

typedef struct
{
    u_i2                   address [3] ;
}   REC_ADD;

typedef struct             cmmds
{
    char                  *cmmd_nm ;
    i4                     cmmd_id ;
    struct cmmds          *left ;
    struct cmmds          *right ;
}   CMMD_NODE;


typedef struct             key_node
{
    char                  *key_label ;
    char                  *key_value ;
    struct key_node       *left ;
    struct key_node       *right ;
}   KEY_NODE;

typedef struct             env_vars
{
    char                  *name ;
    char                  *old_val ;
    char                  *new_val ;
    struct env_vars       *next ;
    struct env_vars       *prev ;
}   ENV_VARS;

typedef struct             sepfile
{
    u_char                *_buf ;
    u_char                *_pos ;
    u_char                *_end ;
    i4                     _buffSize ;
    FILE                  *_fptr ;
}   SEPFILE;

typedef struct             _fillstack
{
    LOCATION              *fs_loc ;
    char                  *fs_buf ;
    struct _fillstack     *fs_next ;
} FILLSTACK;

/* structure for chain of function keys supported */

typedef struct             funckeys
{
    char                  *label ;
    char                  *string ;
    struct funckeys       *next ;
}   FUNCKEYS;

/* structure for chain of Test_Case diff records */

typedef struct             test_case
{
    char                  *file ;
    char                  *name ;
    i4                     diffs ;
    i4                     weight ;
    i4                     begin ;
    i4                     secs ;
    i4                     instances ;
    struct test_case      *next ;
}   TEST_CASE;

/* file names */

#define	CMMD_FILE	ERx("commands.sep")

/* status messages */

#define	INGRES_FAIL 10
#define	FLAG_FAIL   11
#define ABORT_TEST  9901

/* Tracing Flags */

#define	TRACE_TOKENS	0x001
#define	TRACE_LEX	0x002
#define	TRACE_PARM	0x004
#define TRACE_DIALOG    0x008
#define TRACE_SEPCM     0x010
#define TRACE_GRAMMAR   0x020
#define TRACE_ME_TAG    0x040
#define TRACE_ME_HASH   0x080
#define TRACE_NONE      0x100
#define TRACE_TESTCASE  0x200

/* Grammer Flags */

#define GRAMMAR_LEX     0x001
#define GRAMMAR_LEX_R   0x002
#define GRAMMAR_SED_R   0x004
#define GRAMMAR_LEX_C   0x008
#define GRAMMAR_SED_C   0x010

/* Translate Token Flags */

#define TRANS_TOK_PARAM 0x001
#define TRANS_TOK_FILE  0x002
#define TRANS_TOK_VAR   0x004
#define TRANS_TOK_ALL   0x007

/* SEP ME memory tags */

#define SEP_ME_TAG_NODEL    ((u_i4)  0)
#define SEP_ME_TAG_EVAL     ((u_i4)  5)
#define SEP_ME_TAG_CANONS   ((u_i4) 10)
#define SEP_ME_TAG_PAGES    ((u_i4) 15)
#define SEP_ME_TAG_PARAM    ((u_i4) 20)
#define SEP_ME_TAG_SEPFILES ((u_i4) 25)
#define SEP_ME_TAG_GETLOC   ((u_i4) 30)
#define SEP_ME_TAG_TOKEN    ((u_i4) 35)
#define T_BLOCK_TAG	    ((u_i4) 40)
#define T_BUNCH_TAG	    ((u_i4) 45)
#define E_BLOCK_TAG	    ((u_i4) 50)
#define E_BUNCH_TAG	    ((u_i4) 55)
#define SEP_ME_TAG_ENVCHAIN ((u_i4) 60)
#define SEP_ME_TAG_SEPLO    ((u_i4) 65)
#define SEP_ME_TAG_SED      ((u_i4) 70)
#define SEP_ME_TAG_FILL     ((u_i4) 75)
#define SEP_ME_TAG_CMMDS    ((u_i4) 80)
#define SEP_ME_TAG_MISC     ((u_i4) 85)

/* Structure for command line args used with SEP_cmd_line. */

typedef struct option_entry {
	char		    *o_list;
	char		    *o_retstr;
	bool		    *o_active;
	i4		    *o_retnat;
	f8		    *o_retfloat;
	bool		     arg_required;
	struct option_entry *o_next;
	struct option_entry *o_prev;
} OPTION_LIST;

/* deassign main */

#endif
