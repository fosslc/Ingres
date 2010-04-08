/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM sqs_ptx
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
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

/* external routine declarations definitions */
#include    <ex.h>
#define             OPX_EXROUTINES      TRUE
#define		    OPN_TREE		TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNTREE.C - routines to create structurally unique trees
**
**  Description:
**      This file contains the routines which will create structurally 
**      unique trees.
**
**  History:    
**      11-may-86 (seputis)    
**          initial creation
**	04-feb-91 (jkb)
**	    Define NO_OPTIM for sqs_ptx
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      15-dec-91 (seputis)
**          fix b33551 make better guess for initial set of relations to enumerate
**	15-dec-91 (seputis)
**	    add trace point op196 to have a workaround for b40687
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	04-feb-1998 (somsa01) X-integration of 433465 from oping12 (somsa01)
**	    In opn_iarl(), if cop->opo_outer->opo_outer is NULL, the guess
**	    that's being made is not complete. Due to the complexity of the
**	    code, it's not worth it to proceed. Therefore, just exit out of
**	    the procedure, and allow the natural enumeration process to
**	    continue further.  (Bug #87703)
**	11-Mar-2008 (kschendel) b122118
**	    thandler trap handler not used anywhere, delete it.
[@history_line@]...
**/

/*}
** Name: OPN_DEBUG - debugging state for enumeration phase
**
** Description:
**      This structure contains variables which are used for debugging
**      purposes only
**
** History:
**     10-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPN_DEBUG
{
    i4              opn_level;          /* used for controlling indentation
                                        ** during printing of trace information
                                        ** - i.e. current number of spaces to
                                        ** indent
                                        */
    bool            opn_goflg;          /* TRUE - means look for a particular
                                        ** tree structure in opn_strtw
                                        */
    OPN_TDSC	    opn_strtw;		/* don't do any processing until 
                                        ** a tree with this initial 
                                        ** subsequence in opn_tdsc is generated 
                                        ** - used to debug a particular join
                                        ** structure
                                        */
}   OPN_DEBUG;
#define             OPN_JCOCOUNT         8
/* maximum number of CO nodes expected for join of 2 relations */
#define             OPN_PRCOCOUNT       3
/* maximun number of CO nodes required for project-restrict or keying of
** a single relation */

/*}
** Name: OPN_SUNIQUE - used to produce structurally unique trees
**
** Description:
**      This structure contains the "global state" variables used to produce
**      structurally unique trees.
**
**      opn_tdsc holds the number of leaves in each subtree of a node. 
**      opn_tdsc[0] is the total number of leaves (root node), then comes the
**      numbers of leaves in each subtree, left to right.
**      Then comes the descriptions of these subtrees, right to left.
**
**          Example:
**				  A 
**			     /         \
**			    B            C
**			 /    \        /   \
**		       H       I      D     E
**		     /  \     / \    / \
**		    L    M   J   K  F   G
**      
**  	opn_tdsc will contain "7,4,3,2,1,1,1,2,2,1,1,1,1"
**
**      The construction process for this particular example is described below
**
**      A = 7 -number of leaves in root
**        number of leaves in subtree left to right
**        B = 4	 -left subtree (leafs L,M,J,K)
**        C = 3  -right subtree (leafs F,G)
**          descriptions of subtrees right to left
**          description of subtree C
**            number of leaves in subtree C left to right
**            D = 2 - left subtree (leaves F,G)
**            E = 1 - right subtree (leaf E)
**              descriptions of subtrees right to left
**              subtree E is a leaf and has no description
**              description of subtree D
**                number of leaves in subtree D left to right
**                F = 1 - left subtree (leaf F)
**                G = 1 - right subtree (leaf G)
**                  description of subtrees right to left
**                  subtree G is a leaf and has no description
**                  subtree F is a leaf and has no description
**	    description of subtree B
**            number of leaves in subtree B left to right
**            H = 2 - left subtree (leaves L,M)
**            I = 2 - right subtree (leaves J,K)
**              description of subtrees right to left
**              description of subtree I
**                number of leaves in subtree I left to right
**                L = 1 - left subtree (leaf L)
**                M = 1 - right subtree (leaf M)
**                  description of subtrees right to left
**                  subtree M is a leaf and has no description
**                  subtree L is a leaf and has no description
**              description of subtree H
**                number of leaves in subtree H left to right
**                J = 1 - left subtree (leaf J)
**                K = 1 - right subtree (leaf K)
**                  description of subtrees right to left
**                  subtree K is a leaf and has no description
**                  subtree J is a leaf and has no description
**          
**
** History:
**     10-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPN_SUNIQUE
{
    OPS_SUBQUERY    *opn_subquery;      /* ptr to subquery being analyzed */
    OPN_TDSC	    opn_tdsc;		/* opn_gen creates a description of
                                        ** a tree in opn_tdsc, opn_mjtree 
                                        ** uses this to create the tree 
                                        */
    OPN_ITDSC	    opn_stack[OPN_MAXSTACK];/* opn_gen pushes and pops 
                                        ** pointers (i.e. offsets) into 
                                        ** opn_tdsc on this stack 
                                        */
    OPN_ITDSC	    *opn_sp;            /* pointer into opn_stack */
    OPN_CTDSC	    *opn_dscptr;        /* marks position in opn_tdsc (tree 
                                        ** description ) where
                                        ** opn_mjtree is currently working 
                                        */
    OPN_JTREE       *opn_free;          /* pointer to the next free element
                                        ** which can be used for a structurally
                                        ** unique tree - this list began at
                                        ** opn_sroot and uses opn_free->opn_next
                                        ** to obtain the next element
                                        */
    OPN_DEBUG       opn_debug;          /* structure used for debugging only
                                        */

/* the following variables are used for "copy mode" which is done when two
** subtrees with the same number of leaves and the same parent are being
** generated
*/
    bool            opn_copymode;       /* TRUE - then we are
                                        ** copying the description of one
                                        ** subtree to another so as to generate
                                        ** only structurally unique trees 
                                        */
    OPN_ITDSC	    opn_from;           /* contains index into opn_tdsc where
                                        ** copying will begin - see opn_copymode
                                        */
    OPN_ITDSC       opn_end;            /* contains index into opn_tdsc where
                                        ** copying will end - see opn_copymode
                                        */
    OPN_ITDSC	    opn_sd[OPN_MAXNODE];/* for the corresponding element of
                                        ** opn_tdsc, it contains index into
                                        ** opn_tdsc of another element
                                        ** where the subtree description
                                        ** begins
                                        */
}   OPN_SUNIQUE;


/*{
** Name: opn_seqv	- fixup opn_cpeqc and opn_psz
**
** Description:
**      The routine will determine if the left child is structurally 
**      identical to the right child... and record this fact.  A
**      "structure id" was assigned in opn_cpeqc, in which two adjacent
**      children would have the same id IFF they had the same structure.
**      This fact is used to partition the subtrees into "equivalence
**      classes" (not the same as joinop equivalence classes!!) which
**      will be used to avoid evaluating identical trees.
**
** Inputs:
**      node                            ptr to tree node to be fixed up
**          .opn_cpeqc                  contains structure ids
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opn_seqv(
	OPN_JTREE          *node)
{
    OPN_LEAVES	        numchild;		/* number of children in node */
    OPN_CHILD		*idptr;                 /* ptr to structid for current
                                                ** child
                                                */
    OPN_CHILD           *numsubtrees;           /* ptr into opn_cpeqc, which
                                                ** will contain the number
                                                ** of subtrees in the
                                                ** equivalence class
                                                */
    OPN_CHILD           previousid;             /* structid of previous child
                                                */
    OPN_JTREE           **child;                /* ptr to tree node of child
                                                */
    OPN_LEAVES		*partcount;             /* ptr to number of leaves
                                                ** in the respective partition
                                                ** - for binary trees this will
                                                ** be equal to opn_numleaves
                                                ** IFF the left and right
                                                ** subtrees are structurally 
                                                ** identical
                                                */

    idptr = node->opn_cpeqc;                    /* look at leftmost child */
    numsubtrees = idptr - 1;                    /* opn_cpeqc changes its' 
                                                ** meaning after an assignment
                                                ** is made thru this ptr
                                                ** - it will contain the
                                                ** number of adjacent (brothers)
                                                ** structurally unique subtrees
                                                */
    previousid = *idptr + 1;			/* make sure previousid is
                                                ** different so that the
                                                ** first pass through loop
                                                ** will not find an equality
                                                */
    partcount = node->opn_psz - 1;		/* ptr to number of leaves
                                                ** in the current partition
                                                */
    child = node->opn_child;			/* ptr to tree node of first
                                                ** child being analyzed
                                                */

    for (numchild = node->opn_nchild; numchild--;)
    {						/* for each child */
	OPN_CHILD                  structid;	/* this is an ID which is
                                                ** equal to this child's
                                                ** brother if both children's
                                                ** subtrees are structurally
                                                ** identical
                                                */
	
	structid = *idptr++;
	if (previousid == structid)		/* if two subtrees have same
						** structure - note that during
                                                ** the first pass of this loop
                                                ** this will be false
                                                */
	{   /* subtree belongs to same partition so increment opn_cpeqc count */
	    (*numsubtrees)++;			/* indicate the number of
						** subtrees in this equivalence
						** class 
                                                */
	    *partcount += (*child++)->opn_nleaves;	/* indicate the total number of
						** leaves for the subtrees in
						** this equivalence class 
                                                */
	}
	else
	{   /* new partition is found - so update next element of opn_cpeqc */
	    *(++numsubtrees) = 1;		/* otherwise set the number of
						** subtrees to 1 
                                                */
	    *(++partcount) = (*child++)->opn_nleaves; /* add the number of 
						** leaves to those in this 
                                                ** subtree 
                                                */
	    previousid--;			/* the numbers in opn_cpeqc
						** were entered in increasing
						** order from right to left,
						** since we are working left to
						** right, decrement j to next
						** value 
                                                ** - since there was no match
                                                ** generate the "next id"
                                                ** - we should be able to 
                                                ** replace this statement with
                                                ** previousid = structid ??
                                                */
	}
    }
}

/*{
** Name: opn_mtree	- make subtree from tree description
**
** Description:
**      Recursive routine to create subtree from tree description
**
** Inputs:
**      sut                             ptr to state variable describing
**                                      a structurally unique tree
**	node                            a node of the tree which we will 
**                                      attach a subtree to.
**	degree                          number of leaf nodes in the subtree to 
**                                      be attached - this must be 1 or more
**
** Outputs:
**	dsc                             a description of the attached subtree 
**                                      is put here so that the existence of 
**                                      identical subtrees may be determined 
**                                      by calling routine (i.e., previous 
**                                      invocation of mjtree) and opn_cpeqc
**                                      set appropriately.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opn_mtree(
	OPN_SUNIQUE        *sut,
	OPN_CTDSC	   degree,
	OPN_JTREE          *node,
	OPN_TDSC	   dsc)
{
    OPN_CHILD		*chdeqcptr;	    /* ptr to number of subtrees
                                            ** in this partition
                                            */
    OPN_JTREE           **cp;               /* ptr to the element of opn_child
                                            ** representing the current child
                                            ** being analyzed
                                            */
    OPN_CTDSC		*startdsc;          /* ptr to initial position of 
                                            ** opn_tdsc that this routine
                                            ** began working on; i.e. initial
                                            ** child's number of leaves
                                            */
    OPN_ISTLEAVES	*prt;		    /* partition index - offsets into
                                            ** opn_pra, opn_prb
                                            */


    startdsc = sut->opn_dscptr;		    /* opn_dscptr points to initial
					    ** position of opn_tdsc that will
					    ** be worked on; i.e. initial
                                            ** child's number of leaves
                                            */
    chdeqcptr = node->opn_cpeqc;	    /* beginning of array of number
                                            ** of subtrees per child, left to
                                            ** to right
                                            */
    cp = node->opn_child;                   /* ptr to current child being
                                            ** analyzed
                                            */
    {	/* for each subtree eminating from this node, left to right */
	OPN_LEAVES	       numleaves;   /* sum of leaves in children
                                            ** analyzed so far
					    */
	OPN_LEAVES	       chldcnt;     /* number of children of this
                                            ** node
                                            */
	OPN_LEAVES	       *cszp;       /* used to init the child leaf
                                            ** size array opn_csz
                                            */

	numleaves = 0;			    /* no leaves seen yet */
        chldcnt = 0;                        /* no children seen yet */
	cszp = node->opn_csz;               /* used in child leaf size array */
	prt = node->opn_pi;                 /* init partition index for each
                                            ** child
                                            */
	do						
	{
	    OPN_JTREE          *jt;	    /* join tree of current child being
                                            ** analyzed
                                            */

	    chldcnt++;                      /* number of children seen
                                            ** - at least one child will be seen
                                            ** if this routine is called
                                            */
	    chdeqcptr++;		    /* keep pointer into opn_cpeqc
					    ** positioned correctly 
					    */
	    jt = *cp++ = sut->opn_free;      /* allocate first node from free
                                            ** list to child being analyzed
                                            */
	    sut->opn_free = sut->opn_free->opn_next; /* set to next available
                                            ** free element
                                            */
	    jt->opn_nleaves = *cszp++ = *sut->opn_dscptr; /* set child leaf 
                                            ** size , and parent's child leaf
                                            ** size array also
                                            */
            jt->opn_sbstruct[0]=0;          /* initialize for leaf nodes */
	    *prt++ = numleaves;		    /* set the array which holds
					    ** indicies into the opn_prtna and
					    ** opn_prtnb arrays 
                                            */
	}
	while    ((numleaves += *sut->opn_dscptr++) < degree); /* we are done 
                                            ** when the sum of the number of 
                                            ** leaves in the subtrees equals 
                                            ** the number of leaves for this 
                                            ** node 
                                            */
	node->opn_nchild = chldcnt;	    /* save the number of children 
                                            ** of this parent
                                            */
    }
    {	/* for each subtree (child) of this node, right to left */
	i4                    equcnt;	    /* number of structurally unique
                                            ** subtree is generated
                                            */
	OPN_CTDSC	       *lastdsc;    /* "opn_mjtree" format ptr
                                            */
	OPN_CTDSC	       *curdsc;     /* "opn_mjtree" format ptr
                                            */
	OPN_CTDSC	       *dscptr;     /* "opn_gen" format ptr
                                            */
	equcnt = 1;			    /* incremented every time a
					    ** structurally different
					    ** subtree is generated 
                                            */
	lastdsc = curdsc = dsc;		    /* dsc is where we put the
					    ** descriptor of the subtree we
					    ** are generating 
                                            */
	dscptr = sut->opn_dscptr;            /* points to description
                                            ** of the first (rightmost) child
                                            */
	while (--dscptr >= startdsc)	    /* look at each tree description
                                            ** until the initial (leftmost)
                                            ** child is reached
                                            */
	{
	    OPN_CTDSC           *start;	    /* used to remember start of
					    ** description of previously
					    ** generated subtree 
                                            */

	    cp--;			    /* get ptr to tree node of child */
	    start = curdsc;		    /* save ptr to previously generated
                                            ** subtree
                                            */
	    *curdsc = *dscptr;		    /* copy number of leaves in subtree
                                            ** to dsc 
                                            */
	    if (*dscptr > 1)		    /* if not a leaf then generate
                                            ** the subtrees
                                            */
	    {
		opn_mtree (sut, *dscptr, *cp, curdsc + 1); /* make the 
                                            ** subtree 
                                            */
		if (curdsc == dsc)	    /* if this is the first subtree
					    ** for this node 
                                            */
		    while (*++curdsc);	    /* skip over what mjtree just added
                                            */
		else
		{
		    while (*lastdsc++ == *curdsc++);/* otherwise compare what 
                                            ** was added this time with what
					    ** was added for the previous
					    ** subtree 
                                            */
		    if (*--curdsc)	    /* if the descriptors were not
					    ** the same then ...
                                            */
		    {
			++equcnt;	    /* bump to a new number to
					    ** indicate new structure 
                                            */
			while (*++curdsc);  /* skip over rest of what was
					    ** just added by mjtree 
                                            */
		    }
		}
	    }
	    else
		*++curdsc = 0;		    /* make sure dsc ends in null */
	    *(--chdeqcptr) = equcnt;	    /* subtrees with identical
					    ** structures are indicated by
					    ** having identical numbers in
					    ** opn_cpeqc.  the contents of
					    ** opn_cpeqc are used to set
					    ** opn_psz and reset opn_cpeqc
					    ** in routine seteqc. 
                                            */
	    lastdsc = start;		    /* set lastdsc to beginning of
					    ** what was just added by mjtree 
                                            */
	}
	{
	    OPN_CTDSC		*st;        /* ptr used to save subtree
                                            ** structure of children in
                                            ** parent node
                                            */
            st = node->opn_sbstruct;
	    while ( *st++ = *dsc++ );	    /* copy description of children
                                            ** to the tree node
                                            */
	}
	node->opn_npart = equcnt;           /* save the number of structurally
                                            ** unique trees eminating from
                                            ** this node
                                            */
	opn_seqv (node);		    /* fixup opn_cpeqc and opn_psz 
                                            ** arrays 
                                            */
    }
}

/*{
** Name: opn_mjtree	- make a tree from a tree description in opn_tdsc
**
** Description:
**      This routine will create a tree from a tree description in
**      opn_tdsc.
**
** Inputs:
**      sut                             ptr to state variable used to
**                                      produce structurally unique trees
**	    .opn_tdsc			array of integers containing
**                                      tree description
**
** Outputs:
**      sut
**       .opn_subquery                  ptr to subquery being analyzed
**	    .ops_global			ptr to global state variable
**		.ops_estate		enumeration global state variable
**                  .opn_sroot          root of tree created from description
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-may-86 (seputis)
**          initial creation from mkjtree in makejtree.c
[@history_line@]...
*/
static VOID
opn_mjtree(
	OPN_SUNIQUE        *sut)
{
    OPN_CTDSC		leaves;         /* number of leaves in the root node */
    OPN_TDSC            desc;		/* holds a tree description
                                        ** - (slightly
					** different form than opn_tdsc),created
					** and used by mjtree to determine
					** if subtrees have identical structures
					** -tree description in the "opn_mjtree"
					** format (i.e. 3 1 2 1 1 4 2 1 1 2 1 1)
					** corresponds to tree in "opn_gen"
                                        ** format of tree description
					** (i.e., 7 4 3 2 1 1 1 2 2 1 1 1 1 ) 
                                        */
/*
**
**				  X
**			     /          \
**			    X            X
**			 /    \        /   \
**		       X       X      X     X
**		     /  \     / \    / \
**		    X    X   X   X  X   X
**
**             this is the tree described above
*/
    OPN_JTREE           *rootnode;	/* ptr to root node of join tree
                                        ** about to be generated */

    rootnode = sut->opn_subquery->ops_global->ops_estate.opn_sroot; /* get 
					** ptr to root node */
    sut->opn_free = rootnode->opn_next; /* reuse the existing list of 
                                        ** OPN_JTREE nodes - this assignment
                                        ** "allocates" the root node by skipping
                                        ** over it.  When the opn_sroot list was
                                        ** allocated, the proper number of nodes
                                        ** were defined
                                        */
    rootnode->opn_nleaves = leaves = sut->opn_tdsc[0]; /* number of 
                                        ** leafs in the root node
                                        ** - which was allocated
                                        ** in the previous statement
                                        */

    sut->opn_dscptr = sut->opn_tdsc + 1;  /* get ptr to tree descriptors for the
                                        ** root node (which has already been
                                        ** allocated)
                                        */
    if (leaves > 1)			/* call routine only if more than single
                                        ** node tree
                                        */
	opn_mtree ( sut, leaves, rootnode, desc); /* recursively allocate the
                                        ** remainder of the tree */
    else
	rootnode->opn_sbstruct[0] = 0;	/* init substructure descriptor */
}

/*{
** Name: opn_gen	- generate structurally unique tree
**
** Description:
**	Generate all structurally unique trees with "leaves" leaves
**	and a maximum fanout from any node of "degree".
**	for each tree call opn_process().  opn_process will evaluate
**      the tree after attaching all combinations of relations to the leaves.
**      If a particular
**      tree is the best cost tree so far then the CO tree will be place
**      in the ops_bestco field associated with the subquery.
**
** Inputs:
**      sut                             ptr to tree building state variable
**      tdsc_offset                     offset into sut->opn_tdsc at which
**                                      to place tree description
**      leaves                          - 0 means generate a description
**                                      for the element on the opn_stack
**                                      - >0 means generate subtrees with
**                                      "leaves" number of leaves
**      degree                          number of subtrees to generate (this
**                                      parameter can be eliminated if we
**                                      assume binary trees)
**                                      - ignored if leaves = 0
**      maxleaves                       maximum number of leaves in the
**                                      subtree (is this parameter needed?)
**                                      - ignored if leaves = 0
**
** Outputs:
**      sut->
**          opn_global->
**              ops_estate.
**                  opn_subquery
**                      ops_bestco      - best cost ordering will be
**                                      returned.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-may-86 (seputis)
**          initial creation
**	17-nov-99 (inkdo01)
**	    Change opn_process call to return signal status.
[@history_line@]...
*/
static OPN_STATUS
opn_gen(
	OPN_SUNIQUE        *sut,
	OPN_ITDSC	   tdsc_offset,
	OPN_CTDSC	   leaves,
	i4                degree,
	OPN_CTDSC          maxleaves)
{
    
    i4                  firstchild;	/* 0 if not the first child of a node
				        ** 1 otherwise - used to make sure
                                        ** each node (except leaf node) has at
                                        ** least two children
                                        */
    OPN_ITDSC           start;          /* offset into opn_tdsc whose entry
                                        ** will be the number of leaves in the
                                        ** subtree about to be generated */

    firstchild = 0;			/* set to 1 if this if first subtree
                                        ** to be processed
                                        */
    if (!leaves)
    {	/* if current portion of subtree is finished */
	if (sut->opn_copymode && (tdsc_offset == sut->opn_end))
					/* if we were in copy mode and
					** have finished copying 
                                        */
	    sut->opn_copymode = FALSE;	/* then set copy mode off */

	if (sut->opn_sp <= sut->opn_stack)	
	{   /* if there are no more unfinished subtrees then process this tree 
            ** description 
            */
	    opn_mjtree( sut );		/* - make join tree from the description
                                        ** and place root in opn_sroot
                                        */
	    return(opn_process (sut->opn_subquery, *sut->opn_tdsc)); /*enumerate
                                        ** relations
                                        ** on join tree with "*sut->opn_tdsc"
                                        ** leaves, and save best CO if found
                                        */
	}
	else
	{   /* get next subtree to work on (index into sut->opn_tdsc) */
	    sut->opn_sp--;		/* pop stack */
	    start = (*sut->opn_sp);	/* save top of stack in "start", which
                                        ** will contain the offset in opn_tdsc
                                        ** of the number of leaves in the
                                        ** subtree to be generated
                                        */
	    sut->opn_sd[start] = tdsc_offset; /* remember that the description 
                                        ** of this subtree about to be 
                                        ** generated (of size 
                                        ** sut->opn_tdsc[start]) starts at
					** position tdsc_offset in the opn_tdsc
                                        ** array
                                        */
	    leaves = sut->opn_tdsc[start]; /* get the number of leaves in
					** this subtree 
                                        */
	    if ((start < (tdsc_offset - 1)) && /* is the brother's subtree
                                        ** descriptor on the stack already ? */
		(leaves == sut->opn_tdsc[start + 1]) && /* "start + 1" is the
                                        ** location of the number of leaves 
                                        ** in the brother */
		(!sut->opn_copymode))
	    {	/* if the subtree we are about to generate is of the same
		** size as its brother (and we are not currently in copy mode) 
                ** - copy mode is needed to ensure that duplicate tree
                ** structures are not generated when the left and right branch
                ** have the same number of leaves.  
                */
		sut->opn_copymode = TRUE; /* turn on copy mode */
		sut->opn_from = sut->opn_sd[start + 1]; /* start at the
					** brother's descriptor
                                        */
		sut->opn_end = 2 * tdsc_offset - sut->opn_from; /*  length
                                        ** of bother's descriptor == 
                                        ** (tdsc_offset - sut->opn_from)
                                        ** - so this is the position at which
                                        ** the copy of the bother's descriptor
                                        ** will be complete
                                        */
	    }
	    firstchild++;		/* remember that we are generating this
                                        ** first subtree (of possibly many)
					** of the node just popped from
					** the stack 
                                        */
	    degree = OPN_MAXDEGREE;	/* start "degree" at its highest
					** value, decrement by one as
					** we generate more subtrees
					** for this node 
                                        */
	    maxleaves = leaves;		/* maxleaves is used to make sure
					** that if subtree b is to the
					** right of subtree a  then b
					** will not have more leaves
                                        */
	}
    }

    {
	OPN_CTDSC	minleaves;      /* lower bound for number of leaves to
                                        ** generate in the left subtree - used
                                        ** so that the number of leaves in left
                                        ** subtree is greater than the number of
                                        ** leaves in the right subtree, thus
                                        ** avoiding duplicate tree structures
                                        */
	OPN_CTDSC	leafcount;	/* number of leaves to place in the
                                        ** subtree
                                        */
	OPN_STATUS	sigstat;
	minleaves = (leaves + degree - 1 ) /  degree ;  /* minimum number of 
					** leaves for this subtree */
	if (sut->opn_copymode)
	    leafcount = sut->opn_tdsc[sut->opn_from++]; /* if in copy mode 
					** then the max number of leaves for 
                                        ** the left subtree is the same as this
					** position in brother subtree - this
                                        ** will avoid producing duplicate trees
                                        ** in the case of an "ancestor" having
                                        ** left and right subtrees with the
                                        ** same number of nodes
					*/
	else
	{
	    if ((leafcount = leaves - firstchild) > maxleaves)
		leafcount = maxleaves;	/* otherwise start
					** at "leaves" (or "leaves - 1" if this
                                        ** is the first subtree of this node) 
                                        */
	}
	for (; leafcount >= minleaves; leafcount--) 
	{   /* for each possible value for number of leaves for this subtree 
            ** - if degree = 1 then the right subtree will be generated and
            ** is = minleaves so only one iteration of this loop will occur, 
            ** this makes sense since the left subtree has determined 
            ** the number of leaves remaining for the right subtree
            ** - if degree = 2 then the left subtree count is inserted and
            ** there could be many iterations of this loop, but it will stop
            ** if the number of leaves in the left subtree would be less than
            ** the right subtree
            */
	    sut->opn_tdsc[tdsc_offset] = leafcount; /* put the number of leaves
					** of the tree description to generate
                                        ** into sut->opn_tdsc
                                        */
	    if (leafcount > 1)
		*sut->opn_sp++ = tdsc_offset; /* if more than one leaf in the
					** subtree then the subtree will have
                                        ** to be expanded at a later point 
                                        */
	    sigstat = opn_gen (sut, tdsc_offset + 1, leaves - leafcount, degree - 1, 
		leafcount);		/* generate the next subtree of this 
                                        ** node (it will only have 
					** "leaves-leafcount" leaves to work 
                                        ** with, and a max of "degree-1" more 
                                        ** subtrees where no subtree has more
					** than "leafcount" leaves 
                                        */
	    if (sigstat != OPN_SIGOK) return(sigstat);
	    if (leafcount > 1)
		sut->opn_sp--;		/* if we put a subtree on then
					** take it off and try another
					** value for the number of
					** leaves in this subtree 
                                        */
	}
	if (firstchild)
	    *sut->opn_sp++ = start;	/* if this was the first subtree of a 
                                        ** node replace the indication of this 
                                        */
    }
    return(OPN_SIGOK);
}

/*{
** Name: opn_inode	- initialize a join tree node for leaf node
**
** Description:
**      Initialize a join tree node for purposes of making an initial guess 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to leaf node to initialize
**      var                             variable to be placed in node
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-oct-91 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opn_inode(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*nodep,
	OPV_IVARS	var)
{
    nodep->opn_pra[0] = var;
    nodep->opn_prb[0] = var;
    MEcopy((PTR)&subquery->ops_vars.opv_base->opv_rt[var]->opv_maps.opo_eqcmap, 
	sizeof(nodep->opn_eqm), (PTR)&nodep->opn_eqm);
				    /* - provides equivalence classes made
				    ** available by index */
    nodep->opn_rlasg[0] = var;	    /* order in which relations in 
				    ** opn_rlmap are assigned to leaves
				    ** - e.g. a leaf node would contain one
				    ** element */
    BTset((i4)var, (char *)&nodep->opn_rlmap);
}

/*{
** Name: opn_ijnode	- initialize a join tree node for key join
**
** Description:
**      Initialize a join tree node for purposes of making an initial guess 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to join node to initialize
**      leftvar                         variable to be placed in left leaf
**	rightvar			variable to be placed in right leaf
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-oct-91 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opn_ijnode(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*nodep,
	OPV_IVARS	leftvar,
	OPV_IVARS	rightvar)
{
    nodep->opn_pra[0] = leftvar;
    nodep->opn_pra[1] = rightvar;
    nodep->opn_prb[0] = leftvar;
    nodep->opn_prb[1] = rightvar;
    MEcopy((PTR)&subquery->ops_vars.opv_base->opv_rt[leftvar]->opv_maps.opo_eqcmap, 
	sizeof(nodep->opn_eqm), (PTR)&nodep->opn_eqm);
    BTor((i4)subquery->ops_eclass.ope_ev,
	(char *)&subquery->ops_vars.opv_base->opv_rt[rightvar]->opv_maps.opo_eqcmap, 
	(char *)&nodep->opn_eqm);
    nodep->opn_rlasg[0] = leftvar; 
    nodep->opn_rlasg[1] = rightvar; /* order in which relations in 
				    ** opn_rlmap are assigned to leaves
				    ** - e.g. a leaf node would contain one
				    ** element */
    BTset((i4)leftvar, (char *)&nodep->opn_rlmap);
    BTset((i4)rightvar, (char *)&nodep->opn_rlmap);
}

/*{
** Name: opn_ihandler	- exception handler enumeration tree routine
**
** Description:
**	If any exception occurs on the initial best guess code then
**	trap it and attempt to continue without making a guess
** Inputs:
**      args                            arguments which describe the 
**                                      exception which was generated
**
** Outputs:
**	Returns:
**          EXDECLARE                  
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the calling frame to the point at which
**          the exception handler was declared.
**
** History:
**	27-mar-94 (ed)
**          initial creation b59461
[@history_line@]...
*/
static STATUS
opn_ihandler(
	EX_ARGS            *args)
{
	return (EXDECLARE);	    /* return to invoking exception point
				    ** - skip the best guess code since an
				    ** exception has occurred
                                    */
}

/*{
** Name: opn_iarl	- initial attachment of relations to leaves
**
** Description:
**      In order to reduce the search space and time out more quickly,
**      a hueristic guess is made as to the set of indexes which
**      most probably will be in the final plan.  This initial set
**      of relations will be analyzed first and will hopefully produce
**      a fairly optimial plan. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**      iarlp                           ptr to set of relations to use
**	relcountp			ptr to number of relations in set
**
**	Returns:
**	    number of relations in initial set, or 0 if no initial set exists
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-mar-90 (seputis)
**          initial creation
**	30-apr-90 (seputis)
**	    enhanced to fix some more shape query problems, by doing
**	    a transitive closure of the tuple keying
**	18-jul-90 (seputis)
**	    fix b31644
**      16-oct-90 (seputis)
**          - partial fix for b14544, eliminate complete flag cases
**	    from consideration
**	21-feb-91 (seputis)
**	    - check for default handling of complete flag
**	18-apr-91 (seputis)
**	    - remove unused structure for lint
**      23-jun-92 (seputis)
**          - fix b33551 - infinite loop when 2 secondary indices of different
**          relations can be joined, and appropriate statistics are available
**          - b44884 the script for this bug found the problem, but the bug
**          reported was actually different.
**	8-feb-93 (ed)
**	    - fix incorrectly initialized outer join id
**	31-mar-94 (ed)
**	    - b59461 - create float exception handler to skip this processing 
**	04-feb-1998 (somsa01) X-integration of 433465 from oping12 (somsa01)
**	    If cop->opo_outer->opo_outer is NULL, the guess that's being made
**	    is not complete. Due to the complexity of the code, it's not worth
**	    it to proceed. Therefore, just exit out of the procedure, and
**	    and allow the natural enumeration process to continue further.
**	    (Bug #87703)
**	19-jul-00 (hayke02)
**	    We now don't eliminate an index if OPV_EXPLICIT_IDX is set in
**	    opv_mask - this flag indicates that the index has been explicitly
**	    defined in the query and therefore should not be eliminated
**	    even if it is more expensive than just using the base relation.
**	    This change fixes bug 101959.
[@history_template@]...
*/
static i4
opn_iarl(
	OPS_SUBQUERY	    *subquery,
	OPN_SSTLEAVES	    *iarlp)
{
    OPV_IVARS	    index1var;	    /* index variable currently
				    ** being looked at */
    OPV_RT	    *vbase;
    OPN_JTREE	    leftnode;	    /* node used to calculate
				    ** costs of project restricts
				    ** on all relations */
    OPN_JTREE	    rightnode;	    /* node representing the
				    ** base relation of the key join */
    OPN_JTREE	    joinnode;	    /* node representing the
				    ** key join estimates */
    OPV_IVARS	    varcount;	    /* number of relations in the 
				    ** current first set of relations
				    ** to be enumerated */
    bool	    indexused;	    /* TRUE if at least one index was
				    ** used in the analysis */
    OPE_ET	    *ebase;	    /* ptr to base of array of ptrs to
                                    ** equilvalence class elements */
    OPE_IEQCLS	    maxeqcls;	    /* max number of equivalence classes */
    OPN_SUBTREE	    *subtp;
    OPN_EQS	    *eqclp;
    OPN_RLS	    *rlmp;

    OPO_COST	    bestcost[OPV_MAXVAR]; /* an array of best costs for
				    ** each variable or base index */
    OPO_TUPLES	    tuplecount[OPV_MAXVAR]; /* an array of tuple counts
				    ** for each variable after a project restrict
				    ** or the best key join strategy */
    OPO_TUPLES	    otuplecount[OPV_MAXVAR]; /* original tuple count used
				    ** to calculate ratios used to adjust
				    ** keying estimates, created in case
				    ** opv_pr gets deallocated during
				    ** execution of this procedure */
    OPV_IVARS	    outervar[OPV_MAXVAR]; /* this is the var which was
				    ** was used to key into the var
				    ** to set the bestcost, if no keying
				    ** then OPV_NOVAR is used */
    OPS_STATE	    *global;
    EX_CONTEXT	    excontext;	    /* context block for exception 
                                    ** handler*/
    OPN_STATUS	    sigstat = OPN_SIGOK;

    if ((subquery->ops_vars.opv_rv == subquery->ops_vars.opv_prv)
	||
	(subquery->ops_vars.opv_prv <= 1))
	return(0);		    /* no secondary indexes exist or
				    ** only one primary relation so all
				    ** indexes will be looked at immediately
				    ** anyways */
    if ( EXdeclare(opn_ihandler, &excontext) == EX_DECLARE ) /* set
				    ** up exception handler to 
				    ** recover from out of memory
				    ** errors */
    {	/* create exception handler to return without producing a best
	** guess if an exception occurs, but continue on in enumeration
	** which has more involved handling for search space exceptions */
	(VOID)EXdelete();	    /* cancel exception handler prior
				    ** to exiting routine */
	return(0);
    }

    global = subquery->ops_global;
    vbase = subquery->ops_vars.opv_base;
    maxeqcls = subquery->ops_eclass.ope_ev;
    ebase = subquery->ops_eclass.ope_base;
    MEfill(sizeof(leftnode), (u_char)0, (PTR)&leftnode);
    leftnode.opn_next = (OPN_JTREE *)NULL;
    leftnode.opn_child[OPN_LEFT] = (OPN_JTREE *)NULL;
    leftnode.opn_child[OPN_RIGHT] = (OPN_JTREE *)NULL;
    leftnode.opn_nleaves = 1;
    leftnode.opn_nchild = 0;
    leftnode.opn_npart = 1;
    leftnode.opn_ojid = OPL_NOOUTER;

/*	the following initialization are not needed
	leftnode.opn_pi[0] = 0;
	leftnode.opn_pi[1] = 1;
	leftnode.opn_csz[0] = 0;
	leftnode.opn_csz[1] = 1;
	leftnode.opn_psz[0] = 0;
	leftnode.opn_psz[1] = 1;
	leftnode.opn_cpeqc[0] = 0;
	leftnode.opn_cpeqc[1] = 0;
	leftnode.opn_jmask=0;
*/
    leftnode.opn_sbstruct[0] = 0;	    /* descriptor of subtree
				    ** which we are currently generating
				    ** - the description is terminated
				    ** by a 0
				    */
    STRUCT_ASSIGN_MACRO(leftnode,rightnode); /* probably will be used for
				    ** keying estimates */
    MEfill(sizeof(joinnode), (u_char)0, (PTR)&joinnode);
/*  joinnode.opn_next = (OPN_JTREE *)NULL; */
    joinnode.opn_child[OPN_LEFT] = (OPN_JTREE *)&leftnode;
    joinnode.opn_child[OPN_RIGHT] = (OPN_JTREE *)&rightnode;
    joinnode.opn_nleaves = 2;
    joinnode.opn_nchild = 2;
    joinnode.opn_npart = 1;
/*  joinnode.opn_pi[0] = 0; */
    joinnode.opn_pi[1] = 1;
    joinnode.opn_csz[0] = 1;
    joinnode.opn_csz[1] = 1;
    joinnode.opn_psz[0] = 2;
    joinnode.opn_psz[1] = 1;
    joinnode.opn_cpeqc[0] = 2; 
    joinnode.opn_cpeqc[1] = 1; 
    joinnode.opn_sbstruct[0] = 1;
    joinnode.opn_sbstruct[1] = 1;
    joinnode.opn_ojid = OPL_NOOUTER;
#if 0
    joinnode.opn_sbstruct[2] = 0;   /* descriptor of subtree
				    ** which we are currently generating
				    ** - the description is terminated
				    ** by a 0 */
    joinnode.opn_jmask=0;	    /* inited by MEfill */
#endif
    {
	OPV_IVARS	    base1var;	    
	OPV_VARS	    *varp;
	varcount = subquery->ops_vars.opv_prv;
	for (base1var = varcount; --base1var >= 0;)
	    iarlp->stleaves[base1var] = base1var;   /* initialize the set of variables to look at */

	for (base1var = subquery->ops_vars.opv_rv; --base1var >= 0;)
	{   /* get the project-restrict costs for both index and the base relations */
	    opn_inode(subquery, &leftnode, base1var);
	    global->ops_estate.opn_corequired = global->ops_estate.opn_cocount + OPN_PRCOCOUNT;
					    /* make sure all these calculations are
					    ** made in enumeration memory so that
					    ** the copyco routine does not access violation
					    ** because no CO nodes are available
					    ** 3 is the number of CO nodes possibly
					    ** required for this routine */
	    (VOID) opn_nodecost(subquery, &leftnode, &leftnode.opn_eqm, &subtp, &eqclp, &rlmp,
			&sigstat);
					    /* note that all the known attributes
					    ** are typically requested from the index
					    ** even though a TID join would only
					    ** require the TID attribute, an optimization
					    ** would be to require only the TID, FIXME */
	    BTclear((i4)base1var, (char *)&leftnode.opn_rlmap);
	    varp = vbase->opv_rt[base1var];
	    if (varp->opv_pr)
	    {
		bestcost[base1var] = opn_cost(subquery, varp->opv_pr->opo_cost.opo_dio,    
			varp->opv_pr->opo_cost.opo_cpu);
		otuplecount[base1var] =
		tuplecount[base1var] = varp->opv_pr->opo_cost.opo_tups;
	    }
	    else
	    {	/* this is probably an SEJOIN which does not have a project restrict */
		bestcost[base1var] = FMAX;
		otuplecount[base1var] =
		tuplecount[base1var] = FMAX;
	    }
	    outervar[base1var] = OPV_NOVAR;
	}
    }
    {
	bool		    changes;
	OPV_IVARS	    base4var;	    
	OPV_VARS	    *var4p;
	OPV_IVARS	    exit_count;		/* floating point roundoff could make this
						** an endless loop on machines such as the
						** sequent, place a counter to end the
						** iteration after the maximum number of
						** theoretically useful passes */
	changes = TRUE;
	global->ops_estate.opn_corequired = global->ops_estate.opn_cocount + OPN_JCOCOUNT; /*
						** add the number of CO nodes needed
						** to process a join, if this number is
						** too low then opo_copyco can be called
						** prematurely */
	for (exit_count = subquery->ops_vars.opv_rv;
	    changes && (exit_count >= 0); exit_count--)
	{
	    changes = FALSE;
	    for (base4var = subquery->ops_vars.opv_rv; --base4var >= 0;)
	    {   /* obtain the best tuple estimate for keying into the relation, but
		** not using indexes, FIXME it may be reasonable to use indexes here
		** but a separate array is needed to ensure if an estimate is used
		** from an index then it should be in the final set of relations
		** this is not guaranteed here, so prv is used 
		** - b30812 works better if index substitution is done and keying
		** is made on this which would get avoided if opv_prv is used 
		** also if index is not used then the project restrict will be
		** more efficient */
		OPV_IVARS	    base2var;

		var4p = vbase->opv_rt[base4var];
		if (var4p->opv_tcop 
		    && 
		    var4p->opv_pr
		    &&
		    (otuplecount[base4var] == var4p->opv_tcop->opo_cost.opo_tups)
		    &&
		    (global->ops_cb->ops_alter.ops_amask & OPS_ACOMPLETE)
		    )
		    continue;			/* this is a case in which the "complete"
						** flag will cause the joining relation to
						** select all the tuples, so that it would
						** not be useful */
		for (base2var = subquery->ops_vars.opv_rv; --base2var >= 0;)
		{
		    OPV_VARS	*var2p;

		    var2p = vbase->opv_rt[base2var];
		    if (var4p->opv_joinable.opv_bool[base2var]
			&&
			var2p->opv_pr)
		    {
			OPE_BMEQCLS	    keying_map;
			OPV_IVARS	    vartest;
			bool		    valid;
			OPV_IVARS	    primary1;

			primary1 = var2p->opv_index.opv_poffset;
			valid = TRUE;
			/* check if base relation is in the keying sequence which
			** resulted in this lower cost, this needs to be avoided since
			** index placement requires that the base relation is always used 
			** with a TID join to the index */
			for (vartest = base4var; vartest != OPV_NOVAR; vartest=outervar[vartest])
			{
#if 0
/* allow more relaxed relation selection by allowing TID joins */
			    if ((base2var == vartest)
				||
				(primary2 == base2var)
				||
				(   (primary2 != OPE_NOEQCLS)
				    &&
				    (var2p->opv_index.opv_poffset == primary2)
				)
				||
				(var2p->opv_index.opv_poffset == vartest)
				)
#endif
			    if (primary1 == vartest)
			    {	/* do not allow a primary to access the secondary index */
				valid=FALSE;
				break;
			    }
			}
			if (!valid)
			    continue;		    /* keying is based on cyclic
						    ** relation attachment so do
						    ** not consider this */

			/* there is least one joining equivalence class so
			** reduce the tuple and cost factor if keying can be made */
			MEcopy((PTR)&var4p->opv_maps.opo_eqcmap, sizeof(keying_map), (PTR)&keying_map);
			BTand((i4)maxeqcls, (char *)&var2p->opv_maps.opo_eqcmap, (char *)&keying_map);
			if (opo_kordering(subquery, var2p, &keying_map) != OPE_NOEQCLS)
			{
			    /* keying can be used for this relation so adjust the tuple
			    ** counts */
			    OPO_COST	newcost;
			    newcost = opn_cost(subquery, tuplecount[base4var],
				tuplecount[base4var])	/* approximetely one DIO
							** and one CPU unit per tuple */
				+ bestcost[base4var];	/* normally this would not be
							** added since we are guaranteed
							** that the outer relation will
							** exist, but then cyclic estimates
							** occur, so that the PR cost is
							** avoided for 2 relations, but each
							** assumes the other is used for
							** keying, adding the PR cost will
							** avoid the cyclic problem but
							** there probably is a better way
							** to fix this, it occurs in the
							** rep3.qry in the test suites */
			    if (newcost < bestcost[base2var])
			    {
				/* bug 33551 - make sure that estimates are accurate by
				** using histograms */
				OPO_COST	adjusted_cost;
				if (!var2p->opv_jcosts)
				{   /* initial array of costs and tuples counts for
				    ** joins, this will allow partial results to be
				    ** deleted so that out of memory errors  do not arise
				    */
				    OPV_IVARS	    numvars;
				    i4		    costsize;

				    numvars = subquery->ops_vars.opv_rv;
				    costsize = numvars * sizeof(var2p->opv_jcosts->opo_costs[0]);
				    var2p->opv_jcosts = (OPO_ACOST *)opn_memory(global,
					(i4)(costsize + costsize));
				    var2p->opv_jtuples = (OPO_ACOST *)&var2p->opv_jcosts->opo_costs[numvars];
				    while(--numvars>=0)
				    {
					var2p->opv_jcosts->opo_costs[numvars] = 0.0;
					var2p->opv_jtuples->opo_costs[numvars] = 0.0;
				    }
				}
				if (!var4p->opv_jcosts)
				{   /* initial array of costs and tuples counts for
				    ** joins, this will allow partial results to be
				    ** deleted so that out of memory errors  do not arise
				    */
				    OPV_IVARS	    numvars;
				    i4		    costsize;

				    numvars = subquery->ops_vars.opv_rv;
				    costsize = numvars * sizeof(var2p->opv_jcosts->opo_costs[0]);
				    var4p->opv_jcosts = (OPO_ACOST *)opn_memory(global,
					(i4)(costsize + costsize));
				    var4p->opv_jtuples = (OPO_ACOST *)&var4p->opv_jcosts->opo_costs[numvars];
				    while(--numvars>=0)
				    {
					var4p->opv_jcosts->opo_costs[numvars] = 0.0;
					var4p->opv_jtuples->opo_costs[numvars] = 0.0;
				    }
				}
				if (var2p->opv_jcosts->opo_costs[base4var] == 0.0)
				{
				    OPO_CO	    *cop;
				    opn_inode(subquery, &leftnode, base4var); /* init the left
							    ** part of key join */
				    opn_inode(subquery, &rightnode, base2var); /* init the right
							    ** part of key join */
				    opn_ijnode(subquery, &joinnode, base4var, base2var); /*
							    ** initialize join node */
				    (VOID) opn_nodecost(subquery, &joinnode, &joinnode.opn_eqm, 
					    &subtp, &eqclp, &rlmp, &sigstat);
							    /* note that all the known attributes
							    ** are requested but typically this is
							    ** an overestimate FIXME */
				    BTclear((i4)base4var, (char *)&leftnode.opn_rlmap);
				    BTclear((i4)base2var, (char *)&rightnode.opn_rlmap);
				    BTclear((i4)base4var, (char *)&joinnode.opn_rlmap);
				    BTclear((i4)base2var, (char *)&joinnode.opn_rlmap);
				    /* search for cheapest keying strategy and use cost
				    ** estimates and tuple counts */
				    for (cop = subtp->opn_coforw; 
					 cop != (OPO_CO *) subtp; 
					 cop = cop->opo_coforw)
				    {
					OPO_COST    tempcost;
					OPV_IVARS   resultvar;
					OPV_VARS    *resultp;
					OPO_COST    resultcost;

					tempcost = opn_cost(subquery, 
					    cop->opo_cost.opo_dio,
					    cop->opo_cost.opo_cpu);

					/*
					** If cop->opo_outer->opo_outer is NULL, let's not
					** proceed with this analysis, as the "guess" is not
					** complete. The normal enumeration process will find
					** a good query plan.
					*/
					if (cop->opo_outer->opo_outer == NULL)
					{
						(VOID)EXdelete();
						return(0);
					}

					if (cop->opo_outer->opo_outer->opo_sjpr == DB_ORIG)
					{
					    if (cop->opo_outer->opo_outer->opo_union.opo_orig == base2var)
					    {
						resultvar = base2var;
						resultp = var4p;
					    }
					    else
					    {
						resultvar = base4var;
						resultp = var2p;
					    }
					}
					else
					    opx_error(E_OP04A5_SHAPE);
					if (resultvar < 0)
					    opx_error(E_OP04A5_SHAPE);
					resultcost = resultp->opv_jcosts->opo_costs[resultvar];
					if ((resultcost == 0.0)
					    ||
					    (tempcost < resultcost))
					{
					    resultp->opv_jcosts->opo_costs[resultvar] = tempcost;
					    resultp->opv_jtuples->opo_costs[resultvar] = cop->opo_cost.opo_tups;
					}
				    }
				    if (var4p->opv_jcosts->opo_costs[base2var] == 0.0)
					var4p->opv_jcosts->opo_costs[base2var] = OPO_INFINITE;
				    if (var2p->opv_jcosts->opo_costs[base4var] == 0.0)
					var2p->opv_jcosts->opo_costs[base4var] = OPO_INFINITE;

				    /* a number of assumptions are made when calling opn_fmemory */
				    subquery->ops_cost = 0.0; /* set to zero since opn_goodco needs
							    ** to be able to deallocate all CO nodes */
				    rlmp->opn_delflag = TRUE; /* allow the OPN_RLS structure to be deleted */
				    global->ops_estate.opn_swcurrent = 2; /* allows all structures 
							    ** associated with the OPN_RLS structure
							    ** to be deleted */
				    opn_fmemory(subquery, (PTR *)&global->ops_estate.opn_rlsfree);
							    /* since the join nodes have invalid
							    ** approximete result eqc maps , delete
							    ** the OPN_RLS node just created so
							    ** that erroneous results are not used
							    ** in enumeration, also reclaim memory
							    ** to be used in next join estimate, to
							    ** avoid a call to opo_copyco */
				    global->ops_estate.opn_swcurrent = 0;
				    subquery->ops_cost = OPO_LARGEST;		    
				}
				/* since the join estimation is made directly
				** between the two relation, but there may be
				** a better way to access the outer relation,
				** a first approximation would be that the
				** cost of a key join is proportional to the
				** number of tuples in the outer, so that if
				** fewer tuples exist in the outer due to
				** another access path, then the cost would be 
				** proportionally less by a direct ratio */
				adjusted_cost = var2p->opv_jcosts->opo_costs[base4var];
				if (adjusted_cost < (OPO_INFINITE/2.0))
				{
				    OPO_COST	ratio;
                                    if (outervar[base4var] != OPV_NOVAR)
                                    {
                                        ratio = (tuplecount[base4var] /
                                            otuplecount[base4var]);
                                        adjusted_cost = adjusted_cost * ratio
                                            + bestcost[base4var]; /* if base4var
                                                            ** is accessed via some keying
                                                            ** strategy then add the cost
                                                            ** of getting to base4var via
                                                            ** that strategy */
                                    }
                                    else
                                        ratio = 1.0;
                                    if (adjusted_cost < bestcost[base2var])
                                    {
                                        OPV_IVARS       cyclic;
                                        for (cyclic = base4var;
                                            (cyclic != OPV_NOVAR)
                                            &&
                                            (cyclic != base2var);
                                            cyclic=outervar[cyclic])
                                            ;           /* avoid cyclic definition
                                                        ** of best access path */
                                        if (cyclic == OPV_NOVAR)
                                        {
                                            changes = TRUE; /* this will loop thru the keying
                                                ** checks to try to get transitive
                                                ** closure on tuple counts */
                                            outervar[base2var] = base4var;
                                            bestcost[base2var] = adjusted_cost;
                                            tuplecount[base2var] = ratio *
                                                var2p->opv_jtuples->opo_costs[base4var];
                                        }
                                    }
				}
			    }
			}
		    }
		}
	    }
	}
    }

    for (index1var = subquery->ops_vars.opv_prv; index1var < subquery->ops_vars.opv_rv; index1var++)
    {
	OPV_VARS	    *index1p;
	index1p = vbase->opv_rt[index1var];
	if (index1p->opv_mask & OPV_CINDEX)
	{
	    /* this index can be used for table subsitution so place into the array of elements */
	    if (bestcost[index1var]
		    <
		bestcost[iarlp->stleaves[index1p->opv_index.opv_poffset]]
		)
	    {
		iarlp->stleaves[index1p->opv_index.opv_poffset] = index1var;    /* index 
					    ** subsititution is better
					    ** than the current base var or existing index */
		indexused = TRUE;	    /* indicate at least one index was used */
	    }
	}
	else
	{
	    OPV_IVARS	    index2var;	    /* used to traverse indexes which have already
					    ** been processed */
	    bool	    conflict;	    /* TRUE - if another index exists which has
					    ** attributes in common with the current index */
	    bool	    useful;	    /* TRUE - if index is possibly useful, i.e. better
					    ** than existing indexes on same primary */
	    conflict = FALSE;
	    useful = TRUE;
	    for (index2var = subquery->ops_vars.opv_prv; index2var < varcount; index2var++)
	    {
		OPV_VARS	    *index3p;
		OPV_IVARS	    index3;
		index3 = iarlp->stleaves[index2var];	    /* get the previous index being considered */
		index3p = vbase->opv_rt[index3]; 
		if (index3p->opv_index.opv_poffset == index1p->opv_index.opv_poffset)
		{   /* this index is on the same primary, so see if both these indexes
		    ** can be used productively in a TID join, for now make sure the only
		    ** intersection is the TID equivalence class or function attributes */
		    OPE_BMEQCLS		intersect; /* intersection of equivalence classes
						    ** between indexes on same primary */
		    OPE_IEQCLS		eqc;

		    MEcopy((PTR)&index1p->opv_maps.opo_eqcmap, sizeof(intersect), (PTR)&intersect);
		    BTand((i4)maxeqcls, (char *)&index3p->opv_maps.opo_eqcmap, (char *)&intersect);
		    for (eqc = -1; (eqc = BTnext((i4)eqc, (char *)&intersect, (i4)maxeqcls)) >= 0;)
		    {
			if (ebase->ope_eqclist[eqc]->ope_eqctype != OPE_TID)
			{
			    conflict = TRUE;
			    break;
			}
		    }
		}
		if (conflict)
		{
		    /* both these indexes should not be in the same plan (probably)
		    ** so determine which one is better */
		    if ((bestcost[index1var] + tuplecount[index1var])
							/* consider approximetely 1 DIO
							** for each tuple for the TID join and add 
							** the cost of the project restrict */
			    <
			(bestcost[index2var] + tuplecount[index2var])
							/* consider approximetely 1 DIO
							** for each tuple and add the cost of the
							** project restrict */
			)
		    {   /* remove the existing index since the new index is
			** better, FIXME combinations of indexes may be better than
			** this one but there does not seem to be an easy way to find this */
			OPV_IVARS	move;
			for (move = index3; move < varcount; move++)
			    iarlp->stleaves[move] = iarlp->stleaves[move+1];
			varcount--;
		    }
		    else
		    {
			useful = FALSE;
			break;			/* look at the next index since an existing one is
						** cheaper */
		    }
		}
	    }
	    if (useful)
	    {
		iarlp->stleaves[varcount++] = index1var;  /* add new index to set of relations to consider */
		indexused = TRUE;
	    }
	}
    }
    if (indexused)
    {
	/* check to see that index substitution is not used with an index lookup,
	** or if it is then which one is better */
	OPV_IVARS	    basevar;	    /* look at all primaries and determine
					    ** which are indexes */
	indexused = FALSE;
	for (basevar = subquery->ops_vars.opv_prv; --basevar >= 0;)
	{
/*	    if (iarlp->stleaves[basevar] >= subquery->ops_vars.opv_prv) need to check all cases 
*/
	    {	/* index subsititution case found so determine if this
		** is better than the current best set of indexes */
		OPV_IVARS	primary;	/* var number of primary being
						** analyzed */
		OPV_VARS	*subindex;	/* ptr to descriptor for the
						** subsituted index */
		bool		first_time;	/* TRUE - if this is the first
						** secondary associated with the
						** primary */
		OPV_IVARS	index5var;	/* used to traverse remainder of
						** secondaries to determine which
						** will be TID joined */
		OPV_VARS	*index5p;	/* ptr to descriptor for index being
						** analyzed */
		OPV_IVARS	subinvar;

		subinvar = iarlp->stleaves[basevar];	/* base var or subsituted index */
		subindex = vbase->opv_rt[subinvar];
		if (subinvar >= subquery->ops_vars.opv_prv)
		{
		    primary = subindex->opv_index.opv_poffset; /* index subsitution
						** occurred */
		    indexused = TRUE;
		}
		else
		    primary = basevar;		/* no index substitution */
		first_time = TRUE;
		for (index5var = basevar; ++index5var < varcount;)
		{
		    OPV_IVARS	index5;

		    index5 = iarlp->stleaves[index5var];
		    index5p = vbase->opv_rt[index5];
		    if (primary == index5p->opv_index.opv_poffset)
		    {	/* secondary index found */
			/* check if this is cheaper than the index substitution, or base
			** relation and if not then eliminate it */
			
			if (((bestcost[index5] + opn_cost(subquery, tuplecount[index5], tuplecount[index5]))
			    >
			    bestcost[primary])
			    &&
			    !(index5p->opv_mask & OPV_EXPLICIT_IDX)
			    )
			{   /* eliminate the index since it is more expensive than just using base relation */
			    OPV_IVARS	index8;
			    for (index8 = index5var; ++index8 < varcount;)
				iarlp->stleaves[index8-1] = iarlp->stleaves[index8];
			    varcount--;
			    index5var--;
			    continue;
			}
			else
			    first_time = FALSE;
		    }
		}
		if (!first_time)
		{   /* the best index selection was made, so determine if the base relation
		    ** or index subsitution is better */
		    /* replace with base variable since TID lookup is better */
		    indexused = TRUE;
		    iarlp->stleaves[basevar] = primary;
		}
		if (iarlp->stleaves[basevar] >= subquery->ops_vars.opv_prv)
		{   /* place index subsitution variable into proper sorted location */
		    OPV_IVARS	    base3var;
		    OPV_IVARS	    subvar;

		    subvar = iarlp->stleaves[basevar];
		    for (base3var = basevar; ++base3var < varcount;)
		    {
			if (iarlp->stleaves[base3var] < subvar)
			    iarlp->stleaves[base3var-1] = iarlp->stleaves[base3var];
			else
			    break;
		    }
		    iarlp->stleaves[base3var-1] = subvar;
		}
	    }
	}
    }

    (VOID)EXdelete();	    /* cancel exception handler prior
			    ** to exiting routine */

    if (indexused)
	return(varcount);
    else
	return(0);

}

/*{
** Name: opn_jtalloc	- allocate pool of OPN_JTREE nodes
**
** Description:
**      This routine will allocate a sequence of OPN_JTREE nodes from
**	the JTREE memory stream and chain them together.
**
** Inputs:
**      subquery			ptr to current subquery structure
**	jtreep				ptr to anchor of jtree node chain
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-sep-02 (inkdo01)
**          Written.
[@history_line@]...
*/
VOID
opn_jtalloc(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	**jtreep)
{
    OPN_JTREE	*np;
    i4		nodecount;

    /* Allocate the required number of OPN_JTREE structures - this
    ** should be equal to 2n-1 where n is the number of relations
    ** in the joinop range table; i.e. the maximum number of nodes
    ** in a binary tree with n leaves.  These nodes are allocated in
    ** a separate memory stream so that enumeration out-of-memory errors
    ** can be recovered from by reinitializing the enumeration memory
    ** stream inside "cost evaluation" i.e. we need to keep the same
    ** point inside enumeration search space.  Otherwise, the OPN_JTREE
    ** nodes would be lost after the enumeration memory stream is
    ** reinitialized.
    */

    nodecount = 2 * subquery->ops_vars.opv_rv; /* nodes required */
    while (nodecount-- > 0)
    {   /* create the required number of join tree nodes */
	np = (OPN_JTREE *)opn_jmemory(subquery->ops_global, sizeof(OPN_JTREE));
	np->opn_next = *jtreep;		/* link into list */
	*jtreep = np;			/* (link is at start of node) */
    }
}

/*{
** Name: opn_tree	- create structurally unique trees
**
** Description:
**      This routine will create structurally unique trees prior to processing
**      them.  All structurally unique trees with leaf counts between n and m
**      are created; where n is the number of primary relations and m is the
**      total number of relations in the joinop range table.  Each tree is
**      then passed to the cost ordering phase for enumeration over the
**      appropriate ordering of range variables.
**
** Inputs:
**	subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-may-86 (seputis)
**          initial creation from stratdo in stratdo.c
**	19-oct-88 (seputis)
**          allocate more nodes for distributed, and fewer
**          nodes for local optimization
**	16-may-90 (seputis)
**	    - b21582 avoid shutting out server login
**	    - fix lint errors 
**	20-dec-90 (seputis)
**	    - avoid infinite loop due to floating point roundoff errors
**	1-apr-92 (seputis)
**          - bug 43617 fix problem in which balanced tree shapes were skipped, this
**	    may have affected performance in some non-keying, FSM join cases
**	    for complex (>=4) tables
**	14-jul-1993 (rog)
**	    Compare return from EXdeclare() against EXDECLARE, not EX_DECLARE.
**	18-nov-99 (inkdo01)
**	    Changes to support return'ed searchspace indicator, instead of more
**	    expensive EXsignal.
**	17-sep-02 (inkdo01)
**	    Externalize allocation of OPN_JTREE pool.
**	21-mar-06 (dougi)
**	    Add code to enforce table level hints.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
OPN_STATUS
opn_tree(
	OPS_SUBQUERY        *subquery)
{
    OPN_SUNIQUE         sut;	    /* structurally unique tree
                                    ** state variable - used to keep
                                    ** track of current tree structure being
                                    ** analyzed
                                    */
    OPV_IVARS		maxvar;     /* number of element in joinop range table*/
    OPV_IVARS           primary;    /* number of primary relations in table */
    OPS_STATE           *global;    /* ptr to global state variable */
    OPN_SSTLEAVES	arlguess;   /* contains a set of relations which
				    ** is an hueristic guess as to the best
				    ** collection of indexes */

    global = subquery->ops_global;  /* save ptr to global state variable */
    sut.opn_subquery = subquery;    /* save subquery ptr */
    maxvar = subquery->ops_vars.opv_rv; /* number of
                                    ** relations in joinop range table
                                    */
    primary = subquery->ops_vars.opv_prv; /* number of
                                    ** primary relations in joinop range table
                                    */
    opn_jtalloc(subquery, &global->ops_estate.opn_sroot);
				    /* allocate OPN_JTREE node pool */
    global->ops_gmask &= (~(OPS_BALANCED | OPS_GUESS));  /* this will cause all the 
					** unbalanced trees to be analyzed first, 
					** and then all the balanced trees */
    if ((subquery->ops_vars.opv_rv > 4)	/* make guess only for complex queries */
	&&
	!(subquery->ops_mask2 & OPS_HINTPASS)  /* no guess in hint pass */
	&&
	(subquery->ops_vars.opv_rv > subquery->ops_vars.opv_prv) /* check if
					** any indexes exist */
	&&
	(   !global->ops_cb->ops_check 
	    || 
	    !opt_strace( global->ops_cb, OPT_F066_GUESS)
	)
	&&
	(global->ops_estate.ops_arlcount = opn_iarl(subquery, &arlguess))
	)
	global->ops_estate.ops_arlguess = &arlguess;
    else
    {
	global->ops_estate.ops_arlcount = 0;
	global->ops_gmask |= OPS_GUESS;	    /* no special set of relations was found
					    ** continue with normal enumeration */
    }

    do
    {
	OPN_STATUS	sigstat;

	for( *sut.opn_tdsc = (subquery->ops_mask & OPS_SPATF || 
		(subquery->ops_mask2 & OPS_HINTPASS && 
		subquery->ops_mask2 & OPS_IXHINTS)) ?
		primary+1 : primary; *sut.opn_tdsc <= maxvar; (*sut.opn_tdsc)++)
	{   /* all trees with opn_tdsc leaves will be enumerate on this pass of
	    ** the for loop. NOTE: if there's an Rtree scan, or if there is an
	    ** index hint, start off with at least one secondary
	    */
	    for (;;)
	    {
		OPN_LEAVES	    treeleaves;

		global->ops_gmask |= OPS_SKBALANCED;    /* flag will cause the first
					    ** structurally unique tree to be skipped
					    ** if OPS_BALANCED is set */
		global->ops_gmask &= (~OPS_PLANFOUND);	/* flag is used to make sure a
					    ** valid tree orientation is chosen prior to
					    ** go onto the next number of leaves */
		if (!(global->ops_gmask & OPS_GUESS))
		    *sut.opn_tdsc = global->ops_estate.ops_arlcount;   /* modify number of leaves
					    ** to match what is required by the
					    ** hueristic guess */
		treeleaves = *sut.opn_tdsc; /* number of leaves in the trees being
					    ** considered on this pass */
		global->ops_estate.opn_swlevel = treeleaves-1; /* on this pass the
					    ** subtrees with this number of leaves
					    ** will be saved */
		global->ops_estate.opn_corequired = 3 * treeleaves
#ifdef OPM_FLATAGGON
		    + subquery->ops_flat.opm_fav
#endif
					    ; /* possible
					    ** maximum number of CO nodes for one tree
					    ** - n for each original relation
					    ** - n for each possible project restrict
					    ** - n-1 for each interior node
					    ** - 1 for a top sort node
					    ** - m possible sort node for each aggregate
					    ** - used in out-of-memory error recovery */
		global->ops_estate.opn_corequired = 3 * treeleaves; /* possible
					    ** maximum number of CO nodes for one tree
					    ** - n for each original relation
					    ** - n for each possible project restrict
					    ** - n-1 for each interior node
					    ** - 1 for a top sort node
					    ** - used in out-of-memory error recovery */
		if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		{   /* add more CO nodes for distributed arrays of QEPs */
		    switch (subquery->ops_sqtype)
		    {
		    case OPS_MAIN:
		    case OPS_SAGG:
			break;
		    case OPS_HFAGG:
		    case OPS_RFAGG:
		    case OPS_FAGG:
		    case OPS_PROJECTION:
		    case OPS_SELECT:
		    case OPS_RSAGG:
		    case OPS_UNION:
		    case OPS_VIEW:
			global->ops_estate.opn_corequired *= global->ops_gdist.opd_dv;
			break;
#ifdef E_OP048D_QUERYTYPE
		    default:
			opx_error(E_OP048D_QUERYTYPE);
#endif
		    }
		}
		sut.opn_copymode = FALSE;   /* copy mode set TRUE only when two
					    ** subtrees with the same number of leaves
					    ** and the same parent is found
					    ** - used to avoid structurally identical
					    ** trees from being evaluated 
					    */
		sut.opn_sp = sut.opn_stack; /* initialize stack to point to the first
					    ** element of opn_tdsc
					    */
		*sut.opn_sp++ = 0;          /* 0 represents opn_tdsc[0] */
		if (treeleaves == 1)
		{
		    opn_mjtree(&sut);       /* make join tree of one node */
		    sigstat = opn_process(subquery, 1);
					    /* opn_gen cannot handle a single node 
					    ** - second parameter is node count
					    */
		}
		else
		    sigstat = opn_gen ( &sut, /* pass state variable for structurally
					    ** unique trees
					    */
			1,		    /* 1 means place tree description in 
					    ** sut.opn_tdsc[1] for the element of
					    ** opn_tdsc pointed to by the top of the 
					    ** stack; (i.e. generate tree description
					    ** for "opn_tdsc[*(opn_sp-1)]")
					    */
			0,                  /* 0 means use the opn_sp as described
					    ** above
					    */
			0, 0);	            /* ignored during initial call to opn_gen */
					    /* opn_gen will generate 
					    ** descriptions in sut.opn_tdsc 
					    ** of structurally unique trees with 
					    ** *sut.opn_tdsc leaves; for each such 
					    ** tree, opn_gen calls process 
					    */

		/* Check for SEARCHSPACE condition, indicating we're done the 
		** left deep trees. */
		if (sigstat == OPN_SIGSEARCHSPACE)
		{
		    if (!(global->ops_gmask & OPS_GUESS))
		    {
			global->ops_gmask |= OPS_GUESS;
			*sut.opn_tdsc = primary - 1;
		    }
		    break;
		}
		/* Check for timeout (or other opn_exit). */
		if (sigstat == OPN_SIGEXIT) return(sigstat);

		if (global->ops_gmask & OPS_GUESS)
		{
		    global->ops_estate.opn_swcurrent = global->ops_estate.opn_swlevel; /* 
					    ** subtrees with this number of leaves will
					    ** have been considered without any 
					    ** deletions of useful subtrees, 
					    ** so the absence of
					    ** a subtree structure will indicate that 
					    ** the cost is too large */
		    break;		    /* normal enumeration so break out of infinite
					    ** loop */
		}
		else
		{			    /* this first time thru enumeration a guess
					    ** as to the best set of indexes was made
					    ** given rough estimates; now control is
					    ** returned to normal enumeration */
		    *sut.opn_tdsc = primary; /* reset leaf count to normal starting point */
		    if (global->ops_cb->ops_check 
			&& 
			opt_strace( global->ops_cb, OPT_F053_FASTOPT)
			&&
			subquery->ops_bestco
			&&
			(!(global->ops_gmask & OPS_GUESS))
			)
		    {
			return(OPN_SIGOK);    /* exit if a short optimization is requested */
		    }
		    global->ops_gmask |= OPS_GUESS; /* turn off special enumeration code 
					    ** to exit on special set of relations */
		}
	    }
	}
	global->ops_gmask ^= OPS_BALANCED;   /* this will cause all the unbalanced trees
					** to be analyzed first, and then all the
					** balanced trees */
    } while ((global->ops_gmask & OPS_BALANCED) && (maxvar > 3));
    return(OPN_SIGOK);
}
