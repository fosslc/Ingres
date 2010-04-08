/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tm.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSQCURHSH.C - Functions for hashing and finding cursor control blocks.
**
**  Description:
**      This file contains the functions for hashing cursor ids and for finding 
**      and creating cursor control blocks and cursor columns.
**
**	    psq_crhsh	- Hash a cursor id into a hash bucket number
**          psq_crfind - Given a session control block and a cursor id, find
**			    the corresponding cursor control block.
**	    psq_crffind - Given the FE cursor id, find the corresponding cursor
**			    control block.
**	    psq_clhsh	- Hash a cursor column name w/i a cursor
**	    psq_clent	- Enter a cursor column description into the cursor
**			    column hash table
**	    psq_ccol  - Find a column in a cursor
**	    psq_crent  - Enter a cursor control block into the hash table
**			    of cursors
**	    psq_curcreate - Create a cursor control block
**	    psq_victim	- Kill off an unused cursor control block
**	    psq_delcursor - Delete a cursor control block
**	    psq_cltab  - Allocate the column hash table for a cursor
**	    psq_open_rep_cursor - mark a cursor block describing a repeat cursor
**				  as opened
**	    psq_crclose - Close a cursor
**
**  History:
**      06-jan-85 (jeff)    
**          written
**	03-mar-88 (stec)
**	    Cleanup.
**	27-oct-88 (stec)
**	    Initialize psc_iupdmap.
**	11-may-89 (neil)
**	    Allow lookup with a zero-id cursor for Dynamic SQL.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	10-mar-93 (andre)
**	    initialize psc_expmap
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool > 2Gig.
[@history_template@]...
**/


/*{
** Name: psq_crhsh	- Find hash bucket for cursor id
**
** Description:
**      This function finds the hash bucket number for a cursor id.
**	Note that (1) to allow shareable cursors from the front-end (using
**	the FE cursor id), (2) to allow the lookup of the a cursor through
**	the name alone, we MUST only hash the cursor name and not the
**	integers.
**
** Inputs:
**      cursid                          Pointer to the cursor id
**
** Outputs:
**      None
**	Returns:
**	    Hash bucket number
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-may-86 (jeff)
**          written
**	24-nov-86 (daved)
**	    just hash on the cursor name. This way we can find
**	    the FE or DBMS version of the cursor id.
**	11-may-89 (neil)
**	    added comment about why we must only use the cursor name.
*/
u_i4
psq_crhsh(
	DB_CURSOR_ID       *cursid)
{
    register u_char *p = (u_char *) cursid->db_cur_name, *lim;
    register u_i4  bucket = 0;

    for (lim = p + sizeof(cursid->db_cur_name); p < lim; p++)
    {
	bucket += (u_i4)(*p);
    }

    return (bucket % (u_i4) PSS_CURTABSIZE);
}

/*{
** Name: psq_crfind	- Find a cursor control block.
**
** Description:
**      Given a session control block and the cursor id of a cursor within 
**      that control block, this function will find the corresponding cursor 
**      control block.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**      cursor_id                       Pointer to the cursor id to look up
**					If the internal integers of this id
**					are zero then then the cursor is
**					looked up by name only.
**      cursor                          Place to put pointer to cursor control
**					block
**	err_blk				Pointer to error block
**
** Outputs:
**      cursor                          Filled in with pointer to cursor control
**					block if found, NULL if not found
**	err_blk				Errors will be added to error block
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in psq_cb.psq_error chain
**					of error blocks.
**					2226 - Ambiguous cursor name.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					psq_cb.psq_error chain of error blocks.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-86 (jeff)
**          written
**	11-may-89 (neil)
**	    look up cursor by name alone.
*/
DB_STATUS
psq_crfind(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursor_id,
	PSC_CURBLK         **cursor,
	DB_ERROR	   *err_blk)
{
    register PSC_CURBLK *current;
    PSC_CURBLK		*rescsr;		/* Result cursor */
    u_i4		bucket;
    i4			dynamic = FALSE;	/* Dynamic cursor */
    i4			found_dynamic;		/* # of dynamic ones found */
    i4		err_code;

    /* Get the hash bucket */
    bucket = psq_crhsh(cursor_id);

    if (cursor_id->db_cursor_id[0] == 0 && cursor_id->db_cursor_id[1] == 0)
    {
	dynamic = TRUE;
	found_dynamic = 0;
    }

    /* Look up cursor, point rescsr at right one, or NULL if not found */
    for (rescsr = NULL, current = sess_cb->pss_curstab.pss_curque[bucket];
	current != (PSC_CURBLK *) NULL;	current = current->psc_next)
    {
	if (!dynamic)			/* Stop on first one */
	{
	    if (
		current->psc_blkid.db_cursor_id[0] == cursor_id->db_cursor_id[0]
	     && current->psc_blkid.db_cursor_id[1] == cursor_id->db_cursor_id[1]
	     && !MEcmp(current->psc_blkid.db_cur_name, cursor_id->db_cur_name, 
		       sizeof (current->psc_blkid.db_cur_name))
	       )
	    {
		rescsr = current;
		break;
	    }
	}
	else				/* By name and exhaust all open csrs */
	{
	    if (   current->psc_open == TRUE
	        && MEcmp(current->psc_blkid.db_cur_name,
		  	 cursor_id->db_cur_name, 
			 sizeof(current->psc_blkid.db_cur_name)) == 0
	       )
	    {
		rescsr = current;
		if (++found_dynamic > 1)	/* User error */
		    break;
	    }
	}
    }
    *cursor = rescsr;
    sess_cb->pss_crsr = rescsr;

    if (dynamic && found_dynamic > 1)
    {
	(VOID) psf_error(2226L, 0L, PSF_USERERR, &err_code, err_blk, 1,
			 psf_trmwhite(DB_MAXNAME, cursor_id->db_cur_name),
			 cursor_id->db_cur_name);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: psq_crffind	- Find a cursor control block using the FE id.
**
** Description:
**      Given a session control block and the cursor id of a cursor within 
**      that control block, this function will find the corresponding cursor 
**      control block.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**      cursor_id                       Pointer to the cursor id to look up
**      cursor                          Place to put pointer to cursor control
**					block
**	err_blk				Pointer to error block
**
** Outputs:
**      cursor                          Filled in with pointer to cursor control
**					block if found, NULL if not found
**	err_blk				Errors will be added to error block
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					psq_cb.psq_error chain of error blocks.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-nov-86 (daved)
**          written
*/
DB_STATUS
psq_crffind(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursor_id,
	PSC_CURBLK         **cursor,
	DB_ERROR	   *err_blk)
{
    register PSC_CURBLK *current;
    u_i4		bucket;

    /* Get the hash bucket */
    bucket = psq_crhsh(cursor_id);

    /*
    ** Look for cursor, leaving current pointing to right one,
    ** or NULL if not found.
    */
    for (current = sess_cb->pss_curstab.pss_curque[bucket];
	current != (PSC_CURBLK *) NULL;	current = current->psc_next)
    {
	if 
	(
	    current->psc_curid.db_cursor_id[0] == cursor_id->db_cursor_id[0]
	    && 
	    current->psc_curid.db_cursor_id[1] == cursor_id->db_cursor_id[1]
	    &&
	    !MEcmp(current->psc_curid.db_cur_name, cursor_id->db_cur_name,
		sizeof(current->psc_curid.db_cur_name))
	)
	    break;
    }
    *cursor = current;
    sess_cb->pss_crsr = current;

    return (E_DB_OK);
}

/*{
** Name: psq_clhsh	- Hash a cursor column name
**
** Description:
**      This function returns the hash bucket number for a cursor column name. 
**
** Inputs:
**      tabsize                         Number of entries in the hash table
**      name                            The column name
**
** Outputs:
**      NONE
**	Returns:
**	    The hash bucket number
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-may-86 (jeff)
**          written
*/
u_i4
psq_clhsh(
	i4                tabsize,
	DB_ATT_NAME        *name)
{
    register u_char     *p = (u_char *) name, *lim;
    register u_i4	bucket = 0;

    for (lim = p + sizeof(DB_ATT_NAME); p < lim; p++)
    {
	bucket += (u_i4)(*p);
    }

    return (bucket % (u_i4)tabsize);
}

/*{
** Name: psq_clent	- Enter a column description in the cursor hash table
**
** Description:
**      This function enters a column description in the hash table of columns
**	in a cursor control block.
**
** Inputs:
**	colno				Column number
**	colname				Name of the column
**      type                            Datatype id of the column
**	length				Data length of the column
**	precision			Data precision of the column
**	cursor				Pointer to the cursor control block
**	memleft				Pointer to memory left for this session
**	err_blk				Filled in if an error happens
**
** Outputs:
**      cursor                          Hash table gets the column
**	memleft				Decremented by amount allocated
**      err_blk                         Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	20-may-86 (jeff)
**          written
*/
DB_STATUS
psq_clent(
	i4		   colno,
	DB_ATT_NAME	   *colname,
	DB_DT_ID	   type,
	i4		   length,
	i2		   precision,
	PSC_CURBLK	   *cursor,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    u_i4               bucket;
    ULM_RCB		ulm_cb;
    DB_STATUS		status;
    register PSC_RESCOL	*newcol;
    i4		err_code;

    /* Find the hash bucket */
    bucket = psq_clhsh(cursor->psc_restab.psc_tabsize, colname);
    
    /* Allocate the column description */
    ulm_cb.ulm_facility = DB_PSF_ID;
    ulm_cb.ulm_streamid_p = &cursor->psc_stream;
    ulm_cb.ulm_psize = sizeof(PSC_RESCOL);
    ulm_cb.ulm_memleft = memleft;
    status = ulm_palloc(&ulm_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (ulm_cb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A02_BADALLOC, ulm_cb.ulm_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	}
	if (ulm_cb.ulm_error.err_code == E_UL0004_CORRUPT)
	    status = E_DB_FATAL;
        return (status);
    }
    newcol = (PSC_RESCOL *) ulm_cb.ulm_pptr;

    /* Fill in the cursor control block */
    newcol->psc_type = type;
    newcol->psc_len = length;
    newcol->psc_prec = precision;
    newcol->psc_attid.db_att_id = colno;
    STRUCT_ASSIGN_MACRO(*colname, newcol->psc_attname);

    /* Link the new column into the hash table */
    newcol->psc_colnext = cursor->psc_restab.psc_coltab[bucket];
    cursor->psc_restab.psc_coltab[bucket] = newcol;
	
    return (status);
}

/*{
** Name: psq_ccol	- Find a column in a cursor.
**
** Description:
**      This function takes a column name, and looks it up in a cursor control
**	block.  If it can find the column, it returns a pointer to the column
**	description.  If it can't find the column, it returns NULL.
**
** Inputs:
**      curblk                          Pointer to the cursor control block
**      colname                         The name to look up
**
** Outputs:
**      NONE
**	Returns:
**	    Pointer to the column description, or NULL if not found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-may-86 (jeff)
**          written
*/
PSC_RESCOL *
psq_ccol(
	PSC_CURBLK         *curblk,
	DB_ATT_NAME        *colname)
{
    u_i4	bucket;
    PSC_RESCOL	*descr;

    bucket = psq_clhsh(curblk->psc_restab.psc_tabsize, colname);
    descr = curblk->psc_restab.psc_coltab[bucket];
    while (descr != (PSC_RESCOL *) NULL)
    {
	if (!MEcmp((char *) &descr->psc_attname, (char *) colname,
	    sizeof(DB_ATT_NAME)))
	{
	    break;
	}

	descr = descr->psc_colnext;
    }

    return (descr);
}

/*{
** Name: psq_crent	- Enter a cursor control block in the hash table
**
** Description:
**      This function enters a cursor control block in the cursor hash table
**
** Inputs:
**      curblk                          Pointer to the cursor control block
**	curtab				Pointer to the hash table
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
**	19-may-86 (jeff)
**          written
*/
VOID
psq_crent(
	PSC_CURBLK         *curblk,
	PSS_CURSTAB        *curtab)
{
    u_i4   bucket;

    bucket = psq_crhsh(&curblk->psc_blkid);
    curblk->psc_next = curtab->pss_curque[bucket];
    curblk->psc_prev = (PSC_CURBLK *) NULL;
    if (curtab->pss_curque[bucket] != (PSC_CURBLK *) NULL)
    {
	curtab->pss_curque[bucket]->psc_prev = curblk;
    }
    curtab->pss_curque[bucket] = curblk;
}

/*{
** Name: psq_curcreate	- Create a cursor control block
**
** Description:
**      This function creates a cursor control block, enters it in the
**	hash table, and puts the cursor id in it.  If it is a repeat cursor,
**	and there are a lot of cursors already defined, it will look for
**	a repeat cursor that's not currently in use and try to delete it.
**	If it doesn't find one of these, it will go ahead and create the
**	cursor control block anyway; the penalty for having too many cursors
**	open at once is bad performance.
**
**	This function will always try to use the pre-defined cursor in the
**	session control block before allocating one from the memory pool.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	cursid				Pointer to the cursor id
**	qmode				The query mode
**	curblk				Place to put pointer to new cursor
**					control block
**	err_blk				Filled in if an error happens
**
** Outputs:
**      curblk                          Filled in with pointer to cursor control
**					block
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Will open a memory stream.  Can allocate memory.
**
** History:
**	18-may-86 (jeff)
**          written
**	19-sep-86 (daved)
**	    initialize the  update map with MEfill instead of maxcol
**	    number of BTclear calls.
**	27-oct-88 (stec)
**	    Initialize psc_iupdmap.
**	10-mar-93 (andre)
**	    initialize psc_expmap
**	05-apr-93 (andre)
**	    initialize psc_flags and psc_tbl_descr_queue
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for psc_owner which has changed type to PTR.
**	17-mar-94 (andre)
**	    for now we decided to not keep around repeat cursor blocks once the
**	    cursor has been closed - this eliminates need for calling
**	    psq_victim()
**    31-mar-94 (andre)
**          fix for bug 56739:
**          it is a well-known fact that TMnow(), despite what the CL spec
**	    would have you believe, does not generate unique timestamps on
**	    very fast boxes.  This causes a problem if two different cursors
**	    are opened at the same time since they both get assigned the same
**	    timestamp.  To fix that problem, instead of using TMnow(), we will
**	    use the mechanism borrowed from repeat queries to generate the
**	    cursor BE id (i.e. we will use sess_cb->pss_psessid and
**	    sess_cb->pss_crsid
*/
DB_STATUS
psq_crcreate(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursid,
	i4		   qmode,
	PSC_CURBLK	   **curblk,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status;
    ULM_RCB		ulm_cb;
    i4		err_code;
    register PSC_CURBLK *cursor;
    extern PSF_SERVBLK	*Psf_srvblk;

    ulm_cb.ulm_facility = DB_PSF_ID;
    ulm_cb.ulm_poolid = Psf_srvblk->psf_poolid;
    ulm_cb.ulm_blocksize = 0;
    ulm_cb.ulm_memleft = &sess_cb->pss_memleft;
    ulm_cb.ulm_streamid_p = &sess_cb->pss_cstream;
    /* Allocate a shared stream */
    ulm_cb.ulm_flags = ULM_SHARED_STREAM;
    status = ulm_openstream(&ulm_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (ulm_cb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A02_BADALLOC, ulm_cb.ulm_error.err_code,
		    PSF_INTERR, &err_code, err_blk, 0);
	}
	if (ulm_cb.ulm_error.err_code == E_UL0004_CORRUPT)
	    status = E_DB_FATAL;

	return (status);
    }

    /* Look for whether first cursor can be used */
    if (!sess_cb->pss_firstcursor.psc_used)
    {
	*curblk = &sess_cb->pss_firstcursor;
    }
    else
    {
	/* First cursor was used, try allocating one. */

#ifdef  REP_CURS_BLOCKS_MUST_PERSIST
	/*
	** If this is a repeat cursor, see whether we are using too many
	** cursors.  If so, try to find a victim and kill it off.  A "victim"
	** is a repeat cursor that's not currently open.  If we can't kill
	** off a victim, we'll proceed as normal.
	*/
	if ((sess_cb->pss_defqry == PSQ_DEFCURS) &&
	    (sess_cb->pss_numcursors > PSS_MAXCURS))
	{
	    status = psq_victim(sess_cb, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
#endif

	/* Now allocate a cursor */
	ulm_cb.ulm_psize = sizeof(PSC_CURBLK);
	status = ulm_palloc(&ulm_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    if (ulm_cb.ulm_error.err_code == E_UL0005_NOMEM)
	    {
		(VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		    &err_code, err_blk, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0A02_BADALLOC, ulm_cb.ulm_error.err_code,
			PSF_INTERR, &err_code, err_blk, 0);
	    }
	    if (ulm_cb.ulm_error.err_code == E_UL0004_CORRUPT)
		status = E_DB_FATAL;

	    return (status);
	}

	/* Let the caller know where the cursor is */
	*curblk = (PSC_CURBLK *) ulm_cb.ulm_pptr;
    }

    /* Now we have a pointer to the cursor control block.
    ** Let's remember where it is for the sake of future productions,
    ** and also so we can get rid of it if we get an error later.
    */
    cursor = *curblk;
    sess_cb->pss_crsr = cursor;

    /*
    ** Fill in the cursor control block.
    */
    cursor->psc_next = (PSC_CURBLK*) NULL;
    cursor->psc_prev = (PSC_CURBLK*) NULL;
    cursor->psc_length = sizeof(PSC_CURBLK);
    cursor->psc_type = PSC_CRBID;
    cursor->psc_owner = (PTR)DB_PSF_ID;
    cursor->psc_ascii_id = PSCCUR_ID;
    cursor->psc_used = TRUE;
    (VOID) MEfill(sizeof(DB_TAB_NAME), (u_char) ' ',
	(PTR) &cursor->psc_tabnm);
    /* save FE cursid */
    STRUCT_ASSIGN_MACRO(*cursid, cursor->psc_curid);
    STRUCT_ASSIGN_MACRO(*cursid, cursor->psc_blkid);

    /*
    ** instead of using TMnow() use mechanism borrowed from repeat queries
    ** (where, according to comments, it was introduced for exactly the same
    ** reason: TMnow() not generating unique timestamps)
    */
    cursor->psc_blkid.db_cursor_id[0] = (i4) sess_cb->pss_psessid;
    cursor->psc_blkid.db_cursor_id[1] = (i4) ++sess_cb->pss_crsid;

    /*
    ** Repeat cursors start out as not open.  Non-repeat cursors start out
    ** as open.
    */
    if (sess_cb->pss_defqry == PSQ_DEFCURS)
    {
	cursor->psc_open = FALSE;
	cursor->psc_repeat = TRUE;
    }
    else
    {
	cursor->psc_open = TRUE;
	cursor->psc_repeat = FALSE;
    }

    cursor->psc_stream = *ulm_cb.ulm_streamid_p;

    /*
    ** Clear out "for update" column map.
    */
    (VOID) MEfill(sizeof (PSC_COLMAP), (u_char) 0, (PTR) &cursor->psc_updmap);

    cursor->psc_delall	    = FALSE;
    cursor->psc_forupd	    = FALSE;
    cursor->psc_readonly    = FALSE;
    cursor->psc_rdonly	    = FALSE;
    cursor->psc_restab.psc_tabsize = 0;
    cursor->psc_restab.psc_coltab = (PSC_RESCOL **) NULL;
    cursor->psc_integ	= (PST_QNODE*) NULL;
    cursor->psc_flags = 0;
    QUinit(&cursor->psc_tbl_descr_queue);

    /*
    ** Clear out the internal "for update" column map.
    */
    (VOID) MEfill(sizeof (PSC_COLMAP), (u_char) 0, (PTR) &cursor->psc_iupdmap);

    /*
    ** initialize map of attributes which are not simple attributes (i.e. they
    ** are attributes of a view on which a cursor is being declared and they are
    ** based on an expression as opposed to a simple column of the underlying
    ** base table
    */
    (VOID) MEfill(sizeof (PSC_COLMAP), (u_char) 0, (PTR) &cursor->psc_expmap);

    /*
    ** Enter this into the hash table of cursor control blocks.
    */
    (VOID) psq_crent(cursor, &sess_cb->pss_curstab);

    /*
    ** Increase cursor count.
    */
    sess_cb->pss_numcursors++;

    return (status);
}

/*{
** Name: psq_victim	- Try to find a victim and kill it off
**
** Description:
**      Repeat cursor control blocks tend to stay around a long time.  If too
**	many of them are created at once, it could take a lot of memory.  This
**	function tries to find a victim, which is a repeat cursor that's not
**	currently open.  If it finds one, it will kill it off.  Currently, this
**	function will run through the hash table, and kill off the first victim
**	it finds.  This is sort of random, but an LRU algorithm would be better.
**
**	If this function can't find a victim, it will return without doing
**	anything.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	err_blk				Filled in if an error happens
**
** Outputs:
**      err_blk                         Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can deallocate a cursor, and the memory that goes with it
**
** History:
**	20-may-86 (jeff)
**          written
*/
DB_STATUS
psq_victim(
	PSS_SESBLK         *sess_cb,
	DB_ERROR	   *err_blk)
{
    register PSC_CURBLK	**bucket;
    register i4	i;
    register PSC_CURBLK *cursor;
    DB_STATUS		status = E_DB_OK;

    /* VICTIMS SHOULD BE CHOSEN A BETTER WAY (LRU) */
    /* Look through the entire table */
    bucket = sess_cb->pss_curstab.pss_curque;
    for (i = 0; i < PSS_CURTABSIZE; i++)
    {
	cursor = *bucket;

	/* Look through all the cursors in this bucket */
	while (cursor != (PSC_CURBLK *) NULL)
	{
	    /* A non-open repeat cursor is a victim */
	    if (!cursor->psc_open && cursor->psc_repeat)
	    {
		/* Try deleting the cursor control block */
		status = psq_delcursor(cursor, &sess_cb->pss_curstab,
		    &sess_cb->pss_memleft, err_blk);
		return (status);
	    }

	    cursor = cursor->psc_next;
	}

	bucket++;
    }

    /* Couldn't find a victim, which is OK */
    return (status);
}

/*{
** Name: psq_delcursor	- Delete a cursor control block
**
** Description:
**      This function deletes a cursor control block, taking it out of
**	the hash table, and deallocating the memory used by it.
**
** Inputs:
**      curblk                          Pointer to the cursor control block
**	curtab				Hash table of cursor control blocks
**	memleft				Pointer to "memory left" pointer
**	err_blk				Filled in if an error happens
**
** Outputs:
**      curtab                          curblk is removed
**	memleft				Adjusted upwards for newly-freed memory
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failue
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Deallocates memory
**
** History:
**	20-may-86 (jeff)
**          written
*/
DB_STATUS
psq_delcursor(
	PSC_CURBLK         *curblk,
	PSS_CURSTAB	   *curtab,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    u_i4	bucket;
    ULM_RCB	ulm_cb;
    DB_STATUS	status;
    i4	err_code;

    /* Clip the control block out of the table */
    if (curblk->psc_prev != (PSC_CURBLK *) NULL)
    {
	curblk->psc_prev->psc_next = curblk->psc_next;
    }
    else
    {
	bucket = psq_crhsh(&curblk->psc_blkid);
	curtab->pss_curque[bucket] = curblk->psc_next;
    }

    if (curblk->psc_next != (PSC_CURBLK *) NULL)
	curblk->psc_next->psc_prev = curblk->psc_prev;

    /*
    ** Mark the control block unused.  This is necessary for the
    ** pre-allocated cursor control block.
    */
    curblk->psc_used = FALSE;
    curblk->psc_open = FALSE;

    /*
    ** Deallocate the dynamic memory used by this control block.
    */
    ulm_cb.ulm_facility = DB_PSF_ID;
    ulm_cb.ulm_streamid_p = &curblk->psc_stream;
    ulm_cb.ulm_memleft = memleft;
    status = ulm_closestream(&ulm_cb);
    if (DB_FAILURE_MACRO(status))
    {
        (VOID) psf_error(E_PS0A05_BADMEMREQ, ulm_cb.ulm_error.err_code,
	    PSF_INTERR, &err_code, err_blk, 0);
        if (ulm_cb.ulm_error.err_code == E_UL0004_CORRUPT)
	    status = E_DB_FATAL;
	return (status);
    }

    return (status);
}

/*{
** Name: psq_cltab	- Allocate the cursor column hash table
**
** Description:
**      This function allocates the hash table of columns for a cursor
**	control block.  It tries to keep the number of entries in the table
**	at least twice as big as the number of columns in the cursor.  That
**	way we can avoid collisions.
**
** Inputs:
**      cursor                          Pointer to the cursor control block
**	numcols				Number of columns in cursor
**	memleft				Pointer to memory left for this session
**	err_blk				Filled in if an error happens
**
** Outputs:
**      cursor                          Hash table allocated
**	memleft				Decremented by the size of the table
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	20-may-86 (jeff)
**          written
*/
DB_STATUS
psq_cltab(
	PSC_CURBLK         *cursor,
	i4		   numcols,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    i4                  tabsize;
    ULM_RCB		ulm_cb;
    DB_STATUS		status;
    PSC_RESCOL		**hashtab;
    register PSC_RESCOL	**p;
    register i4	i;
    i4		err_code;

    /*
    ** Figure out the size of the hash table.  It should be a prime number
    ** at least twice as big as the number of columns in the cursor.
    */
    if (numcols <= 5)
	tabsize = 11;
    else if (numcols <= 11)
	tabsize = 23;
    else if (numcols <= 23)
	tabsize = 47;
    else if (numcols <= 48)
	tabsize = 97;
    else if (numcols <= 98)
	tabsize = 197;
    else
	tabsize = 257;

    /* Allocate the hash table */
    ulm_cb.ulm_facility = DB_PSF_ID;
    ulm_cb.ulm_streamid_p = &cursor->psc_stream;
    ulm_cb.ulm_psize = tabsize * sizeof(PSC_RESCOL *);
    ulm_cb.ulm_memleft = memleft;
    status = ulm_palloc(&ulm_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (ulm_cb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	(VOID) psf_error(E_PS0A02_BADALLOC, ulm_cb.ulm_error.err_code,
		    PSF_INTERR, &err_code, err_blk, 0);
        if (ulm_cb.ulm_error.err_code == E_UL0004_CORRUPT)
	    status = E_DB_FATAL;
	return (status);
    }
    
    /* Clear the hash table */
    p = hashtab = (PSC_RESCOL **) ulm_cb.ulm_pptr;
    for (i = 0; i < tabsize; i++)
    {
	*p++ = (PSC_RESCOL *) NULL;
    }

    /* Link the hash table into the cursor */
    cursor->psc_restab.psc_coltab = hashtab;

    /* Remember how big it is */
    cursor->psc_restab.psc_tabsize = tabsize;

    return (status);
}

/*{
** Name: psq_curclose		- Close a cursor.
**
** Description:
**      This function closes a cursor, cleaning up any resources used by it.
**	It will deallocate the memory used by a cursor, unless it is a repeat
**	cursor, in which case it will leave the cursor control block there.
**
** Inputs:
**      cursor                          Pointer to the cursor control block
**	curstab				Pointer to the hash table of cursors
**	memleft				Pointer to memory left
**	err_blk				Filled in if an error happens
**
** Outputs:
**      err_blk                         Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_SEVERE			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can deallocate memory
**
** History:
**      12-aug-86 (jeff)
**          written
**	17-mar-94 (andre)
**	    for the time being we will not bother keeping around repeat cursor
**	    blocks once the cursor has been closed
*/
DB_STATUS
psq_crclose(
	PSC_CURBLK         *cursor,
	PSS_CURSTAB	   *curstab,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    DB_STATUS           status = E_DB_OK;

    /* Mark cursor closed */
    cursor->psc_open = FALSE;

#ifdef  REP_CURS_BLOCKS_MUST_PERSIST
    /* Delete cursor control block, unless it is a repeat cursor */
    if (!cursor->psc_repeat)
#endif
    {
	status = psq_delcursor(cursor, curstab, memleft, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    return (status);
}

/*{
** Name: psq_open_rep_cursor - mark a repeat cursor as opened
**
** Description:
**      This function will find a cursor block describing a repeat cursor and
**	will mark it opened; if the block cannot be found, it will return
**	an error which will indicate to SCF that cursor definition needs to be
**	reparsed.  If the cursor is already marked opened, we will complain
**
** Inputs:
**      psq_cb			PSF request CB
**	    psq_cursid		BE cursor id
**	sess_cb			PSF session CB
**
** Outputs:
**	psq_cb
**	    psq_error		filled in if an error occurred
**		E_PS0401_CUR_NOT_FOUND	curosr block could not be found
**		2210L			cursor is already opened
** Returns:
**	    E_DB_OK			Success
**	    E_DB_SEVERE			Non-catastrophic failure
** Exceptions:
**	none
**
** Side Effects:
**	    will mark a cursor block to indicate that the cursor has been opened
**
** History:
**	17-mar-94 (andre)
**	    written
**	17-mar-94 (andre)
**	    for the time being we will only support repeated read-only cursors
**	    which enables us to cheat and not require that all fields of
**	    cursor block be properly initialzied: we will no longer insist that
**	    a cursor block for a given repeat cursor must exist
*/
DB_STATUS
psq_open_rep_cursor(
		PSQ_CB	    *psq_cb,
		PSS_SESBLK  *sess_cb)
{
    PSC_CURBLK	    *curblk;
    DB_STATUS	    status;

    status = psq_crfind(sess_cb, &psq_cb->psq_cursid, &curblk,
	&psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
        return(status);

    if (curblk == (PSC_CURBLK *) NULL)
    {
#ifdef	REP_CURS_BLOCKS_MUST_PERSIST
	/*
	** cursor block could not be found - most likely it means that it has
	** been reused while the cursor was closed.  This is not a user error -
	** we must tell SCF to request that FE sends down the query defining the
	** cursor
	*/
	psq_cb->psq_error.err_code = E_PS0401_CUR_NOT_FOUND;
	return(E_DB_ERROR);
#else
	/* tell psq_crcreate() that this is a repeat cursor */
	sess_cb->pss_defqry = PSQ_DEFCURS;

	status = psq_crcreate(sess_cb, &psq_cb->psq_cursid, PSQ_DEFCURS,
	    &curblk, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/*
	** we were called with the backend cursor id, but psq_crcreate() didn't
	** know better and stored it in psc_curid and then proceeded to
	** overwrite psc_blkid.db_cursor_id with a new timestamp.  Unless we
	** undo it here, this will result in PS0401 when it's time to process
	** CLOSE CURSOR
	*/
	STRUCT_ASSIGN_MACRO(curblk->psc_curid, curblk->psc_blkid);
#endif
    }

    /*
    ** we found (or created) our cursor block; if it says that the cursor is
    ** already opened, we will complain; otherwise we will mark it opened
    */
    if (curblk->psc_open)
    {
        i4	    err_code;
	i4	    lineno = 1;
	
        (VOID) psf_error(2210L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 2,
	    sizeof(lineno), &lineno,	/*
					** we are lying about line number
					** because we were not given a query
					** text to parse
					*/
	    psf_trmwhite(DB_MAXNAME, psq_cb->psq_cursid.db_cur_name),
	    psq_cb->psq_cursid.db_cur_name);

	return(E_DB_ERROR);
	
    }
    else
    {
        curblk->psc_open = TRUE;
    }
    
    return(E_DB_OK);
}
