/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTTRDUP.C - Functions for duplicating query trees
**
**  Description:
**      This file contains the functions for duplicating query trees
**	in PSF memory.
**
**          pst_trdup - Duplicate a query tree
**
**
**  History:    $Log-for RCS$
**      22-jul-86 (jeff)
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
[@history_template@]...
**/

/*{
** Name: pst_trdup	- Duplicate a query tree
**
** Description:
**      This function duplicates a query tree in PSF memory.  It is like
**	the function pst_treedup() in pstnorml.c, except that the latter
**	duplicates the tree into QSF memory.
**
** Inputs:
**      stream                          Pointer to ULM stream
**	tree				Pointer to tree to duplicate
**	dup				Place to put pointer to copy
**	err_blk				Filled in if an error happens
**
** Outputs:
**      dup                             Filled in with pointer to copy
**	err_blk				Filled in if an error happened
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
**      22-jul-86 (jeff)
**          written
**	19-feb-87 (daved)
**	    don't assume all data is contiguous to constant nodes.
**	23-jan-90 (andre)
**	    Fix two bugs:
**	    - we don't ALWAYS have to copy sizeof(PST_QNODE) bytes (most of the
**	      time there are fewer;
**	    - If there is datavalue to be copied, db_data pointer of the copy
**	      must be set to point at the memory allocated to hold datavalue,
**	      whereas until now we have been copying source datavalue unto
**	      itself.
**	    Make use of pst_node_size() to determine size of variant part of the
**	    node and the size of data, if any.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  19-aug-91 (andre)
**	    changed casting from (PTR) to (char *) so that pointer arithmetic 
**	    works on all platforms.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
pst_trdup(
	PTR                stream,
	PST_QNODE	   *tree,
	PST_QNODE	   **dup,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk)
{
    DB_STATUS		    status;
    PST_QNODE		    *newtree;
    i4			    offset;
    i4			    symsize, datasize, size;
    ULM_RCB		    ulm_cb;
    i4		    err_code;
    FUNC_EXTERN DB_STATUS   ulm_alloc();

    if (tree == (PST_QNODE *) NULL)
    {
	*dup = (PST_QNODE *) NULL;
	return (E_DB_OK);
    }

    /*
    **  Determine the size of the variant part of the node and the size of data
    */
    status = pst_node_size(&tree->pst_sym, &symsize, &datasize, err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /*
    ** allocate enough space (data value included), i.e. space for the constant
    ** part of the node (common to all node types), variable part, and data
    ** if any
    */
    ulm_cb.ulm_psize = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) +
		       symsize + datasize;
    
    ulm_cb.ulm_facility = DB_PSF_ID;
    ulm_cb.ulm_streamid_p = &stream;

    ulm_cb.ulm_memleft = memleft;
    status = ulm_palloc(&ulm_cb);
    if (status != E_DB_OK)
    {
	if (ulm_cb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, err_blk, 0);
	}
	else
	    (VOID) psf_error(E_PS0A05_BADMEMREQ, ulm_cb.ulm_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }
    *dup = newtree = (PST_QNODE *) ulm_cb.ulm_pptr;

    size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + symsize;
    
    /* Copy the old node (except for datvalue) to the new node */
    MEcopy((char *) tree, size, (char *) newtree);

    /*
    ** Make the pst_dataval field point to the new data value that was just
    ** copied.
    */
    if (datasize != 0)
    {
	/*
	** note that of the allocated memory,
	** (sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + symsize) bytes
	** have been used to copy the node, the remaining datasize bytes
	** will be used to contain the datavalue
	*/
	newtree->pst_sym.pst_dataval.db_data = (char *) newtree + size;
	
	MEcopy((char *) tree->pst_sym.pst_dataval.db_data,
	    datasize, 
	    (char*) newtree->pst_sym.pst_dataval.db_data);
    }

    /*
    ** Now duplicate the left and right sub-trees.
    */
    status = pst_trdup(stream, tree->pst_left, &newtree->pst_left, memleft,
	err_blk);
    if (status != E_DB_OK)
	return (status);

    status = pst_trdup(stream, tree->pst_right, &newtree->pst_right, memleft,
	err_blk);
    if (status != E_DB_OK)
	return (status);

    /* duplicate a union */
    if (tree->pst_sym.pst_type == PST_ROOT || 
	tree->pst_sym.pst_type == PST_AGHEAD ||
	tree->pst_sym.pst_type == PST_SUBSEL)
    {
	status = pst_trdup(stream, 
		tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next, 
		&newtree->pst_sym.pst_value.pst_s_root.pst_union.pst_next,
		memleft, err_blk);
	if (status != E_DB_OK)
	    return (status);
    }	
    return (E_DB_OK);
}
