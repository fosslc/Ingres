/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#define             OPNPARTITION       TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNPART.C - routine to generate partitions of numbers
**
**  Description:
**      Routine to generate partitions of numbers
**
**  History:    
**      8-jun-86 (seputis)    
**          initial creation from partittion.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opn_partition(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES partbase,
	OPN_LEAVES numleaves,
	OPN_PARTSZ partsz,
	OPN_CHILD numpartitions,
	bool parttype);

/*{
** Name: opn_partition	- generate next partition of numbers
**
** Description:
**	Generate all partitions of "numleaves" numbers where the number and 
**      sizes of the partitions are specified by "numpartitions"
**      and partsz[0]...partsz[numpartitions-1] respectively.
**
** 	The order of numbers within a partition is insignificant (and
**	so is always in increasing order).  However, this routine depends on
**      the fact that all variable numbers in a partition are sorted.  Each
**      partition will remain sorted on successive calls to this routine.
**
**	In the following, a "leaf" is an element of array partbase.  The leaves
**      are initially in increasing order in "partbase" (with max value of 
**      OPV_MAXVAR).  Calls to partition generate new orderings of leaves 
**      (i.e. the same numbers are in partition but in a different order),
**      calling partition until it returns FALSE will generate all unique 
**	partitionings of the "numleaves" elements into the "numpartition"
**	groups of size specified by partsz.
**
** Inputs:
**      partbase                        ptr to base of array of numbers
**					which are the partitioned numbers. 
**                                      - where partbase[partsz[i]]...
**				        partbase[partsz[i+1]-1] holds
**					the numbers in partition i 
**                                      where (0<=i<=numpartitions-1).
**      numleaves                       number of elements in the partbase
**                                      structure
**      partsz                          array of partition sizes, where the
**                                      i'th element is the size of the i'th
**                                      partition
**      numpartitions                   number of partitions (or number of
**                                      elements in partsz array)
**      parttype                        TRUE - then the order of partitions is
**					insignificant and the partitions must be
**					of equal size.
**
** Outputs:
**	Returns:
**	    TRUE - if another partition of numbers is generated
**          FALSE - if no more partitions exist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-jun-86 (seputis)
**          initial creation from partition
[@history_line@]...
*/
bool
opn_partition(
	OPS_SUBQUERY       *subquery,
	OPN_STLEAVES       partbase,
	OPN_LEAVES         numleaves,
	OPN_PARTSZ         partsz,
	OPN_CHILD          numpartitions,
	bool               parttype)
{
    OPV_MVARS           free;		/* map of "free" range variables
                                        ** free[varno]=TRUE if leaf is free 
					** - when a leaf becomes "free", it
                                        ** will be moved from its current
                                        ** position in the partition */
    OPN_LEAVES          cnts[OPN_MAXLEAF]; /* cnts[varno] is the number of
                                        ** "free" elements with higher varno's
                                        ** (joinop range variable numbers)
                                        ** than this one */
    OPV_IVARS           maxvar;         /* number of range variables defined
                                        ** in subquery */
    OPN_LEAVES		*partszp;	/* pointer into partsz array, points at
					** size (i.e. number of leaves) 
                                        ** of current partition being
                                        ** analyzed */
    OPV_IVARS           *begpartp;      /* ptr to joinop range variable element
                                        ** in partbase which is at the being
                                        ** of the current partition being
                                        ** analyzed */
    OPV_IVARS           *ivarp;		/* ptr to current range variable
                                        ** in partbase, whose respective
                                        ** elements in "free" and "cnts"
                                        ** will be initialzed */

    if (numpartitions <= 1)
	return (FALSE);			    /* if there is only one group then 
					    ** there are no more partitionings
					    */
    maxvar = subquery->ops_vars.opv_rv;  /* number of range variables defined */
    {
	OPV_BTYPE              *initfreep;  /* ptr to current element of "free"
                                            ** to be initialized */
	OPN_LEAVES             leafcount;   /* number of leaves to initialize
                                            */
	
	initfreep = &free.opv_bool[0];
	for (leafcount = maxvar; leafcount--; *initfreep++ = FALSE); /* 
					    ** set all leaves to "not free" */
    }
    partszp = partsz + numpartitions;	    /* ptr to number of leaves in the
                                            ** last partition */
    begpartp = partbase + numleaves - *(--partszp); /* set begpartp to point 
					    ** to beginning of last partition
					    ** in partbase */
    {	
	/* initialize the "free" and "cnts" arrays */
	OPN_LEAVES             higher;	    /* number of leaves which have 
                                            ** 1) higher joinop range variable 
                                            ** numbers than the current one
                                            ** being analyzed, and 
                                            ** 2) is in a higher relative
                                            ** position in the partbase array
                                            */
	OPN_LEAVES             *cntp;       /* ptr to cnts array element
                                            ** currently being analyzed */
	higher = 0;
	cntp = cnts + maxvar - 1;	    /* set cntp to point to last
					    ** valid element in cnts */
	for (ivarp = partbase + numleaves; --ivarp >= begpartp; higher++)
	{   /* for all elements in last partition */
	    OPN_LEAVES         *curcntp;    /* ptr to element of count field
                                            ** corresponding to current element
                                            ** of the last partition being
                                            ** analzyed */
	    free.opv_bool[*ivarp] = TRUE;   /* set leaf specified by this
					    ** element in partbase to "free" */
	    curcntp = cnts + *ivarp;        /* update counts of all variables
                                            ** higher than this one */
	    while (cntp >= curcntp)	    /* for higher number leaves
					    ** than "*ivarp" */
		*cntp-- = higher;	    /* set cnts to number of free
					    ** higher numbered leaves so
					    ** far */
	}
	while (cntp >= cnts)
	    *cntp-- = higher;		    /* set the rest of cnts */
    }
    {
	OPN_LEAVES		mincnt;	    /* number of available higher level
					    ** leaves necessary for a leave to
					    ** occupy current position in 
					    ** "partbase" array
                                            ** - it is the number of elements
                                            ** which have already been analzyed
                                            ** in the current partition */
	OPV_BTYPE		*maxfreep;  /* ptr to largest element of 
                                            ** "free" array which is defined */

	maxfreep = &free.opv_bool[maxvar];
	mincnt = 0;                         /* no elements analyzed yet in
                                            ** current partition */
	begpartp -= *(--partszp);	    /* begpartp points to beginning of
                                            ** next to last partition */
	for (ivarp++; --ivarp >= partbase; mincnt++)
	{
	    /* For each position in partbase see if there is any leaf with a 
	    ** higher number than current occupant to fill this position */
	    OPV_IVARS          varno;       /* joinop range variable number
                                            ** (or current leaf being analzyed
                                            */
	    
	    varno = *ivarp;		    /* varno is joinop range variable
                                            ** number of leaf at this
					    ** position in partbase */
	    if (ivarp < begpartp)	    /* if we have just crossed into
					    ** a new partition */
	    {
		begpartp -= *(--partszp); /* make begpartp point to 
					    ** beginning of next lower 
					    ** partition */
		mincnt = 0;		    /* reset the restriction on how
					    ** large cnts must be for a leaf
					    ** to occupy a position 
                                            ** - number of leaves analyzed in
                                            ** this partition reset to 0
                                            */
	    }
	    {
		OPN_LEAVES            *cntp; /* ptr to count field currently
					    ** be updated */
		for (cntp = cnts + varno; --cntp >= cnts; (*cntp)++);
					    /* increment the counts for
					    ** leaves with lower numbers
					    ** than this leaf */
	    }
	    {
		OPV_BTYPE             *freep; /* ptr to element of "free" of
					    ** current varno being analyzed */
		OPN_LEAVES	      *cntp;  /* ptr to element of "cnts" of
                                            ** current varno being analyzed */

		*(freep = &free.opv_bool[varno]) = TRUE; /* set this leaf 
                                            ** to "free" */
		if (parttype && ivarp == begpartp)
		    continue;		    /* if "parttype" is true then we are
					    ** making partitions (all of
					    ** equal size) where the
					    ** ordering is ignored, so if
					    ** at beginning of partition,
					    ** we cannot choose another
					    ** value for this position */
		cntp = cnts + varno;
		while (++freep < maxfreep)  /* for each leaf with a higher
					    ** number - update freep ptr
                                            ** to next varno being analzyed */
		{
		    cntp++;		    /* update to next varno being
                                            ** analyzed */
		    varno++;                /* next varno to analyze */
		    if (*freep && *cntp >= mincnt) /* if this leaf is "free" 
					    ** and its count is high enough 
					    */
		    {
			*ivarp++ = varno;   /* set partbase to this leaf number 
					    */
			*freep = FALSE;	    /* set this leaf to "not free" 
	                                    ** since it has been moved */
			for (begpartp += *partszp++; ivarp < begpartp;) /* fill
                                            ** up the current partition with 
                                            ** "free" leaves with higher 
                                            ** numbers */
			{
			    while (!*(++freep))	/* if not free skip */
				varno++;    /* keep varno in step with freep */
			    *ivarp++ = ++varno;	/* set partbase to this leaf 
					    ** range variable number - also keep
                                            ** varno in step with freep */
			    *freep = FALSE; /* set leaf to not free */
			}
			varno = 0;	    
			for (freep = &free.opv_bool[0]; freep < maxfreep; 
				varno++)    /* fill 
					    ** up the rest of the partitions 
					    ** with free leaves, lowest 
					    ** numbered first */
			{
			    if (*freep++)   /* if free */
				*ivarp++ = varno;
			}
			return (TRUE);
		    }
		}
	    }
	}
    }
    return (FALSE);			    /* no more ways to partition */
}
