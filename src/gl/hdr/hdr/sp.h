/**
**
**  Name: sp.h -- Global header for SP functions.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  The following type declarations provide the binary tree
**  representation of event-sets or priority queues needed by splay trees
**
**  Assumes that key will be provided by the application, compatible
**  with the compare function applied to the addresses of two keys.
*/

# ifndef SPTREE_H
# define SPTREE_H

/* original comments:
**
**  The code implements the following operations on an event-set
**  or priority-queue implemented using splay trees:
**  
**  SPTREE *SPinit( compare )	Make a new tree
**  int SPempty();		Is tree empty?
**  SPBLK *SPenq( n, t )	Insert n in t after all equal keys.
**  SPBLK *SPdeq( np )		Return first key under *np, removing it.
**  SPBLK *SPenqprior( n, t )	Insert n in t before all equal keys.
**  VOID SPsplay( n, t )	n (already in q) becomes the root.
**  n = SPhead( t )		n is the head item in t (not removed).
**  SPdelete( n, t )		n is removed from q.
**  n = SPnext( np, t )		n is the successor of np in q.
**  n = SPprev( np, t )		n is the predecessor of np in q.
**  SPenqbefore( n, np, t )	n becomes the predecessor of np in q.
**  SPenqafter( n, np, t )	n becomes the successor of np in q.
**
**  In the above, n points to an SPBLK type, while t points to an
**  SPTREE.
**
**  Many of the routines in this package use the splay procedure,
**  for bottom-up splaying of the queue.  Consequently, item n in
**  delete and item np in all operations listed above must be in the
**  event-set prior to the call or the results will be
**  unpredictable (eg:  chaos will ensue).
**  
**  Note that, in all cases, these operations can be replaced with
**  the corresponding operations formulated for a conventional
**  lexicographically ordered tree.  The versions here all use the
**  splay operation to ensure the amortized bounds; this usually
**  leads to a very compact formulation of the operations
**  themselves, but it may slow the average performance.
**  
**  Alternative versions based on simple binary tree operations are
**  provided (commented out) for head, next, and prev, since these
**  are frequently used to traverse the entire data structure, and
**  the cost of traversal is independent of the shape of the
**  structure, so the extra time taken by splay in this context is
**  wasted.
**  
**  The implementation used here is based on the implementation
**  which was used in the tests of splay trees reported in:
**
**    An Empirical Comparison of Priority-Queue and Event-Set Implementations,
**	by Douglas W. Jones, Comm. ACM 29, 4 (Apr. 1986) 300-311.
**
**  The changes made include the addition of the enqprior
**  operation and the addition of up-links to allow for the splay
**  operation.  The basic splay tree algorithms were originally
**  presented in:
**
**	Self Adjusting Binary Trees,
**		by D. D. Sleator and R. E. Tarjan,
**			Proc. ACM SIGACT Symposium on Theory
**			of Computing (Boston, Apr 1983) 235-245.
**
**  The enq and enqprior routines use variations on the
**  top-down splay operation, while the splay routine is bottom-up.
**  All are coded for speed.
**
**  Written by:
**    Douglas W. Jones
**
**  Translated to C by:
**    David Brower, daveb@rtech.uucp,
**    Published as comp.sources.unix v14i087, May 1988.
**
**  Thu Oct  6 12:11:33 PDT 1988 (daveb) Fixed SPdeq, which was broken
** 	handling one-node trees.  I botched the pascal translation of
** 	a VAR parameter.  Changed interface, so callers must also be
**	corrected to pass the node by address rather than value.
*/

/*}
** Name:	SPBLK -- basic SP tree node
**
** Description:
**	This block must appear somewhere in all SP nodes.
**
** History:
**	27-Jan-92 (daveb)
**	   Updated to INGRES standard.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
*/
typedef struct spblk SPBLK;

struct spblk
{
    /* clients should not look at these */
    SPBLK	*leftlink;
    SPBLK	*rightlink;
    SPBLK	*uplink;

    /* visible to clients */
    PTR		key;		
};

/*}
** Name:	SP_COMPARE_FUNC -- comparison function for SP tree
**
** Description:
**	This is the type of a function used by SP for tree ordering.
**
** History:
**	27-Jan-92 (daveb)
**	   Updated to INGRES standard.
**	9-jul-92 (daveb)
**	    Add name field to tree.
**	11-aug-93 (ed)
**	    changed CL_HAS_PROTOS to CL_PROTOTYPED
*/
typedef i4  SP_COMPARE_FUNC( 
# ifdef CL_PROTOTYPED
			const char *key1, const char *key2 
# endif
);
			    

/*}
** Name:	SPTREE -- tree root/description for SP
**
** Description:
**	This defines the tree, and is set up by SPinit.
**
** History:
**	27-Jan-92 (daveb)
**	   Updated to INGRES standard.
**	9-jul-92 (daveb)
**	    Add name field to tree.
*/

typedef struct
{
    SPBLK		*root;		/* root node */
    SP_COMPARE_FUNC	*compare;	/* key comparison function */

    /* Statistics, not strictly necessary, but handy for tuning  */

    i4	lookups;	/* number of SPlookup()s */
    i4	lkpcmps;	/* number of lookup comparisons */
    i4	enqs;		/* number of SPenq()s */
    i4	enqcmps;	/* compares in SPenq */
    i4	splays;		/* number of splays */
    i4	splayloops;	/* number of loops inside splays */
    i4	prevnexts;	/* number of SPprev/SPnext calls */
    char	*name;		/* administative name */
} SPTREE;

# ifdef CL_HAS_PROTOS

FUNC_EXTERN SPTREE *SPinit( SPTREE *t, SP_COMPARE_FUNC *compare );
FUNC_EXTERN SPBLK *SPhead(SPTREE *t);
FUNC_EXTERN VOID SPdelete(SPBLK *n, SPTREE *t );
FUNC_EXTERN SPBLK *SPhead(SPTREE *t);
FUNC_EXTERN SPBLK *SPtail(SPTREE *t);
FUNC_EXTERN SPBLK *SPnext(SPBLK *n, SPTREE *t);
FUNC_EXTERN SPBLK *SPprev(SPBLK *n, SPTREE *t);
FUNC_EXTERN SPBLK *SPlookup(SPBLK *keyblk, SPTREE *t );
FUNC_EXTERN SPBLK *SPinstall(SPBLK *keyblk, SPTREE *t);
FUNC_EXTERN SPBLK *SPfhead(SPTREE *t);
FUNC_EXTERN SPBLK *SPftail(SPTREE *t);
FUNC_EXTERN SPBLK *SPfnext(SPBLK *n);
FUNC_EXTERN SPBLK *SPfprev(SPBLK *n);
FUNC_EXTERN SPBLK *SPdeq(SPBLK **np);	
FUNC_EXTERN SPBLK *SPenq(SPBLK *n, SPTREE *q);

FUNC_EXTERN VOID SPsplay(SPBLK *n, SPTREE *q);

# else

FUNC_EXTERN SPTREE *SPinit();	/* init tree */
FUNC_EXTERN SPBLK *SPhead();	/* return first node in tree */
FUNC_EXTERN VOID SPdelete();	/* delete node from tree */
FUNC_EXTERN SPBLK *SPhead();	/* fast non-splaying head */
FUNC_EXTERN SPBLK *SPtail();	/* fast non-splaying tail */
FUNC_EXTERN SPBLK *SPnext();	/* return next node in tree */
FUNC_EXTERN SPBLK *SPprev();	/* return previous node in tree */
FUNC_EXTERN SPBLK *SPlookup();	/* find key in a tree */
FUNC_EXTERN SPBLK *SPinstall();	/* enter an item, allocating or replacing */
FUNC_EXTERN SPBLK *SPfhead();	/* fast non-splaying head */
FUNC_EXTERN SPBLK *SPftail();	/* fast non-splaying tail */
FUNC_EXTERN SPBLK *SPfnext();	/* fast non-splaying next */
FUNC_EXTERN SPBLK *SPfprev();	/* fast non-splaying prev */
FUNC_EXTERN SPBLK *SPdeq();	/* dequeue node */
FUNC_EXTERN SPBLK *SPenq();	/* enqueue node */

# endif

# endif				/* SPTREE_H */

