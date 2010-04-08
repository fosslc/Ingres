/*
**	Copyright (c) 2004, 2008, 2009 Ingres Corporation
**	All rights reserved.
*/

#ifndef OSTREE_H_INCLUDED
#define OSTREE_H_INCLUDED

/**
** Name:    ostree.h -	OSL Parser Expression Tree Definitions File.
**
** Description:
**	Contains the definition of the tree node structure used in
**	expression trees by the OSL parsers.  Defines:
**
**	OSNODE		OSL expression tree node structure.
**
** History:
**	Revision 3.0  84/04  joe
**	Initial revision.
**
**	Revision 5.1  86/12/10  17:30:40  wong
**	Modified from 5.0 OSL compiler version:
**		Added DML bit definition to distinguish DML expression context.
**		Added in predicates, dotall and byref changes.
**		Deleted some unused node types (DECL & HIDCOL);
**		added type (PREDE) for predicates.
**
**	Revision 6.0  87/06  wong
**	Added support for ADF expressions.  Added table field reference
**	support for TBLFLD nodes (instead of having special symbol nodes.)
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Added N_COMPLEXOBJ, N_TABLEFLD, and N_ROW bits in n_flags.
**		Deleted n_sub (use N_ROW instead).
**		Added n_targlfrag to OSNODE, and added u_p to U_ARG union.
**		(For bug 34846).
**
**	Revision 6.5
**	18-jun-92 (davel)
**		Added tkDCONST for decimal support.
**	21-jul-92 (davel)
**		Broke out n_length from the non-leaf node union, and also
**		added n_prec.  It appears that in several cases, the
**		db_length and db_prec may be needed for a node (e.g. constants
**		and non-leaf nodes).
**	25-aug-92 (davel)
**		Added tkEXEPROC for EXECUTE PROCEDURE.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  27-nov-2001 (rodjo04)
**      Added new OSL node type ONCLAUSE.
**	01-Feb-2005 (drivi01)
**	    Changed u_char from extern to GLOBALREF.
**	21-Jan-2008 (kiria01) b119806
**	    Extending grammar for postfix operators beyond IS NULL
**	    has removed the need for the NULLOP type
**	07-Jun-2009 (kiria01) b122185
**	    Add tree tags for CASE/IF expression support for SQL
**	24-Aug-2009 (kschendel) 121804
**	    Multiple inclusion protection.
*/

/*}
** Name:    OSNODE -	OSL Parser Expression Tree Node.
**
** Description:
**	An expression tree node is a structure that holds parse-tree information
**	for OSL semantic constructs.  The various constructs and the relevant
**	node variants are as follows:
**
**	    Lists
**		COMMA, NLIST, BLANK
**
**	    Ingres references
**		tkID, tkSCONST, tkICONST, VALUE.
**
**	    OSL expressions
**		OP, UNARYOP, RELOP, NULLOP, LOGOP, tkPCALL, PARENS
**		tkSCONST, tkFCONST, tkICONST, tkNULL, tkEXEPROC
**
**	    DML expressions
**		OP, UNARYOP, RELOP, NULLOP, LOGOP, PARENS
**		tkSCONST, tkFCONST, tkICONST, tkNULL, tkXCONST, tkDCONST
**		tkID, ATTR
**		PRED
**	     Quel specific:  AGGLIST
**	     Sql specific:   tkQUERY tkCASE tkWHEN tkTHEN
**
**	    Parameters
**		ASNOP.
**
**	    Set/Inquire elements.
**		TLASSIGN, FRSCONST.
*/
typedef struct _osnode	OSNODE;

struct _osnode
{
    i4	n_token;		/* token (e.g., VALUE, OP) */
    i2	n_type;			/* DB_DT_ID type */
    i2	n_ilref;		/* IL reference number */
    i2	n_flags;		/* information about node */
#define N_READONLY	0x02 	/* can't be LHS in assignment */
#define N_ERROR		0x04	/* error already reported on this node. */
#define N_VISIBLE	0x08	/* form-display object. */
#define N_UNLOAD	0x10	/* this object being unloaded */
#define N_TABLEFLD	0x20	/* this is a tablefield or row thereof */
#define N_COMPLEXOBJ	0x40	/* this is a "complex" object (record or array);
				** (applicable to VALUE, DOT, ARRAYREF nodes) */
#define N_ROW		0x80	/* this is a record (or row of an array
				** or a row of a tablefield);
				** (applicable to VALUE, DOT, ARRAYREF nodes) */
    i2  n_prec;			/* precision (when n_type is DB_DEC_TYPE) */
    i4  n_length;		/* length; usually for ADF result length */

    union
    {
	char	*vn_const;	/* for constants */
#define		n_const	n_tag.vn_const
	struct			/* for fields references (VALUE, FLD) */
	{
	    OSSYM	*vn_sym;	/* value/symbol reference */
	    OSNODE	*vn_sexpr;	/* subscript expression */
	    OSSYM	*vn_tfref;	/* table field ref. (column) */
	    i2		vn_sref;	/* subscript IL value reference */
#define		n_sym	n_tag.n_field.vn_sym
#define		n_sexpr	n_tag.n_field.vn_sexpr
#define		n_tfref	n_tag.n_field.vn_tfref
#define		n_sref	n_tag.n_field.vn_sref
	} n_field;
	struct		/* for attribute references (ID . ID) */
	{
	    char	*vn_name;	/* object name */
	    char	*vn_attr;	/* object attribute */
#define		n_name	n_tag.n_attrib.vn_name
#define		n_attr	n_tag.n_attrib.vn_attr
	} n_attrib;
	struct		/* for non-leaf nodes */
	{
	    char	*vn_value;  /* value of this node */
	    OSNODE	*vn_left;   /* left hand child */
	    OSNODE	*vn_right;  /* right hand child */
	    i2		vn_fiid;    /* ADF function ID for node */
	    OSSYM	*vn_lhstemp; /* temp. for RELOP or NULLOP */
	    OSSYM	*vn_rhstemp; /* temp. for RELOP */
#define		n_value		n_tag.n_struct.vn_value
#define		n_left		n_tag.n_struct.vn_left
#define		n_right 	n_tag.n_struct.vn_right
#define		n_fiid		n_tag.n_struct.vn_fiid
#define		n_lhstemp	n_tag.n_struct.vn_lhstemp
#define		n_rhstemp	n_tag.n_struct.vn_rhstemp
	} n_struct;
	struct		/* for predicate elements */
	{
	    char	*vn_dbname;	/* db object name */
	    char	*vn_dbattrib;	/* db object attribute */
	    OSSYM	*vn_pfld;
#define		n_dbname	n_tag.n_prede.vn_dbname
#define		n_dbattr	n_tag.n_prede.vn_dbattrib
#define		n_pfld		n_tag.n_prede.vn_pfld
	} n_prede;
	struct		/* for predicate NPRED */
	{
	    OSNODE	*vn_plist;
	    i4		vn_pnum;
# define	n_plist		n_tag.n_pred.vn_plist
# define	n_pnum		n_tag.n_pred.vn_pnum
	} n_pred;
	struct		/* for NLIST */
	{
	    OSNODE	*vn_next;
	    OSNODE	*vn_ele;
# define	n_next		n_tag.n_list.vn_next
# define	n_ele		n_tag.n_list.vn_ele
	} n_list;
	struct		/* for AGGLIST */
	{
	    OSNODE	*vn_agexpr;
	    OSNODE	*vn_agbylist;
	    OSNODE	*vn_agqual;
# define	n_agexpr	n_tag.n_agglist.vn_agexpr
# define	n_agbylist	n_tag.n_agglist.vn_agbylist
# define	n_agqual	n_tag.n_agglist.vn_agqual
	} n_agglist;
	struct		/* for DOTALL */
	{
	    char	*vn_lhsobj; /* object assigned to */
	    char	*vn_rhsobj; /* object assigned from */
	    OSNODE	*vn_rownum; /* row num of rhs tblfld */
# define	n_lhsobj	n_tag.n_dotall.vn_lhsobj
# define	n_rhsobj	n_tag.n_dotall.vn_rhsobj
# define	n_rownum	n_tag.n_dotall.vn_rownum
# define	n_slist	n_tag.n_dotall.vn_rownum
	} n_dotall;
	struct		/* for tkBYREF */
	{
	    OSSYM	*vn_actual;
# define	n_actual	n_tag.n_byref.vn_actual
	} n_byref;
	struct		/* for tkASSIGN */
	{
	    OSSYM	*vn_lhs;
	    OSNODE	*vn_rhs;
# define	n_lhs		n_tag.n_assign.vn_lhs
# define	n_rhs		n_tag.n_assign.vn_rhs
	} n_assign;
	struct		/* for frame or procedure calls */
	{
	    OSNODE	*vn_object; /* node with frame/procedure object name */
	    OSSYM	*vn_objsym; /* symbol node for frame/procedure object */
	    i4		vn_reslen; /* result length */
	    /* A parameter list is a comma-separated list of expressions */
	    OSNODE	*vn_parlist;	/* parameter list */
#define		n_object    n_tag.n_call.vn_object
#define		n_frame	    n_tag.n_call.vn_object
#define		n_proc	    n_tag.n_call.vn_object
#define		n_fsym	    n_tag.n_call.vn_objsym
#define		n_psym	    n_tag.n_call.vn_objsym
#define		n_parlist   n_tag.n_call.vn_parlist
#define		n_rlen	    n_tag.n_call.vn_reslen
	} n_call;
	struct		/* for tkQUERY */
	{
	    /*
	    ** formobj is the form object being retrieved into.
	    */
	    OSNODE	*vn_formobj;	/* form object attribute node */
	    struct _osqry *vn_query;
	    OSNODE	*vn_child;	/* detail of master-detail query. */
	    PTR		vn_targlfrag;	/* IL frag to assign to target list */
#define		n_formobj	n_tag.n_tkquery.vn_formobj
#define		n_query		n_tag.n_tkquery.vn_query
#define		n_child		n_tag.n_tkquery.vn_child
#define		n_targlfrag	n_tag.n_tkquery.vn_targlfrag
	} n_tkquery;
	struct {
	    union {
		OSSYM	*vn_fref;	/* frs var. reference */
		u_i2	vn_fval;	/* frs var. value */
	    } vn_fsref;
	    i4		vn_fcode;	/* frs constant code */
	    i4		vn_fvalue;	/* frs constant value */
	    u_i2	vn_obj;		/* object name reference */
#define		n_setref	n_tag.n_frs.vn_fsref.vn_fval
#define		n_inqvar	n_tag.n_frs.vn_fsref.vn_fref
#define		n_frscode	n_tag.n_frs.vn_fcode
#define		n_frsval	n_tag.n_frs.vn_fvalue
#define		n_objref	n_tag.n_frs.vn_obj
	} n_frs;
	struct {
	    u_i2	vn_col;
	    u_i2	vn_tlexpr;
	    bool	vn_byref;
#define		n_coln		n_tag.n_tle.vn_col
#define		n_tlexpr	n_tag.n_tle.vn_tlexpr
#define		n_byreff	n_tag.n_tle.vn_byref
	} n_tle;
    } n_tag;
};

/*
** union of pointers so that pointers of varying types can be passed
** to the node construction functions
*/
typedef struct {
	i4	u_type;
	union {
		char		*un_p;
		char		*un_cp;
		OSSYM		*un_symp;
		OSNODE		*un_nodep;
		struct _osqry	*un_qrynodep;
	} u_tag;
} U_ARG;

# define	u_p		u_tag.un_p
# define	u_cp		u_tag.un_cp
# define	u_symp		u_tag.un_symp
# define	u_nodep		u_tag.un_nodep
# define	u_qrynodep	u_tag.un_qrynodep

GLOBALREF	U_ARG	u_ptr[3];

/*
** extern declarations for procedure referenced by gram.y
** in order to declare its return type
*/
OSNODE	*osmknode();
OSNODE	*osmkconst();
OSNODE	*osmkident();
OSNODE	*osmkassign();
OSNODE	*oscpnode();
OSNODE	*osvalnode();
OSNODE	*ostvalnode();
bool	osiserr();

OSNODE	*osdotnode();
OSNODE	*osarraynode();
OSNODE	*osqrydot();
OSNODE	*osval();
OSNODE	*os_lhs();

/*
** Name:    OSL Node Tokens.
**
**	These are used to identify nodes and are passed in to 'osmakenode()'.
**	When passed to 'osmakenode()', the can be ORed with DML to show
**	that the operator is being used in a DML (query) context.
**
**	Because DML sets a bit, all token values must be less than it, except
**	for FREE_NODE, which is never used externally to this module.
*/
#define	    DML		01000

#define	    FREE_NODE	-1
#define	    VALUE	2
#define	    ATTR	3

/* Operators */
#define	    OP		4
#define	    UNARYOP	5
#define	    RELOP	6

#define	    LOGOP	8
#define	    PARENS	9

/* Lists */
#define	    COMMA	10
#define	    NLIST	11

/* Token Nodes */
#define	    tkID	12
#define	    tkFCONST	13
#define	    tkSCONST	14
#define	    tkICONST	15
#define     tkXCONST	16
#define     tkDCONST	17
#define	    tkNULL	18

/* handy macro to check for constant nodes */
#define 	tkIS_CONST_MACRO(x)	((x) >= 13 && (x) <= 17)

/* Frame and Procedure Calls */
#define	    tkPCALL	19
#define	    tkFCALL	20
#define	    tkBYREF	21

/* Query (and DML) Constructs */
#define	    tkQUERY	22
#define	    PREDE	23
#define	    NPRED	24
#define	    AGGLIST	25
#define	    DOTALL	26

/* Set/Inquire Constructs */
#define	    TLASSIGN	27
#define	    FRSCONST	28

/**/
#define	    ASNOP	29
#define	    ASSOCOP	30
#define	    tkASSIGN	31

/* Another List type */
#define	    BLANK	32

/* record attribute de-reference */
#define	    DOT		33
#define	    ARRAYREF	34

/* EXECUTE PROCEDURE */
#define	    tkEXEPROC	35

#define     ONCLAUSE    36

#define	    tkCASE	37
#define	    tkWHEN	38
#define	    tkTHEN	39

#endif /* OSTREE_H_INCLUDED */
