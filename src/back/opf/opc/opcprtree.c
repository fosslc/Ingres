/*
**Copyright (c) 2004 Ingres Corporation
**
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qu.h>
#include    <cs.h>
#include    <tr.h>
#include    <st.h>
#include    <me.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>

#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <opckey.h>
#include    <opcd.h>
#include    <opxlint.h>
#include    <opclint.h>
 
/**
**
**  Name: OPCPRTREE.C - General-purpose tree printing functions
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
**          opc_prtree - Format and print a tree
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
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-oct-93 (swm)
**	    Bug #56448
**	    Made opc_prnode() portable by eliminating variable-sized
**	    arguments a1, a2 .. a9. Instead caller is required to pass
**	    an already-formatted buffer in frmt; this can be achieved
**	    by using a real varargs function from the CL. It is not
**	    possible to implement opc_prnode as a real varargs function
**	    as this practice is outlawed in main line code.
**	    At present, no callers pass variable arguments so no other
**	    changes were necessary.
**	15-feb-94 (swm)
**	    Bug #59611
**	    Replace STprintf calls with calls to TRformat which checks
**	    for buffer overrun and use global format buffer to reduce
**	    likelihood of stack overflow. The TRformat would need a callback
**	    routine, opc_tr_callback say, to put back any trailing line feed
**	    removed by TRformat and to put a '\0' string terminator in at
**	    the offset specified to the callback function.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool > 2Gig.
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
#define			LLNMX		1024
 
/* It probably wouldn't be useful to change the following constants */
 
/* At the root node */
#define			ROOT		1
 
/* Processing the left subnode of the father */
#define			LEFT		2
 
/* ditto for right */
#define			RIGHT		3

# define		PST_1WIDTH	3
# define		PST_1HEIGHT	3

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
   PARATREE        pts[PTMX];
   char         mod[MAXH];
   char         ned[MAXH];
   char         pbf[PMX + 1][LLNMX + 1];
} ULD_STORAGE;

/*
**  Definition of static variables and forward static functions.
*/
 
static PARATREE *
walk1(
	PTR                tnode,
	i4		   hh,
	ULD_CONTROL	   *control );

static VOID
walk2(
	PARATREE           *pnode,
	register ULD_CONTROL *control );

static VOID
walk3(
	PTR                tnode,
	PARATREE	   *pnode,
	i4		   ch,
	i4		   ph,
	ULD_CONTROL	   *control );

static VOID
prflsh(
	i4                init,
	register ULD_CONTROL *control );


/*{
** Name: opc_prtree	- Format and print a tree
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
**			OPS_CB		*opscb;
**			OPS_STATE	*global;
**
**			opscb = ops_getcb();
**			global = opscb->ops_state;
**
**			opc_prnode(control, "*****");
**			TRformat(opc_tr_callback, (i4 *)0,
**				(char *)&global->ops_trace.opt_trformat[0],
**				(i4)sizeof(global->ops_trace.opt_trformat),
**				"* %d *", node->data1);
**			opc_prnode(control, &global->ops_trace.opt_trformat[0]);
**			TRformat(opc_tr_callback, (i4 *)0,
**				(char *)&global->ops_trace.opt_trformat[0],
**				(i4)sizeof(global->ops_trace.opt_trformat),
**				"*%d %d*", node->data2, node->data1);
**			opc_prnode(control, &global->ops_trace.opt_trformat[0]);
**			opc_prnode(control, "*****");
**		}
**
**
**		then the call:
**
**		opc_prtree((PTR) Root, printnode, leftson, rightson, 8, 5);
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
*/
VOID
opc_prtree(
	PTR                root,
	VOID		   (*printnode)(),
	PTR		   (*leftson)(),
	PTR		   (*rightson)(),
	i4		   indent,
	i4		   lbl,
	i4                facility )
{
    register i4         i;
    register PARATREE	*pnode;
    ULD_CONTROL		control;
    ULD_STORAGE		*storage = 0;
    ULM_RCB		ulm_rcb;
    SIZE_TYPE		memleft;
    STATUS		status;
 
    control.facility = facility;
    ulm_rcb.ulm_facility = DB_ULF_ID;
    ulm_rcb.ulm_sizepool = sizeof(ULD_STORAGE) + 1024;
    ulm_rcb.ulm_blocksize = sizeof(ULD_STORAGE) + 128;
    memleft = sizeof(ULD_STORAGE) + 1024;
    ulm_rcb.ulm_memleft	    = &memleft;
 
    /* allocate the memory stream */
    status = ulm_startup(&ulm_rcb);
    if (status != E_DB_OK)
    {
	 (VOID) ule_format(E_UL0201_ULM_FAILED, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  (i4 *)NULL, 0);
	 return;
     }
 
    /* open a private memory stream and get the memory in one call */
    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(ULD_STORAGE);
    ulm_rcb.ulm_streamid_p = NULL;

    if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
    {
	(VOID) ule_format(E_UL0201_ULM_FAILED, (CL_ERR_DESC *)NULL,
		  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		  (i4 *)NULL, 0);
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
    prflsh(1, &control);
 
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
	prflsh(0, &control);
    }
 
    /* if we got any memory, return it */
    if (storage)
       ulm_shutdown(&ulm_rcb);
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
walk1(
	PTR                tnode,
	i4		   hh,
	ULD_CONTROL	   *control )
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
 
 control->modifier[h] = max(control->modifier[h], control->next_pos[h] - place);
 
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
walk2(
	PARATREE           *pnode,
	register ULD_CONTROL *control )
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
walk3(
	PTR                tnode,
	PARATREE	   *pnode,
	i4		   ch,
	i4		   ph,
	ULD_CONTROL	   *control )
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
** Name: opc_prnode	- Buffer up info about a node
**
** Description:
**      This function is meant to be called by the node formatting routine
**	supplied to opc_prtree().  It is passed a formmatted buffer.
**
** Inputs:
**      control                         Pointer to ULD_CONTROL structure
**	frmt				Pointer to formatted buffer
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
**	    Made opc_prnode() portable by eliminating variable-sized
**	    arguments a1, a2 .. a9. Instead caller is required to pass
**	    an already-formatted buffer in frmt; this can be achieved
**	    by using a real varargs function from the CL. It is not
**	    possible to implement opc_prnode as a real varargs function
**	    as this practice is outlawed in main line code.
**	    At present, no callers pass variable arguments so no other
**	    changes were necessary.
**	16-feb-94 (swm)
**	    Corrected comments in opc_prnode function header.
*/
VOID
opc_prnode( control, frmt )
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
** Name: opc_scctrace	- send output line to front end
**
** Description:
**      send a formatted output line to the front end
**
** Inputs:
**      msg_buffer                      ptr to null terminated message
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
opc_scctrace(
	ULD_CONTROL	   *control,
	char               *msg_buffer )
{
    TRdisplay(msg_buffer);
}

/*{
** Name: opc_nl	- send newline to front ends
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
opc_nl(
	ULD_CONTROL	*control )
{
    char		   msg_buffer[5];   /* temp buffer to contain newline*/
    STprintf(&msg_buffer[0], "\n");
    opc_scctrace(control, msg_buffer);
}

/*
** Name: prflsh	- Flush buffers
**
** Description:
**      This function flushes the buffers saved up by opc_prnode().  This is
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
prflsh(
	i4                init,
	register ULD_CONTROL *control )
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
		opc_scctrace( control, s ); /* send non-empty line to user */
	    opc_nl(control);		    /* blank line required */
	}
 
	while (i++ < control->lbl)
	    opc_nl(control);		    /* start new line */
    }

    for (i = 0; i <= control->bmax; i++)
    {
	j = control->pbuf[i];
	for (k = LLNMX; k--; *j++ = ' ')
	    ;
    }
 
    control->first++;
}

/*{
** Name: opc_1prnode	- Format a query tree node for the tree formatter
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
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	09-jul-93 (andre)
**	    removed FUNC_EXTERN for pst_1ftype() since PSFPARSE.H now contains
**	    its prototype declaration
*/
static VOID
opc_1prnode(
	PTR                node,
	PTR                control )
{
    char                buf[DB_MAXNAME + 3];   /* Big enough for anything */
    register PST_QNODE	*qnode = (PST_QNODE *) node;

    pst_1ftype(qnode, buf);
    buf[PST_1WIDTH] = '\0';
    opc_prnode(control, buf);

    MEfill(DB_MAXNAME, ' ', buf);
    buf[PST_1WIDTH] = '\0';

    /* Now do the symbol for the node */
    switch (qnode->pst_sym.pst_type)
    {

    case PST_UOP:
    case PST_BOP:
    case PST_COP:
    case PST_AOP:
	/* 2 blank lines */
	pst_1fidop(qnode, buf);
	if (qnode->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA)
	    STcat(buf, "M");
	buf[PST_1WIDTH] = '\0';
	opc_prnode(control,buf);
        CVla(qnode->pst_sym.pst_value.pst_s_op.pst_opno, buf);
	buf[PST_1WIDTH] = '\0';
	opc_prnode(control,buf);
	break;

    case PST_VAR:
	/* Do the variable number */
	CVla(qnode->pst_sym.pst_value.pst_s_var.pst_vno, buf);
	buf[PST_1WIDTH] = '\0';
	opc_prnode(control, buf);

	/* Do the attribute number */
	CVla(qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, buf);
	buf[PST_1WIDTH] = '\0';
	opc_prnode(control, buf);
	break;

    case PST_RESDOM:
	/* the resdom number and the target type */
	CVla(qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno, buf);
	buf[PST_1WIDTH] = '\0';
	opc_prnode(control, buf);
	switch (qnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
	{
	 case PST_ATTNO:
	    opc_prnode(control, "ATT");
	    break;

	 case PST_LOCALVARNO:
	    opc_prnode(control, "LVR");
	    break;

	 case PST_RQPARAMNO:
	    opc_prnode(control, "RQP");
	    break;

	 case PST_USER:
	    opc_prnode(control, "USR");
	    break;

	 case PST_DBPARAMNO:
	    opc_prnode(control, "DBP");
	    break;

	 case PST_BYREF_DBPARAMNO:
	    opc_prnode(control, "BDP");
	    break;

	 case PST_RSDNO:
	    opc_prnode(control, "RSD");
	    break;

	 default:
	    opc_prnode(control, "???");
	    break;
	}
	break;

    case PST_RULEVAR:	/* Nothing special yet */
    default:
	/* 2 blank lines */
	opc_prnode(control,buf);
	opc_prnode(control,buf);
	break;

    }
}

/*{
** Name: opc_left	- Return left child of node
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
opc_left(
	PTR                node )
{
    return ((PTR) (((PST_QNODE *) node)->pst_left));
}

/*{
** Name: opc_right	- Return right child of node
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
opc_right(
	PTR                node )
{
    return ((PTR) (((PST_QNODE *) node)->pst_right));
}

/*{
** Name: opc_treedump	- Format and print the contents of a query tree.
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
*/
DB_STATUS
opc_treedump(
	PST_QNODE   *tree )
{
    opc_prtree((PTR) tree, opc_1prnode, opc_left, opc_right, 
	PST_1HEIGHT, PST_1WIDTH, 0);

}
