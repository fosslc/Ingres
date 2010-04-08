/*
**	Copyright (c) 1989 Ingres Corporation
*/

/**
** Name:	framegen.h - #defines for framegen
**
** Description:
**
** History:
**	4/21/89 (pete)	written
**	28-feb-1992 (pete)
**	    Changed MAXTFWORDS from 12 to 50. Also added #defines for
**	    valid data types for new ##DEFINE statement. Also, added 
**	    FG_STMT... defines for ##statement types (if, while, etc).
**	    Also, add typedef FG_STATEMENT.
**	15-jun-92 (pete)
**	    Added typedefs: TOKEN and TLSTMT. Also changed MAXTFWORDS from
**	    50 to 128.
**	15-jun-92 (pete)
**	    Added typedefs: TOKEN and TLSTMT. Also changed MAXTFWORDS from
**	    50 to 128.
**	6-dec-92 (blaise)
**	    Added FG_VALUES_CLS and FG_SET_CLS.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
VOID	IIFGerr();	/* error handler called throughout 'fg'. uses EX */

/* static's */

/* # define's */

/* data types of $variables */
#define FG_TYPE_NONE    0
#define FG_TYPE_INTEGER 1
#define FG_TYPE_VARCHAR 2
#define FG_TYPE_BOOLEAN 3

/* types of statement clause */
#define FG_VALUES_CLS   0
#define FG_SET_CLS      1

#define RECSIZE		256
#define TOKSIZE		256
#define MAXTFWORDS	128	/* max # words on ## line */
#define MAXWHITE	80	/* max # white space chars at begin of ##line */
#define MAXIF_NEST	20	/* stack size for nested if stmts.	*/
#define MAXINCLUDE_NEST	20	/* max # of nested include stmts.	*/
#define G4ERROR		-99	/*
				** Used to signal errors. Value should differ
				** from TRUE, FALSE + all locally created
				** symbolic constants.
				*/
/* constants used with EX for error processing */
#define FGFAIL		0x10	/* return FAIL to IIFGstart caller */
#define FGCONTINUE	0x20	/* EXCONTINUE after printing error msg */
#define FGSETBIT	0x40	/* set METAFRAME.state to MFST_GENBAD */

#define TFNAME ERx("iitf")	/* name of table field */
#define PAD0 ERx("")		/* indent amount in generated code. */
#define PAD2 ERx("  ")		/* indent amount in generated code. */
#define PAD4 ERx("    ")	/* indent amount in generated code. */
#define PAD8 ERx("        ")	/* indent amount in generated code. */
#define PAD11 ERx("           ")	/* indent amount in generated code. */
#define PAD12 ERx("            ")	/* indent amount in generated code. */

/*}
** Name:	IFINFO - structure with info about a "##if" statement.
**
** Description:
**	Information about a particular ##if statement.
**	A stack of these is maintained since "##if/endif" statements can
**	be nested. Each ##if encountered in the template file pushes one
**	of these on the stack; ##endif pops one.
**
** History:
**	4/1/89 (pete)	written
*/
typedef struct
{
	bool	iftest;		/* result of ##if test (TRUE or FALSE) */
	i4	pgm_state;	/* program state at time "##if" encountered */
	/* program states */
#define PROCESSING	1
#define FLUSHING	2	/* if a ##if evaluates to FALSE */
	bool	has_else;	/* does this "##if" have a "##else" stmt? */
} IFINFO;

/*}
** Name:        FG_STATEMENT - info about a ## statement
**
** Description:
**      Description of a ## statement.
**
** History:
**      20-mar-1992 (pete)
**          Initial Version.
**	15-jun-1992 (pete)
**	    Altered definition. Removed .subtype. Added save_input_word,
**	    expr_begin.
*/
typedef struct
{
        char *stmt;     /* statement keyword in UPPER CASE */
        i4    type;     /* statement type */
#define FG_STMT_NONE    0
#define FG_STMT_COMMENT 1
#define FG_STMT_DEFINE	2
#define FG_STMT_ELSE	3
#define FG_STMT_ENDIF	4
#define FG_STMT_GENERATE 5
#define FG_STMT_IF      6
#define FG_STMT_IFDEF	7
#define FG_STMT_IFNDEF	8
#define FG_STMT_INCLUDE	9
#define FG_STMT_UNDEF	10
#define FG_STMT_DOC	11
	i4	save_input_word;	/* Save this statement word. Valid
					** word number (0-->n) or FG_NOWORD.
					*/
#define FG_NOWORD	-1
	i4	expr_begin;		/* This word begins an expression
					** in the statement. Position of this
					** word in statement must be remembered
					** for future expression processing.
					*/
	char	*strip_last_word;	/* If this word is the last one in
					** statement (after comment removal),
					** then remove it. E.g. "THEN".
					** UPPER CASE word, or NULL.
					*/
} FG_STATEMENT;

/*}
** Name:	CGEN_FNS - structure of info about a function that generates 4gl
**
** History:
**	4/10/89 (pete)	written
*/
typedef struct
{
	char 	cgen_type[FE_MAXNAME+1];  /*
					  ** Type of 4gl code generation:
					  ** "user_escape", "query", etc.
					  */
	i4	(*cgen_fn)();		  /*
					  ** Function that does this
					  ** type of 4gl code generation.
					  */
} CGEN_FNS;


/*}
** Name:	IITOKINFO	- structure with info about a "$" token.
**
** Description:
**	Certain tokens in the template file have their values set up
**	by the Visual Query (VQ). The names of these tokens always begin
**	with "$". IITOKINFO contains information about each known VQ token.
**	The program translates these known tokens and writes their translation
**	value into the generated code.
**
** History:
**	4/1/89 (pete)	written
**	4-23-90 (pete) changed tokname[FE_MAXNAME+1] to *tokname, to cut memory
**	27-feb-1992 (pete)
**	    Added attribute "inttoktran" & description of new 'i' toktype.
**	    Added description of type 'X', Deleted tokens. Added attribute
**	    "tokclass".
*/
typedef struct
{
	char	*tokname;
	char	*toktran;	/* Character string value of $variable */
#if 0
	char	toktype;	/* 'b'=boolean, 's'=string, 'i'=integer,
				** 'X' = Deleted (pretend it's not present).
				*/
#endif
	i4	tokclass;	/* Tells if the $variable is a built-in one,
				** created by Vision, or if it was created
				** by a ##DEFINE statement in a template file.
				** or if it has been ##UNDEFed (deleted).
				*/
#define	FG_TOK_BUILTIN	1
#define FG_TOK_DEFINED	2
#define FG_TOK_DELETED	3	/* has been ##UNDEF'd */
#if 0
	i4 inttoktran;	/* Integer value of $variable. 'i' & 'b'
				** $variables only.
				*/
#endif
} IITOKINFO;

struct _declared_iitoks
{
	IITOKINFO tok;			/* info about a $variable */
	struct _declared_iitoks *next;	/* next $variable in linked list */
};
typedef struct _declared_iitoks DECLARED_IITOKS;

/*}
** Name:  ESC_TYPE - info about one of the "##generate user_escape" types.
**
** Description:
**	A list of these determines all the valid values for user_escape types,
**	except "field_activates", which are handled differently. This list
**	is searched to see what type of user_escape was specified on the
**	"## generate user_escape" line in the template file. The "section"
**	member is used in calls to IIFGbsBeginSection to bracket the
**	user entered code with special ^G comments processed by 4GL & Emerald.
**
** History:
**	6/6/89 (pete)	Written.
*/
typedef struct
{
	char *name;	/* name of escape type (e.g. "form_start",
			** "query_end", etc
			*/
	i4   type;	/* the corresponding MFESC type in METAFRAME */
	ER_MSGID section;	/* message id of a string description that
				** describes this section of user-entered code.
				** Used to bracket user-entered code with
				** special ^G comments. Each "type" has
				** exactly one corresponding "section".
				*/
} ESC_TYPE;

/*}
** Name:	FIELD_EXIT - information about a field exit activate.
**
** Description:
**	Field exit activates are the most complicated types because within
**	the field activate block we must include the Lookup Validation
**	SELECT, the Field Change activate code and the Field Exit activate
**	code, as well as 4gl statements to check the change bit for
**	the field, and to check the row count after the lookup validation.
**
**	The information about these 3 pieces of the field activate is
**	built up in a linked list of these structures, by traversing the
**	METAFRAME. Then this structure is used to generate code.
**	This is used in 'fgusresc.c'.
**
** History:
**	6/13/89 (pete)	Written.
**	1/29/90 (pete)  Add fields "table_fld" and "fld_all".
*/
struct _field_exit
{
	char	*item;		/* name of field on form (MFCOL.alias) */
	bool	table_fld;	/* TRUE if "item" is format "tblfld.col";
				** FALSE otherwise (simple field).
				*/
	bool	fld_all;	/* TRUE if "item" is "all" or "tblfld.all";
				** FALSE otherwise.
				*/
	i4	jtindx;		/* MFJOIN index for a MASLUP or DETLUP lookup
				** join for this item (set to -1 if no lookup
				** join on this item).
				*/
	MFESC	*changeesc;	/* field change escape structure (if any) */
	MFESC	*exitesc;	/* field exit escape structure (if any) */
	struct _field_exit *next;  /* next one in linked list */
};
typedef struct _field_exit FIELD_EXIT ;

/*}
** Name:        TOKEN - details about a token.
**
** Description:
**      Details about a token from input stream.
**
** History:
**      15-jun-1992 (pete)
**          Initial version.
*/
typedef struct {
        u_i4 type;             /* token type */
#define	FG_TOK_NONE		0x0
#define FG_TOK_VARIABLE         0x01
#define FG_TOK_ENV_VAR          0x02
#define FG_TOK_NOTFOUND         0x1000  /* can be OR'd w/ VARIABLE or ENV_VAR */
#define FG_TOK_QUOTE		0x04
#define FG_TOK_QUOTED_STRING	0x08
#define FG_TOK_OTHER		0x10
#define FG_TOK_DELIM            0x20
#define	FG_TOK_WORD		0x40
#define	FG_TOK_COMMENT		0x80
#define	FG_TOK_WHITESPACE	0x100
        char *tok;              /* The token itself (NULL terminated).
                                ** Quotes removed from string constants.
                                ** Points to value for types VARIABLE
                                ** & ENV_VAR, rather than actual token,
                                ** unless FG_TOK_NOTFOUND is set,
                                ** in which case this points to actual
                                ** token, rather than value.
                                */
} TOKEN;

/*}
** Name:        TLSTMT - info about a ##statement in input stream.
**
** Description:
**      Info about a ##statement from input stream.
**
** History:
**      15-jun-1992 (pete)
**          Initial version.
*/
typedef struct {
        i4  type;               /* statement type. */
        i4  nmbr_tokens;        /* number of tokens in statement. */
        TOKEN *tokens[MAXTFWORDS]; /* statement tokens. */
} TLSTMT;

