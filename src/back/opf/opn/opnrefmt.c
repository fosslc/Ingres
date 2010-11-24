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
#define	       OPX_EXROUTINES    TRUE
#define        OPN_REFORMAT      TRUE
#include    <mh.h>
#include    <ex.h>
#include    <sr.h>
#include    <opxlint.h>


/**
**
**  Name: OPNREFMT.C - calculate cost to reformat a node
**
**  Description:
**      Routine to calcuate cost to reformat a node
**
**  History:    
**      29-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      8-sep-92 (ed)
**          remove star OPF_DIST code
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_reformat(
	OPS_SUBQUERY *subquery,
	OPO_TUPLES tuples,
	OPS_WIDTH width,
	OPO_BLOCKS *diop,
	OPO_CPU *cpup);

/*{
** Name: opn_reformat	- cost to reformat a node
**
** Description:
**      Calculate cost to reformat a relation
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      type                            type of cost ordering requested
**                                      (sort, hash, isam)
**      blocks                          number of blocks in relation to reformat
**      tuples                          number of tuples in relation to reformat
**      width                           width in bytes of one tuple
**
** Outputs:
**      storage                         DMF storage class of reformatted 
**                                      relation
**      diop                            estimated disk i/o cost for reformat
**      cpup                            estimated cpu cost for reformat
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-may-86 (seputis)
**	    initial creation
**	6-dec-88 (seputis)
**	    compenstate for bad CPU estimates from DMF
**	21-dec-88 (seputis)
**          make sure DMF f8 does not overflow
**	11-apr-89 (seputis)
**          check for tuple width case of 1 or 0, and very large row count
**	19-dec-89 (seputis)
**	    - disable initializing opn_sblock to session startup
**      15-may-90 (jkb)
**          Add EXmath to make sure all Floating point exceptions are
**          returned before the dmfcall (this forces the 387 co-processor
**          on Sequent to report any unreported FPE's)
**	14-jan-91 (seputis)
**	    Use EX_ON since floating point exceptions are handled by an
**	    exception handler.
**	28-jan-91 (seputis)
**	    changed to use actsess instead of mxsess for OPF ACTIVE flag
**	    support
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**      06-mar-96 (nanpr01)
**	    Use the parameters consistently as OPS_WIDTH instead of nat.
**	18-aug-04 (inkdo01)
**	    Fix odd recalc. of tuples that made it even bigger.
**	10-may-05 (inkdo01)
**	    Add tp op248 to allow scaling down the sort CPU estimate.
**	27-july-06 (dougi)
**	    Dropped support of op248 in favour of new DMF CBF parms. Also
**	    changed parms to dmse_cost() to float to increase capacity and
**	    eliminate overflow problems.
*/
VOID
opn_reformat(
	OPS_SUBQUERY       *subquery,
	OPO_TUPLES         tuples,
	OPS_WIDTH          width,
	OPO_BLOCKS         *diop,
	OPO_CPU            *cpup)
{
    OPO_BLOCKS          dio;		/* estimate of disk i/o used to
                                        ** reformat the relation */
    OPO_CPU             cpu;            /* estimate of cpu for reformat */
    i4			i;

#if 0
OPN_IOMTYPE        type;
OPO_BLOCKS         blocks;
OPO_STORAGE        *storage;
    switch (type)			/* set the storage structure */
    {
    case OPN_IOSORT:
    {
	*storage = DB_SORT_STORE;
	break;
    }
    case OPN_IOISAM:
    {
	*storage = DB_ISAM_STORE;
	break;
    }
    {
    case OPN_IOBTREE: /* FIXME - BTREE is not reformatted yet */
	*storage = DB_BTRE_STORE;
	break;
    }
    case OPN_IOHASH:
    {
	*storage = DB_HASH_STORE;
	break;
    }
	/* FIXME - put in default error check case here */
    }
#endif

    dio = 0.0;				/* init to 0 disk blocks read/written*/
    cpu = 0.0;                          /* init to 0 cpu time used */
    if (tuples <= OPO_MINTUPLES)
	tuples = 1.0;			/* need at least one tuple to sort */

    {
	OPS_WIDTH	   reformatwid; /* width in bytes of a tuple of the
                                        ** reformatted relation */
	if (width < 1)
	    reformatwid = 1;
	else
	    reformatwid = width;
#if 0
	if (type == OPN_IOHASH)
	    reformatwid += DB_HASHWIDTH;/* hash value is appended to record */
	tpb = opn_tpblk(width);		/* tuples per block */
	resultblks = (f8)MHceil((f8)(tuples / tpb));	/* number of blocks 
					** in result */
#endif
        {
	    /* calculate sort cost in terms of dio and cpu given
	    ** the number of tuples and the width in bytes of a tuple */
            /* FIXME - should be in RDF */
	    DMT_CB	dmt_cb;		/* DMF control block used to get cost*/
            DB_STATUS   dmf_status;     /* DMF return status */

            dmt_cb.type = DMT_TABLE_CB;
            dmt_cb.length = sizeof(DMT_CB);
            dmt_cb.dmt_flags_mask = 0;
            dmt_cb.dmt_mustlock = FALSE;
            dmt_cb.dmt_s_estimated_records = tuples; /* estimated number of
					** tuples to be sorted */
            dmt_cb.dmt_s_width = reformatwid; /* width of one tuple */
	    EXmath(EX_ON);
            dmf_status = dmf_call( DMT_SORT_COST, &dmt_cb);
            if (dmf_status != E_DB_OK)
		opx_verror( dmf_status, E_OP048C_SORTCOST, 
		    dmt_cb.error.err_code);
            dio = dmt_cb.dmt_s_io_cost;
            cpu = dmt_cb.dmt_s_cpu_cost;

	    if (dio < 1.0)
		dio = 1.0;
	    if (cpu < dio)
		cpu = dio;		/* DMF cpu bug, can be 0.0 if
					** number of tuples is small, also
					** cpu is smaller than DIO for small
					** sorts which is probably incorrect
					** even though DIO includes temp file
					** creation cost
					*/
	    for (i = 0; i < DB_NO_OF_POOL; i++) 
	      if (subquery->ops_global->ops_estate.opn_maxcache[i] == 0.0)
	      {	/* update only one for query to make it reproducible */
    		/* subquery->ops_global->ops_estate.opn_sblock = dmt_cb.dmt_io_blocking; */
		subquery->ops_global->ops_estate.opn_maxcache[i] = 
		    dmt_cb.dmt_cache_size[i]/subquery->ops_global->ops_cb->ops_server->opg_actsess; 
					/* this should
					** be the "single page" cache as opposed to
					** the "group page" cache,... currently project
					** restricts will use 2 pages of the single
					** page cache and then start using the
					** group level cache,... sort file will only
					** use the group cache */
		/* FIXME - after SCF is changed to pass the currently running number of
		** sessions use this for the non-repeat query case instead of opg_actsess
		*/
	      }
        }
#if 0
	if (type != OPN_IOSORT)
	{
	    dio += 3.0 * blocks;	/* write out temp relation (2 per page)
					** then read in to sort package. */
	    cpu += 3.0 * tuples;	/* add one for each time a tuple is 
					** read or written */
	}

	if (type == OPN_IOHASH)
	{
	    /* 2 * resultblks is actual number of result blocks because
	    ** fillfactor is 50%.  assume another 10% overflow pages.
	    ** primary pages have to be written twice, overflow pages do
	    ** not but cause the associated primary to be written again. 
	    */
	    dio += 4.4 * resultblks;
	}
	else if (type == OPN_IOISAM || type == OPN_IOBTREE)
	{
	    i4         dirblocks;	/* number of blocks in the directory */

	    resultblks *= 1.2;		/* 80% fillfactor */

	    /* a guess at number of blocks in directory */
	    dirblocks = (f8)MHceil( (f8)MHln( (f8)( resultblks / MHln(10.0)) ));

	    /* for each directory page we need to link it in, and then
	    ** write it and then read it in again for the next level
	    ** of the directory.  we also need to read in the primary
	    ** pages to build the directory and initially we need to
	    ** write out the primary pages (2 writes each) 
	    */
	    dio += 3.0 * (dirblocks + resultblks);
	}
#endif
    }


#ifdef xNTR1
	if (tTf(23, 8))
	{
		TRdisplay("costreformat type:%d, blocks:%.2f, tuples:%.2f, width:%d\n",
			type, blocks, tuples, width);
		TRdisplay("\t\tcpu:%.2f, dio:%.2f\n", cpu, dio);
	}
#endif

    *diop = dio;		    /* return estimated disk i/o */
    *cpup = cpu;		    /* return estimated cpu */
}
