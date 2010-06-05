/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <cm.h>
#include    <cv.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <st.h>
#include    <me.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

/**
**
**  Name: PSTPRMDMP.C - Functions used to print the contents of a query tree.
**
**  Description:
**      This file contains the functions necessary to format and print the 
**      contents of a query tree.
**
**          pst_prmdump - Format and print the contents of a query tree.
**	    pst_1prmdmp - Format and print a mini version of the query tree.
**	    pst_sccdump - Send an output line to terminal.
**	    pst_display - Send an output line to terminal.
**          pst_dbpdump - Format and print the contents of a database procedure
**			  query tree.
**
**
**  History:
**      03-oct-85 (jeff)    
**          written
**	18-mar-87 (stec)
**	    Added pst_resjour handling.
**	26-mar-87 (daved)
**	    added a compact form of tree formatting
**	13-jul-87 (stec)
**	    changed calls to uld_prtree, pst_prmdump interface.
**	19-jan-88 (stec)
**	    Changed code printing out pst_resloc (multiple locs).
**	09-feb-88 (stec)
**	    Improved pst_1prmdump.
**	10-feb-88 (stec)
**	    Copied psf_scctrace, psf_display from PSFDEBUG, renamed to pst_...
**	    to make it independent for OPC, which links it in.
**	17-feb-88 (stec)
**	    Corrected code errors detected by Lint.
**	22-feb-88 (stec)
**	    Change pst_scctrace.
**	26-sep-88 (stec)
**	    Fixed IBM compiler errors/warnings.
**	29-sep-88 (stec)
**	    pst_prmdump needs to process unions only for ROOT nodes.
**	14-nov-88 (stec)
**	    Initialize ADF_CB.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	24-may-90 (stec)
**	    Modified pst_1prnode() to mark non-printing resdoms.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  27-feb-91 (ralph)
**	    Tracing for DESTINATION of MESSAGE and RAISE ERROR statements
**	  merged from 6.4:  6-mar-1991 (bryanp)
**	    Changed pst_compress from bool to flag word; made the appropriate
**	    changes here.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	09-nov-92 (rblumer)
**	    Added output for new field pst_statementEndRules to pst_dbpdump.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	23-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Format new node types in pst_dbpdump
**	20-apr-93 (rblumer)
**	    added handling for CREATE_TABLE and CREATE_INTEGRITY nodes;
**	    got rid of some complier warnings.
**	23-mar-93 (smc)
**	    Commented out text following endif.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed unnecessary external func declarations
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in pst_dbpdump(),
**	    pst_display().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	10-Jan-2001 (jenjo02)
**	    ADF_CB* is know in session control block; use it 
**	    instead of SCU_INFORMATION calls.
**	8-Jun-2006 (kschendel)
**	    Update PS145 node type strings, and use expanded header dump
**	    for PS145 as well as 144.  (I'm considering ditching the latter,
**	    but will leave it alone for now.)  Make many routines static.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	05-Feb-2009 (kiria01) b121607
**	    Having defined pst_type as enum with table macro to keep things
**	    in sync we now build the tag texts from the table macro.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Defines
*/
#define	    PST_1WIDTH	    3
#define	    PST_1HEIGHT	    3
#define	    PST_BUF_SIZE    (DB_MAXNAME + 3)
#define	    PST_MAX_STR_LEN (PST_BUF_SIZE - 1)

/*
**  Forward and/or External function references.
*/
static void pst_hddump(PST_QTREE *header);

static void pst_fblank(
	char               *buf,
	i4		    max_str_len);
static void pst_f_joinid(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_f_1joinid(
	PST_QNODE          *qnode,
	char               *buf);
static void pst_fccol(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fcid(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fcnm(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fdlen(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fdprec(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fdtype(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fdval(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fidop(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_finop(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_flcv(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fnlen(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fnmat(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fnoat(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fparm(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fprmtyp(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_frcv(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_frdno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_frname(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_frtuser(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_ftgno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_ftgtyp(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_ftype(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fupdt(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fvars(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);
static void pst_fvno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len);

static PTR
pst_left(
	PTR                node);
static PTR
pst_right(
	PTR                node);
static VOID
pst_prnode(
	PTR                node,
	PTR                control);
static VOID
pst_1prnode(
	PTR                node,
	PTR                control);

/* Local static data. */

/* Table of 3-letter abbreviations for pst_type codes.*/
static const char pst_typestr3[][4] = {
#define _DEFINE(n,s,v) {#s},
#define _DEFINEEND
    PST_TYPES_MACRO {""}
#undef _DEFINE
#undef _DEFINEEND
};


/*{
** Name: pst_prmdump	- Format and print the contents of a query tree.
**
** Description:
**      The pst_prmdump function formats and prints the contents of a query 
**      tree and/or a query tree header.  The output will go to the output
**	terminal and/or file named by the user in the "SET TRACE TERMINAL" and
**	"SET TRACE OUTPUT" commands.
**
** Inputs:
**      tree                            Pointer to the query tree
**					(NULL means don't print it)
**	header				Pointer to the query tree header
**					(NULL means don't print it)
**      error                           Error block filled in in case of error
**	facility			Facility
**
** Outputs:
**	error				Error block filled in in case of error
**	    .err_code			    What the error was
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Prints formatted query tree on terminal and/or into file.
**
** History:
**	03-oct-85 (jeff)
**          written
**	29-sep-88 (stec)
**	    pst_prmdump needs to process unions only for ROOT nodes.
*/
DB_STATUS
pst_prmdump(
	PST_QNODE           *tree,
	PST_QTREE	    *header,
	DB_ERROR	    *error,
	i4		    facility)
{
    error->err_code = E_PS0000_OK;

    if (header != (PST_QTREE *) NULL)
    {
	pst_hddump(header);
    }

    /* Assume it is a ROOT node and get ready to process unions,
    ** if it's other type just exit after first iteration.
    */
    for (;tree != (PST_QNODE *) NULL; 
	 tree = tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next
	)
    {
	(VOID) pst_display("\nQuery tree:\n");
	uld_prtree((PTR) tree, pst_prnode, pst_left, pst_right,
	    DB_MAXNAME / 2 + 3 , 13, facility);
	
	/* Exit if not a ROOT node */
	if (tree->pst_sym.pst_type != PST_ROOT)
	    break;
    }

    return    (E_DB_OK);
}

/*{
** Name: pst_1prmdump	- Format and print the contents of a query tree.
**
** Description:
**      The pst_prmdump function formats and prints the contents of a query 
**      tree and/or a query tree header.  The output will go to the output
**	terminal and/or file named by the user in the "SET TRACE TERMINAL" and
**	"SET TRACE OUTPUT" commands.
**
** Inputs:
**      tree                            Pointer to the query tree
**					(NULL means don't print it)
**	header				Pointer to the query tree header
**					(NULL means don't print it)
**      error                           Error block filled in in case of error
**
** Outputs:
**	error				Error block filled in in case of error
**	    .err_code			    What the error was
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Prints formatted query tree on terminal and/or into file.
**
** History:
**	26-mar-87 (daved)
**	    written
**	09-feb-88 (stec)
**	    Print UNION, UNION ALL; use pst_display.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as a variable length
**	    array of pointers (some of which may be set to NULL) to PST_RNGENTRY
**	    structure.
*/
DB_STATUS
pst_1prmdump(
	PST_QNODE          *tree,
	PST_QTREE	   *header,
	DB_ERROR	    *error)
{
    if (error != NULL)
	error->err_code = E_PS0000_OK;

    if (header != (PST_QTREE *) NULL)
    {
	pst_hddump(header);
    }

    for (;;)
    {
	(VOID) pst_display("\nQuery tree:\n");
	uld_prtree((PTR) tree, pst_1prnode, pst_left, 
	    pst_right, PST_1HEIGHT, PST_1WIDTH, DB_PSF_ID);

	if (tree->pst_sym.pst_type == PST_ROOT)
	{
	    if (!tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	    {
		break;
	    }
	    if (tree->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
		(VOID) pst_display("\nUNION\n");
	    else
		(VOID) pst_display("\nUNION ALL\n");

	    tree = tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
	}
	else
	{
	    break;
	}
    }

    if (header != NULL && (header->pst_mask1 & PST_RNGTREE))
    {
	i4	i;

	for (i = 0; i < header->pst_rngvar_count; i++)
	{
	    if (header->pst_rangetab[i] == (PST_RNGENTRY *) NULL ||
		header->pst_rangetab[i]->pst_rgtype != PST_RTREE)
		continue;

	    tree = header->pst_rangetab[i]->pst_rgroot;
	    (VOID) pst_display("\n\t\tRANGE TREE [%d]\n", (i4) i);
	    (VOID) pst_display("\n\t\tPARENT [%d]\n", 
		(i4) header->pst_rangetab[i]->pst_rgparent);

	    for (; tree != (PST_QNODE *) NULL;)
	    {
		(VOID) pst_display("\nQuery tree:\n");
		uld_prtree((PTR) tree, pst_1prnode, 
		    pst_left, pst_right, PST_1HEIGHT, PST_1WIDTH, DB_PSF_ID);

		if (!tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    break;
		}
		if (tree->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
		    (VOID) pst_display("\nUNION\n");
		else
		    (VOID) pst_display("\nUNION ALL\n");

		tree = tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
	    }
	}

	(VOID) pst_display("\n\n");

    }

    return    (E_DB_OK);
}

/*{
** Name: pst_left	- Return left child of node
**
** Description:
**      This function returns the left child of a query tree node.
**
** Inputs:
**      node                            Pointer to the node
**
** Outputs:
**      None
**	Returns:
**	    Pointer to left child, NULL if none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-may-86 (jeff)
**          written
*/
static PTR
pst_left(
	PTR                node)
{
    return ((PTR) (((PST_QNODE *) node)->pst_left));
}

/*{
** Name: pst_right	- Return right child of node
**
** Description:
**      This function returns the right child of a query tree node
**
** Inputs:
**      node
**
** Outputs:
**      None
**	Returns:
**	    Pointer to right child, NULL if none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-may-86 (jeff)
**          written
*/
static PTR
pst_right(
	PTR                node)
{
    return ((PTR) (((PST_QNODE *) node)->pst_right));
}

/*{
** Name: pst_prnode	- Format a query tree node for the tree formatter
**
** Description:
**      This function is used by the tree formatter to print a query tree
**	node.  The formatter stores up formatted nodes; therefore, we must
**	send strings to it.  To avoid redundancy, this function calls other
**	functions which return the strings to be formatted, and then sends
**	the strings to the tree formatter.
**
** Inputs:
**      node                            Pointer to the node to format
**      control                         Pointer to the control structure for
**					the tree formatter.
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-may-86 (jeff)
**          written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	18-sep-89 (andre)
**	    Show group id of operator nodes.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    Change interface to functions used to display node info for ps144 to
**	    enable caller to specify the size of the buffer.  This change was
**	    introduced to display the contents of pst_lvrm and pst_rvrm in
**	    PST_ROOT, PST_SUBSEL, and PST_AGHEAD.  After making the changes, I
**	    realized that the fields in question are not set inside PSF, so
**	    displaying them is of questionable value.  I chose to preserve the
**	    interface changes in case we ever decide to vary the width of
**	    display box for different nodes.
**	3-sep-99 (inkdo01)
**	    Added new nodes for case function.
*/
static VOID
pst_prnode(
	PTR                node,
	PTR                control)
{
    char                buf[PST_BUF_SIZE];   /* Big enough for anything */
    i4			lc = 0;
    register PST_QNODE	*qnode = (PST_QNODE *) node;
    
    /* Top of the box is all stars */
    MEfill(PST_MAX_STR_LEN, '*', buf);
    buf[PST_MAX_STR_LEN] = '\0';
    uld_prnode(control, buf);
    
    /* Now do the node type */
    pst_ftype(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the datatype for the node */
    pst_fdtype(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the data length for the node */
    pst_fdlen(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the precision for the node */
    pst_fdprec(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the data value for the node (may be truncated) */
    pst_fdval(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the node length for the node */
    pst_fnlen(qnode, buf, PST_MAX_STR_LEN);
    uld_prnode(control, buf);

    /* Now do the symbol for the node */
    switch (qnode->pst_sym.pst_type)
    {
    case PST_AND:       /* PST_OP_NODE */
    case PST_OR:        /* PST_OP_NODE */
	/* Print join id */
	pst_f_joinid(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);
	lc = 1;			    /*
				    ** indicate that one line has been used and
				    ** drop through to produce some blank lines
				    */
    case PST_UOP:       /* PST_OP_NODE */
    case PST_BOP:       /* PST_OP_NODE */
    case PST_COP:       /* PST_OP_NODE */
    case PST_AOP:       /* PST_OP_NODE */
    case PST_MOP:       /* PST_OP_NODE */
	/* Do the operator id */
	pst_fidop(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the function instance id */
	pst_finop(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the left conversion id, if any */
	pst_flcv(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the right conversion id, if any */
	pst_frcv(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Print join id */
	pst_f_joinid(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);
	break;

    case PST_CONST:         /* PST_CNST_NODE */
	/* Do the parameter type */
	pst_fprmtyp(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the parameter number */
	pst_fparm(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do 3 blank lines */
	pst_fblank(buf, PST_MAX_STR_LEN);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	break;

    case PST_RESDOM:        /* PST_RSDM_NODE */
    case PST_BYHEAD:        /* PST_RSDM_NODE */
	/* Do the result domain number */
	pst_frdno(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the pst_ntargno number */
	pst_ftgno(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the pst_ttargtype number */
	pst_ftgtyp(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the "for update" flag */
	pst_fupdt(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the result name */
	pst_frname(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);
	break;

    case PST_VAR:       /* PST_VAR_NODE */
	/* Do the variable number */
	pst_fvno(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the attribute number */
	pst_fnoat(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the attribute name */
	pst_fnmat(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do 2 blank lines */
	pst_fblank(buf, PST_MAX_STR_LEN);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	break;

    case PST_ROOT:      /* PST_RT_NODE */
    case PST_SUBSEL:    /* PST_RT_NODE */
    case PST_AGHEAD:    /* PST_RT_NODE */
	/* Do the number of range vars */
	pst_fvars(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the root user flag */
	pst_frtuser(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do 3 blank lines */
	pst_fblank(buf, PST_MAX_STR_LEN);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	break;

    case PST_CURVAL:    /* PST_CRVAL_NODE */
	/* Do the cursor id */
	pst_fcid(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the cursor name */
	pst_fcnm(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do the attribute number */
	pst_fccol(qnode, buf, PST_MAX_STR_LEN);
	uld_prnode(control, buf);

	/* Do 2 blank lines */
	pst_fblank(buf, PST_MAX_STR_LEN);
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	break;

    case PST_RULEVAR:	/* PST_RL_NODE */
    case PST_SORT:      /* PST_SRT_NODE */
    case PST_TAB:       /* PST_TAB_NODE */
    case PST_SEQOP:     /* PST_SEQOP_NODE */
    case PST_GBCR:      /* PST_GROUP_NODE */
    case PST_CASEOP:    /* PST_CASE_NODE */
    case PST_OPERAND:
    case PST_GSET:
    case PST_GCL:
    case PST_QLEND:
    case PST_TREE:
    case PST_NOT:
    case PST_WHLIST:
    case PST_WHOP:
    default:
	/* 5 blank lines */
	pst_fblank(buf, PST_MAX_STR_LEN);
	for (;lc < 5; lc++)
	{
	    uld_prnode(control,buf);
	}
	break;
   }

    /* Top of the box is all stars */
    MEfill(PST_MAX_STR_LEN, '*', buf);
    buf[PST_MAX_STR_LEN] = '\0';
    uld_prnode(control, buf);

    return;
}

/*{
** Name: pst_1prnode	- Format a query tree node for the tree formatter
**
** Description:
**      This function is used by the tree formatter to print a query tree
**	node.  The formatter stores up formatted nodes; therefore, we must
**	send strings to it.  To avoid redundancy, this function calls other
**	functions which return the strings to be formatted, and then sends
**	the strings to the tree formatter.
**
** Inputs:
**      node                            Pointer to the node to format
**      control                         Pointer to the control structure for
**					the tree formatter.
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-mar-87 (daved)
**          written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	19-sep-89 (andre)
**	    print join id for operator nodes.  There will be 3 lines
**	    (blank or otherwise) printed below each node
**	24-may-90 (stec)
**	    Modified to mark non-printing resdoms.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
*/
static VOID
pst_1prnode(
	PTR                node,
	PTR                control)
{
    char                buf[PST_BUF_SIZE];   /* Big enough for anything */
    char		blanks[PST_BUF_SIZE];
    bool		opnode = FALSE;
    register PST_QNODE	*qnode = (PST_QNODE *) node;

    pst_1ftype(qnode, buf);
    buf[PST_1WIDTH] = '\0';
    uld_prnode(control, buf);

    MEfill(sizeof(buf), ' ', buf);
    buf[PST_1WIDTH] = '\0';
    MEfill(sizeof(blanks), ' ', blanks);
    blanks[PST_1WIDTH] = '\0';

    /* Now do the symbol for the node */
    switch (qnode->pst_sym.pst_type)
    {

    case PST_UOP:
    case PST_BOP:
    case PST_COP:
    case PST_AOP:
	opnode = TRUE;
	pst_1fidop(qnode, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control,buf);
        CVla(qnode->pst_sym.pst_value.pst_s_op.pst_opno, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control,buf);
	pst_f_1joinid(qnode, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control,buf);
	break;

    case PST_AND:
    case PST_OR:
	opnode = TRUE;
	pst_f_1joinid(qnode, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control,buf);
        uld_prnode(control, blanks);
        uld_prnode(control, blanks);
	break;

    case PST_VAR:
	/* Do the variable number */
	CVla(qnode->pst_sym.pst_value.pst_s_var.pst_vno, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control, buf);

	/* Do the attribute number */
	CVla(qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control, buf);
	break;

    case PST_RESDOM:
    {
	char	    *str;

	/* the resdom number and the target type */
	CVla(qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno, buf);
	buf[PST_1WIDTH] = '\0';
	uld_prnode(control, buf);
	if (qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	{
	    switch (qnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
	    {
		case PST_ATTNO:
		    str = "ATT";
		    break;

		case PST_LOCALVARNO:
		    str = "LVR";
		    break;

		case PST_RQPARAMNO:
		    str = "RQP";
		    break;

		case PST_USER:
		    str = "USR";
		    break;

		case PST_DBPARAMNO:
		    str = "DBP";
		    break;

		case PST_BYREF_DBPARAMNO:
		    str = "BDP";
		    break;

		case PST_RSDNO:
		    str = "RSD";
		    break;

		default:
		    str = "???";
		    break;
	    }
	}
	else
	{
	    /* Mark non-printing resdom */
	    str = "NPT";
	}

	uld_prnode(control, str);
	break;
    }
    case PST_RULEVAR:	/* Nothing special yet */
    default:
	/* 2 blank lines */
	uld_prnode(control,buf);
	uld_prnode(control,buf);
	break;

    }
    
    /* For non-operator nodes, print an extra blank line */
    if (!opnode)
    {
        uld_prnode(control, blanks);
    }
}

/*{
** Name: pst_ftype	- Format a node type
**
** Description:
**      This function formats a node type for debug printing within a query
**	tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted node type
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted node type
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
**	3-sep-99 (inkdo01)
**	    Added new nodes for case function.
*/
static VOID
pst_ftype(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    static const char *ids[] = {
#define _DEFINE(n,s,v) "PST_" #n,
#define _DEFINEEND
        PST_TYPES_MACRO 0
#undef _DEFINE
#undef _DEFINEEND
                        };
    const char *string;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    if (qnode->pst_sym.pst_type < PST_MAX_ENUM)
	string = ids[qnode->pst_sym.pst_type];
    else
	string = "UNKNOWN TYPE";

    STmove(string, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fdtype	- Format a datatype id
**
** Description:
**      This function formats a datatype id for inclusion in a formatted query
**	tree node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**	buf				Place to put the formatted datatype id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted datatype id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fdtype(
	PST_QNODE          *node,
	char               *buf,
	i4		    max_str_len)
{
    ADI_DT_NAME         adt_name;
    ADF_CB		*adf_scb;
    PSS_SESBLK		*sess_cb;
    PTR			ad_errmsgp;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Get the ADF session control block */
    sess_cb = psf_sesscb();
    adf_scb = (ADF_CB*)sess_cb->pss_adfcb;

    /* Get datatype name */
    ad_errmsgp = adf_scb->adf_errcb.ad_errmsgp;
    adf_scb->adf_errcb.ad_errmsgp = NULL;
    if (adi_tyname(adf_scb, node->pst_sym.pst_dataval.db_datatype, &adt_name)
	== E_DB_OK)
    {
	STmove(adt_name.adi_dtname, ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	STmove("Bad Datatype", ' ', max_str_len - 2, buf + 1);
    }

    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

    return;
}

/*{
** Name: pst_f_joinid - Format the group (join) id for an operator node.
**
** Description:
**      This function formats the group (join) id of an operator node for
**	printing of a query tree node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted length
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted join id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-sep-89 (andre)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:
**	    added new parameter - max_str_len
*/
static VOID
pst_f_joinid(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    MEcopy ((PTR) "Group ID = ", sizeof("Group ID = ") - 1, (PTR) numbuf);
    CVna((i4) qnode->pst_sym.pst_value.pst_s_op.pst_joinid,
	 numbuf + sizeof("Group ID = ") - 1);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_f_1joinid - Format the group (join) id for an operator node.
**
** Description:
**      This function formats the group (join) id of an operator node for
**	printing of a query tree node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted length
**
** Outputs:
**      buf                             Filled with the formatted join id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-sep-89 (andre)
**          written
*/
static VOID
pst_f_1joinid(
	PST_QNODE          *qnode,
	char               *buf)
{
    char                numbuf[30];

    CMcpychar("J", numbuf);
    CVna((i4) qnode->pst_sym.pst_value.pst_s_op.pst_joinid,
	 numbuf + sizeof("J") - 1);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', PST_MAX_STR_LEN - 2, buf);
    return;
}

/*{
** Name: pst_fdlen	- Format the datatype length
**
** Description:
**      This function formats the datatype length in a query tree node for
**	printing of a query tree node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted length
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted length
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fdlen(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_dataval.db_length, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fdprec	- Format the precision for a query tree node
**
** Description:
**      This function formats the precision in a query tree node for the
**	formatted printing of the node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted precision
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted precision
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fdprec(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the scale to a string */
    STprintf(numbuf, "%d(%d,%d)",
		qnode->pst_sym.pst_dataval.db_prec,
		DB_P_DECODE_MACRO(qnode->pst_sym.pst_dataval.db_prec),
		DB_S_DECODE_MACRO(qnode->pst_sym.pst_dataval.db_prec));

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fdval	- Format a data value for a query tree node
**
** Description:
**      This function formats a data value for a query tree node, to be
**	printed as part of the node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted data value
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted data value
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	24-mar-87 (stec)
**	    Fixed data value printing.
**	14-sep-87 (stec)
**	    adc_tmcvt needs to be called with pointer to adf_cb.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
**	20-apr-93 (rblumer)
**	  change adf_scb from PTR to ADF_CB*
*/
static VOID
pst_fdval(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    ADF_CB		*adf_scb;
    PSS_SESBLK		*sess_cb;
    DB_DATA_VALUE	out_value;
    char		out_string[DB_MAXTUP];
    i4			outlen;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Get the ADF session control block */
    sess_cb = psf_sesscb();
    adf_scb = (ADF_CB*)sess_cb->pss_adfcb;

    /* don't need to set output datatype or precision to use adc_tmcvt	    */
    out_value.db_length = sizeof(out_string);
    out_value.db_data = (PTR) out_string;

    /* Convert to display format */
    if (qnode->pst_sym.pst_dataval.db_datatype == DB_NODT)
    {
	STmove("Invalid datatype", ' ', max_str_len - 2, buf + 1);
    }
    else if (qnode->pst_sym.pst_dataval.db_data == NULL)
    {
	STmove("No data", ' ', max_str_len - 2, buf + 1);
    }
    else if (adc_tmcvt(adf_scb, &qnode->pst_sym.pst_dataval,
	&out_value, &outlen) != E_AD0000_OK)
    {
	STmove("Convert error", ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	/* Move the diplay string to the output buffer */
	MEmove((i4) outlen, (char *) out_value.db_data,
	    ' ', max_str_len - 2, buf + 1);
    }

    return;
}

/*{
** Name: pst_fnlen	- Format the node length of a query tree node
**
** Description:
**      This function formats the node length of a query tree node for
**	display when printing the node.
**
** Inputs:
**      qnode                           Pointer to the query tree node
**      buf                             Place to put the formatted length
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with the formatted length
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fnlen(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_len, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fblank	- Format a blank line for a query tree node
**
** Description:
**      This function formats a blank line for printing a query tree node.
**
** Inputs:
**      buf                             Place to put the blank line
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with blank line
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fblank(
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';
    MEfill(max_str_len - 2, ' ', buf + 1);

    return;
}

/*{
** Name: pst_fidop	- Format an operator id for a query tree node
**
** Description:
**      This function formats a query tree node for printing a query tree
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted operator id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted operator id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fidop(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    ADI_OP_NAME         opname;
    ADF_CB		*adf_scb;
    PSS_SESBLK		*sess_cb;
    PTR			ad_errmsgp;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Get the ADF session control block */
    sess_cb = psf_sesscb();
    adf_scb = (ADF_CB*)sess_cb->pss_adfcb;

    ad_errmsgp = adf_scb->adf_errcb.ad_errmsgp;
    adf_scb->adf_errcb.ad_errmsgp = NULL;
    if (adi_opname(adf_scb, qnode->pst_sym.pst_value.pst_s_op.pst_opno,
	&opname) != E_DB_OK)
    {
	STmove("BAD OP ID", ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	MEmove(DB_TYPE_MAXLEN, opname.adi_opname, ' ', max_str_len - 2, buf + 1);
    }
    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

    return;
}

/*{
** Name: pst_1idop	- Format an operator id for a small query tree node
**
** Description:
**      This function formats a query tree node for printing a small query tree
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted operator id
**
** Outputs:
**      buf                             Filled with formatted operator id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
*/
VOID
pst_1fidop(
	PST_QNODE          *qnode,
	char               *buf)
{
    ADI_OP_NAME         opname;
    ADF_CB		*adf_scb;
    PSS_SESBLK		*sess_cb;
    PTR			ad_errmsgp;

    /* Get the ADF session control block */
    sess_cb = psf_sesscb();
    adf_scb = (ADF_CB*)sess_cb->pss_adfcb;

    /* Create box */
    buf[PST_MAX_STR_LEN] = '\0';

    ad_errmsgp = adf_scb->adf_errcb.ad_errmsgp;
    adf_scb->adf_errcb.ad_errmsgp = NULL;
    if (adi_opname(adf_scb, qnode->pst_sym.pst_value.pst_s_op.pst_opno,
	&opname) != E_DB_OK)
    {
	MEcopy("BAD OP ID", 9, buf);
    }
    else
    {
	MEcopy(opname.adi_opname, DB_TYPE_MAXLEN, buf);
    }

    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;
}

/*{
** Name: pst_finop	- Format a function instance id for display
**
** Description:
**      This function formats a function instance id for display as part
**	of a query tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_finop(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_op.pst_opinst, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_flcv	- Format the left conversion id for display
**
** Description:
**      This function formats the left conversion id of an operator node
**	for display as part of a query tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_flcv(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Only format the left conversion id for those nodes that have them */
    if (qnode->pst_sym.pst_type == PST_BOP || qnode->pst_sym.pst_type == PST_UOP
	|| qnode->pst_sym.pst_type == PST_AOP)
    {
	/* Convert the number to a string */
	CVla((i4) qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid, numbuf);
	/* Store it in the return buffer */
	STmove(numbuf, ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	STmove("NO LEFT CVRS", ' ', max_str_len - 2, buf + 1);
    }

    return;
}

/*{
** Name: pst_frcv	- Format the right conversion id for display
**
** Description:
**      This function formats the right conversion id of an operator node
**	for display as part of a query tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_frcv(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Only format the left conversion id for those nodes that have them */
    if (qnode->pst_sym.pst_type == PST_BOP)
    {
	/* Convert the number to a string */
	CVla((i4) qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid, numbuf);
	/* Store it in the return buffer */
	STmove(numbuf, ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	STmove("NO RIGHT CVRS", ' ', max_str_len - 2, buf + 1);
    }

    return;
}

/*{
** Name: pst_fprmtyp	- Format a parameter type for display
**
** Description:
**      This function formats a parameter type in a query tree constant node
**	for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted param type
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted param type
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-88 (stec)
**          written
**	26-sep-88 (stec)
**	    Fixed IBM compiler errors/warnings (name conflicted
**	    with pst_fparm - first 8 chars).
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
*/
static VOID
pst_fprmtyp(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                *string;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    switch (qnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype)
    {
	case PST_ATTNO:
	    string = "PST_ATTNO";
	    break;

	case PST_LOCALVARNO:
	    string = "PST_LOCALVARNO";
	    break;

	case PST_RQPARAMNO:
	    string = "PST_RQPARAMNO";
	    break;

	case PST_USER:
	    string = "PST_USER";
	    break;

	case PST_DBPARAMNO:
	    string = "PST_DBPARAMNO";
	    break;

	case PST_BYREF_DBPARAMNO:
	    string = "PST_BYREF_DBPARAMNO";
	    break;

	case PST_RSDNO:
	    string = "PST_RSDNO";
	    break;

	default:
	    string = "UNKNOWN TYPE";
	    break;
    }

    STmove(string, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fparm	- Format a parameter number for display
**
** Description:
**      This function formats a parameter number in a query tree constant node
**	for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted param number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted param number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fparm(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_frdno	- Format a result domain number for display
**
** Description:
**      This function formats a result domain number in a query tree result
**	domain node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted resdom number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted resdom number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_frdno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_ftgno	- Format a pst_ntargno number for display
**
** Description:
**      This function formats a pst_ntargno in a query tree result
**	domain node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-88 (stec)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_ftgno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_rsdm.pst_ntargno, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_ftgtyp	- Format a pst_ttargtype number for display
**
** Description:
**      This function formats a pst_ttargtype in a query tree result
**	domain node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-88 (stec)
**          written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
*/
static VOID
pst_ftgtyp(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                *string;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    switch (qnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
    {
	case PST_ATTNO:
	    string = "PST_ATTNO";
	    break;

	case PST_LOCALVARNO:
	    string = "PST_LOCALVARNO";
	    break;

	case PST_RQPARAMNO:
	    string = "PST_RQPARAMNO";
	    break;

	case PST_USER:
	    string = "PST_USER";
	    break;

	case PST_DBPARAMNO:
	    string = "PST_DBPARAMNO";
	    break;

	case PST_BYREF_DBPARAMNO:
	    string = "PST_BYREF_DBPARAMNO";
	    break;

	case PST_RSDNO:
	    string = "PST_RSDNO";
	    break;

	default:
	    string = "UNKNOWN TYPE";
	    break;
    }

    STmove(string, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fupdt	- Format a result domain "for update" flag
**
** Description:
**      This function formats the "for update flag" in a query tree
**	result domain node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted conversion id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted conversion id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fupdt(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Say "for update" if for update, nothing if not */
    if (qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt)
    {
	STmove("FOR UPDATE", ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	MEfill(max_str_len - 2, ' ', buf + 1);
    }

    return;
}

/*{
** Name: pst_frname	- Format the result name of a result domain for display
**
** Description:
**      This function formats the result name in a query tree
**	result domain node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted conversion id
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted conversion id
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_frname(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    MEmove(DB_ATT_MAXNAME, qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname, ' ',
	max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fvno	- Format the variable number of a var node for display
**
** Description:
**      This function formats the variable number in a query tree
**	var node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted variable number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fvno(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_var.pst_vno, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fnoat	- Format the attribute number of a var node for display
**
** Description:
**      This function formats the attribute number in a query tree
**	var node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fnoat(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fnmat	- Format the attribute name of a var node for display
**
** Description:
**      This function formats the attribute name in a query tree
**	var node for display as part of the node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute name
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fnmat(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    MEmove(DB_ATT_MAXNAME,
	qnode->pst_sym.pst_value.pst_s_var.pst_atname.db_att_name,
	' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fvars	- Format the number of vars in a query tree root node
**
** Description:
**      This function formats for display the number of variables in a query
**	tree as stored in a query tree root node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fvars(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_root.pst_tvrc, numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_frtuser	- Format the root user flag in a query tree root node
**
** Description:
**      This function formats for display the root user flag in the root node
**	of a query tree.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_frtuser(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    if (qnode->pst_sym.pst_value.pst_s_root.pst_rtuser)
    {
	STmove("USER QRY", ' ', max_str_len - 2, buf + 1);
    }
    else
    {
	STmove("NOT USER QRY", ' ', max_str_len - 2, buf+ 1);
    }

    return;
}

/*{
** Name: pst_fcid	- Format the cursor id in a current-value node
**
** Description:
**      This function formats for display the cursor id in a current-value query
**	tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fcid(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[60];
    char		*place;

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the first part of the id to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_curval.pst_cursor.db_cursor_id[0],
	numbuf);

    /* Put in a comma */
    place = numbuf + STlength(numbuf);
    *place = ',';

    /* Convert the second part of the id to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_curval.pst_cursor.db_cursor_id[1],
	place + 1);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fcnm	- Format the cursor name in a current-value node
**
** Description:
**      This function formats for display the cursor name in a current-value
**	query tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fcnm(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Copy the name into the buffer */
    MEmove(DB_CURSOR_MAXNAME,
	qnode->pst_sym.pst_value.pst_s_curval.pst_cursor.db_cur_name,
	' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_fccol	- Format the cursor attribute number in a curval node
**
** Description:
**      This function formats for display the attribute number in a
**	current-value query tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put variable number
**	max_str_len			Maximum length of a string (not counting
**					EOS) which may be placed into buf
**
** Outputs:
**      buf                             Filled with formatted attribute number
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-86 (jeff)
**          written
**	11-nov-91 (rblumer)
**	  merged from 6.4:  02-jul-91 (andre)
**	    added new parameter - max_str_len
*/
static VOID
pst_fccol(
	PST_QNODE          *qnode,
	char               *buf,
	i4		    max_str_len)
{
    char                numbuf[30];

    /* Create box */
    buf[0] = buf[max_str_len - 1] = '*';
    buf[max_str_len] = '\0';

    /* Convert the number to a string */
    CVla((i4) qnode->pst_sym.pst_value.pst_s_curval.pst_curcol.db_att_id,
	numbuf);

    /* Store it in the return buffer */
    STmove(numbuf, ' ', max_str_len - 2, buf + 1);

    return;
}

/*{
** Name: pst_hddump	- Dump out a query tree header
**
** Description:
**      This function formats and prints a query tree header for debugging
**	purposes.
**
** Inputs:
**      header                          Pointer to the query tree header
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to the terminal and/or error log file.
**
** History:
**	02-jun-86 (jeff)
**          written
**	19-sep-89 (andre)
**	    call psq_jrel_dmp() to display inner and outer relation masks
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as a variable length
**	    array of pointers (some of which may be set to NULL) to PST_RNGENTRY
**	    structure.
**	18-nov-93 (andre)
**	    added code to display contents of pst_mask1
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
static VOID
pst_hddump(
	PST_QTREE          *header)
{
    i4                  i;
    PST_QNODE		*node;

    TRdisplay("Query tree header:\n\n");

    TRdisplay("\tpst_qmode:\t");
    if (psq_modedmp(header->pst_mode) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_rangetab:");
    for (i = 0; i < header->pst_rngvar_count; i++)
    {
	/* note that some entries may remain set to NULL */
	if (header->pst_rangetab[i] == (PST_RNGENTRY *) NULL)
	    continue;

	TRdisplay("\n\t\t[%d]\n", i);
	TRdisplay("\t\t\tpst_rngvar:\t");
	if (psq_tbiddmp(&header->pst_rangetab[i]->pst_rngvar) != E_DB_OK)
	    return;
	TRdisplay("\n\t\t\tpst_timestamp:\t");
	if (psq_tmdmp(&header->pst_rangetab[i]->pst_timestamp) != E_DB_OK)
	    return;
	if (psq_jrel_dmp(&header->pst_rangetab[i]->pst_inner_rel,
			  &header->pst_rangetab[i]->pst_outer_rel) != E_DB_OK)
	    return;
	TRdisplay("\n\t\t\tpst_corr_name:\t");
	if (psq_str_dmp((u_char *) &header->pst_rangetab[i]->pst_corr_name,
		(i4) sizeof(DB_TAB_NAME)) != E_DB_OK)
	    return;
    }
    TRdisplay("\n\n");

    TRdisplay("\tpst_qtree:\t0x%p\n", header->pst_qtree);

    TRdisplay("\tpst_agintree:\t");
    if (psq_booldmp(header->pst_agintree) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_subintree:\t");
    if (psq_booldmp(header->pst_subintree) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_wchk:\t");
    if (psq_booldmp(header->pst_wchk) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_numparm:\t%d\n", header->pst_numparm);

    TRdisplay("\tpst_stflag:\t");
    if (psq_booldmp(header->pst_stflag) != E_DB_OK)
	return;
    TRdisplay("\n");

    if (header->pst_stflag)
    {
	TRdisplay("\tpst_stlist:\n");
	for (node = header->pst_stlist; node != (PST_QNODE *) NULL;
	    node = node->pst_right)
	{
	    TRdisplay("\n\t\tpst_srvno:\t%d\n",
		node->pst_sym.pst_value.pst_s_sort.pst_srvno);
	    TRdisplay("\t\tpst_srasc:\t");
	    if (psq_booldmp(node->pst_sym.pst_value.pst_s_sort.pst_srasc)
		!= E_DB_OK)
		return;
	    TRdisplay("\n");
	}
    }

    if (pst_dmpres(&header->pst_restab) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_cursid:\n");
    if (psq_ciddmp(&header->pst_cursid) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_delete:\t");
    if (psq_booldmp(header->pst_delete) != E_DB_OK)
	return;
    TRdisplay("\n");

    TRdisplay("\tpst_updtmode:\t");
    if (psq_upddmp(header->pst_updtmode) != E_DB_OK)
	return;

    TRdisplay("\n\tpst_mask1: %v\n",
	"TIDREQUIRED,CDB_IIDD_TABLES,NO_FLATTEN,CORR_AGGR,SHAREABLE,SQL,CATUPD_OK,\
AGG_IN_OUTERMOST,GROUPBY_OUTERMOST,SINGLETON_SUBSEL,IMPLICIT_CURSOR_UPDATE,\
SUBSEL_IN_OR_TREE,ALL_IN_TREE,MULT_CORR_ATTRS,RNGTREE,RPTQRY,,NEED_SYSCAT,XTABLE_UPDATE,UPD_CONST,INNER_OJ",
	header->pst_mask1);

    return;
}

/*{
** Name: pst_1ftype	- Format a node type
**
** Description:
**      This function formats a node type for debug printing within a query
**	tree node.
**
** Inputs:
**      qnode                           Pointer to query tree node
**      buf                             Place to put formatted node type
**
** Outputs:
**      buf                             Filled with formatted node type
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-aug-87(daved)
**          written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	3-sep-99 (inkdo01)
**	    Added new nodes for case function.
*/
VOID
pst_1ftype(
	PST_QNODE          *qnode,
	char               *buf)
{
    const char *string;

    if (qnode->pst_sym.pst_type >= 0 && qnode->pst_sym.pst_type < PST_MAX_ENUM)
	string = pst_typestr3[qnode->pst_sym.pst_type];
    else
	string = "???";

    STmove(string, ' ', PST_MAX_STR_LEN - 2, buf);
}

/*{
** Name: pst_dmpres	- dump PST_RESTAB struct.
**
** Description:
**      This function dumps the contents of a PST_RESTAB struct.
**
** Inputs:
**      restab		Pointer to PST_RESTAB struct
**
** Outputs:
**	Returns:
**	    E_DB_OK.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-oct-87 (stec)
**          Written.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  7-mar-1991 (bryanp)
**	    Changed handling of pst_compress as part of changing it from a bool
**	    to a flag word.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Update resjour display.
*/
DB_STATUS
pst_dmpres(
	PST_RESTAB  *restab)
{
    register PST_RESKEY	*key;
    i4		i;
    DB_LOC_NAME *name, *lim;

    TRdisplay("\tpst_restabid:\t");
    if (psq_tbiddmp(&restab->pst_restabid))
	return (E_DB_ERROR);
    TRdisplay("\n");

    TRdisplay("\tpst_resname:\t%#s\n", sizeof (restab->pst_resname),
	 &restab->pst_resname);

    TRdisplay("\tpst_resown:\t%#s\n", sizeof (restab->pst_resown),
	&restab->pst_resown);

    for (i = 0, name = (DB_LOC_NAME *) restab->pst_resloc.data_address,
	 lim =(DB_LOC_NAME*)((char*)name + restab->pst_resloc.data_in_size);
	 name < lim;
	 name++, i++
	)
    {
	TRdisplay("\tresloc[%d]:\t%#s\n", i, sizeof(DB_LOC_NAME), name);
    }

    TRdisplay("\tpst_resjour: %w\n", "OFF,ON,LATER", restab->pst_resjour);

    TRdisplay("\tpst_resvno:\t%d\n", restab->pst_resvno);

    TRdisplay("\tpst_fillfac:\t%d\n", restab->pst_fillfac);

    TRdisplay("\tpst_leaff:\t%d\n", restab->pst_leaff);

    TRdisplay("\tpst_nonleaff:\t%d\n", restab->pst_nonleaff);

    TRdisplay("\tpst_minpgs:\t%d\n", restab->pst_minpgs);

    TRdisplay("\tpst_maxpgs:\t%d\n", restab->pst_maxpgs);

    TRdisplay("\tpst_struct:\t");
    switch (restab->pst_struct)
    {
	case DB_HEAP_STORE: TRdisplay("heap");
			    break;
	case DB_ISAM_STORE: TRdisplay("isam");
			    break;
	case DB_HASH_STORE: TRdisplay("hash");
			    break;
	case DB_BTRE_STORE: TRdisplay("btree");
			    break;
	case DB_SORT_STORE: TRdisplay("sort");
			    break;
	default:	    TRdisplay("unknown");
    }
    TRdisplay("\n");

    TRdisplay("\tpst_compress:\t%v\n",
	    "DATA_COMP,NO_DATA_COMP,INDEX_COMP,NO_INDEX_COMP,DEFERRED,NO_COMP",
	    restab->pst_compress);

    TRdisplay("\tpst_resdup:\t");
    if (psq_booldmp(restab->pst_resdup) != E_DB_OK)
	return (E_DB_ERROR);
    TRdisplay("\n");

    TRdisplay("\tpst_reskey:\t%d\n", restab->pst_reskey);
    for (key = restab->pst_reskey; key; key = key->pst_nxtkey)
    {
	TRdisplay("\tkey->pst_attname:\t%#s\n", sizeof(key->pst_attname),
	    &key->pst_attname);
    }

    return (E_DB_OK);
}

/*{
** Name: pst_scctrace	- send output line to front end
**
** Description:
**      Send a formatted output line to the front end. A NL character
**	is added to the end of the message. Buffer to which a pointer
**	is received should have on extra byte for `\n'.
**
** Inputs:
**	arg				place holder
**	msg_len				length of message
**      msg_buffer                      ptr to message
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	10-feb-88 (stec)
**	    Copied from PSFDEBUG.
**	22-feb-88 (stec)
**	    Fix `set printqry' problem. `\n' should not
**	    be preceded by `\0'.
**	23-may-89 (jrb)
**	    Changed scf_enumber to scf_local_error.
*/
STATUS
pst_scctrace(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer)
{
    SCF_CB       scf_cb;
    DB_STATUS    scf_status;
    char	 *p;

    if (msg_length == 0)
	return;

    /* Truncate message if necessary */
    if (msg_length > PSF_MAX_TEXT)
	msg_length = PSF_MAX_TEXT;

    /* If last char is null, replace with NL, else add NL */
    if (*(p = (msg_buffer + msg_length - 1)) == '\0')
	*p = '\n';
    else
    {
	*(++p) = '\n';
	msg_length++;
    }

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_PSF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0; /* this is an ingres error number
					** returned to user, use 0 just in case 
					*/
    scf_cb.scf_len_union.scf_blength = (u_i4) msg_length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */

    scf_status = scf_call(SCC_TRACE, &scf_cb);

    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d displaying a message to user\n",
	    scf_cb.scf_error.err_code);
    }
}

/*{
** Name: pst_display	- like TRdisplay but output goes to user's terminal
**
** Description:
**      send a formatted output line to the front end
**
** Inputs:
**	format				format string
**	params				variable number of parameters
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	If output requires more than 131 chars, part of it will be truncated.
**
** History:
**	10-feb-88 (stec)
**	    Copied from PSFDEBUG.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
VOID
pst_display(format, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
char               *format;
i4		    p1;
i4		    p2;
i4		    p3;
i4		    p4;
i4		    p5;
i4		    p6;
i4		    p7;
i4		    p8;
i4		    p9;
i4		    p10;
/*
pst_display(
	char               *format,
	i4		    p1,
	i4		    p2,
	i4		    p3,
	i4		    p4,
	i4		    p5,
	i4		    p6,
	i4		    p7,
	i4		    p8,
	i4		    p9,
	i4		    p10)
*/
{
    char		buffer[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    /* TRformat removes `\n' chars, so to ensure that pst_scctrace()
    ** outputs a logical line (which it is supposed to do), we allocate
    ** a buffer with one extra char for NL and will hide it from TRformat
    ** by specifying length of 1 byte less. The NL char will be inserted
    ** at the end of the message by pst_scctrace().
    */
    TRformat(pst_scctrace, 0, buffer, sizeof(buffer) - 1, format, 
	p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

#ifdef xDEBUG
/*{
** Name: pst_dbpdump	- dump database procedure query tree.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	21-jun-88 (stec)
**	    Written
**	10-may-89 (neil)
**	    Tracing of new rule-related objects.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  27-feb-91 (ralph)
**	    Tracing for DESTINATION of MESSAGE and RAISE ERROR statements
**	09-nov-92 (rblumer)
**	    Added output for new field pst_statementEndRules.
**	23-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Format new node types in pst_dbpdump
**	11-oct-1993 (tad)
**	    Changed %x to %p for pointer values.
*/
VOID
pst_dbpdump(
	PST_PROCEDURE	*pnode,
	i4		whichlink)
{
    /*
    ** Print the final query tree.
    */
    (VOID) pst_display("\t\tPROCEDURE TREE:\n\t\t===============\n\n");
    
    /* Print out procedure node info */
    (VOID) pst_display("Procedure node:\n");
    (VOID) pst_display("\tmode:\t%d\n", pnode->pst_mode);
    (VOID) pst_display("\tvsn:\t%d\n", pnode->pst_vsn);
    (VOID) pst_display("\tisdbp:\t%d\n", pnode->pst_isdbp);
    (VOID) pst_display("\tdbpid:%d:%d:%t\n",
	pnode->pst_dbpid.db_cursor_id[0],
	pnode->pst_dbpid.db_cursor_id[0],
	sizeof(pnode->pst_dbpid.db_cur_name),
	pnode->pst_dbpid.db_cur_name);
    if ((pnode->pst_parms) &&
	pnode->pst_parms->pst_nvars
       )
    {
	i4		i, count = pnode->pst_parms->pst_nvars;
	DB_DATA_VALUE	*dbvalue;
	i4		first = pnode->pst_parms->pst_first_varno;

	(VOID) pst_display("\tThere are parms\n\n");
	(VOID) pst_display("\nParms:\n");
	(VOID) pst_display("\tcount:\t%d\n", count);
	(VOID) pst_display("\tfirst:\t%d\n", first);

	/* Display parm info */
	for (i = 0,
	     dbvalue = pnode->pst_parms->pst_vardef;
	     i < count;
	     i++, dbvalue++
	    )
	{
	    (VOID) pst_display("\t%d:len:%d     prec:%d     type:%d     data:%x\n",
		i+first, (i4)dbvalue->db_length, (i4)dbvalue->db_prec,
		(i4)dbvalue->db_datatype, (i4)dbvalue->db_data);
	}
    }
    else
    {
	(VOID) pst_display("\tThere are no parms\n");
    }
    if (pnode->pst_stmts)
    {
	PST_STATEMENT   *stmt;
	i4		count = 1, no = 0;
	char	    *s;

	(VOID) pst_display("\tThere are statements\n");

	for (stmt = pnode->pst_stmts;
	     stmt != (PST_STATEMENT *) NULL;
	     stmt = stmt->pst_link
	    )
	{
	    no++;
	}

	(VOID) pst_display("\tpst_link links %d statements.\n", no);


	/* Print out statement info */
	for (stmt = pnode->pst_stmts;
	     stmt != (PST_STATEMENT *) NULL;
	    )
	{
	    (VOID) pst_display("\n");
	    (VOID) pst_display("Statement %d:(%p)\n", count++, stmt);
	    (VOID) pst_display("\tmode:\t%d\n", stmt->pst_mode);
	    (VOID) pst_display("\tlineno:\t%d\n", stmt->pst_lineno);
	    (VOID) pst_display("\tpst_next:\t%p\n", stmt->pst_next);
	    (VOID) pst_display("\tpst_after_stmt:\t0x%p\n",
				stmt->pst_after_stmt);
	    (VOID) pst_display("\tpst_statementEndRules:\t0x%p\n",
				stmt->pst_statementEndRules);
	    (VOID) pst_display("\tpst_link:\t%p\n", stmt->pst_link);
	    switch(stmt->pst_type)
	    {
	    case PST_DV_TYPE:
		s = "declare";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if ((stmt->pst_specific.pst_dbpvar) &&
		    stmt->pst_specific.pst_dbpvar->pst_nvars
		   )
		{
		    i4		i, count = stmt->pst_specific.pst_dbpvar->pst_nvars;
		    DB_DATA_VALUE	*dbvalue;
		    i4		first = stmt->pst_specific.pst_dbpvar->pst_first_varno;

		    (VOID) pst_display("\tThere are local variables\n\n");
		    (VOID) pst_display("\tLocal variables:\n");
		    (VOID) pst_display("\tcount:\t%d\n", count);
		    (VOID) pst_display("\tfirst:\t%d\n", first);

		    /* Display local var info */
		    for (i = 0,
			 dbvalue = stmt->pst_specific.pst_dbpvar->pst_vardef;
			 i < count;
			 i++, dbvalue++
			)
		    {
			(VOID) pst_display(
			    "\t%d:len:%d     prec:%d     type:%d     data:%x\n",
			    i+first, (i4)dbvalue->db_length,
			    (i4)dbvalue->db_prec,
			    (i4)dbvalue->db_datatype,
			    (i4)dbvalue->db_data);
		    }
		}
		else
		{
		    (VOID) pst_display("\tThere are no local variables\n");
		}
		break;
	    case PST_QT_TYPE:
		s = "dml";
		(VOID) pst_display("\ttype:\t%d (%s)\tmode:\t%d\n",
		    stmt->pst_type, s, stmt->pst_specific.pst_tree->pst_mode);
		break;
	    case PST_IF_TYPE:
		s = "if";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if (stmt->pst_specific.pst_if.pst_condition
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no condition\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is a condition\n");
		}
		if (stmt->pst_specific.pst_if.pst_true
		    == (PST_STATEMENT *) NULL)
		{
		    (VOID) pst_display("\tthere is no THEN part\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is a THEN part\n");
		    (VOID) pst_display("\tpst_true:\t%p\n",
			stmt->pst_specific.pst_if.pst_true);
		}
		if (stmt->pst_specific.pst_if.pst_false
		    == (PST_STATEMENT *) NULL)
		{
		    (VOID) pst_display("\tthere is no ELSE part\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is an ELSE part\n");
		    (VOID) pst_display("\tpst_false:\t%p\n",
			stmt->pst_specific.pst_if.pst_false);
		}
		break;
	    case PST_WH_TYPE:
		s = "while";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if (stmt->pst_specific.pst_if.pst_condition
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no condition\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is a condition\n");
		}
		if (stmt->pst_specific.pst_if.pst_true
		    == (PST_STATEMENT *) NULL)
		{
		    (VOID) pst_display("\tthere is no statements\n");
		}
		else
		{
		    (VOID) pst_display("\tthere are statements\n");
		    (VOID) pst_display("\tpst_true:\t%p\n",
			stmt->pst_specific.pst_if.pst_true);
		}
		(VOID) pst_display("\tpst_false:\t%p\n",
		    stmt->pst_specific.pst_if.pst_false);
		break;
	    case PST_MSG_TYPE:
		s = "message";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if (stmt->pst_specific.pst_msg.pst_msgnumber
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no msg number\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is a msg number\n");
		}
		if (stmt->pst_specific.pst_msg.pst_msgtext
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no msg text\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is msg text\n");
		}
		if (stmt->pst_specific.pst_msg.pst_msgdest)
		{
		    (VOID) pst_display("\tdestination was specified\n");
		}
		else
		{
		    (VOID) pst_display("\tdestination was not specified\n");
		}
		break;
	    case PST_RTN_TYPE:
		s = "return";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if (stmt->pst_specific.pst_rtn.pst_rtn_value
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no return value\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is return value\n");
		}
		break;
	    case PST_CMT_TYPE:
		s = "commit";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		break;
	    case PST_RBK_TYPE:
		s = "rollback";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		break;
	    case PST_RSP_TYPE:
		s = "rollback to savepoint";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		break;
	    case PST_EMSG_TYPE:
		s = "raise error";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		if (stmt->pst_specific.pst_msg.pst_msgnumber
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no error number\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is an error number\n");
		}
		if (stmt->pst_specific.pst_msg.pst_msgtext
		    == (PST_QNODE *) NULL)
		{
		    (VOID) pst_display("\tthere is no error text\n");
		}
		else
		{
		    (VOID) pst_display("\tthere is error text\n");
		}
		if (stmt->pst_specific.pst_msg.pst_msgdest)
		{
		    (VOID) pst_display("\tdestination was specified\n");
		}
		else
		{
		    (VOID) pst_display("\tdestination was not specified\n");
		}
		break;
	    case PST_CP_TYPE:
		s = "callproc";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		break;
	    case PST_CREATE_SCHEMA_TYPE:
	    {
		PST_CREATE_SCHEMA   *crt_node;
		char		    objname[DB_OBJ_MAXNAME+1];

		crt_node = &(stmt->pst_specific.pst_createSchema);

		s = "create schema";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		MEmove( sizeof(crt_node->pst_schname.db_schema_name),
			(PTR)crt_node->pst_schname.db_schema_name,
			EOS, sizeof(objname), (PTR)objname);
		(VOID) pst_display("\tschema:\t%s\n", objname);
		MEmove( sizeof(crt_node->pst_owner.db_own_name),
			(PTR)crt_node->pst_owner.db_own_name,
			EOS, sizeof(objname), (PTR)objname);
		(VOID) pst_display("\towner:\t%s\n", objname);
		break;
	    }
	    case PST_EXEC_IMM_TYPE:
	    {
		PST_EXEC_IMM        *exec_node;
		PST_OBJDEP	    *dependency;
		char		    schname[DB_SCHEMA_MAXNAME+1];
		char		    objname[DB_OBJ_MAXNAME+1];

		exec_node = &(stmt->pst_specific.pst_execImm);

		s = "execute immediate";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		(VOID) pst_display("\ttext:\t%s\n",
					exec_node->pst_qrytext->db_t_text);

		for (dependency = exec_node->pst_info.pst_deplist;
		     dependency != (PST_OBJDEP *) NULL;
		     dependency = dependency->pst_nextdep)
		{
		    MEmove( psf_trmwhite(
				sizeof(dependency->pst_depown),
				(char *)&dependency->pst_depown),
			    (PTR)dependency->pst_depown.db_own_name,
			    EOS, sizeof(schname), (PTR)schname);
		    MEmove( psf_trmwhite(
				sizeof(dependency->pst_depname),
				(char *)&dependency->pst_depname),
			    (PTR)dependency->pst_depname.db_tab_name,
			    EOS, sizeof(objname), (PTR)objname);
		    if (schname[0] == EOS)
		    	(VOID) pst_display("\tdependency:\t%s\t%s\n",
				    objname,
				    (dependency->pst_depuse & PST_DEP_NOORDER) ?
				    "NOORDER" : " " );
		    else
		    	(VOID) pst_display("\tdependency:\t%s.%s\t%s\n",
				    schname, objname, 
				    (dependency->pst_depuse & PST_DEP_NOORDER) ?
				    "NOORDER" : " " );
		}
		break;
	    }
	    case PST_CREATE_TABLE_TYPE:
	    {
		PST_CREATE_TABLE   *crt_node;
		QEU_CB		   *qeu_cb;
		DMU_CB		   *dmu_cb;
		DMF_ATTR_ENTRY	   **attrs, *attr;
		i4		   num_attrs, i;
		
		crt_node = &(stmt->pst_specific.pst_createTable);
		qeu_cb   = (QEU_CB *) crt_node->pst_createTableQEUCB;
		dmu_cb   = (DMU_CB *) qeu_cb->qeu_d_cb;

		pst_display("\ttype:\t\t%d (create table)\n", stmt->pst_type);

		if (crt_node->pst_createTableFlags & PST_CRT_TABLE)
		    s = "table";
		else if (crt_node->pst_createTableFlags & PST_CRT_INDEX)
		    s = "index";
		else 
		    s = "unknown";
		pst_display("\tsubtype:\t%d (%s)\n", 
			    crt_node->pst_createTableFlags, s);
		
		pst_display("\tname:\t%.#s\n", DB_TAB_MAXNAME,
			    dmu_cb->dmu_table_name.db_tab_name);

		pst_display("\towner:\t%.#s\n", DB_OWN_MAXNAME,
			    dmu_cb->dmu_owner.db_own_name);

		attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
		num_attrs = dmu_cb->dmu_attr_array.ptr_in_count;
		pst_display("\tnumber of columns:\t%d\n", num_attrs);

		/* print out attribute names 
		 */
		for (i = 1; i <= num_attrs; i++, attrs++)
		{
		    attr = *attrs;
		    pst_display("\tCol %d:\n", i);
		    
		    pst_display("\t\tname:\t%.#s\n", DB_ATT_MAXNAME,
				attr->attr_name.db_att_name);
		    pst_display("\t\ttype:\t%d\n", attr->attr_type);
		    pst_display("\t\tsize:\t%d\n", attr->attr_size);
		    pst_display("\t\tprec:\t%d\n", attr->attr_precision);
		    pst_display("\t\tflags:\t0x%x\n", attr->attr_flags_mask);
		    pst_display("\t\tdefault ID:\t(%d,%d)\n",
				attr->attr_defaultID.db_tab_base,
				attr->attr_defaultID.db_tab_index);
		    if (attr->attr_defaultTuple == (DB_IIDEFAULT *) NULL)
		    {
			pst_display("\t\tNo default tuple\n");
		    }
		    else
		    {
			pst_display("\tdefault value:\t%.#s\n", 
			    (i4) attr->attr_defaultTuple->dbd_def_length,
				      attr->attr_defaultTuple->dbd_def_value);
		    }
		    pst_display("  \n");

		} /* end print out attributes */
		
		break;
	    }
		
	    case PST_CREATE_INTEGRITY_TYPE:
	    {
		PST_CREATE_INTEGRITY	*int_node;
		DB_INTEGRITY		*int_tuple;
		PST_COL_ID		*col;
		i4			unique, check, ref;

		int_node = &(stmt->pst_specific.pst_createIntegrity);
		int_tuple = int_node->pst_integrityTuple;

		s = "create integrity";
		pst_display("\ttype:\t\t%d (%s)\n", stmt->pst_type, s);

		if (int_node->pst_createIntegrityFlags & PST_CONSTRAINT)
		    s = "ANSI constraint";
		else
		    s = "INGRES integrity";
		pst_display("\tsubtype:\t0x%x (%s)\n", 
			    int_node->pst_createIntegrityFlags, s);
		
		if (int_node->pst_integrityQueryText != (DB_TEXT_STRING *) NULL)
		{
		    pst_display("\tquery text:\t%.#s\n", 
				int_node->pst_integrityQueryText->db_t_count,
				int_node->pst_integrityQueryText->db_t_text);
		}
		else
		    pst_display("\tQuery text MISSING!!\n");
		
		if (~int_node->pst_createIntegrityFlags & PST_CONSTRAINT)
		    break;

		if (int_node->pst_createIntegrityFlags & PST_CONS_NAME_SPEC)
		{
		    pst_display("\tConstraint name: %.#s\n", DB_CONS_MAXNAME,
				int_tuple->dbi_consname.db_constraint_name);
		}

		pst_display("\tSchema name:\t%.#s\n", DB_SCHEMA_MAXNAME,
			    int_node->pst_integritySchemaName.db_schema_name);

		unique  = int_tuple->dbi_consflags & CONS_UNIQUE;
		check   = int_tuple->dbi_consflags & CONS_CHECK;
		ref     = int_tuple->dbi_consflags & CONS_REF;

		if (unique)
		{
		    if (int_tuple->dbi_consflags & CONS_PRIMARY)
			pst_display("\tPrimary Key constraint\n");
		    else
			pst_display("\tUnique constraint\n");
		}
		
		if (check)
		    pst_display("\tCheck constraint\n");

		if (ref)
		    pst_display("\tReferential constraint\n");
		    
		pst_display("\t    on table:\t%.#s.%.#s\n",
				psf_trmwhite(DB_OWN_MAXNAME, 
				       int_node->pst_cons_ownname.db_own_name),
				int_node->pst_cons_ownname.db_own_name,
				psf_trmwhite(DB_TAB_MAXNAME, 
				       int_node->pst_cons_tabname.db_tab_name),
				int_node->pst_cons_tabname.db_tab_name);

		pst_display("\tColumns in constraint are (by number, in order):\n");
		for (col = int_node->pst_cons_collist; 
		     col != (PST_COL_ID *) NULL;
		     col = col->next)
		{
		    pst_display("\t\tcolumn #%d\n", col->col_id.db_att_id);
		}

		if (unique)
		    break;
		
		if (ref)
		{
		    pst_display("\tReferenced table name:\t%.#s.%.#s\n", 
				psf_trmwhite(DB_OWN_MAXNAME, 
					int_node->pst_ref_ownname.db_own_name),
				int_node->pst_ref_ownname.db_own_name,
				psf_trmwhite(DB_TAB_MAXNAME, 
					int_node->pst_ref_tabname.db_tab_name),
				int_node->pst_ref_tabname.db_tab_name);

		    pst_display("\tReferenced column numbers are (in order):\n");
		    for (col = int_node->pst_ref_collist; 
			 col != (PST_COL_ID *) NULL;
			 col = col->next)
		    {
			pst_display("\t\tcolumn #%d\n", col->col_id.db_att_id);
		    }
		}

		if (int_node->pst_checkRuleText != (DB_TEXT_STRING *) NULL)
		{
		    pst_display("\ttext for Rule WHERE clause:\n");
		    pst_display("\t\t%.#s\n", 
				int_node->pst_checkRuleText->db_t_count,
				int_node->pst_checkRuleText->db_t_text);
		}

		if (int_node->pst_createIntegrityFlags & PST_SUPPORT_SUPPLIED)
		{
		    pst_display("\tID of user-supplied index: (%d,%d)\n",
				int_node->pst_suppliedSupport.db_tab_base,
				int_node->pst_suppliedSupport.db_tab_index);
		    pst_display("\t\t(used for UNIQUE constraint)\n");
		}
		break;
	    }
	    default:
		s = "unknown";
		(VOID) pst_display("\ttype:\t%d (%s)\n", stmt->pst_type, s);
		break;
	    }

	    /* Now see which link to follow */
	    if (!whichlink)
		stmt = stmt->pst_next;
	    else
		stmt = stmt->pst_link;
	}		    
    }
    else
    {
	(VOID) pst_display("\tThere are no statements\n");
    }
}
#endif /* xDEBUG */
