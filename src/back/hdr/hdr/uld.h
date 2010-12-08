/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ULD.H - state of building a linked list of text blocks
**
** Description:
**      This file contains structures and function headers for functions
**	that describe the state of a linked list of text blocks used when
**	converting a query tree into text.
**
** History: 
**      31-aug-1922 (rog)
**          Created for prototyping ULF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Jan-2004 (schka24)
**	    Added prototype for qmode lookup
**	30-Dec-2004 (schka24)
**	    Syscat lookup prototype; avoid pointer typedef usage so
**	    allow this header to be use with fewer dependencies.
**	23-Nov-2005 (kschendel)
**	    Added storage structure lookup proto.
**	4-Dec-2008 (kschendel) b122119
**	    Prototype for math exception lookup.
**	19-Aug-2009 (kibro01) b122509
**	    Add uld_prtree_x for sc930 tracing
**      17-Aug-2010 (horda03) b124274
**          Add flags parameter to uld_prtree_x, and permitted
**          values. Note if ULD_FLAG_SEGMENTS specified
**          the QEP will be dividied into connected segments.
*/

typedef struct _ULD_TSTATE
{
    char	    uld_tempbuf[ULD_TSIZE]; /* temp buffer used to build string
                                        ** this should be larger than any other
                                        ** component */
    i4		    uld_offset;		/* location of next free byte */
    i4		    uld_remaining;	/* number of bytes remaining in tempbuf 
					*/
    ULD_TSTRING     **uld_tstring;      /* linked list of text strings to return
                                        ** to the user */
} ULD_TSTATE;

/*}
** Name: ULD_TTCB - control block for tree to text conversion
**
** Description:
**      Internal control block for tree to text conversion
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**      26-aug-88 (seputis)
**          fix casting for *conjunct
*/
typedef struct _ULD_TTCB
{
    PTR             handle;             /* user handle */
    struct _ADF_CB  *adfcb;             /* adf session control block */
    struct _ULM_RCB *ulmcb;             /* ulm memory for text string
                                        ** output */
    i4		    uld_mode;		/* mode of the query */
    ULD_TSTRING	    *uld_rnamep;        /* ptr to name of the result variable
                                        ** or NULL if it is not needed, a struct
                                        ** is used to that the caller can keep
                                        ** a ptr to the text string if
					** necessary
					*/
    struct _PST_QNODE *uld_qroot;	/* root of user's query tree to convert 
                                        ** - if a root node is supplied then the
                                        ** entire statement is generated, if a
                                        ** non root node is supplied then an
                                        ** expression is generated */
    VOID            (*varnode)();       /* routine to get variable name
                                        ** and attribute name of a 
                                        ** var node */
    bool            (*uld_range)();     /* routine to call to get text for
					** a table name, and the alais */
    i4		    uld_linelength;	/* max length of line to be
                                        ** returned or 0 if ULD_TSIZE is
                                        ** to be used */
    bool	    pretty;             /* TRUE - if output is to be used for
                                        ** visual display, causes "from list"
                                        ** for SQL to be linked into query
                                        ** so that caller has control */
    bool            (*conjunct)();      /* routine to call in order to obtain
					** the next conjunct of the qual
                                        ** - useful for traversing bitmaps
                                        ** of boolean factors */
    struct _PST_QNODE *(*resdom)();	/* routine to call in order to obtain
					** the next resdom of a query
                                        ** - useful for traversing bitmaps
                                        ** of equivalence classes */
    DB_ERROR        *error;		/* error status to return to the user*/
    DB_STATUS	    uld_retstatus;
    ULD_TSTATE      uld_primary;        /* describes current state of text
					** string being built
					*/
    ULD_TSTATE      uld_secondary;      /* used to save alternate state of a
					** text build
                                        */
    DB_LANG	    language;		/* language that should be used to build
					** the query text */
    bool	    is_quel;		/* TRUE if query language is QUEL */
    bool	    is_sql;             /* TRUE if query language to be
					** generated is SQL
					*/
} ULD_TTCB;


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN VOID      uld_msignal     ( ULD_TTCB *global, DB_ERRTYPE error,
				        DB_STATUS status, DB_ERRTYPE facility );

FUNC_EXTERN char * uld_qef_actstr(i4 ahd_type);
FUNC_EXTERN char * uld_mathex_xlate(i4 exnum);
FUNC_EXTERN char * uld_psq_modestr(i4 qmode);
FUNC_EXTERN int uld_struct_xlate(char *strname);
FUNC_EXTERN char * uld_struct_name(i4 strnum);
FUNC_EXTERN char * uld_syscat_namestr(i4 table_id);

FUNC_EXTERN DB_STATUS uld_tree_to_text( PTR handle, i4  qmode,
				        ULD_TSTRING *rnamep,
					struct _ADF_CB *adfcb,
				        struct _ULM_RCB *ulmcb,
					ULD_LENGTH linelength,
				        bool pretty,
					struct _PST_QNODE *qroot,
				        VOID (*varnode)(), bool (*tablename)(),
				        bool (*conjunct)(),
				        struct _PST_QNODE *(*resdom)(),
				        ULD_TSTRING **tstring, DB_ERROR *error,
				        DB_LANG language );

FUNC_EXTERN VOID uld_prtree_x( i4 flags, PTR root, VOID (*printnode)(), PTR (*leftson)(),
	PTR (*rightson)(), i4  indent, i4  lbl, i4  facility, PTR sc930_trace );

#define ULD_FLAG_NONE     0x00000000
#define ULD_FLAG_SEGMENTS 0x00000001
#define ULD_FLAG_OUT_SEG  0x00000002 /* For use within uldprtree.c only */

