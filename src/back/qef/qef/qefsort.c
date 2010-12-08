/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qefkon.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <scf.h>
#include    <qefcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <ex.h>
#include    <tm.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefscb.h>
#include    <qefdsh.h>
#include    <qefqp.h>
#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <sxf.h>
#include    <qefprotos.h>


typedef struct _QEN_SARRAY
{
    i4		level;		/* level of this block -- level 0 points
				** to tuples */
    i4		ptr_count;	/* number of pointers in this block   
				** also index of upper boundary of array */
    i4		ptr_max;	/* number of pointers which can be 
				** accomodated in this block */
    i4		insert_point;	/* last inserted index */
    struct _QEN_SARRAY	*ptr[1];	/* first of n pointers */
} QEN_SARRAY;

static DB_STATUS
qes_heapalloc(
	QEE_DSH		*dsh,
	QEN_SHD		*shd);

static void
qes_tidydup(
	QEN_SHD		*shd,
	i4		stop,
	i4		current);

static i4  compcount;

/**
**
**  Name: QEFSORT.C		- QEF in-memory sorter
**
**  Description:
**	The following assumptions must be true for QEF in-memory sort routine
**	to work:
**	Assumption 1: The order of elements in the attribute descriptor array
**		      is the same as the order of attributes in the tuple.
**	Assumption 2: The attributes of the tuple (in the tuple buffer) are
**		      continguous.  That is the next one begins immediately
**		      at the end of the previous one. 
**	Assumption 3: Each key must be an attribute of the tuple.
**	Assumption 4: The order of the elements in the key array is the same
**		      as the order of the sort key.
**
**	Used in both sorts
**	    qes_init	 - Initialize the sort key descriptors.
**	Used in insert sort
**	    qes_insert   - Insert sort a tuple
**	    qes_endsort  - End of sort, set up for retrievals
**	    qes_dumploop - Dump the tuples in the sort buffer into DMF sorter.
**	Used in heap sort
**	    qes_putheap  - Put a tuple into buffer and heapify (for tsort).
**	    qes_put	 - Put a tuple into sort buffer (for sort).
**          qes_getheap	 - Retrieve the next tuple in the sort order from 
**			   the buffer (for tsort).
**          qes_sorter	 - Perform heapsort for tuples in the sort buffer
**			   (for sort).
**	    qes_dump	 - Dump the tuples in the sort buffer into DMF sorter.
**
**
**  History:    $Log-for RCS$
**      27-sep-88 (puree)
**          Written for small sort
**	07-jan-91 (davebf)
**	    New routines to support insert sort
**	24-jan-91 (davebf)
**	    More sort tuning.
**	27-dec-91 (stevet)
**	    Fixed duplicate row problem and session limit problem in 
**	    qes_insert().
**	13-apr-91 (nancy)
**	    Fix bug 43242 where very large est_tuple size causes overflow
**	    in calculating in-memory sort requirements.  Causes AV.
**	28-oct-92 (rickh)
**	    Replaced references to ADT_CMP_LIST with DB_CMP_LIST.
**	08-dec-92 (anitap)
**	    Added #include <psfparse.h> and <qsf.h>. 
**	21-dec-92 (anitap)
**	    Prototyped qes_init(), qes_skbuild(), qes_insert(), qes_endsort(),
**	    qes_dumploop(), qes_put(), qes_get(), qes_sorter() and qes_dump(). 
**      18-jan-1992 (stevet)
**          Call adt_sortcmp with the correct number of arguments.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	9-may-94 (robf)
**          Add error checks from adt_sortcmp, these should not be ignored
**	    especially if caused by security label comparison problems.
**	19-may-95 (ramra01)
**	    Improvements to the heap sort and hooks to utilize them from
**	    QEN
**	10-jul-97 (hayke02)
**	    In the function qes_dump(), we now decrement dsh->dsh_shd_cnt
**	    to show that this node is no longer using an in-memory sort.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	24-nov-97 (inkdo01)
**	    Major changes to heap sort to (1) allocate 100 buffers at a time
**	    (to reduce ULM calls), and (2) heapify on input for top sorts (to
**	    improve performance and allow partial detection of dups on way in).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-dec-03 (inkdo01)
**	    Numerous functions now pass "dsh" as a parameter.
**	23-feb-04 (inkdo01)
**	    Change qef_rcb->error to dsh->dsh_error for || thread safety.
**	6-Dec-2005 (kschendel)
**	    Kill off skbuild, opc will build compare lists now.
**	    Remove incorrect u_i2 casts.
**	14-Dec-2005 (kschendel)
**	    Use new ADF CB pointer in dsh.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning / prototype fixes.
*/
/*

**{
** Name: qes_init	- Initialize QEF in-memory sort for a node.
**
** Description:
**
** Inputs:
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	dsh				Ptr to the current DSH.
**	node				Ptr to the QEN_NODE
**	rowno				index to DSH_ROW for tuple output
**	dups				duplicate handling, DMR_NODUPLICATES
**					is duplictes is are to be removed, 
**					0 otherwise.
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0201_BAD_SORT_KEY
**					E_QE000D_NO_MEMORY_LEFT
**					E_QE1006_BAD_SORT_DESCRIPTOR
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    dsh_qp_status in the DSH may be marked invalid.
**
**
** History:
**	21-dec-92 (anitap)
**	    Prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      8-aug-93 (shailaja)
**          Unnested comments.
*/
DB_STATUS
qes_init(
	QEE_DSH		*dsh,
	QEN_SHD		*shd,
	QEN_NODE	*node,
	i4		rowno, 
	i4		dups)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    DB_STATUS		status;
    ULM_RCB		ulm_rcb;
			
    dsh->dsh_error.err_code = E_QE0000_OK;
    for (;;)    /* to break off in case of error */
    {
#ifdef xDEBUG
	/* Consistency check to avoid lost memory */
	if (shd->shd_streamid != (PTR)NULL || 
	    shd->shd_mdata != (DM_MDATA *) NULL ||
	    shd->shd_vector != (PTR *) NULL)
	{
	    dsh->dsh_error.err_code = E_QE1006_BAD_SORT_DESCRIPTOR;
	    status = E_DB_ERROR;
	    break;
	}
#endif		

	/* Initialize parts of the SHD */
	shd->shd_node = node;
	shd->shd_vector = (PTR *) NULL;
	shd->shd_mdata = (DM_MDATA *) NULL;
	shd->shd_free = (DM_MDATA *) NULL;
	shd->shd_tup_cnt = 0;
	shd->shd_next_tup = 0;
	shd->shd_row = dsh->dsh_row[rowno];
	shd->shd_width = dsh->dsh_qp_ptr->qp_row_len[rowno];
	shd->shd_dups = dups;

	/* Open memory stream for the sort buffer */
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	/* Point ULM to where we want streamid returned */
	ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	status = ulm_openstream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    else
	    	dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	    break;
	}

	break;
    }
    return (status);
}


/*{
** Name: qes_insert	    -insert tuple in sorted order using binary search
**
** Description:
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	sortkey			        Ptr to array of DB_CMP_LISTs that
**					describe the sort keys.
**	kcount				Count of sort keys.
**
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**
**
** History:
**	07-jan-91 (davebf)
**	    written.
**	24-jan-91 (davebf)
**	    More sort tuning.
**	27-dec-91 (stevet)
**	    Removed GOTO.  Increment shd->shd_size and qef_cb->qef_s_used
**	    so that checks for session limit would work correctly.  Also
**	    moved codes so that pointer array will be rearranged ONLY 
**	    when the session limits has not be exceeded, this fixed 
**	    the problem where duplicate rows will return when session limit
**	    exceeded.
**	13-apr-91 (nancy)
**	    Fix bug 43242 where very large est_tuple causes overflow while
**	    calculating in-memory sort requirements.  Results in small number
**	    for memory size and av's on first sort comparison.  Calculate
**	    largest size based on MAXI4 as threshold for in-memory sort. Return
**	    error if larger (will go straight to dmf for sorting).
**	21-dec-92 (anitap)
**	    Prototyped.
**      25-Oct-1993 (fred)
**          Correct duplicate handling for large (and other
**          non-sortable) objects.  For these objects, the last
**          parameter of adt_sortcmp() is the value to return if the
**          tuples are equal except for unsortable objects (e.g. large
**          objects).  In these cases, we consider the objects equal
**          for sorting purposes, but not for duplicate elimination.
**          Thus, if we are eliminating duplicates, then as check (via
**          adt_sortcmp()) with some difference value (last parm)
**          other than 0.
*/
DB_STATUS
qes_insert(   
	QEE_DSH		*dsh,
	QEN_SHD		*shd,
	DB_CMP_LIST	*sortkey,
	i4		kcount)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    ADF_CB		*adf_cb = dsh->dsh_adf_cb;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    ULM_RCB		ulm_rcb;
    QEN_SARRAY		*sarray; 
    QEN_SARRAY		**parray; 
    QEN_NODE		*node = shd->shd_node;
    i4			high_bound;
    i4			low_bound;
    i4			compare_point;
    i4			insert_point;
    i4			i, j, k, l, result;
    i4			max_array_size;
    i4			count = 0;

    dsh->dsh_error.err_code = E_QE0000_OK;


    /* we're assuming for now that there is only one block of pointers */
    sarray = (QEN_SARRAY *) shd->shd_vector;

    if(sarray == (QEN_SARRAY *)NULL)
    { 
	/* must allocate QEN_SARRAY */

	/* number of pointers in block is 
	** max(100, qen_est_tuples, current session capacity / num sort-holds)*/

	i = 100;			/* guarantee we try 100 */

	j = node->qen_est_tuples;
	    
	if(i < j && shd->shd_dups != DMR_NODUPLICATES)
	    i = j;	/* take est as max only if not removing duplicates */

	if (dsh)
	{
	    j = dsh->dsh_shd_cnt;	/* count of potential sort/holds */
	    if(j < 1)
		j = 1;			/* failsafe */
	    k = qef_cb->qef_s_max / j;		/* sort capacity in bytes */
	    l = shd->shd_width + sizeof(PTR);	/* bytes per tuple */
	    j = k / l;				/* sort capacity in tuples */
	    if(i < j)
		i = j;
	}

        max_array_size = (MAXI4 / (sizeof(QEN_SARRAY *))) - (sizeof(QEN_SARRAY) +
                         sizeof(QEN_SARRAY *));
	if ( i > max_array_size)
	{
	    dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    return(E_DB_ERROR);
	}

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	ulm_rcb.ulm_psize = (sizeof(QEN_SARRAY) - sizeof(QEN_SARRAY *)) +
			    (i * sizeof(QEN_SARRAY *));

	if((qef_cb->qef_s_used + ulm_rcb.ulm_psize) > qef_cb->qef_s_max)
	{
	    dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    return(E_DB_ERROR);
	}

	status = ulm_palloc(&ulm_rcb);
	if(status != E_DB_OK)
	{
	    if(ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    else
		dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	    return(E_DB_ERROR);
	}
	shd->shd_vector = (PTR *) ulm_rcb.ulm_pptr;
	sarray = (QEN_SARRAY *) shd->shd_vector;
	shd->shd_size += ulm_rcb.ulm_psize;
	qef_cb->qef_s_used += ulm_rcb.ulm_psize;
	sarray->ptr_max = i;
	sarray->ptr_count = 0;
	parray = sarray->ptr;
	insert_point = 0;		/* first one */
	sarray->level = 0;	/*????temporary use  */
    }    
    else
    {	
        if(sarray->ptr_count >= sarray->ptr_max)       /* if we're full */
	{
	    /* force spill for now.  Later we may split */
	    dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    return(E_DB_ERROR);
	}  

	/* set initial search values */
	parray = sarray->ptr;
	low_bound = 0;
	high_bound = sarray->ptr_count;	    /* boundary */
	insert_point = high_bound;

	for(;;) /* search loop   ??? limited ??? */
	{
	    compare_point = high_bound - low_bound;
	    if(compare_point == 1)
		compare_point = low_bound;
	    else if(compare_point <= 0)
		break;			        /* use insert_point */
	    else
	    {
		if(count)				/* not first iteration */
		{
		    i = compare_point % 2;
		    compare_point /= 2;		/* rounded down */
		    if(i)
			++compare_point;		/* rounded up */
		}
		else
		    /* start with last insert point */
		    compare_point = sarray->insert_point;
		compare_point += low_bound;
	    }

	    /*call adt_sortcmp    for shd_row and *parray[compare_point] */

	    adf_cb->adf_errcb.ad_errcode=0;
	    result = adt_sortcmp(adf_cb, sortkey, kcount,
			    shd->shd_row,  
			    (PTR)parray[compare_point], 0);
	    if(adf_cb->adf_errcb.ad_errcode!=0)
	    {
	        dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		return(E_DB_ERROR);
	    }
	    ++count;

	    if(result > 0)
	    {
		/* row > *parray[compare_point] */
		low_bound = compare_point + 1;
	    }
	    else if(result < 0)
	    {
		/* row < *parray[compare_point] */
		high_bound = compare_point;
		insert_point = compare_point;
	    }
	    else /* row == *parray[compare_point] */
	    {
		/* 
		** If we have a case where the rows are equal AND we
		** are dropping duplicates, then we must determine if
		** they are really equal.  To do this, we compare
		** again, asking for a different result if the only
		** differences are in columns which are inherently
		** unsortable.  If these are the only differences
		** (i.e. on this compare, we don't get zero back),
		** then we consider the tuples different since they
		** are, by definition, uncomparable in a sort.
		*/

		if(shd->shd_dups == DMR_NODUPLICATES)	/* dupes dropped */
		{
	    	    adf_cb->adf_errcb.ad_errcode=0;
		    result = adt_sortcmp(adf_cb,
					 sortkey,
					 kcount,
					 shd->shd_row,  
					 (PTR)parray[compare_point],
					 1);
	    	    if(adf_cb->adf_errcb.ad_errcode!=0)
	    	    {
			dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
			return(E_DB_ERROR);
	    	    }
		    if (result == 0)
			return(E_DB_OK);
		}

		insert_point = compare_point;
		break;				/* to insert duplicate */
	    }

	}  /* end of search loop */

    }  /* end of if stmt */


    /* Allocate memory for the tuple */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
    ulm_rcb.ulm_streamid_p = &shd->shd_streamid;

    ulm_rcb.ulm_psize = shd->shd_width;
    if((qef_cb->qef_s_used + ulm_rcb.ulm_psize) > qef_cb->qef_s_max)
    {
	dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	return(E_DB_ERROR);
    }

    status = ulm_palloc(&ulm_rcb);
    if(status != E_DB_OK)
    {
	if(ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	    dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	else
	    dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	return(E_DB_ERROR);
    }

    /* make room for new pointer at this insert_point */
    j = sarray->ptr_count;	    /* boundary */
    i = j - 1;			    /* last slot */

    while(i >= insert_point)
	parray[j--] = parray[i--];

    parray[insert_point] = (QEN_SARRAY *)ulm_rcb.ulm_pptr;

    /* Copy tuple into the sort buffer */
    MEcopy((PTR)shd->shd_row, shd->shd_width, (PTR)parray[insert_point]);
    ++shd->shd_tup_cnt;
    ++sarray->ptr_count;
    sarray->insert_point = insert_point;    /* save this insert_point */
    shd->shd_size += ulm_rcb.ulm_psize;
    qef_cb->qef_s_used += ulm_rcb.ulm_psize;

    /* following is temporary code */
    sarray->level += count;
    /* preceeding is temporary code */

    return(status);
}

/*{
** Name: qes_endsort	Set up for returning sorted tuples
**
** Description:
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:

**
**
** History:
**	07-jan-91 (davebf)
**	    Written.
**	21-dec-92 (anitap)
**	    Prototyped.
[@history_template@]...
*/
DB_STATUS
qes_endsort(
	QEE_DSH		*dsh,
	QEN_SHD		*shd)
{
    QEN_SARRAY	*sarray;

    dsh->dsh_error.err_code = E_QE0000_OK;
    /* point vector to array proper */
    sarray = (QEN_SARRAY *) shd->shd_vector;
    if(sarray)
	shd->shd_vector = (PTR *) sarray->ptr;

    return(E_DB_OK);
}

/*{
** Name: qes_dumploop	- dump the in-memory sort buffer into DMF sorter
**
** Description:
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	dmr				Ptr to DMR_CB block for the DMF sorter.
** Outputs:
**
** Side Effects:
**
** History:
**	07-jan-91 (davebf)
**	    Adapted from qes_dump.
**	24-jan-91 (davebf)
**	    More sort tuning.
**	21-dec-92 (anitap)
**	    Prototyped.
**      16-oct-98 (stial01)
**          qes_dumploop() set DMR_HEAPSORT for DMR_LOAD call
*/

DB_STATUS
qes_dumploop(
	QEE_DSH		 *dsh,
	QEN_SHD		 *shd,
	DMR_CB		 *dmr)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DM_MDATA		dump_mdata;       
    DM_MDATA		*saved_ptr;        
    QEN_SARRAY		*sarray = (QEN_SARRAY *)shd->shd_vector;
    QEN_SARRAY		**parray;
    DB_STATUS		status = E_DB_OK;
    i4			count;
    i4			i = 0;

    dsh->dsh_error.err_code = E_QE0000_OK;

    if(sarray != (QEN_SARRAY *)NULL)
    {
	/* Dump data to DMF sorter */
	dump_mdata.data_next = 0;
	dump_mdata.data_size = shd->shd_width;
	saved_ptr = dmr->dmr_mdata;
	dmr->dmr_count = 1;
	dmr->dmr_mdata = &dump_mdata;
	parray = sarray->ptr;

	for(count = sarray->ptr_count; count; --count)
	{
	    dump_mdata.data_address = (char *) parray[i++];
	    dmr->dmr_flags_mask |= DMR_HEAPSORT;
	    status = dmf_call(DMR_LOAD, dmr);
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = dmr->error.err_code;
		return(status);
	    }
	}	/* end of loop */

	/* get back indirect ptr to row that caused spill */
	dmr->dmr_mdata = saved_ptr;		

    }

    /* show that this node is no longer using in-memory sort */
    if(dsh)
	--dsh->dsh_shd_cnt;	

    /* Release sort buffer if exists. */
    if (shd->shd_streamid != (PTR)NULL)
    {
	ULM_RCB	    ulm_rcb;

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	    /* fall through even if error to clean up the SHD */
	}
	/* ULM has nullified shd_streamid */
	qef_cb->qef_s_used -= shd->shd_size;
    }

    /*  Clean up sort buffer descriptor */
    shd->shd_size = 0;
    shd->shd_vector = (PTR *) NULL;
    shd->shd_mdata = (DM_MDATA *)NULL;
    shd->shd_tup_cnt = 0;
    shd->shd_next_tup = 0;

    return (status);
}

/*{
** Name: qes_heapalloc	- Allocate/initialize SHD pointer vector. 
**
** Description:	If no pointer vector yet, allocates/inits. Also allocates
**		chunk of 100 row buffers at a time (used to allocate one 
**		at a time) and adds to free chain (reduces ULM calls).
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**
**
**
** History:
**	24-nov-97 (inkdo01)
**	    Cloned from original qes_put to separate init from put. Also
**	    incorporates change to alloc. 100 buffers at a time (to reduce
**	    ULM calls).
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
static DB_STATUS
qes_heapalloc(
	QEE_DSH		*dsh,
	QEN_SHD		*shd)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DM_MDATA		*free;
    DB_STATUS		status = E_DB_OK;
    ULM_RCB		ulm_rcb;
    QEN_NODE            *node = shd->shd_node;
    i4		buffalign;
    i4                  i, j, k, l;
    i4                  max_array_size;
   

    dsh->dsh_error.err_code = E_QE0000_OK;
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
    ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
    buffalign = DB_ALIGN_MACRO(shd->shd_width);

    if ( shd->shd_vector == (PTR *)NULL)
    {
        /* First call - must also allocate pointer array */
 
        /* number of pointers in block is
        ** max(100, qen_est_tuples, current session capacity / num sort-holds)*/
 
        i = 100;                        /* guarantee we try 100 */
 
        j = node->qen_est_tuples;
 
        if(i < j && shd->shd_dups != DMR_NODUPLICATES)
            i = j;      /* take est as max only if not removing duplicates */
 
        if (dsh)
        {
            j = dsh->dsh_shd_cnt;       /* count of potential sort/holds */
            if(j < 1)
                j = 1;                  /* failsafe */
            k = qef_cb->qef_s_max / j;          /* sort capacity in bytes */
            l = shd->shd_width + sizeof(PTR);   /* bytes per tuple */
            j = k / l;                          /* sort capacity in tuples */
            if(i < j)
                i = j;
        }
 
        max_array_size = MAXI4 / sizeof(PTR);
 
        if ( i > max_array_size)
        {
            dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
            return(E_DB_ERROR);
        }

        ulm_rcb.ulm_psize = (i * sizeof(PTR)) +
		(100 * (sizeof(DM_MDATA) + buffalign));
				/* ptr vector + first 100 buffers */
 
        if((qef_cb->qef_s_used + ulm_rcb.ulm_psize) > qef_cb->qef_s_max)
        {
            dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
            return(E_DB_ERROR);
        }
 
        status = ulm_palloc(&ulm_rcb);
        if(status != E_DB_OK)
        {
            if(ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
                dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
            else
                dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
            return(E_DB_ERROR);
        }
        shd->shd_vector = (PTR *) ulm_rcb.ulm_pptr;
        shd->shd_size += ulm_rcb.ulm_psize;
        qef_cb->qef_s_used += ulm_rcb.ulm_psize;
	shd->shd_tup_max = i;

	/* Finally, init the free chain with 1st 100 buffers. */
	shd->shd_free = (DM_MDATA *) NULL;	/* init anchor, first */
	for (free = (DM_MDATA *)&shd->shd_vector[i], j = 0; j < 100; j++,
		free = (DM_MDATA *)((char *)free + (sizeof(DM_MDATA) + 
		buffalign)))
	{
	    free->data_size = shd->shd_width;
	    free->data_address = (char *)free + sizeof(DM_MDATA);
	    free->data_next = shd->shd_free;
	    shd->shd_free = free;
	}
	compcount = 0;
	return (E_DB_OK);
    }
    else		/* just allocate a bunch more buffers */
    {
	if (shd->shd_free == NULL)
	{		/* get another 100 buffers */
	    if ((i = shd->shd_tup_max-shd->shd_tup_cnt+1) > 100) i = 100;
	    ulm_rcb.ulm_psize = i * (buffalign + sizeof(DM_MDATA));
            if ((qef_cb->qef_s_used + ulm_rcb.ulm_psize) > qef_cb->qef_s_max)
            {
        	dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
        	return (E_DB_ERROR);
            }
 
            status = ulm_palloc(&ulm_rcb);
            if (status != E_DB_OK)
            {
        	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
                 dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
        	else
                 dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
        	return(E_DB_ERROR);
            }
	    free = (DM_MDATA *)ulm_rcb.ulm_pptr;
	
            qef_cb->qef_s_used += ulm_rcb.ulm_psize;
            shd->shd_size += ulm_rcb.ulm_psize;

	    /* Structure the added MDATA's into free chain. */
	    for (j = 0; j < i; j++, free = (DM_MDATA *)((char *)free + 
		(sizeof(DM_MDATA) + buffalign)))
	    {
		free->data_size = shd->shd_width;
		free->data_address = (char *)free + sizeof(DM_MDATA);
		free->data_next = shd->shd_free;
		shd->shd_free = free;
	    }
	}
    }
			
    return (status);
}


/*{
** Name: qes_put	- Put tuple in the row buffer into sort buffer
**
** Description:
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**
**
**
** History:
**	21-dec-92 (anitap)
**	    Prototyped.
**	24-nov-97 (inkdo01)
**	    Separated memory allocation logic (part of heap sort overhaul).
**	    qes_put is now only used for non-top sorts (inners of merge
**	    joins).
*/
DB_STATUS
qes_put(
	QEE_DSH		*dsh,
	QEN_SHD		*shd)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    DM_MDATA		*dt;
    DB_STATUS		status;
    PTR                 *p_ptr;

    dsh->dsh_error.err_code = E_QE0000_OK;
    status = E_DB_OK;
  
    if (shd->shd_free == NULL)
    {
	status = qes_heapalloc(dsh, shd);
	if (status != E_DB_OK) return(status);
    }

    if(shd->shd_tup_cnt >= shd->shd_tup_max)       /* if we're full */
    {
	/* force spill for now.  Later we may split */
	dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	return(E_DB_ERROR);
    }
 
    {	/* Get next free buffer and prepend it to the list in SHD */
	dt = shd->shd_free;
	shd->shd_free = dt->data_next;		/* reset free chain head */
        dt->data_next = shd->shd_mdata;
        shd->shd_mdata = dt;

        p_ptr = shd->shd_vector;
        p_ptr[shd->shd_tup_cnt] = dt->data_address;
                                               
        /* Prepend it to the list in SHD */
 
        /* Copy tuple into the sort buffer */
        MEcopy((PTR)shd->shd_row, dt->data_size, (PTR)dt->data_address);

        ++shd->shd_tup_cnt;
    }
			
    return (status);
}


/*{
** Name: qes_putheap	- Put tuple in the row buffer into sort buffer and
**			reheapify.
**
** Description:	Add entry to end of pointer array, and position new row on
**		heap branch which terminates there. Also does partial 
**		duplicates elimination, if necessary. Called only from 
**		top sort. 
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**
**
**
** History:
**	24-nov-97 (inkdo01)
**	    New logic to heapify the rows as they are added to the hold 
**	    structure.
*/
DB_STATUS
qes_putheap(
	QEE_DSH		*dsh, 
	QEN_SHD		*shd,
	DB_CMP_LIST	*sortkey,
	i4		kcount)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    ADF_CB		*adf_cb = dsh->dsh_adf_cb;
    DM_MDATA		*dt;
    DB_STATUS		status;
    PTR                 *p_ptr;
    i4			i, j, cmpres;

    dsh->dsh_error.err_code = E_QE0000_OK;
    adf_cb->adf_errcb.ad_errcode = 0;

    if (shd->shd_tup_cnt == 0) i = 1;		/* 1st row goes straight in */
			
    else
    {
	/* Adding row to existing heap */

	/* See if there is any room for next row. */
	if(shd->shd_tup_cnt >= shd->shd_tup_max)
	{
	    /* force spill for now.  Later we may split */
	    dsh->dsh_error.err_code = E_QE000D_NO_MEMORY_LEFT;
	    return(E_DB_ERROR);
	}

	/* Loop locates row in heap branch ending in next pointer array slot */
	p_ptr = shd->shd_vector;
	for (i = shd->shd_tup_cnt+1, j = i>>1; j > 0; i = j, j >>= 1)
	{
	    compcount++;
	    cmpres = adt_sortcmp(adf_cb, sortkey, kcount, 
		p_ptr[j-1], shd->shd_row, 1);	/* compare current with new */
	    if (cmpres == 0 && shd->shd_dups == DMR_NODUPLICATES)
	    {	
		/* found a dup - just restore branch & drop new guy */
		if (i < (shd->shd_tup_cnt+1)>>1) 
			qes_tidydup(shd, i, shd->shd_tup_cnt+1);
		return(E_DB_OK);
	    }
	    if (cmpres < 0) break;			/* found the slot */

	    p_ptr[i-1] = p_ptr[j-1];			/* make room for new */
	}
    }	/* end of "adding a row" */

    /* We arrive here with i indexing the slot for the new row. */

    if (shd->shd_free == NULL)
    {
	status = qes_heapalloc(dsh, shd);
	if (status != E_DB_OK) return(status);
	p_ptr = shd->shd_vector;		/* make sure we've got p_ptr */
    }

    dt = shd->shd_free;				/* peel one off free chain */
    shd->shd_free = dt->data_next;
    dt->data_next = shd->shd_mdata;
    shd->shd_mdata = dt;
    p_ptr[i-1] = dt->data_address;

    /* Copy tuple into the sort buffer */
    MEcopy((PTR)shd->shd_row, dt->data_size, (PTR)dt->data_address);
    shd->shd_tup_cnt++;
    shd->shd_next_tup = shd->shd_tup_cnt;
    return(E_DB_OK);

}


/*{
** Name: qes_tidydup	- Restore current heap branch (we've just found a
**			duplicate)
**
** Description:	
**
** Inputs:
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	stop				Index into heap array at which we've
**					found our position.
**	current				Current position in heap array.
**
** Outputs:
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**
**
**
** History:
**	25-nov-97 (inkdo01)
**	    Written.
*/
static void
qes_tidydup(
	QEN_SHD		*shd,
	i4		stop,
	i4		current)
{
    i4		newcurr;
    PTR		*p_ptr;

    /* Simply recurse until correct array location is found, then
    ** restore pointers on way back out. */

    newcurr = current>>1;
    if (newcurr > stop) qes_tidydup(shd, stop, newcurr);
					/* the recursion */
    p_ptr = shd->shd_vector;
    p_ptr[newcurr-1] = p_ptr[current-1];
					/* and the restoration */
}


/*{
** Name: qes_getheap	- get the next tuple in the sort order from the buffer.
**
** Description:	Gets next row off top of heap (eliminating dups, if needed),
**		for top sort only.
**      
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0015_NO_MORE_ROWS
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-nov-97 (inkdo01)
**	    Written to support heapsort changes.
*/
DB_STATUS
qes_getheap(
	QEE_DSH		*dsh,
	QEN_SHD		*shd,
	DB_CMP_LIST	*sortkey,
	i4		kcount)
{
    ADF_CB		*adf_cb = dsh->dsh_adf_cb;
    DB_STATUS		status;
    PTR			*p_ptr;
    i4			i, j;
    bool		gotadup;
			
			
    /* Propagate next smallest row to top of heap and return it. */

    p_ptr = shd->shd_vector;		/* init local ptr vector ptr */
    adf_cb->adf_errcb.ad_errcode = 0;

    if (shd->shd_tup_cnt == shd->shd_next_tup); /* 1st row - just return [0] */

    else for ( ; ; )	/* outer loop discards duplicates (if necessary) */
    {
	/* Big for-loop sucks next smallest row to top of heap. */
	gotadup = FALSE;		/* init for possible dups check */
	for (i = 1, j = 2; j <= shd->shd_tup_cnt && shd->shd_next_tup > 0; )
	{
	    /* Pick smaller of j, j+1 */
	    if (j < shd->shd_tup_cnt && (p_ptr[j-1] == NULL ||
		(p_ptr[j] != NULL && (compcount++,
		 adt_sortcmp(adf_cb, sortkey,
			kcount, p_ptr[j-1], p_ptr[j], 0) > 0)))) j++;

	    if (adf_cb->adf_errcb.ad_errcode != 0)
	    {
		dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		return (E_DB_ERROR);
	    }
	    
	    if (shd->shd_dups == DMR_NODUPLICATES && i == 1 &&
		p_ptr[j-1] != NULL && (compcount++,
		adt_sortcmp(adf_cb, sortkey,
		kcount, p_ptr[0], p_ptr[j-1], 1) == 0))
	     gotadup = TRUE;		/* show the next row is just a dup */

	    /* Found a row to propagate (maybe). Move it up and replace it
	    ** with NULL (for now). */
	    if (p_ptr[j-1] == NULL) break;	/* quit, if end of branch */
	    p_ptr[i-1] = p_ptr[j-1];
	    p_ptr[j-1] = NULL;
	    i = j;
	    j += j;
	}	/* end of propagation loop */

	/* If dups not allowed, check 'em here and possibly stay in loop. */
	if (gotadup) shd->shd_next_tup--;
	else break;				/* no dup, exit with row */
    }	/* end of "not 1st row" */

    if (shd->shd_next_tup-- > 0)
    {
	MEcopy(p_ptr[0], shd->shd_width, (PTR)shd->shd_row);
	status = E_DB_OK;
    }
    else
    {
	/* end-of-file */
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	status = E_DB_ERROR;
    }
    return (status);
}


/*{
** Name: qes_dump	- dump the in-memory sort buffer into DMF sorter
**
** Description:
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	dmr				Ptr to DMR_CB block for the DMF sorter.
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	21-dec-92 (anitap)
**	    Prototyped.
**	10-jul-97 (hayke02)
**	    We now decrement dsh->dsh_shd_cnt (potential # of in-memory
**	    sort/holds) to show that this node is no longer using an in-memory
**	    sort. This makes more memory available for subsequent in-memory
**	    sorts (see sort capacity calculation in qes_put()).
**      16-oct-98 (stial01)
**          qes_dump() set DMR_HEAPSORT for DMR_LOAD call
**	16-Jul-2004 (jenjo02)
**	    Fix another missed qef_dsh reference.
*/
DB_STATUS
qes_dump(
	QEE_DSH		 *dsh,
	QEN_SHD		 *shd,
	DMR_CB		 *dmr)
{
    GLOBALREF QEF_S_CB  *Qef_s_cb;
    QEF_CB              *qef_cb = dsh->dsh_qefcb;
    DM_MDATA            *saved_ptr;
    DB_STATUS           status = E_DB_OK;
 
 
    dsh->dsh_error.err_code = E_QE0000_OK;

    /* show that this node is no longer using in-memory sort */
    --dsh->dsh_shd_cnt;

    for (;;)        /* to break off in case of error */
    {
 
        /* Dump data to DMF sorter */
        saved_ptr = dmr->dmr_mdata;
        dmr->dmr_count = shd->shd_tup_cnt;
        dmr->dmr_mdata = shd->shd_mdata;
	dmr->dmr_flags_mask |= DMR_HEAPSORT;
        status = dmf_call(DMR_LOAD, dmr);
        if (status != E_DB_OK)
        {
            dsh->dsh_error.err_code = dmr->error.err_code;
            break;
        }
        dmr->dmr_count = 1;
        dmr->dmr_mdata = saved_ptr;
 
        /* Release sort buffer if exists. */
        if (shd->shd_streamid != (PTR)NULL)
        {
            ULM_RCB         ulm_rcb;
 
            STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
            ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
            status = ulm_closestream(&ulm_rcb);
            if (status != E_DB_OK)
            {
                dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
                /* fall through even if error to clean up the SHD */
            }
	    /* ULM has nullified shd_streamid */
        }
 
        /*  Clean up sort buffer descriptor */
        qef_cb->qef_s_used -= shd->shd_size;
        shd->shd_size = 0;
        shd->shd_vector = (PTR *)NULL;
        shd->shd_mdata = (DM_MDATA *)NULL;
        shd->shd_tup_cnt = 0;
        shd->shd_next_tup = 0;
        break;
    }
    return (status);
}


/*
** {
** Name: qes_sorter	- sort tuples in QEF sort buffer
**
** Description:
**	The sort algorithm used is a heapsort adopted from DMSE.C
**	It has been modified so that the array index starts from 0
**	instead of 1.  Duplicates removal is added to the end of
**	the sort. This is now only used by non-top sorts (inners of
**	merge joins).
**
** Inputs:
**      dsh				Ptr to thread data segment header.
**      shd                             Ptr to QEF sort descriptor (QEN_SHD).
**	sortkey			        Ptr to array of DB_CMP_LISTs that
**					describe the sort keys.
**	kcount				Count of sort keys.
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE1007_BAD_SORT_COUNT
**					E_QE000D_NO_MEMORY_LEFT
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-dec-92 (anitap)
**	    Prototyped.
**      25-Oct-1993 (fred)
**          Correct duplicate handling for large (and other
**          non-sortable) objects.  For these objects, the last
**          parameter of adt_sortcmp() is the value to return if the
**          tuples are equal except for unsortable objects (e.g. large
**          objects).  In these cases, we consider the objects equal
**          for sorting purposes, but not for duplicate elimination.
**          Thus, if we are eliminating duplicates, then as check (via
**          adt_sortcmp()) with some difference value (last parm)
**          other than 0.
*/
DB_STATUS
qes_sorter(
	QEE_DSH		   *dsh,
	QEN_SHD		   *shd,
	DB_CMP_LIST	   *sortkey,
	i4		   kcount)
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    ADF_CB		*adf_cb = dsh->dsh_adf_cb;
    PTR			*vector;
    PTR			tmpptr;
    i4			heapsize = shd->shd_tup_cnt;
    i4			last = heapsize - 1;
    i4			current_node, parent, child, i, j;
    i4			compare_count = 0;   /* temp for comparing sorts */
			/* FIXME remove all references to compare_count */

			
#ifdef xDEBUG
    if (heapsize == 0)
    {
	dsh->dsh_error.err_code = E_QE1007_BAD_SORT_COUNT;
	return (E_DB_ERROR);
    }
#endif
	
    /* The main sort loop:
    ** Build the heap.  Start from the right-most, lowest-level non-leaf
    ** node.  Work to the left, and up to the root (node 0).
    ** 
    ** Note:  The array index starts from 0. Thus for any node,
    **	      the left child = 2*i + 1, and the right node = 2*i + 2.
    */
    vector = shd->shd_vector;
    if (!vector)
	return (E_DB_OK);

    for (current_node = heapsize / 2; --current_node >= 0; )
    {
	parent = current_node;
	child = parent + parent + 1;/* the left child */
	tmpptr = vector[parent];    /* save the value of current node */

	adf_cb->adf_errcb.ad_errcode=0;
	while (child < heapsize)    /* prevent out of array bound */
	{
	    if (child < last)
	    {
		/* select the bigger of the two children */
		++compare_count;
		if (adt_sortcmp(adf_cb, sortkey, kcount,
			    vector[child], vector[child+1], 0) < 0)
		{
		    ++child;
		}
	        if(adf_cb->adf_errcb.ad_errcode!=0)
	        {
	            dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		    return(E_DB_ERROR);
	        }
	    }

	    /* replace parent with the bigger child value */
	    vector[parent] = vector[child]; 

	    /* go down the tree */
	    parent = child;		/* new parent */
	    child = parent + parent + 1; /* new (left) child */
	}

        adf_cb->adf_errcb.ad_errcode=0;
	for (;;)
	{
	    child = parent;
	    parent = (child - 1) / 2;
	    if(child != current_node) ++compare_count;
	    if (child == current_node ||
		adt_sortcmp(adf_cb, sortkey, kcount, 
			tmpptr,  vector[parent], 0) <= 0)
	    {
		vector[child] = tmpptr;
		break;
	    }
	    if(adf_cb->adf_errcb.ad_errcode!=0)
	    {
	        dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		return(E_DB_ERROR);
	    }
	    vector[child] = vector[parent];
	}
    }

    /* Now take the top of the heap, which contains the highest value, to
    ** the nth element of the array.  The heap now reduce its size by one.
    ** Readjust the heap to have the highest value at the top again.
    ** Repeat until no more elements left in the heap.
    */
    while (--heapsize > 0)
    {
	last = heapsize - 1;
	tmpptr = vector[0];	    /* top of the heap is highest value */
	vector[0] = vector[heapsize];
	vector[heapsize] = tmpptr;  /* to the last element of the current
				    ** array */
	parent = 0;		    /* always start from the top */
	child = 1;
	tmpptr = vector[parent];
        adf_cb->adf_errcb.ad_errcode=0;
	while (child < heapsize)
	{
	    if (child < last)	    /* prevent out of array bound */
	    {
		/* select the bigger of the two children */
		++compare_count;
		if (adt_sortcmp(adf_cb, sortkey, kcount,
			vector[child], vector[child+1], 0) < 0)
		{
	    	    child++;
		}
	        if(adf_cb->adf_errcb.ad_errcode!=0)
	        {
	            dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		    return(E_DB_ERROR);
	        }
	    }
	    
	    /* replace the parent with the bigger child */
	    vector[parent] = vector[child];

	    /* go down the tree */
	    parent = child;
	    child = parent + parent + 1;    /* left child */
	}

        adf_cb->adf_errcb.ad_errcode=0;
	for (;;)
	{
	    child = parent;
	    parent = (child - 1) / 2;
	    if(child != 0) ++compare_count; 
	    if (child == 0 || 
		adt_sortcmp(adf_cb, sortkey, kcount,
			tmpptr, vector[parent], 0) <= 0)
	    {
		vector[child] = tmpptr;
		break;
	    }
	    if(adf_cb->adf_errcb.ad_errcode!=0)
	    {
	        dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		return(E_DB_ERROR);
	    }
	    vector[child] = vector[parent];
	}
    }

    /* remove duplicates if necessary */
    if (shd->shd_dups == DMR_NODUPLICATES)
    {
        adf_cb->adf_errcb.ad_errcode=0;
	heapsize = shd->shd_tup_cnt;
	for (i = 0, j = 1; j < heapsize; i++)
	{
	    /* 
	    ** When checking for duplicates, we must use a difference
	    ** value other than 0.  This last parameter to
	    ** adt_sortcmp() is the value returned by the routine if
	    ** the difference is only due to DB_DIF_TYPE's -- that is,
	    ** the difference is only due to columns whose type is not
	    ** sortable.  We do not consider these columns duplicates.
	    */
	    
	    while (j < heapsize &&
		   ++compare_count > 0 &&
		   adt_sortcmp(adf_cb, sortkey, kcount,
			       vector[i], vector[j], 1) == 0)
	    {
		++j;
	    }
	    if(adf_cb->adf_errcb.ad_errcode!=0)
	    {
	        dsh->dsh_error.err_code = adf_cb->adf_errcb.ad_errcode;
		return(E_DB_ERROR);
	    }
	    if (j == heapsize)
		break;

	    vector[i+1] = vector[j];
	    ++j;
	}
	shd->shd_tup_cnt = i + 1;
    }
    return (E_DB_OK);
}
