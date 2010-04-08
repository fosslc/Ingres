/*
** Copyright (c) 2004 Ingres Corporation
*/

/* {
** Name:	eqtgt.h - Defs for processing statements with target lists.
**
** Description:
**	Contains the function and structure declarations and definitions
**	for callers of the eqtarget module.  These structures are used
**	in parsing and emitting code for SQL statements containing
**	target lists.  For a picture of how these data structures are
**	used, see the equel/eqtarget.c module.
**
** 	Note that eqrun.h contains the attribute definitions (because these
**	are also used by run-time routines that process target lists);
**	eqrun.h must therefore be included before eqtgt.h.
**
** History:
**	13-nov-1989	(barbara)
**	    Written for Phoenix/Alerters
**	01-nov-1992 	(kathryn)
**	    Update MAXTL_ELEMS - the INQUIRE_SQL statement has the highest
**	    number of valid attributes now at 34.
**	10-dec-1992 (kathryn)
**	    Removed prototype for eqtl_setelem() - function deleted.
**      21-dec-1992 (larrym)
**          Up'd MAXTL_ELEMS to 35 after adding CONNECTION_TARGET
**      9-nov-93 (robf)
**          Up'd MAXTL_ELEMS to 37 after adding DBPROMPTHANDLER,
**	    SAVEPASSWORD
**      06-mar-1996 (thoda04)
**          Added function and argument prototypes for
**          eqtl_init() and eqtl_add()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* {
** Name:  TL_ATTRS - Information on a target list attribute
**
** Description:
**      This structure describes an attribute.  It is used as a lookup to
**      validate the user's target list.  For example, for each element in
**      a target list, is the attribute valid in the enclosing statement,
**      is the type of the user var/val compatible?
**
** History
**      13-nov-1989     (barbara)
**          Written for Phoenix/Alerters
**	15-mar-1991	(kathryn)
**	    Moved this struct from equel/eqtarget.c to this file.
*/

typedef struct {
    char        *tla_string;    /* Attribute string */
    i4          tla_id;         /* Attribute id */
    i4          tla_type;       /* Attribute type */
    } TL_ATTRS;

/* {
** Name		TL_ELEM - Description of an element of a target list
**
** Description:
**	This structure defines an element of a target list.  It is
**	used to save away information about an item in a target
**	list for later code generation.  A list of target list elements
**	is contained in the TL_DESC structure.
**
** History:
**	13-nov-1989	(barbara)
**	    Written for Phoenix/Alerters
*/

typedef struct {
    char	*tle_val;	/* User variable name or value */
    PTR		tle_var;	/* Variable pointer */
    char	*tle_indname;	/* User null indicator name */
    PTR		tle_indvar;	/* Indicator pointer */	
#define		   TL_ATTR_UNDEF	-1	/* Attribute not defined */
    i4		tle_type;	/* Type of non-variable value */
    i4		tle_attr;	/* Code for attrib (defined in eqrun.h */
} TL_ELEM;

/*
** Definition of attributes is in eqrun.h.  The following constant must
** be set to the maximum number of attributes that any one statement can
** have.
*/
# define	MAXTL_ELEMS	37


/*{
** Name:	TL_DESC - Description of a target list
**
** Description:
**	This structure defines the target list and its elements.  It
**	is initialized and assigned values when the parser processes
**	a target list.  When the statement containing the target list
**	has been parsed, this structure is used to generate the code.
**	When using this structure for code generation, callers should
**	not generate any code if the tld_error field is set to TRUE.
**
** History:
**	13-nov-1989	(barbara)
**	    Written for Phoenix/Alerters
**	15-mar-1991	(kathryn)
**	    Added tld_nelems (number of elements) and tld_attrtbl
**	    (pointer to correct attribute table).
*/

typedef struct {
    i4		tld_stmtid;	/* Id of stmt containing target list */
    bool	tld_error;	/* TRUE indicates error with stmtid */
    i4		tld_nelems; 	/* Number of elements in tld_elem */
    TL_ATTRS	*tld_attrtbl;	/* Attribute table */
    TL_ELEM	tld_elem[MAXTL_ELEMS];	/* Target list elements */
} TL_DESC;

/* PROTOTYTPES */

/*
** eqtarget.c:
*/

FUNC_EXTERN  VOID       eqtl_init(i4 stmt_id);
FUNC_EXTERN  TL_ELEM  	*eqtl_elem(i4 tlid);
FUNC_EXTERN  VOID       eqtl_add(char *vname, PTR vsym, char *iname,
                                 PTR isym, i4  vtype, char *attr);
FUNC_EXTERN  TL_DESC *	eqtl_get(void);


