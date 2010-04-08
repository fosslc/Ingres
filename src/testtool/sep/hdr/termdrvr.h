/*
**  Termdrvr.h
**
**	basic keys used by SEP during normal operation
**
**  History:
**	##-###-#### (XXXX)
**	    Created
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**       7-may-1993 (donj)
**          Fixed prototyping in this file.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef BELL
#define BELL	    '\07'
#endif
#define DEL	    '\177'
#define ESC	    '\033'
#define FORMFD	    '\014'
#define S_REFRESH   '\027'
#define XON	    '\021'
#define XOFF	    '\023'

/*
    escape sequences defined in termcap file
*/

GLOBALREF    char         *PROMPT_POS ; /* to last row, first col */
GLOBALREF    char         *DEL_EOL ;    /* del 'til EOL */
GLOBALREF    char         *ATTR_OFF ;   /* display attributes off */
GLOBALREF    char         *REV_VIDEO ;  /* reverse video on */
GLOBALREF    char         *BLNK ;       /* blinking on */
GLOBALREF    char         *DRAW_LINES ; /* initialize terminal to draw solid lines */
GLOBALREF    char         *DRAW_CHAR ;  /* interpret next characters for drawing solid lines */
GLOBALREF    char         *REG_CHAR ;   /* interpret next characters for drawing reg chars */
GLOBALREF    KEY_NODE     *termKeys ;   /* head of basic keys tree */
GLOBALREF    bool          USINGFK ;

FUNC_EXTERN  VOID          encode_seqs (char *source,char *dest) ;
FUNC_EXTERN  VOID          decode_seqs (char *source,char *dest) ;
FUNC_EXTERN  i4            comp_seqs (char *seq1,char *seq2) ;
FUNC_EXTERN  i4            TEchkAheadBuffer () ;
FUNC_EXTERN  STATUS        get_key_label (KEY_NODE *master,char **value,char *label) ;
