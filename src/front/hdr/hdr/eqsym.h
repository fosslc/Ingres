/*
**	Copyright (c) 2004 Ingres Corporation
*/
/* ===ICCS===\50\3\4\   ingres 5.0/03  (pyr.u42/01)     */
/*
** eqsym.h
**	- definitions of the symbol table macros and data structures.
**
** Mark Wittenberg
** RTI
** October 1984
**      12-Dec-95 (fanra01)
**          Added definitions for referencing data in DLLs on windows NT from
**          code built for static libraries.
**      06-Feb-96 (fanra01)
**          Changed flag from __USE_LIB__ to IMPORT_DLL_DATA.
**      04-Mar-96 (thoda04)
**          Added function prototypes and function prototype arguments.
**
*/
# ifndef EQSYM_H_included
# define EQSYM_H_included

#define	sySCOP_MAX		20
#define	syVIS_MAX		sySCOP_MAX
#define	sySCOP_SIZ		(sySCOP_MAX+1)

#define syIS_PERV(s)            (I1_CHECK_MACRO((s)->st_vislim) < 0)
#define	syIS_VIS(s,closure)	((s)->st_vislim>=(closure) || syIS_PERV(s))
#define	syMIN(a,b)		((a) < (b) ? (a) : (b))

#define	syHASH_SIZ		64	/* size of hash table; must be <=256 */

#define	sym_str_name(s)		((char *) (&((s)->st_name)[1]))

typedef u_i2		u_tiny;	/* u_i1 when we settle u_char trouble */
typedef u_i1		syNAME;	/* struct { u_i1 n_len; char n_name[]; } */

/* base types */
#define	syT_STRUCT	0x80	/* structured type; sign bit */
#define	syT_INT		1
#define	syT_CHAR	2
#define	syT_FLOAT	3

/* flag bits */
#define	syFisVAR	0x01
#define	syFisTYPE	0x02
#define	syFisCONST	0x04
#define	syFisTAG	0x08	/* struct, union, or enum tag */
#define	syFisBASE	0x10	/* ie, not a structured type */
#define	syFisSYS	0x20	/* predefined var -- just for sc_symdbg */
#define	syFisFORWARD	0x40	/* forward reference */
#define	syFisORPHAN	0x80	/* ignore parents */
#define syFuseMASK      (syFisVAR|syFisTYPE|syFisCONST|syFisTAG|syFisBASE|syFisFORWARD|syFisORPHAN)
#define	syFuseOF(p)	((p)->st_flags & syFuseMASK)
#define	syBitAnd(x, y)	((x) & (y))
#define	syFisDEF(p)	syFuseOF(p)

typedef struct sym_tab {
    struct sym_tab
		*st_hash;	/* next (older) entry on this hash chain */
    syNAME	*st_name;	/* string name */
    struct sym_tab
		*st_type,	/* type for vars or pointer fields of types */
		*st_prev,	/* previous use of same name */
		*st_parent,	/* parent in record */
		*st_child,	/* child in record */
		*st_sibling;	/* sibling in record */
    i4		*st_value;	/* free for the user */
    i1		st_vislim,	/* highest block at which visible */
				/* if < 0, then pervasive, and abs(st_vislim) */
				/* is the level at which it became pervasive */
		st_block,	/* block defined at or exported to */
		st_reclev;	/* record level nesting */
    u_i1	st_btype,	/* base type */
		st_dims,	/* number of dimensions */
		st_space;	/* address space index -- more than enough */
    u_tiny	st_hval,	/* hash index; assumed to fit in byte. */
		st_indir;	/* level of indirection to a base type */
    u_i2	st_flags;	/* useful flag bits; see above */
    i4		st_dsize;	/* size of last dimension */
} SYM;

/*
** struct symHint symHint
**	is used in sydeclare.c, sy*sym.c for redeclaration checking.
*/
struct symHint {
    SYM		*sh_type;
    i4		sh_btype;
    i4		sh_indir;
};
GLOBALREF struct symHint	symHint;

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF  i4	(*sym_delval)(), (*sym_prtval)(), (*sym_adjval)(),
		(*sym_prbtyp)();
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF  i4	(*sym_delval)(), (*sym_prtval)(), (*sym_adjval)(),
		(*sym_prbtyp)();
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

void	sym_init(bool ignore_case);
		    /* Initialize the symbol table */
SYM 	*sym_lookup(char *name, bool install, i4  use, i4  block);
		    /* Lookup (or install) a name in the symbol table */
SYM 	*sym_find(char *name);
		    /* Find a name in the symbol table -- don't ever install it */
SYM 	*sym_resolve(SYM *parent, char	*name, i4  closure, i4  use);
		    /* Find the entry referenced by a name with the specified parent */
SYM 	*sym_which_parent(SYM *sym_start, i4  rec_level);
		    /* Find the parent of an entry with the given level */
bool	sym_install(SYM *s, SYM *sym_start, i4  rec_level, SYM **elder);
		    /* Install an entry into the correct place in its family tree */
bool	sym_is_redec(SYM *s, i4  closure, SYM **which);
		    /* Test an entry to see if it is a redeclaration */
char	*sym_rparentage(char *p, SYM *s);
		    /* Build up full ancestral name (for printing forwards) */
char	*sym_parentage(SYM * s);
		    /* Stuff a buffer with the fully-qualified name */
char	*sym_name_print(void);
		    /* Turn a "variable reference" into a printable name string */
void	sym_hint_type(SYM *type, i4  btype, i4  indir);
		    /* Hint about the next declaration (for ESQL redeclaration errs) */
bool	sym_eqname(char *name, i4  length, char *stname);
		    /* Fast name compare */
syNAME	*sym_store_name(char *name, i4  length);
		    /* Stash away a name in the name table */
i4  	sym_hash(char *s);
		    /* Hash a name */

void	sym_sinit(void);
		    /* Initialize the scope stack */
void	sym_s_begin(i4 closure);
		    /* Open a new scope, using the given closure */
i4  	sym_s_end(i4 current_scope);
		    /* End a scope */
i4  	sym_s_val(void);
		    /* Get the current scope level */
i4  	sym_s_which(SYM *e);
		    /* Return the number of the block to which the entry belongs */
void	sym_s_xport(void);
		    /* Raise the highwater mark for the name table */
void	sym_s_debug(void);
		    /* Print out some info useful for debugging scopes */

SYM 	*sym_q_new(void);
		    /* Get a new queue element from the symbol table queue manager*/
void	sym_q_free(SYM *q_elm);
		    /* Free the storage associated with a node */
STATUS	sym_q_delete(SYM *q_elm, SYM *q_before);
		    /* Remove "q_elm" from its chain and free it */
void	sym_q_empty(void);
		    /* Throw away the entire symbol table */
void	sym_q_append(SYM *q_elm, SYM *q_after);
		    /* Append the node to its hash chain */
STATUS	sym_q_remove(SYM *q_elm, SYM *q_before);
		    /* Remove "q_elm" from its chain */
STATUS	sym_q_head(SYM *q_elm);
		    /* Move an element to the front of its queue */

void	sym_f_init(void);
		    /* Initialize the record field stack */
void	sym_fpush(SYM *s);
		    /* Push an entry onto the field name stack */
SYM 	*sym_fpop(void);
		    /* Pop an entry from the field name stack */
i4  	sym_f_count(void);
		    /* Count the entries on the field stack */
char	*sym_f_name(i4 i);
		    /* Get a pointer to the i'th name on the field stack */
SYM 	*sym_f_entry(i4 i);
		    /* Get a pointer to the i'th entry on the field stack */
SYM 	*sym_f_list(void);
		    /* Turn the stack of entries into a list */
void	sym_f_stack(SYM *s);
		    /* Turn a list into a stack */

bool	symExtType(SYM *sy);
		    /* Resolve the base type, etc, of a forward reference to a sym */
bool	syAdjust(SYM *sy);
		    /* Fix up an incompletely resolved symtab entry */
SYM 	*syStrInit(SYM *sy);
		    /* Fetch the first child of a symtab entry (if any) */
SYM 	*syStrNext(SYM *sy);
		    /* Return the next member of a structure */
VOID	syStrFixup(SYM *sy);

void	syPrintf( /* variable argument list*/ );
		    /* Like SIprintf, but goes to the dump file */

void	trPrval(/* char *format, 2nd arg type can vary */);
		    /* Print one line of a client-formatted string */
void	trPrintNode(SYM *t);
		    /* Print one symbol table node */
void	trDodebug(bool all);
		    /* Dump the symbol table */

#define	sym_g_useof(s)		syFuseOF((SYM *)s)

#define	sym_g_type(s)		(((SYM *)(s))->st_type)
#define	sym_g_parent(s)		(((SYM *)(s))->st_parent)
#define	sym_g_child(s)		(((SYM *)(s))->st_child)
#define	sym_g_sibling(s)	(((SYM *)(s))->st_sibling)
#define	sym_g_vlue(s)		(((SYM *)(s))->st_value)
#define	sym_g_block(s)		(((SYM *)(s))->st_block)
#define	sym_g_reclev(s)		(((SYM *)(s))->st_reclev)
#define	sym_g_btype(s)		(((SYM *)(s))->st_btype)
#define	sym_g_dims(s)		(((SYM *)(s))->st_dims)
#define	sym_g_space(s)		(((SYM *)(s))->st_space)
#define	sym_g_indir(s)		(((SYM *)(s))->st_indir)
#define	sym_g_dsize(s)		(((SYM *)(s))->st_dsize)

#define	sym_s_type(s,v)		( ((SYM *)(s))->st_type = (SYM *)    (v) )
#define sym_s_sibling(s,v)	( ((SYM *)(s))->st_sibling = (SYM *) (v) )
#define	sym_s_vlue(s,v)		( ((SYM *)(s))->st_value = (i4 *)   (v) )
#define	sym_s_btype(s,v)	( ((SYM *)(s))->st_btype = (u_i1)    (v) )
#define	sym_s_dims(s,v)		( ((SYM *)(s))->st_dims  = (u_i1)    (v) )
#define	sym_s_space(s,v)	( ((SYM *)(s))->st_space = (u_i1)    (v) )
#define	sym_s_indir(s,v)	( ((SYM *)(s))->st_indir = (u_tiny)  (v) )
#define	sym_s_dsize(s,v)	( ((SYM *)(s))->st_dsize = (i4)     (v) )

#define	sym_o_vlue(s,op,v)	( ((SYM *)(s))->st_value op (i4 *)  (v) )
#define	sym_o_btype(s,op,v)	( ((SYM *)(s))->st_btype op (u_i1)   (v) )
#define	sym_o_dims(s,op,v)	( ((SYM *)(s))->st_dims  op (u_i1)   (v) )
#define	sym_o_space(s,op,v)	( ((SYM *)(s))->st_space op (u_i1)   (v) )
#define	sym_o_indir(s,op,v)	( ((SYM *)(s))->st_indir op (u_tiny) (v) )
#define	sym_o_dsize(s,op,v)	( ((SYM *)(s))->st_dsize op (i4)    (v) )

#ifdef	EQ_EUC_LANG
    /*
    **	- Euclid-specific function declarations
    */

    i4		symRsEuc(SYM **result, i4  closure, i4  use);
    SYM		*syEucUpdate(SYM *sy, char *name, i4  record_level, i4  cur_scope, 
                         i4  state, i4  closure, i4  space);
    SYM		*symDcEuc(char *name, i4  record_level, i4  cur_scope, 
                         i4  state, i4  closure, i4  space);
    bool	sym_modify_euc(char *name, i4  record_level, i4  cur_scope, 
                         i4  state, i4  closure);
    void	sym_type_euc(SYM *list, SYM *type);
    bool	sym_check_type_loop(SYM *old, SYM *new);

    /*
    ** language specific bits start at the high bit and work down.
    ** DO NOT hit the language independent bits which start at the low bit
    ** and work up
    */
#    define	syFisPERVAD	0x8000	/* pervasive */
#    define	syFisIMPORT	0x4000
#    define	syFisEXPORT	0x2000
#    define	syFisOPAQUE	0x1000

    /* lookup return values */
#    define	syL_OK			0
#    define	syL_NO_NAMES		1
#    define	syL_NOT_FOUND		2
#    define	syL_BAD_REF		3
#    define	syL_RECURSIVE		4
#    define	syL_AMBIG		5
#endif	/* EQ_EUC_LANG */

#ifdef	EQ_PL1_LANG
    /*
    **	- PL/1-specific function declarations
    */

    SYM		*symDcPL1(), *syPL1Update();
    i4		symRsPL1();
    SYM		*sym_f_entry(), *sym_f_list();
    char	*sym_f_name();
    void	sym_f_init();
    void	sym_fpush();

    /*
    ** language specific bits start at the high bit and work down.
    ** DO NOT hit the language independent bits which start at the low bit
    ** and work up
    */
#    define	syFisREVERSE	0x8000	/* if set then resolve name backwards */

    /* lookup return values */
#    define	syL_OK			0
#    define	syL_NO_NAMES		1
#    define	syL_NOT_FOUND		2
#    define	syL_BAD_REF		3
#    define	syL_RECURSIVE		4
#    define	syL_AMBIG		5
#endif	/* EQ_PL1_LANG */

# endif /* EQSYM_H_included */

