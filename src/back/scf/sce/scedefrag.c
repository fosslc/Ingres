/*
** Copyright (c) 2008 Ingres Corporation
**
*/

/**
**
**  Name: scedefrag.c - Server event subsystem (EV) manager defragmentor
**
**  Description:
**
**  The event subsystem may, after time, become littered with many small
**  sized free memory blocks to an extent where error SCE will be seen
**  in the logs even though the sum of the free blocks exceeds the space
**  required to store an event instance.
**
**  This module defragments the memory, pushing all but the free block
**  records towards the high end, leaving a single pool of free memory
**  bewteen the bucket areas (static areas) and the dynamically allocated
**  area.
**  
**  It accomplishes this in four passes. The first two are over the exisiting
**  memory allocation, where the order in which records appear are discovered 
**  and thereby create an ordered memory map. The map is examined to make sure
**  it is complete, that no unexplained gaps are discovered, that the ordering 
**  offsets are monotonic increasing. The third pass is over the newly discovered 
**  map, and is to ensure that the offsets held in the memory map correspond to 
**  those in the event records themselves. This ensures the memory map may now
**  be used to re-write the offsets in the records, ensuring the event structure
**  we be retained. 
**
**  The final pass moves backwards over the sorted memory map, relocating the
**  highest event records first. As this proceeds, the offset fields in the
**  parent records are adjusted appropriately.
**
**  Internal Routines:
**
**      sce_recordOffsetEntry()  :: Records a description of an EV record in a map fragment
**      sce_qsortCompare()       :: A mandated routine for the UNIX qsort utility.
**      sce_sortFragments()      :: Sorts memory map fragments, and then looks for leaks.
**      sce_createMemoryMap()    :: Reads the EV memory, and creates a representation of it.
**      sce_compactMemoryMap()   :: Reads the memory map, and moves the EV records.
**
**  External Routines:
**
**      sce_defragment()         :: The entry point for the module 
**
**  Data Structures:
**
**       Apart from the EV layout strutures, all structures used are internal to 
**       the module.
**
**       ENUM   : offsetType : Tags the type of EV record at a given offset
**
**       ENUM   : anchorType : Tags the location in the EV record where the 
**                             offset was stored.
**
**       STRUCT : offsetDataType : Base type for array held in memory map.
**                                 Each item in the array represents one EV
**                                 record.
** 
**       STRUCT : fragmentType   : The type for the memory map.
**
**  History:
**       17-Apr-2008 : (coomi01) Created.
**  
*/

/*
**  A simple define to remove a meaningless looking constant in the code.
**  All we intend is to tag certain records, so we can treat them specially.
**  These records are just offsets which are stored in 'fixed' locations. 
*/
#define IS_ECB_BASED -1

/*
** A private enum to type each record describing an EV structure.
*/ 
typedef enum { 
    is_instance = -2000,   /* Value just to set it apart */
    is_svrentry,
    is_revent, 
    is_rserver, 
    is_free 
} offsetType;

/*
** A private enum to name the offset fields in the EV structures.
*/
typedef enum {
    a_foffset = -1000,  /* Value just to set it apart */
    a_ioffset,
    a_roffset,
    a_alloffset,
    a_fnextoff,
    a_shoffset,
    a_svrinstoff,
    a_innextoff,
    a_svrnextoff,
    a_rsnextoff,
    a_ehoffset,
    a_resvroff,
    a_renextoff
} anchorType;

/*
** A private structure relating our memory map data to the EV structure.
*/
typedef struct {
    int     unsortedPos;
    int       parentRec;
    int          offset;
    int            size;
    int       newOffset;

    anchorType refField;
    offsetType  refType;
} offsetDataType;


/*
** Name:  fragmentStatsType
**
** Description:
**
**    A structure to record the count of items
**    we have seen.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
**/
typedef struct {
    int count;

    int SVRENTRY;
    int RSERVER;
    int REVENT;
    int INST;
    int FREE;
}
fragmentStatsType;

/*
** Name:  fragmentType
**
** Description:
**
**     A private structure holding our memory map.
**     Note : There are two array substructures.
**     Both are allocated once we have scanned the
**     EV memory segment, to size it up.
**
**     The first array, offsetData, is our recording
**     of the EV memory layout, created by walking
**     the links in EV memory without taking note
**     of offset ordering.
**
**     The second array, points at elements of the 
**     the first array. The UNIX utility, qsort,
**     is given this array to sort and puts the
**     pointers in order according to the offsets
**     in EV memory.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
typedef struct {
    EV_SCB                       *ecb;
    fragmentStatsType           stats;

    int                    maxEntries;
    int                  currentEntry;

    offsetDataType        *offsetData;
    offsetDataType **sortedOffsetData;
} fragmentType;

/*
** Name:  sce_recordOffsetEntry
**
** Description:
**        To initialise next available element in offsetData array.
**
** Inputs:
**
**        *map      :: Pointer to our memory map
**        offset    :: Our record's offset in memory.
**        refType   :: The type of our record
**
**        parentRec :: If positive  the position of parent's record in the map.
**                     If negative, an indicator to where in the ecb reference may be found.
**        refField  :: Names the field in the parent where our offset is recorded
**
** Outputs:
**        position  :: Tells the caller where we put the details in the map. 
**
** Result Code:
**        OK        :: If we had the space to perform the request
**        FAIL      :: Otherwise.
**
** History:
**
**       17-Apr-2008 : (coomi01) Created.
*/
static 
STATUS sce_recordOffsetEntry(fragmentType *map, int offset, offsetType refType, 
			     int parentRec, anchorType refField,  int *position)
{
    /* 
    ** Double check for array over-run
    */
    if (map->currentEntry < map->maxEntries)
    {
	/* 
	** Just take note of our parameters
	*/
	map->offsetData[map->currentEntry].unsortedPos = map->currentEntry;
	map->offsetData[map->currentEntry].parentRec   = parentRec;	
	map->offsetData[map->currentEntry].refField    = refField;
	map->offsetData[map->currentEntry].refType     = refType;
	map->offsetData[map->currentEntry].offset      = offset;
	map->offsetData[map->currentEntry].newOffset   = -1;
	map->offsetData[map->currentEntry].size        = -1;

	/* 
	** Make ready for sorting
	** - we actually leave the sort to the end.
	*/
	map->sortedOffsetData[map->currentEntry] = &map->offsetData[map->currentEntry];

	/* 
	** OUTPUT :: Inform user where we got to
	*/
	*position = map->currentEntry;

	/*
	** Make next entry available. 
	*/
	map->currentEntry++;
	return OK;
    }
    else
    {
	return FAIL;
    }
}

/*
** Name:         sce_qsortCompare
**
** Description:  A standard comparator function for qsort
**
** Inputs:       Left and Right hand sides to be compared
**               Both are pointers to an element in the pointer array.
**
** Result Code:  
**               1  Left bigger than Right
**               0  Left equals Right
**               -1  Left smaller than Right
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
static 
int sce_qsortCompare(const void *lhs, const void *rhs)
{
    if ( (*(offsetDataType **)lhs)->offset > (*(offsetDataType **)rhs)->offset )  return  1;
    if ( (*(offsetDataType **)lhs)->offset < (*(offsetDataType **)rhs)->offset )  return -1;
    return 0;
}

/*
** Name: sce_sortFragments
**
** Description:
**
**       We have a memory map, but link can go fprward/backwards within.
**       We want now to sort by the order in which the records appear
**       within the EV memory. It is thus safe to move the highest record
**       even higher, as there is anothing above it, and so on.
**
** Inputs:
**       map   :: The memory map to be sorted.
**
** Outputs:
**       map   :: The sortedOffsetData[] array will have values adjusted.
**
** Result Code:
**       OK    :: The process worked
**       FAIL  :: Otherwise.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
static 
STATUS sce_sortFragments(fragmentType *map)
{
    int calculatedSize;
    int expectedOffset;
    int previousOffset;
    int   svrentrySize;
    int    rserverSize;
    int     reventSize;
    int     holesInMap;
    int    sizeOfHoles;
    int           loop;
    int            pad;

    /*
    ** External UNIX util, very quick!
    */
    qsort(map->sortedOffsetData,
	  map->currentEntry,
	  sizeof(offsetDataType *),
	  sce_qsortCompare
	);

    /*
    ** Calc all the fixed sizes, according to the sce_eallocate() receipe
    ** - No need to repeat this calc every time in the switch below.
    */
    svrentrySize = sizeof(EV_SVRENTRY);
    if (svrentrySize < sizeof(EV_FREESPACE))
	svrentrySize = sizeof(EV_FREESPACE);
    pad = svrentrySize % sizeof(ALIGN_RESTRICT);
    if (pad != 0)
	svrentrySize += sizeof(ALIGN_RESTRICT) - pad;

    rserverSize = sizeof(EV_RSERVER);
    if (rserverSize < sizeof(EV_FREESPACE))
	rserverSize = sizeof(EV_FREESPACE);
    pad = rserverSize % sizeof(ALIGN_RESTRICT);
    if (pad != 0)
	rserverSize += sizeof(ALIGN_RESTRICT) - pad;

    reventSize = sizeof(EV_REVENT);
    if (reventSize < sizeof(EV_FREESPACE))
	reventSize = sizeof(EV_FREESPACE);
    pad = reventSize % sizeof(ALIGN_RESTRICT);
    if (pad != 0)
	reventSize += sizeof(ALIGN_RESTRICT) - pad;

    /* 
    ** Will count holes in map
    */
    holesInMap  = 0;
    sizeOfHoles = 0;

    /*
    ** Fix up the first tests to pass.
    */
    expectedOffset = map->sortedOffsetData[0]->offset;
    previousOffset = 0;

    /*
    ** If the sort worked, then we can test the memory map.
    */
    for (loop=0;loop<map->currentEntry;loop++)
    {
	/**
	 ** Ensure monotone increasing offset values 
	 **/ 
	if ( map->sortedOffsetData[loop]->offset < previousOffset )
	{
	    /*
	    **  If we try this we will corrupt
	    **  - Should never happen
	    */
	    return FAIL;
	}
	previousOffset = map->sortedOffsetData[loop]->offset;

	/*
	** This should not happen, unless we have memory leak!
	*/
	if ( map->sortedOffsetData[loop]->offset != expectedOffset )
	{
	    /*
	    ** Our map is incomplete, and more to the point 
	    ** we do not know what is in the hole
	    */
	    return FAIL;
	}

	switch ( map->sortedOffsetData[loop]->refType )
	{
	    case (is_instance):
	    {
		EV_INSTANCE *instance;

		/*
		** Instance records are the only variant records in the system
		** We have to calculate their length in the same manner as sce
		*/
		instance = (EV_INSTANCE *)( (PTR)map->ecb + map->sortedOffsetData[loop]->offset); 
		calculatedSize = sizeof(EV_INSTANCE) + instance->ev_inlname + instance->ev_inldata;	

		/*
		** The size is adjusted to ensure alignment properties upheld.
		*/
		if (calculatedSize < sizeof(EV_FREESPACE))
		    calculatedSize = sizeof(EV_FREESPACE);
		pad = calculatedSize % sizeof(ALIGN_RESTRICT);
		if (pad != 0)
		    calculatedSize += sizeof(ALIGN_RESTRICT) - pad;
		
	    }
	    break;

	    case (is_svrentry):
	    {
		calculatedSize = svrentrySize;
	    }
	    break;

	    case (is_revent):
	    {
		calculatedSize = reventSize;
	    }
	    break;

	    case (is_rserver):
	    {
		calculatedSize = rserverSize;
	    }
	    break;

	    case (is_free):
	    {
		EV_FREESPACE *free;
		free = (EV_FREESPACE *)((PTR)map->ecb + map->sortedOffsetData[loop]->offset); 
		calculatedSize = free->ev_fsize;
	    }
	    break;

	    default:
	    {
		/*
		** Unexpected type
		*/
		return FAIL;
	    }
	    break;
	}

	/*
	** Record size of this item
	*/
	map->sortedOffsetData[loop]->size = calculatedSize;

	/* 
	** We are about to go round again, the next record
	** ought to be right on our tail, if not we have a hole.
	** Prepare for the test.
	*/
	expectedOffset = map->sortedOffsetData[loop]->offset + calculatedSize;
    }

    return OK;
}


/*
** Name:   sce_createMemoryMap
**
** Description: 
**         To walk over the ECB memory, and create a data structure
**         through which we can then defregment it.
** Inputs:
**         evc    :: A pointer to a context block, in which details
**                   of the ECB are stored.
**         passNo :: Called from outside, this should ALWAYS be zero.
**                   Called from within indicates we are on the second pass.
** Outputs:
**         map    :: We create a memory map for our client, and store it here.
**
** Result Code:
**         OK     :: We succeeded
**         FAIL   :: We had a problem.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
static 
STATUS sce_createMemoryMap(SCEV_CONTEXT *evc, int passNo, fragmentType *map)
{
    STATUS result = OK;
    int   foundProblem;
    int     svrIndex=0;
    int      evIndex=0;
    int        index=0;
    int         bucket;
    EV_SCB              *ecb;
    EV_EVHASH        *evHash;
    EV_SVRHASH      *svrHash;
    EV_SVRENTRY    *svrEntry;
    EV_INSTANCE      *evInst;
    EV_REVENT       *evEntry;
    EV_RSERVER         *rSvr;
    EV_FREESPACE     *evFree;
    STATUS            status;

    ecb = (EV_SCB *)evc->evc_root;    
    map->ecb = ecb;
    map->maxEntries = -1;
    map->currentEntry = 0;

    /*
    ** First call, passNo  = 0 ~ sizing up
    ** Second call, passNo = 1 ~ now know how much space to allocate
    */
    if (1 == passNo)
    {
	status = OK;
	map->maxEntries       = map->stats.count;
	map->offsetData       = (offsetDataType *)  MEcalloc(map->maxEntries * sizeof(offsetDataType  ), &status);
	if (status != OK)
	    return FAIL;

	map->sortedOffsetData = (offsetDataType **) MEcalloc(map->maxEntries * sizeof(offsetDataType *), &status);
	if (status != OK)
	    return FAIL;
    }
    else
    {
	map->maxEntries       = 0;
	map->offsetData       = 0;
	map->sortedOffsetData = 0;
    }

    /*
    ** Each pass, reset the stats
    */
    map->stats.count    = 0;
    map->stats.SVRENTRY = 0;
    map->stats.RSERVER  = 0;
    map->stats.REVENT   = 0;
    map->stats.INST     = 0;
    map->stats.FREE     = 0;

    foundProblem = 0;


    /* 
    ** First task, walk through the Free space list
    */
    if (0 != ecb->evs_foffset) 
    {
	map->stats.FREE++;
	if (1 == passNo)
	{
	    /* 
	    ** Record first record as anchored in ecb at foffset
	    */
	    foundProblem += sce_recordOffsetEntry(map,
						  ecb->evs_foffset, 
						  is_free,
						  IS_ECB_BASED,
						  a_foffset,
						  &index);
	}

	/*
	** Get pointer to the EV mem record
	*/
	evFree = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);

	while ( ! foundProblem )
	{
            if (0 == evFree->ev_fnextoff)
                break;

	    map->stats.FREE++;

            if (1 == passNo)
	    {
		/*
		** Record subsequent records as anchored in their parent 
		** (was output as index in above, then repeatedly this call)
		*/
		foundProblem += sce_recordOffsetEntry(map, 
						 evFree->ev_fnextoff, 
						 is_free, 
						 index, 
						 a_fnextoff,
						 &index);
	    }

	    /*
	    ** Get pointer to the EV mem record, ready for next round.
	    */
	    evFree = (EV_FREESPACE *)((PTR)ecb + evFree->ev_fnextoff);
	}
    }

    /*
    ** Check status
    */
    if ( foundProblem )
	return FAIL;


    /*
    ** Now ready to read event instance records
    */
    if (0 != ecb->evs_ioffset)
    {
	/*
	** Get a pointer to bucket set
	*/
	svrHash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);

	/*
	** Now walk through all buckets
	*/
	for (bucket=0; bucket<evc->evc_ibuckets; bucket++)
	{
	    /*
	    ** Test bucket for content
	    */
	    if (svrHash[bucket].ev_shoffset == 0)
		continue;
	    
	    map->stats.SVRENTRY++;

	    if (1 == passNo) 
	    {
		/* 
		** Record first record as anchored in ecb/bucket at shoffset
		** - Which bucket must also be recorded. This must be done
		**   by using a negative number (being negative gives rise
		**   to special treatment later)
		*/
		foundProblem += sce_recordOffsetEntry(map, 
						 svrHash[bucket].ev_shoffset,
						 is_svrentry,
						 -(bucket+1),
						 a_shoffset,
						 &svrIndex);
	    }

	    /*
	    ** Get pointer to this EV mem record
	    */
	    svrEntry = (EV_SVRENTRY *) ((PTR)ecb + svrHash[bucket].ev_shoffset);
	    
	    /*
	    ** Now ready to loop over server chains
	    */
	    while (! foundProblem)
	    {
		/*
		** Carefully note the value of the parent record.
		** It comes from  sce_recordOffsetEntry above, or 
		** right at the bottom of this outer loop below.
		**
		** The inner loop spins index, not svrIndex
		** The outer loop spins svrIndex.
		*/
		index = svrIndex;

		/*
		** Walk down this server chain looking at its' instances
		*/
		if ( 0 != svrEntry->ev_svrinstoff )
		{
		    map->stats.INST++;		    

		    if (1 == passNo) 
		    {
			/*
			** Record subsequent records as anchored in their parent 
			** (was output as svrIndex in above, copied to index)
			**
			** The record was anchored in svrinstoff of the parent
			*/
			foundProblem += sce_recordOffsetEntry(map, 
							      svrEntry->ev_svrinstoff, 
							      is_instance, 
							      index,
							      a_svrinstoff, /*NOTE*/
							      &index);
		    }

		    /*
		    ** Get pointer to this EV mem record
		    */
		    evInst = (EV_INSTANCE *)((PTR)ecb + svrEntry->ev_svrinstoff);
		    
		    /*
		    ** The inner loop
		    */
		    while (! foundProblem)
		    {
			/*
			** Test for end of instance chain set.
			*/
			if (0 == evInst->ev_innextoff)
			    break;
			
			map->stats.INST++;

			if (1 == passNo) 
			{
			    /*
			    ** Record subsequent records as anchored in their parent 
			    ** (was output as index in above, then repeatedly this call)
			    **
			    ** The record was anchored in innextoff of the parent
			    */
			    foundProblem += sce_recordOffsetEntry(map, 
								  evInst->ev_innextoff, 
								  is_instance, 
								  index, 
								  a_innextoff, /*NOTE*/
								  &index);
			}

			/*
			** Get pointer to this EV mem record
			*/
			evInst = (EV_INSTANCE *)((PTR)ecb + evInst->ev_innextoff);

		    } /** inner while ends */

		} /* End of this server chain */

		/*
		** Outer while now resumes, 
		** we shall now move onto next server in our bucket chain.
		*/

		/*
		** First Test for problems, or end of bucket chain set.
		*/
		if ( foundProblem || (0 == svrEntry->ev_svrnextoff) )
		    break;

		/*
		** To the next server (in the bucket chain) using the outer loop.
		** - svrIndex now bumps
		*/
		
		map->stats.SVRENTRY++;
		if (1 == passNo) 
		{
		    /* Record the new server record.
		    ** Its' parent was the previous server record, 
		    ** which is why we preserved svrIndex, and was
		    ** achored in to svrnextoff field
		    */
		    foundProblem += sce_recordOffsetEntry(map, 
							  svrEntry->ev_svrnextoff, 
							  is_svrentry, 
							  svrIndex, 
							  a_svrnextoff,
							  &svrIndex  /*The Bump*/);
		}

		/*
		** Get pointer to this EV mem record
		*/
		svrEntry = (EV_SVRENTRY *)((PTR)ecb + svrEntry->ev_svrnextoff);

	    } /* ends the outter while loop */

	} /* So onto the next bucket in for loop */

    } /* Ends if (ecb->evs_ioffset) */

    /*
    ** Test for problems
    */
    if ( foundProblem )
	return FAIL;

    /*
    ** Now ready to read all-events registered servers
    */
    if ( ecb->evs_alloffset != 0 )
    {

	map->stats.RSERVER++;

	if (1 == passNo)
	{
	    /* 
	    ** Record first rserver record as anchored in ecb at alloffset
	    */
	    foundProblem += sce_recordOffsetEntry(map, 
						  ecb->evs_alloffset, 
						  is_rserver, 
						  IS_ECB_BASED, 
						  a_alloffset,
						  &index);
	}

	/*
	** Get pointer to this EV mem record
	*/
	rSvr = (EV_RSERVER *)((PTR)ecb + ecb->evs_alloffset);

	while (! foundProblem )
	{
	    /* 
	    ** Does this rserver record lead to another
	    */
	    if (0 == rSvr->ev_rsnextoff)
		break;

	    map->stats.RSERVER++;

	    if (1 == passNo) 
	    {
		/* 
		** Record subsequent rserver records as anchored in predecessor
		*/
		foundProblem += sce_recordOffsetEntry(map, 
						 rSvr->ev_rsnextoff, 
						 is_rserver, 
						 index, 
						 a_rsnextoff,
						 &index);
	    }
	    
	    /*
	    ** Get pointer to this EV mem record
	    */
	    rSvr = (EV_RSERVER *)((PTR)ecb + rSvr->ev_rsnextoff);
	}
    }

    /*
    ** Test for problems again.
    */
    if ( foundProblem )
	return FAIL;

    /*
    ** Now ready to read event rserver records
    */
    if ( ecb->evs_roffset != 0 )
    {
	/*
	** Get a pointer to bucket set
        */
	evHash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);

        /*
	** Now walk through all buckets
        */
	for (bucket=0; bucket<evc->evc_rbuckets; bucket++)
	{

	    /*
	    ** Test bucket for content
            */
	    if (evHash[bucket].ev_ehoffset == 0)
		continue;

	    map->stats.REVENT++;
	    if (1 == passNo)
	    {
                /*
		** Record first record as anchored in ecb/bucket at ehoffset
                ** - Which bucket must also be recorded. This must be done
                **   by using a negative number (being negative gives rise
                **   to special treatment later)
                */
                foundProblem += sce_recordOffsetEntry(map, 
						      evHash[bucket].ev_ehoffset, 
						      is_revent, 
						      -(bucket+1), 
						      a_ehoffset,
						      &evIndex);
	    }

	    /*
	    ** Get pointer to this EV mem record
            */
	    evEntry = (EV_REVENT *)((PTR)ecb + evHash[bucket].ev_ehoffset);


	    /*
	    ** Now ready to loop over rserver chains
            */
	    while (! foundProblem)
	    {
		/*
		** Carefully note the value of the parent record.
                ** It comes from  sce_recordOffsetEntry above, or
                ** right at the bottom of this outer loop below.
                **
                ** The inner loop spins index, not evIndex
                ** The outer loop spins evIndex.
                */
		index = evIndex;
		if ( 0 != evEntry->ev_resvroff )
		{
		    map->stats.RSERVER++;
		    if (1 == passNo)
		    {

                        /*
			** Record subsequent records as anchored in their parent
                        ** (was output as evIndex in above, copied to index)
                        **
                        ** The record was anchored in resvroff of the parent
                        */
			foundProblem += sce_recordOffsetEntry(map, 
							      evEntry->ev_resvroff, 
							      is_rserver, 
							      index, 
							      a_resvroff, /*NOTE*/
							      &index);
		    }

		    /*
		    ** Get pointer to this EV mem record
                    */ 
		    rSvr = (EV_RSERVER *)((PTR)ecb + evEntry->ev_resvroff);

                    /*
		    ** The inner loop
                    */
		    while (! foundProblem)
		    {
			/*
			** Test for end of rserver chain set.
                        */
			if (0 == rSvr->ev_rsnextoff)
                            break;

			map->stats.RSERVER++;

			if (1 == passNo)
			{
			    /*
			    ** Record subsequent records as anchored in their parent
                            ** (was output as index in above, then repeatedly this call)
                            **
                            ** The record was anchored in rsnextoff of the parent
                            */
			    foundProblem += sce_recordOffsetEntry(map, 
								  rSvr->ev_rsnextoff, 
								  is_rserver, 
								  index,
								  a_rsnextoff, /*NOTE*/ 
								  &index);
			}

                        /*
			** Get pointer to this EV mem record
                        */
                        rSvr = (EV_RSERVER *)((PTR)ecb + rSvr->ev_rsnextoff);

		    } /* inner while ends */

		} /* End of this rserver chain */
		

		/*
		** Outer while now resumes,
                ** we shall now move onto next rserver in our bucket chain.
                */

		/*
		** First Test for problems, or end of bucket chain set.
		*/
		if ( foundProblem || (0 == evEntry->ev_renextoff ))
		    break;

		/*
		** To the next rserver (in the bucket chain) using the outer loop.
		** - evIndex now bumps
		*/

		map->stats.REVENT++;
		if (1 == passNo)
		{
		    /* Record the new rserver record.
		    ** Its' parent was the previous server record,
                    ** which is why we preserved evIndex, and was
                    ** achored in to renextoff field
                    */
		    foundProblem += sce_recordOffsetEntry(map, 
							  evEntry->ev_renextoff, 
							  is_revent, 
							  evIndex, 
							  a_renextoff,
							  &evIndex /*The Bump*/);
		}

		/*
		** Get pointer to this EV mem record
                */
		evEntry = (EV_REVENT *)((PTR)ecb + evEntry->ev_renextoff);

	    } /* ends the outter while loop */

	} /* So onto the next bucket in for loop */

    } /* Ends if (ecb->evs_roffset) */

    /*
    ** Test for problems again.
    */
    if (foundProblem)
	return FAIL;

    /* 
    ** OK, EV data structure inspection is complete
    */

    /* 
    ** Finish off the stats record, how many records did we find.
    */
    map->stats.count = ( map->stats.SVRENTRY +
			 map->stats.RSERVER  +
			 map->stats.REVENT   +
			 map->stats.INST     +
			 map->stats.FREE     );
    
    /*
    ** If just completed the first pass, we have the stats
    ** so we can now recurse and create the map knowing how
    ** big it will be.
    */
    if (0 == passNo)
    {
	passNo = 1;

	/*
	** Do walk again, but this time note the fragments 
	** - The first run was just a sizing up exercise.
	*/
	foundProblem = sce_createMemoryMap(evc, passNo, map);
	
	if ( foundProblem ) 
	    return FAIL;
	
	/* The map is ok IFF the sorted Fragments
	** exhibit proper record order, and compactness
	*/
	return sce_sortFragments(map);
    }

    /*
    ** Done second pass!
    */
    return OK;
}

/*
** Name: sce_compactMemoryMap 
**
** Description:
**       This routine does the work required.
**
**       Prior to calling it, a faithful description of the EV memory
**       layout will have been constructed and checked.
**
**       This routine now walks over the map in two passes, calling
**       itself to effect the second pass.
**
**       In the first pass, new offsets are calculated and free space
**       sizes accumulated. The current offsets in EV memory are also
**       checked to ensure that the linkage reported in the mmemory
**       map exists in EV memory too.
**
**       The second pass walks the memory map in exactly the same
**       fashion as the first, except it now writes the newly calculated
**       offset values into EV memory having shifted the record up
**       towards the high end.
**
**       Finally, it creates a new freelist near the base of the EV memory.
**
** Inputs:
**         evc    :: A pointer to a context block, in which details
**                   of the ECB are stored.
**         passNo :: Called from outside, this should ALWAYS be zero.
**                   Called from within indicates we are on the second pass.
**         map    :: This is a faithful map of the memory to be compacted.
**
** Result Code:
**         OK     :: We succeeded
**         FAIL   :: We had a problem.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
static 
STATUS sce_compactMemoryMap(SCEV_CONTEXT *evc, int passNo, fragmentType *map)
{
    PTR  srcMemPtr;
    PTR  dstMemPtr;
    int       loop;
    EV_SCB    *ecb;
    EV_FREESPACE *freeSp;
    int       unsortedPos,bucket,size,alloc, pages, farEnd, newOffset, oldOffset, freeSize = 0,parentRec;
    int *offsetPtr;

    ecb = (EV_SCB *)evc->evc_root;

    /* 
    ** Do same calcs from sce_einitialize to determin memory size. 
    */
    alloc = (sizeof(EV_SCB)) +
	    (evc->evc_rbuckets * sizeof(EV_EVHASH)) +
	    (evc->evc_mxre * (sizeof(EV_REVENT) + sizeof(EV_RSERVER))) +
	    (evc->evc_ibuckets * sizeof(EV_SVRHASH)) +
	    (evc->evc_mxei * (sizeof(EV_SVRENTRY) + EV_INST_ALLOC_MAX));
    pages = (alloc + ME_MPAGESIZE) / ME_MPAGESIZE;	/* Bytes to pages */
    farEnd = pages * ME_MPAGESIZE;

    /* 
    ** Walk backwards, relocating high records first
    */
    for (loop=map->currentEntry-1; loop>=0; loop--)
    {

	/*
	**  Transfer from our ordered regime into the unordered one!
	**  - This *just* to expedite clarity! 
	**    The parentRec, for instance, always indexes the 
	**    unsorted data.
	**
	**    Rather than bounce back and forth we do it just the ONE way.
	*/
	unsortedPos  = map->sortedOffsetData[loop]->unsortedPos;

	/* 
	** Never shift free blocks
	*/	
	if ( map->offsetData[unsortedPos].refType != is_free)
	{
	    /*
	    ** Ensure pointer to our parent's offset is NULLed
	    */
	    offsetPtr    = (int *)0;

	    /*
	    ** Set up a few vars for short hand
	    */
	    parentRec    = map->offsetData[unsortedPos].parentRec;
	    size         = map->offsetData[unsortedPos].size;
	    oldOffset    = map->offsetData[unsortedPos].offset;
	    newOffset    = farEnd - size;

	    /*
	    ** These are ready for the record move later
	    **/
	    dstMemPtr    = (PTR)ecb + newOffset;
	    srcMemPtr    = (PTR)ecb + oldOffset;

	    /*
	    ** First we must find the reference to this record is our parent
	    ** - We are going to correct it after the move 
	    */

	    /* 
	    ** OK, Test for ECB anchored records.
	    ** - The test looks odd:-
	    **   All ECB/Bucket records are recorded as a negative
	    **   All other (positive) records are references into our unsorted array.
	    **   The macro IS_ECB_BASED, is just a hardwired -1.
	    */
	    if ( parentRec > IS_ECB_BASED )
	    {
		/*
		** Ok, our parent, what sort of record was it ?
		*/
		switch ( map->offsetData[unsortedPos].refType )
		{
		    case (is_revent):
		    {
			EV_REVENT *evEntry;
			EV_EVHASH *evHash;

			/* 
			** Ok, it was an REVENT record.
			*/

			/*
			** Now, was our anchor in the  ehoffset, or renextoff field
			*/
			if (map->offsetData[unsortedPos].refField == a_ehoffset)
			{
			    /* 
			    ** ehoffset, right at the top of the chain
			    */

			    /*
			    ** So, was the parent in the middle of the chain, 
			    ** or anchored in the ECB ?
			    */
			    if (map->offsetData[parentRec].parentRec > IS_ECB_BASED )
			    {
				/*
				** Clear, it was in mid chain.
				*/

				/*
				** Now Check to see if our parent is already relocated
				*/
				if ( map->offsetData[parentRec].newOffset > 0)
				{
				    evEntry  = (EV_REVENT *)  ((PTR)ecb + map->offsetData[parentRec].newOffset);
				}
				else
				{
				    evEntry  = (EV_REVENT *)  ((PTR)ecb + map->offsetData[parentRec].offset);
				}
			    }
			    else
			    {
				/*
				** Ah, in the ECB. This means it was in a bucket
				** - The position of the offset record does not get moved.
				*/

				/*
				** Recover the bucket
				*/
				bucket   = -1 - map->offsetData[parentRec].parentRec;

				/*
				** And from there our parent's record, using the bucket ident.
				*/
				evHash   = (EV_EVHASH  *) ((PTR)ecb + ecb->evs_roffset);
				evEntry  = (EV_REVENT *)  ((PTR)ecb + evHash[bucket].ev_ehoffset);
			    }

			    /*
			    ** OK, this is our parent's offset field
			    */
			    offsetPtr = &(evEntry->ev_resvroff);
			}
			else if (map->offsetData[unsortedPos].refField == a_renextoff)
			{

			    /*
			    ** OK, the parent REVENT was stored in our relocatable set
			    */

			    /*
			    ** So, check to see if our parent relocated
			    */
			    if ( map->offsetData[parentRec].newOffset > 0)
				evEntry = (EV_REVENT *)((PTR)ecb + map->offsetData[parentRec].newOffset);
			    else
				evEntry = (EV_REVENT *)((PTR)ecb + map->offsetData[parentRec].offset);

			    /*
			    ** And here is a pointer to the field
			    */
			    offsetPtr = &(evEntry->ev_renextoff);
			}

			/*
			** If at this stage offsetPtr is not set, then we have a problem
			** - But we will recognise it a little latter by testing offsetPtr
			*/
		    }
		    break; /** case of is_revent **/

		    case (is_rserver):
		    {
			EV_EVHASH  *evHash;
			EV_REVENT  *evEntry;
			EV_RSERVER *rSvr;

			/* 
			** Ok, it was a RSERVER record.
			*/

			/*
			** Now, was our anchor in the  resvroff, or rsnextoff field
                        */
			if (map->offsetData[unsortedPos].refField == a_resvroff)
			{

			    /*
			    ** resvroff, right at the top of the chain
                            */

			    /*
			    ** So, was the parent in the middle of the chain,
                            ** or anchored in the ECB ?
                            */

			    if (map->offsetData[parentRec].parentRec > IS_ECB_BASED)
			    {
				/*
				** Clear, it was in mid chain.
                                */

				/*
				** Check to see if our parent relocated
				*/
				if ( map->offsetData[parentRec].newOffset > 0)
				{
				    evEntry  = (EV_REVENT *)  ((PTR)ecb + map->offsetData[parentRec].newOffset);
				}
				else
				{
				    evEntry  = (EV_REVENT *)  ((PTR)ecb + map->offsetData[parentRec].offset);
				}
			    }
			    else
			    {
				/*
				** Ah, in the ECB. This means it was in a bucket
                                */

                                /*
				** Recover the bucket
                                */
                                bucket   = -1 - map->offsetData[parentRec].parentRec;

				/*
				** And from there our parent's record, using the bucket ident.
                                */
				evHash   = (EV_EVHASH  *) ((PTR)ecb + ecb->evs_roffset);
                                evEntry  = (EV_REVENT *)  ((PTR)ecb + evHash[bucket].ev_ehoffset);
			    }

			    /*
			    ** OK, this is our parent's offset field
			    */
			    offsetPtr = &(evEntry->ev_resvroff);
			}
			else if (map->offsetData[unsortedPos].refField == a_rsnextoff)
			{
			    /*
			    ** OK, the parent RSERVER was stored in our relocatable set
			    */

			    /*
			    ** Check to see if our parent relocated
			    */
			    if ( map->offsetData[parentRec].newOffset > 0)
				rSvr = (EV_RSERVER *)((PTR)ecb + map->offsetData[parentRec].newOffset);
			    else
				rSvr = (EV_RSERVER *)((PTR)ecb + map->offsetData[parentRec].offset);

			    /*
			    ** Note the location
			    */
			    offsetPtr = &(rSvr->ev_rsnextoff);
			}
		    }
		    break; /** case of is_rserver **/

		    case (is_instance):
		    {
			/* 
			** Ok, it was an INSTANCE record.
			*/

			EV_INSTANCE *evInst;
			EV_SVRENTRY *svrEntry;
			EV_SVRHASH  *svrHash;

			/*
			** Instance records are anchored by svrinstoff, in a bucket,
			** or as innextoff in a chain.
			*/
			if (map->offsetData[unsortedPos].refField == a_svrinstoff)
			{
			    /*
			    ** In a svrinstoff, which is in a bucket
			    */

			    /*
			    ** Recover the bucket
			    */
			    bucket   = -1 - map->offsetData[parentRec].parentRec;

			    /*
			    ** And from there a pointer to the record
			    */
			    svrHash  = (EV_SVRHASH  *) ((PTR)ecb + ecb->evs_ioffset);
			    svrEntry = (EV_SVRENTRY *) ((PTR)ecb + svrHash[bucket].ev_shoffset);

			    /*
			    ** This is a pointer to the offset required
			    */
			    offsetPtr = &(svrEntry->ev_svrinstoff);
			}
			else if (map->offsetData[unsortedPos].refField == a_innextoff)
			{
			    /*
			    ** Check to see if our parent already relocated
			    */
			    if ( map->offsetData[parentRec].newOffset > 0)
				evInst = (EV_INSTANCE *)((PTR)ecb + map->offsetData[parentRec].newOffset);
			    else
				evInst = (EV_INSTANCE *)((PTR)ecb + map->offsetData[parentRec].offset);

			    /*
			    ** This is a pointer to the offset required
			    */
			    offsetPtr = &(evInst->ev_innextoff);
			}
		    }
		    break; /** case of is_instance **/

		    default:
		    {
			/**
			 ** Should not get here, ensure we go no further. 
			 **/
			offsetPtr = (int *)0;
		    }
		    break;
		}
	    }
	    else /* Test For ECB Based Records */
	    {
		/* 
		** Reference is from an ecb/bucket anchor
		*/
		switch ( map->offsetData[unsortedPos].refField )
		{
		    case (a_foffset):
		    {
			/* Direct from ECB */
			offsetPtr = &ecb->evs_foffset;
		    }
		    break;
		    case (a_ioffset):
		    {
			/* Direct from ECB */
			offsetPtr = &ecb->evs_ioffset;			
		    }
		    break;
		    case (a_roffset):
		    {
			/* Direct from ECB */
			offsetPtr = &ecb->evs_roffset;			
		    }
		    break;
		    case (a_alloffset):
		    {
			/* Direct from ECB */
			offsetPtr = &ecb->evs_alloffset;			
		    }
		    break;
		    case (a_shoffset):
		    {
			/* Recover the bucket ident */
			bucket = -parentRec-1;

			/* Then the reference */
			offsetPtr = &(((EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset))[bucket].ev_shoffset);
		    }
		    break;
		    case (a_ehoffset):
		    {
			/* Recover the bucket ident */
			bucket = -parentRec-1;

                        /* Then the reference */
			offsetPtr = &(((EV_EVHASH *)((PTR)ecb + ecb->evs_roffset))[bucket].ev_ehoffset);
		    }
		    break;
			
		    default:
		    {
			/** Should not get here **/
			offsetPtr = (int *)0;
		    }
		    break;
		}
	    }


	    /* 
	    ** We have calculated where our parent links into our own record.
	    ** If the calculation was correct, the contents of index in the
	    **  parent record should match to where we think we are....
	    **
	    ** Test it, and return immediately if we have failed.
	    **/
	    if (((int *)0 == offsetPtr) || (oldOffset != *offsetPtr))
	    {
		/*
		** Our Memory Map is faulty/corrupted
		** - Should be caught in pass 1
		** - Aside, the only way for this to fail in pass 2 is
		**   if our parent was moved, and we did not record the
		**   new position. Then we have a crisis, we don't know
		**   where it went, and so we can't patch it.
		**   Aka we really do have a problem.
		*/
		return FAIL;
	    }

	    if (1 == passNo)
	    {
		/*
		** When we get here, we are ready for the move
		*/

		/* Carefully take note in our memory map as 
		** to where we are going
		*/
		map->offsetData[unsortedPos].newOffset = newOffset;

		/*
		** Now, alter the ECB memory refernce to us.
		*/
		*offsetPtr = newOffset;

		/*
		** And so to our goal, move the record itself.
		*/

		/*
		** WARNING
		** =======
		**
		** UNIX memmove has an important quality in that if
		** the source and dest memory overlap, it will be
		** careful about copy direction (forwards from start
		** or backwards from end). This prevents corruption.
		**
		** For our first few records we are very likely to
		** overlap and require this feature.
		**
		** MEmemmove is a cpp macro wrapper, which if gets
		** defined to be memcpy() would be a disaster!
		*/
		MEmemmove(dstMemPtr,srcMemPtr,size);
	    }
	    
	    /*
	    ** Adjust far end position, so next time round our
	    ** record will be moved to the next position below us.
	    */
	    farEnd -= size;

	}
	else /** for if ( .refType != is_free ) **/
	{
	    /*
	    ** Do not relocate, but do accumulate
	    ** - The final sum will be checked.
	    ** - This is done in pass one and two.
	    ** - At the end of pass one, we use the value as a cross check.
	    */
	    freeSize += map->offsetData[unsortedPos].size;
	}

    } /** Ends For loop **/

    /* 
    ** Check the EV recorded free tally is the same as our calculated one
    */
    if ( ecb->evs_favailable != freeSize )
	return FAIL;


    if (1 == passNo)
    {
	/*
	** Create a free list
	*/
	ecb->evs_foffset = ( ecb->evs_ioffset +
			     (ecb->evs_ibuckets * sizeof(EV_SVRHASH)));
	freeSp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
	freeSp->ev_fsize = freeSize;
	freeSp->ev_fnextoff = 0;

	return OK;
    }
     
    if (0 == passNo)
    {
	passNo = 1;
	return sce_compactMemoryMap(evc, passNo, map);
    }
}

/*
** Name:  sce_defragment
**
** Description:
**        The entry point routine for the module.
**
** Inputs:
**        SCEV_CONTEXT *evc  : Pointer to the EV memory context. 
**
** Outputs:
**        None.
**
** Result Code:
**
**        OK   : The operation was sucessful
**        FAIL : There was a problem.
**
** History:
**       17-Apr-2008 : (coomi01) Created.
*/
STATUS sce_defragment(SCEV_CONTEXT *evc)
{
    STATUS result = FAIL;
    fragmentType memMap;
    int passNo = 0; /** Fixed, we are the client. **/

    /*
    ** First create a map
    */
    if ( OK == sce_createMemoryMap(evc, passNo, &memMap ) )
    {
	/*
	** Now compact it.
	*/
	result = sce_compactMemoryMap(evc, passNo, &memMap);
    }

    /*
    ** Done
    */
    return result;
}
