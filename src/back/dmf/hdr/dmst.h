/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMST.H - Sort Threads
**
** Description:
**      This file describes the sort thread pool structures.
**
** History:
**      18-feb-98 (inkdo01)
**	    Written (for the parallel sort threads project).
**	13-apr-98 (inkdo01)
**	    Removed DM_STHCB because of switch to factotum threads
**	    from thread pool.
**	9-jul-98 (inkdo01)
**	    Add flag STE_CEND (as distinct from STE_CEOF) for dmse_end
**	    coordination.
**	14-dec-98 (inkdo01)
**	    Change DMST_ROWS to 10000 (default exchange buff. capacity).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization.
**	14-Apr-2005 (jenjo02)
**	    Added "cmp_count" param to dmse_begin_serial
**	    prototype.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dmse_? functions converted to DB_ERROR *
**/

FUNC_EXTERN DB_STATUS dmse_child_thread(SCF_FTX	*ftxcb);
 
#define DMST_STROWS	10000
#define DMST_STNTHREADS 2
#define DMST_BSIZE	5000

#define SEM_EXCL	1

/*}
** Name: DM_STECB - The sort threads detail descriptor block.
**
** Description:
**      Contains the sort threads pool entry description.
**
** History:
**	18-feb-98 (inkdo01)
**	    Written.
**	20-jul-98 (inkdo01)
**	    Change work location from i4 to *i4 (it's definitely 
**	    bigger than i4!).
**	6-jan-98 (inkdo01)
**	    Removed obsolete STE_CAVAIL flag.
**	04-Apr-2003 (jenjo02)
**	    Moved most STE_P? flags and STE_CERR flags to 
**	    (new) srt_pstate in DMS_SRT_CB.
**	    Split STE_CEOF into STE_CEOP (end of puts) and
**	    STE_CEOG (end of gets) for clearer barrier
**	    sync with parent.
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization.
**	28-Jan-2006 (kschendel)
**	    Remove EMPTY flag, same as c_puts.  Consolidate p- and c-flags.
**	    Rename PWAIT flag to PRSTRT, as its real function is to tell
**	    the child that the parent has really set a restart wakeup
**	    point, not that the parent is actually waiting.
**	06-Aug-2009 (thaju02) (B121945)
**	    Change ste_pbix, ste_cbix, ste_prstrt, ste_crstrt, 
**	    ste_c_puts, ste_c_gets from i4 to i8.
*/
typedef struct _DM_STECB
{
    DMS_SRT_CB	    *ste_pcb;		/* parent thread sort cb ptr */
    DMS_SRT_CB	    *ste_ccb;		/* child thread sort cb ptr */
    CS_SID	    ste_ctid;		/* child thread ID */
    DMP_MISC	    *ste_misc;		/* anchor of exchange buffer struct */
    char	    **ste_xbufp;	/* exchange buffer array ptr */
    i8	    	    ste_pbix;		/* parent index to exch buffer array */
    i8	    	    ste_cbix;		/* child index to exch buffer array */
    i8	    	    ste_prstrt;		/* ex array index to restart parent */
    i8	    	    ste_crstrt;		/* ex array index to restart child */
    i4	    	    ste_bsize;		/* # elements in exch buffer array */
    i4	    	    ste_records;	/* est # recs per thread */
    i4	    	    ste_npwaits;	/* # parent suspends */
    i4	    	    ste_ncwaits;	/* # child suspends */
    i8	    	    ste_c_puts;		/* rows "put" to thread */
    i8	    	    ste_c_gets;		/* rows "gotten" from thread */
    i4	    	    *ste_wrkloc;	/* ptr to work locs avail to thread */
    i4	    	    ste_npres;		/* # parent resumes */
    i4	    	    ste_ncres;		/* # child resumes */

    /* The asymmetry between parent and child in the flags is because
    ** there is one parent but many children.  If a child catches a
    ** parent, it sets crstrt and waits, no ambiguity.  If however the
    ** parent catches up to a child, it might set prstrt but then
    ** discover some other child that it can continue with.
    */

    volatile i4	    ste_cflag;		/* child thread flag */
#define STE_PRSTRT	0x01		/* Parent has set restart point for
					** child, and may be waiting for it.
					** (Without this flag set, prstrt may
					** be stale and should be ignored.)
					*/
#define STE_CWAIT	0x02		/* child waiting on exch buffer load */
#define STE_CEOG	0x04		/* End of "gets" in child */
#define STE_CEND	0x08		/* child sort "end"ed */
#define STE_CTERM	0x80		/* child sort terminated */

    float	    ste_wait;		/* child thread elapsed wait time */
    float	    ste_exec;		/* child thread elapsed exec time */
    float	    ste_twait;		/* child thread total elapsed wait time */
    float	    ste_texec;		/* child thread total elapsed exec time */
    i4	    	    ste_secs;		/* last TMnow values */
    i4	    	    ste_msecs;
} DM_STECB;

/*}
** Name: DMS_PMERGE_DESC - parallel sort merge heap.
**
** Description:
**      Describes heap entries of the final merge of a parallel sort.
**
** History:
**	26-feb-98 (inkdo01
**	    Written.
*/
typedef struct _DMS_PMERGE_DESC
{
    char	    *record;		/* row ptr */
    DM_STECB	    *step;		/* thread CB ptr */
} DMS_PMERGE_DESC;


/*
** Serial DMSE external function references called
** from Children threads.
*/

FUNC_EXTERN DB_STATUS dmse_begin_serial( 
			    i4		flags,
			    DB_TAB_ID	*table_id,
			    DB_CMP_LIST		*att_list,
			    i4		att_count,
			    i4		key_count,
			    i4		cmp_count,
			    DMP_LOC_ENTRY	*loc_array,
			    i4		*wrkloc_mask,
			    i4		mask_size,
			    i4		*spill_index,
			    i4		width,
			    i4		records,
			    i4		*uiptr,
			    i4		lock_list,
			    i4		log_id,
			    DMS_SRT_CB		**sort_cb,
			    PTR			collation,
			    PTR			ucollation,
			    DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmse_end_serial(DMS_SRT_CB *sort_cb, 
		   	    DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_get_record_serial(DMS_SRT_CB *sort_cb,
			    char **record,
		   	    DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_input_end_serial(DMS_SRT_CB *sort_cb,
		   	    DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_put_record_serial(DMS_SRT_CB *sort_cb,
			    char *record,
		   	    DB_ERROR *dberr);
