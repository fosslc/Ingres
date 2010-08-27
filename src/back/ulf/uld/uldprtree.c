/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/* NO_OPTIM = usl_us5 */
 
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <ex.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <ulm.h>
#include    <uld.h>
#include    <ulx.h>
#include    <tr.h>
#include    <st.h>
 
/**
**
**  Name: ULDPRTREE.C - General-purpose tree printing functions
**
**  Description:
**      This file contains the functions for printing trees of any type.
**	It was adapted from fmttree.c in jutil in version 4.0 of Ingres.
**	The algorithm is Bob Kooi's adaptation of that described in:
**
**	    Wetherell, C. and Shannon, A., "Tidy Drawings of Trees,"
**	    IEEE Transactions on Software Engineering, Vol. SE-5, No. 5
**	    September, 1979
**
**	The main change for the Jupiter project was to make it re-entrant.
**
**          uld_prtree - Format and print a tree
**	    prnode - Store a line for output
**	    walk1 - Build a tree in parallel with the one to be formatted
**	    walk2 - Figure out horizontal and vertical displacements
**	    walk3 - Print out all the nodes in the tree, level by level
**	    prflsh - Flush out the buffers
**
**
**  History:
**      06-may-86 (jeff)
**          Adapted from fmttree.c in jutil in version 4.0
**	21-may-90 (seng)
**	    IBM RS/6000 workaround for compiler bug in max().  Explicitly
**	    change macro to an if statement to have compiler understand
**	    what we need.  Integrated from 6.3/02 port.  This is a generic
**	    change.
**	31-aug-1992 (rog)
**	    Prototype ULF.  Replaced calls to uld_scctrace with ulx_sccerror.
**	22-sep-1992 (bryanp)
**	    Pass an err_code argument to ule_format. It demands one.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	    Redefined LLNMX to be ULD_PRNODE_LINE_WIDTH from ulf.h (was
**	    1024); the value remains the same, this change makes it visible
**	    to callers of uld_prnode so they can declare an STprintf buffer,
**	    if needed.
**	15-feb-94 (swm)
**	    Bug #59611
**	    Replace STprintf calls with calls to TRformat in example code
**	    to do printnode(); TRformat checks for buffer overrun, also
**	    use of a global format buffer is advised to reduce likelihood
**	    of stack overflow. The TRformat needs a callback routine,
**	    uld_tr_callback, to put back any trailing line feed
**	    removed by TRformat and to put a '\0' string terminator in at
**	    the offset specified to the callback function.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	10-mar-1999 (popri01)
**	    Add NO OPTIM for Unixware 7 (usl_us5). Otherwise the branches ("/" "\")
**	    are dropped when the query tree is printed.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**     28-Oct-2009 (maspa05) b122725
**          ult_print_tracefile now uses integer constants instead of string
**          for type parameter - SC930_LTYPE_PARM instead of "PARM" and so on
**	11-Jun-2010 (kiria01) b123908
**	    Init ulm_streamid_p for ulm_openstream to fix potential segvs.
**/

/*
**  Defines of other constants.
*/
 
/*
**      The following constants may be changed.
*/
 
/* Max number of nodes in caller's tree */
#define                 PTMX            500
 
/* Max height of caller's tree */
#define			MAXH		200
 
/*
** Max number of lines to be printed for a node,
** excluding the line with slanted marks above each node.
*/
#define			PMX		15
 
/* Width of a print line */
#define			LLNMX		ULD_PRNODE_LINE_WIDTH
 
/* It probably wouldn't be useful to change the following constants */
 
/* At the root node */
#define			ROOT		1
 
/* Processing the left subnode of the father */
#define			LEFT		2
 
/* ditto for right */
#define			RIGHT		3

/*}
** Name: PARATREE - Tree built in parallel with caller's tree
**
** Description:
**      The first phase of the algorithm builds a tree in parallel with the
**	caller's tree, using this data structure.
**
** History:
**     06-may-86 (jeff)
**          Adapted from same structure in jutil!fmttree.c in 4.0
*/
typedef struct _PARATREE
{
    struct _PARATREE *lf;
    struct _PARATREE *rt;
    i4		     x;
    i4		     modifier;
} PARATREE;

/*}
** Name: ULD_CONTROL - Controlling info for printing trees
**
** Description:
**      This file was adapted from jutil!fmttree.c in 4.0.  Originally,
**	everything in this structure was allocated as static variables.
**	The Jupiter project introduced a server, and so it had to be made
**	re-entrant.  Thus, everything was put into this single structure,
**	which is meant to be passed around from function to function, to
**	eliminate global variable references.
**
** History:
**     06-may-86 (jeff)
**          Adapted from jutil!fmttree.c in 4.0
*/
typedef struct _ULD_CONTROL
{
    PARATREE        *ptp;               /* Start of parallel tree free space */
    i4		    first;		/* TRUE means first node printed at
					** this level
					*/
    i4		    bmax;		/* max # of lines printed for a node
					** at this level
					*/
    i4		    bcnt;		/* line # for current node being printed
					** by the caller's printnode() function
					*/
    i4		    maxh;		/* Actual height of caller's tree */
    i4		    isf;		/* Spaces per horizontal level of
					** tree
					*/
    i4		    type;		/* ROOT, LEFT, or RIGHT */
    i4		    ilvl;		/* Horizontal level of current node */
    i4		    lbl;		/* lines per vertical level */
    i4		    pti;		/* index into parallel tree free space
					** list
					*/
    i4		    ncmx;		/* max width of node.  If caller prints
					** more, they are truncated.
					*/
    i4		    modfsum;		/* TRUE means it has called
					** "modifier_sum" in the reference.
					*/
    char	    *next_pos;
    char	    *modifier;
    char	    *pbuf[PMX + 1];
    PTR		    (*lson)();		/* Pointer to function that returns
					** pointer to left subtree.
					*/
    PTR		    (*rson)();		/* Pointer to function that returns
					** pointer to right subtree.
					*/
    VOID	    (*pnod)();
    i4		    pof;
    i4              facility;		/* calling facility */
} ULD_CONTROL;

/*}
** Name: ULD_STORAGE - Working storage for print tree routine
**
** Description:
**      This structure contains the working memory used to build the
**	tree to be printed.   Originally, this memory was on the stack,
**	which caused problems for VMS.  Then it was made static memory,
**	which caused problems for MVS.  Now it is obtained from ULM, which
**	hopefully will work for everyone.
**
** History:
**     18-Jun-87 (DaveS)
**          Created new.
*/
typedef struct _ULD_STORAGE
{
   PARATREE     pts[PTMX];
   char         mod[MAXH];
   char         ned[MAXH];
   char         pbf[PMX + 1][LLNMX + 1];
} ULD_STORAGE;

/*
**  Definition of static variables and forward static functions.
*/
 
static PARATREE	*walk1( PTR tnode, i4  hh, ULD_CONTROL *control );
static VOID	walk2 ( PARATREE *pnode, register ULD_CONTROL *control );
static VOID	walk3 ( PTR tnode, PARATREE *pnode, i4  ch, i4  ph,
			ULD_CONTROL *control );
static VOID	prflsh( i4  init, register ULD_CONTROL *control, PTR sc930_trace );
static VOID	uld_nl( ULD_CONTROL *control );

/*{
** Name: uld_prtree	- Format and print a tree
**
** Description:
**      This function formats and prints a tree, with help from caller-
**	supplied functions.  The algorithm was adapted by Bob Kooi from
**		Wetherell, C. and Shannon, A., "Tidy Drawings of Trees,"
**		IEEE Transactions on Software Engineering, Vol. SE-5, No. 5
**		September, 1979
**
**	Later mods made it re-entrant so it could run as part of a server.
**
**	EXAMPLE:
**		suppose you have a tree made out of nodes of type tnode:
**
**		struct tnode
**		{
**			i4		data1;
**			i4		data2;
**			struct tnode	*left;
**			struct tnode	*right;
**		}
**
**		where Root is a pointer to the root.  you must provide
**		three routines, call them leftson, rightson and printnode:
**
**		PTR
**		leftson(t)
**		PTR	    t;
**		{
**			return ((PTR) ((struct tnode *) t)->left);
**		}
**
**		PTR
**		rightson(t)
**		PTR	    t;
**		{
**			return ((PTR) ((struct tnode *) t)->right);
**		}
**
**		VOID 
**		printnode(t, control)
**		PTR	    t;
**		PTR	    control;
**		{
**			struct tnode	*node = (struct tnode *) t;
**
**			uld_prnode(control, "*****");
**			TRformat(uld_tr_callback, (i4 *)0,
**			    global_buffer, sizeof(global_buffer),
**			    "* %d *",
**			    node->data1);
**			uld_prnode(control, global_buffer);
**			TRformat(uld_tr_callback, (i4 *)0,
**			    global_buffer, sizeof(global_buffer),
**			    %d*",
**			    node->data2, node->data1);
**			uld_prnode(control, global_buffer);
**			uld_prnode(control, "*****");
**		}
**
**
**		then the call:
**
**		uld_prtree((PTR) Root, printnode, leftson, rightson, 8, 5);
**
**		would print a tree where each node is 8 characters wide
**		by 5 characters long and would contain data1 on the first
**		line, data2 and data1 on the second line with a border of
**		stars.
**
**		a sample output might be:
 
                                        *****
                                        * 7 *
                                        *3 7*
                                        *****
 
                         /                              \
                        *****                           *****
                        * 4 *                           * 3 *
                        *3 4*                           *3 3*
                        *****                           *****
 
                 /              \                /              \
                *****           *****           *****           *****
                * 3 *           * 1 *           * 2 *           * 1 *
                *3 3*           *3 1*           *3 2*           *3 1*
                *****           *****           *****           *****
 
         /              \                /              \
        *****           *****           *****           *****
        * 2 *           * 1 *           * 1 *           * 1 *
        *3 2*           *3 1*           *3 1*           *3 1*
        *****           *****           *****           *****
 
 /              \
*****           *****
* 1 *           * 1 *
*3 1*           *3 1*
*****           *****
 
 
**
** Parts of a tree that exceed the manifest constants are not printed
** but should not otherwise cause difficulty (supposedly).
**
**
** the main difference between this and the reference is that here a
** "parallel" tree is built to hold the info (x,modifier) that, in the
** reference, is held in the nodes of the tree being printed.
** this means that the user need not plan for this routine but can
** easily add it in later. this is almost imperative when the user
** is working with variable size nodes in his tree.
**
** Inputs:
**      root                            Pointer to the root of the tree
**	printnode			Pointer to function to format a node
**	leftson				Pointer to function to find left son
**	rightson			Pointer to function to find right son
**	indent				Indent scale factor (number of spaces
**					to indent for each horizontal level)
**	lbl				Lines between levels (should at least
**					equal the maximum number of lines that
**					the user would ever print for a node
**					to ensure an evenly spaced tree)
**      facility                        facility ID of caller
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
**	06-may-86 (jeff)
**          Adapted from fmttree() in jutil!fmttree.c in 4.0
**	24-mar-87 (daved)
**	    set exclusive semaphore so that large static structures
**	    for the control struct are single threaded.
**	18-Jun-87 (DaveS)
**	    Modified to get memory from ULM instead of using statics.
**	    SCF semaphore removed since we no longer need it.
**	04-nov-87 (thurston)
**          On ulm_startup() call, I made the block size almost as large as the
**          pool since all that this routine does is create a pool, start a
**          stream, and allocate one L*A*R*G*E piece out of it.  (Wouldn't this
**          be better served by SCU_MALLOC directly???
**	08-feb-89 (jrb)
**	    Fixed calls to ule_format which made no sense at all.
**	21-may-89 (jrb)
**	    Updated for new ule_format interface.
**	22-sep-1992 (bryanp)
**	    Pass an err_code argument to ule_format. It demands one.
**	7-oct-2004 (thaju02)
**	    Change memleft to SIZE_TYPE.
**	19-Aug-2009 (kibro01) b122509
**	    Add new uld_prtree_x function which allows an extra sc930-tracing
**	    parameter to uld_prtree
*/
VOID
uld_prtree_x( PTR root, VOID (*printnode)(), PTR (*leftson)(), PTR (*rightson)(),
	    i4  indent, i4  lbl, i4  facility, PTR sc930_trace )
{
    register i4         i;
    register PARATREE	*pnode;
    ULD_CONTROL		control;
    ULD_STORAGE		*storage = 0;
    ULM_RCB		ulm_rcb;
    SIZE_TYPE		memleft;
    STATUS		status;
    i4		err_code;
 
    control.facility = facility;
    ulm_rcb.ulm_facility = DB_ULF_ID;
    ulm_rcb.ulm_sizepool = sizeof(ULD_STORAGE) + 1024;
    ulm_rcb.ulm_blocksize = sizeof(ULD_STORAGE) + 128;
    memleft = sizeof(ULD_STORAGE) + 1024;
    ulm_rcb.ulm_memleft	    = &memleft;
 
    /* create a memory pool */
    status = ulm_startup(&ulm_rcb);
    if (status != E_DB_OK)
    {
	 (VOID) ule_format(ulm_rcb.ulm_error.err_code, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  &err_code, 0);
	 (VOID) ule_format(E_UL0201_ULM_FAILED, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  &err_code, 0);
	 return;
     }
 
    /* open the private memory stream and allocate ULD_STORAGE */
    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize	    = sizeof (ULD_STORAGE);
    ulm_rcb.ulm_streamid_p = NULL;
    if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
    {
	 (VOID) ule_format(ulm_rcb.ulm_error.err_code, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  &err_code, 0);
	(VOID) ule_format(E_UL0201_ULM_FAILED, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  &err_code, 0);
	return;
    }
 
    storage   		    = (ULD_STORAGE *)ulm_rcb.ulm_pptr;
 
    control.lbl = lbl + 1;
    control.isf = indent;
    control.pti = 0;
    control.ptp = storage->pts;
    control.ncmx = indent * 2;
    control.first = 0;
 
    for (i = PMX + 1; i--; )
	control.pbuf[i] = &(storage->pbf[i][0]);
 
    control.modifier = storage->mod;
    control.next_pos = storage->ned;
 
    /* Initialize line buffers */
    control.bmax = PMX;
    prflsh(1, &control, NULL);
 
    /* Initialize arrays */
    for (i = MAXH; i--; )
    {
	control.next_pos[i] = '\1';
	control.modifier[i] = '\0';
    }
 
    /* Initialize the function pointers */
    control.lson = leftson;
    control.rson = rightson;
    control.pnod = printnode;
 
    /* Do the first post-order walk */
    control.maxh = -1;
    pnode = walk1(root, 0, &control);
 
    /* Do the second pre-order walk */
    control.modfsum = 0;
    walk2(pnode, &control);
 
    /* For each level, print out the nodes at that level */
    control.type = ROOT;
    for (i = 0; i <= control.maxh; i++)
    {
	walk3(root, pnode, 0, i, &control);
	prflsh(0, &control, sc930_trace);
    }
 
    /* if we got any memory, return it */
    if (storage)
       ulm_shutdown(&ulm_rcb);
}

VOID
uld_prtree( PTR root, VOID (*printnode)(), PTR (*leftson)(), PTR (*rightson)(),
	    i4  indent, i4  lbl, i4  facility)
{
    uld_prtree_x(root,printnode,leftson,rightson,indent,lbl,facility,NULL);
}


/*
** Name: walk1	- Post-order walk
**
** Description:
**      This is a recursive implementation of the post-order walk of
**	algorithm 3 described in the reference.  It has been made re-entrant.
**
** Inputs:
**      tnode                           The tree to walk
**	hh				Depth in tree
**	control				Control structure
**
** Outputs:
**      control                         May be changed in various ways
**	Returns:
**	    Pointer to a parallel tree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-may-86 (jeff)
**          adapted from walk1() in jutil!fmttree.c in 4.0
*/
static PARATREE *
walk1( PTR tnode, i4  hh, ULD_CONTROL *control )
{
    register PARATREE   *l;
    register PARATREE   *r;
    register PARATREE	*p;
    i4			h;
    i4			place;
    PTR			t;
 
    h = hh;
    t = tnode;
 
    if (t == (PTR) NULL)
	return (NULL);
 
    /* Allocate a node for the parallel tree */
    if (control->pti >= PTMX || h >= MAXH)
	return (NULL);
 
    p = control->ptp + control->pti++;
 
    /*
    ** Walk the left and right subtrees of the users tree and
    ** attach the corresponding nodes of the parallel tree.
    */
    l = p->lf = walk1((*control->lson)(t), h + 1, control);
    r = p->rt = walk1((*control->rson)(t), h + 1, control);
 
    /* The rest is pretty much identical to the reference */
    if (l == (PARATREE *) NULL && r == (PARATREE *) NULL)  /* a leaf node */
    {
	place = control->next_pos[h];
    }
    else if (l == (PARATREE *) NULL)
    {
	place = r->x - 1;
    }
    else if (r == (PARATREE *) NULL)
    {
	place = l->x + 1;
    }
    else 
    {
	place = (l->x + r->x) / 2;
    }
 
    if ((int) control->modifier[h] < (int) (control->next_pos[h] - place))
	   control->modifier[h] = control->next_pos[h] - place;
 
    if (l == (PARATREE *) NULL && r == (PARATREE *) NULL)
	p->x = place;
    else 
	p->x = place + control->modifier[h];
 
    control->next_pos[h] = p->x + 2;
    p->modifier = control->modifier[h];
    control->maxh = max(h, control->maxh);
 
    return (p);
}

/*** Name: walk2	- Second pre-order walk
**
** Description:
**      This function is the second pre-order walk.  It is a recursive
**	implementation of the second part of algorithm 3 in the reference.
**
** Inputs:
**      pnode                           Pointer to the tree to walk
**	control				Pointer to control structure
**
** Outputs:
**      pnode                           This function does something to the
**					tree.  Why was it never documented
**					in-line?
**	control				Various things done to this
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-may-86 (jeff)
**          Adapted from walk2() in jutil!fmttree.c in 4.0
*/
static VOID
walk2( PARATREE *pnode, register ULD_CONTROL *control )
{
    register PARATREE   *p = pnode;
 
    if (p == (PARATREE *) NULL)
	return; 
 
    p->x += control->modfsum;
    control->modfsum += p->modifier;
    walk2(p->lf, control);
    walk2(p->rt, control);
    control->modfsum -= p->modifier;
}

/*
** Name: walk3	- Print out all nodes at a given level
**
** Description:
**      This function prints out all nodes at a given level of the tree.
**
** Inputs:
**      tnode                           Pointer to root of tree
**	pnode				Pointer to root of parallel tree
**	ch				Current level
**	ph				Level to print
**	control				Pointer to control structure
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Prints nodes
**
** History:
**	07-may-86 (jeff)
**          Adapted from walk3() in jutil!fmttree.c in 4.0
*/
static VOID
walk3( PTR tnode, PARATREE *pnode, i4  ch, i4  ph, ULD_CONTROL *control )
{
    register PTR        t = tnode;
    register PARATREE	*p = pnode;
 
    if (p == (PARATREE *) NULL)
	return; 
 
    if (ch == ph)
    {
	control->ilvl = p->x;
	(*control->pnod)(t, (PTR) control);
	return; 
    }
 
    control->type = LEFT;
    walk3((*control->lson)(t), p->lf, ch + 1, ph, control);
    control->type = RIGHT;
    walk3((*control->rson)(t), p->rt, ch + 1, ph, control);
}

/*{
** Name: uld_prnode	- Buffer up info about a node
**
** Description:
**      This function is meant to be called by the node formatting routine
**	supplied to uld_prtree().  It is supplied with a formatted buffer.
**
** Inputs:
**      control                         Pointer to ULD_CONTROL structure
**	frmt				Pointer to formatted string
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
**          Adapted from prnode() in jutil!fmttree.c in 4.0
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	16-feb-94 (swm)
**	    Corrected uld_prnode function header comments.
*/
VOID
uld_prnode(control, frmt)
PTR                control;
char		   *frmt;
{
    register char       *cp;
    register char	*p;
    register i4	of;
    /* Lint complains about unaligned pointer combination, but it's OK */
    register ULD_CONTROL *ctl = (ULD_CONTROL *) control;
 
    of = (ctl->ilvl - 1) * ctl->isf;
 
    if (of > LLNMX - ctl->ncmx + 1)
	return; 
 
    if (ctl->first)
    {
	ctl->first = 0;
	ctl->pof = of;
	ctl->bmax = 1;
	ctl->bcnt = 1;
    }
    else if (ctl->pof == of)
    {
	ctl->bcnt = min(ctl->bcnt + 1, PMX);
	ctl->bmax = max(ctl->bcnt, ctl->bmax);
    }
    else 
    {
	ctl->bcnt = 1;
	ctl->pof = of;
    }
 
    cp = ctl->pbuf[ctl->bcnt] + of;
 
    if (ctl->bcnt == 1)
    {
	p = ctl->pbuf[0] + of;
 
	if (ctl->type == LEFT)
	{
	    *p++ = ' ';
	    *p++ = '/';
	    *p   = '\0';
	}
	else if (ctl->type == RIGHT)
	{
	    *p++ = '\\';
	    *p   = '\0';
	}
    }
 
    STprintf(cp, frmt);
}

/*{
** Name: uld_tr_callback
**
** Description:
**      This function is meant to be called by TRformat. It keeps trailing
**	'\n's discarded by TRformat and inserts a '\0' after the formatted
**	string (since TRformat does not null-terminate the formatted buffer).
**	
** Inputs:
**	nl		if non-zero, add trailing newline
**	length		length of formatted buffer
**	buffer		formatted buffer from TRformat
**
** Outputs:
**	buffer		processed format buffer
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-feb-94 (swm)
**	    Added.
*/
i4
uld_tr_callback(nl, length, buffer)
PTR		nl;
i4		length;
char		*buffer;
{
	char *p = buffer + length;	/* end of buffer pointer */

	/* nl is used as a flag, it is not really a pointer */
	if (nl != (PTR)0)
		*p++ = '\n';
	*p = '\0';
}

/*{
** Name: uld_nl	- send newline to front ends
**
** Description:
**      send a new line to the front ends
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
**      13-jul-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
uld_nl( ULD_CONTROL *control )
{
    char	msg_buffer[5];		/* temp buffer to contain newline*/

    STprintf(&msg_buffer[0], "\n");
    ulx_sccerror(E_DB_OK, (DB_SQLSTATE *) NULL, 0, msg_buffer,
	STlength(msg_buffer), control->facility);
}

/*
** Name: prflsh	- Flush buffers
**
** Description:
**      This function flushes the buffers saved up by uld_prnode().  This is
**	done once for each level of the tree.
**
** Inputs:
**      init                            TRUE means just init the buffers
**      control                         Pointer to control structure
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
**          Adapted from prflsh() in jutil!fmttree.c in 4.0
*/
static VOID
prflsh( i4  init, register ULD_CONTROL *control, PTR sc930_trace )
{
    register i4         i;
    register i4	k;
    register char	*j;
    char		*r;
    char		*s;
 
    if (!init)
    {
	for (i = 0; i <= control->bmax; i++)
	{
	    /*
	    ** The STprintfs put nulls after every write, so change all
	    ** but last to spaces.
	    */
	    s = r = j = control->pbuf[i];
 
	    for (k = LLNMX; k--; j++)
	    {
		if (!*j)
		{
		    *j = ' ';
		    r = j;
		}
	    }
 
	    *r = '\0';
	    if (r != s)
	    {
		if (sc930_trace)
		{
			ult_print_tracefile(sc930_trace,SC930_LTYPE_QEP,s);
		} else
		{
		    /* send non-empty line to user */
		    ulx_sccerror(E_DB_OK, (DB_SQLSTATE *) NULL, 0, s, STlength(s),
		        control->facility);
		}
	    }
	    if (!sc930_trace)
	        uld_nl(control);		    /* blank line required */
	}
 
	while (i++ < control->lbl)
	{
	    if (!sc930_trace)
	    	uld_nl(control);		    /* start new line */
	}
    }

    for (i = 0; i <= control->bmax; i++)
    {
	j = control->pbuf[i];
	for (k = LLNMX; k--; *j++ = ' ')
	    ;
    }
 
    control->first++;
}
