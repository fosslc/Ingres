/*
**  Token.h
**
**  History:
**	##-###-#### (XXXX)
**	    Created
**	14-jan-1992 (donj)
**	    Added constant definition for A_COPYRIGHT for MILLER.C to mask
**	    the Ingres copyright message. Reformatted variable declarations
**	    to one per line for clarity.
**	02-apr-1992 (donj)
**	    Added constant definition for A_DBNAME for MILLER.C to mask
**	    the phrase "database '{word}'" found in createdb and destroydb,
**	    etc.
**	04-feb-1993 (donj)
**          Added constant definition for A_DBLWORD, A_DESTROY, A_DROP,
**	    A_RULE, A_PROCEDURE, A_TABLE, A_OWNER, A_USER and A_COLUMN.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**      15-oct-1993 (donj)
**	    Changed token ID type constants to bit values so that differ can be
**	    faster. Added an "A_ALWAYS_MATCH" constant bit mask to set which
**	    token types always match (are masked). "A_WORD_MATCH" are the normal
**	    character matching types.
**	28-jan-1994 (donj)
**	    Added new A_PRODUCT token to mask for matching "INGRES" and "OpenINGRES"
**	28-Jul-1998 (lamia01)
**		Added new version identifiers, Computer Associates as the company name,
**		and NT directory masking. Version identifier also works now with DBL.
**		New tokens - Database ID, Server ID, Alarm ID, Transaction ID and
**		Checkpoint ID introduced to avoid unnecessary diffs.
**		Rearranged the numbering of the tokens so that different types are in 
**		order, and more than 32 tokens are possible (keep the ones in 
**		A_ALWAYS_MATCH with numbers that don't set bits used by the others).
**	25-Mar-1999 (popri01)
**	    Now using sep's sed masking for checkpoint ID, which is specific to LAR. 
**	    (See grammar.lex for additional explanation.)
**
*/
#define A_NUMBER    0x00000001
#define A_OCTAL     0x00000002
#define A_HEXA      0x00000004
#define A_FLOAT     0x00000008
#define A_WORD      0x00000010
#define A_SPACE     0x00000020
#define A_DBLWORD   0x00000040
#define A_DELIMITER 0x00000080
#define A_CONTROL   0x00000100
#define A_UNKNOWN   0x00000200
#define A_GROUP     0x00000400
#define A_DIRECTORY 0x00001000
#define A_BANNER    0x00002000
#define A_VERSION   0x00003000
#define A_COMPNAME  0x00004000
#define A_DATE      0x00005000
#define A_COPYRIGHT 0x00006000
#define A_DBNAME    0x00007000
#define A_TIME      0x00008000
#define A_DESTROY   0x00009000
#define A_DROP      0x0000A000
#define A_RULE      0x0000B000
#define A_PROCEDURE 0x0000C000
#define A_TABLE     0x0000D000
#define A_OWNER     0x0000E000
#define A_USER      0x0000F000
#define A_COLUMN    0x00010000
#define A_RETCOLUMN 0x00011000
#define A_PRODUCT   0x00012000
#define A_COMPILE   0x00013000
#define A_DATABSID  0x00014000
#define A_ALARMID   0x00015000
#define A_SERVERID  0x00016000
#define A_TRANID    0x00017000

#define A_ALWAYS_MATCH  ( A_DATE | A_OWNER  | A_PRODUCT  | A_PROCEDURE | \
                          A_TIME | A_TABLE  | A_VERSION  | A_DIRECTORY | \
                          A_DROP | A_BANNER | A_DESTROY  | A_RETCOLUMN | \
                          A_RULE | A_DBNAME | A_COMPNAME | A_COPYRIGHT | \
                          A_USER | A_COLUMN | A_COMPILE  | A_DATABSID  | \
                          A_ALARMID | A_SERVERID | \
                          A_TRANID )

#define A_WORD_MATCH    ( A_WORD | A_GROUP )

typedef struct token_list {
    char		*token;
    int			tokenID;
    int			token_no;
    int			lineno;
    struct token_list	*prev,*next;
}   FILE_TOKENS;

