/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>

#include    <ulf.h>
#include    <gca.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <dmf.h>
#include    <qsf.h>

#include    <qefrcb.h>
#include    <psfparse.h>
#include    <st.h>
#include    <me.h>
#include    <uls.h>

/**
**
**  Name: ULC_DBMS.C - Communication routines for the DBMS Server
**
**  Description:
**      This file contains the code for those routines which format data for
**      interprocess communication between the DBMS and the frontend program.
**
**          ulc_bld_descriptor - Build a descriptor from qtree
**
**
**  History:
**      09-Aug-1986 (fred)
**          Created on Jupiter.
**      09-feb-1987 (fred)
**          Added language capability to ulc_gtquery().
**      13-jul-1987 (fred)
**          Axed all routines and added ulc_bld_descriptor().
**      16-jan-1990 (fredp)
**          Added alignment fixes for GCA_COL_ATT.  Include <me.h> to
**          get MECOPY_CONST_MACRO().
**      01-sep-1992 (rog)
**          Prototype ULF.
**      04-sep-1992 (fred)
**          Added `fake_big_as_small' parameter to ulc_bld_descriptor()
**	    and ulc_att_bld().  This is set by OPC when a trace point
**	    is set to pretend in the descriptor that a long varchar datatype is
**	    a regular varchar.  Useful for testing -- especially before
**	    long varchar works in the FE's.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      17-nov-1993 (rog)
**          Added PTR casts to MEcopy() calls.
**	20-nov-1994 (andyw)
**	    Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	5-apr-1996 (loera01)
**	    In ulc_bld_descriptor(), passed along GCA_COMP_MASK if the
**	    front end has requested variable datatype compression.
**	18-jun-1999 (schte01)
**	    Added char cast macro to avoid unaligned access in axp_osf.
**	20-jul-1999 (schte01)
**	    Added ifdef for axp for char cast change.
**	05-Nov-2001 (devjo01)
**	    Rewrote this to layout the attributes with the GCA_COL_ATT
**	    structures aligned correctly in one pass, then pack array
**	    to GCA's expectations in a second pass, both for efficiency,
**	    and because Tru64 optimizer was converting memcpy's we had put
**	    in to avoid unaligned memory accesses into simple load and stores.
**	    Alternate choice would be to suppress optimization.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
[@history_template@]...
**/

static GCA_COL_ATT	*ulc_att_bld( i4  *modifier, PST_QNODE *target_list,
				      GCA_COL_ATT *sqlatts, i4  *tup_size,
				      i4  fake_big_as_small, i4 att_size );

/*{
** Name: ulc_bld_descriptor	- Build a GCF Descriptor for GCA/F & the FE.
**
** Description:
**	This routine takes a query tree as input and builds a tuple descriptor 
**      for sending to the FE and for use by GCF during the send.  This routine 
**      is called by OPC and PSF (during a describe or prepare).
**
** Inputs:
**      target_list                     Left side of qtree 
**      names                           Boolean -- 0 => no names in the
**					descriptor
**      qsf_mem_id                      Where to get memory from.
**	fake_big_as_small		Should blobs be described as little
**					things.
**
** Outputs:
**      *descrip                        Pointer to descriptor filled in here.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Jul-1987 (fred)
**          Adapted from opcqp.c
**      04-sep-1992 (fred)
**          Added `fake_big_as_small' parameter to ulc_bld_descriptor()
**	    and ulc_att_bld().  This is set by OPC when a trace point
**	    is set to pretend in the descriptor that a long varchar datatype is
**	    a regular varchar.  Useful for testing -- especially before
**	    long varchar works in the FE's.
**	5-apr-1996 (loera01)
**	    Passed along GCA_COMP_MASK if the front end has requested 
**          variable-length datatype compression.
**	05-nov-2001 (devjo01)
**	    Tweak memory required calculation, and squash attribute
*	    array after ulc_att_bld builds aligned vrsion of array.
[@history_template@]...
*/
DB_STATUS
ulc_bld_descriptor( PST_QNODE *target_list, i4  names, QSF_RCB *qsf_rb,
		    GCA_TD_DATA **descrip, i4  fake_big_as_small )
{
    PST_QNODE	    *resdom;
    GCA_TD_DATA	    *da;
    GCA_COL_ATT	    *sqlatt;
    i4		    da_size;
    i4		    da_num;
    i4		    attsize, basesize, cpyamt;
    DB_STATUS	    status;
    char	    *sp, *dp;

    da_num = 0;
    for (   resdom = target_list->pst_left;
	    resdom->pst_sym.pst_type == PST_RESDOM; 
	    resdom = resdom->pst_left
	)
    {
	if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    da_num += 1;
    }

    /* Allow for base portion of attribute structures */
    attsize = basesize = (((GCA_COL_ATT *)0)->gca_attname - (char*)0);
    if (names & GCA_NAMES_MASK)
    {
	/* Allow for a maximum sized name */
	attsize += DB_MAXNAME;
    }
    /* Make sure enough space is allowed for GCA_COL_ATT's to be aligned */
    attsize += sizeof(ALIGN_RESTRICT) - 1;
    attsize &= ~ ( sizeof(ALIGN_RESTRICT) - 1 );
    da_size = sizeof (GCA_TD_DATA) + da_num * attsize;

    qsf_rb->qsf_sz_piece = da_size;
    status = qsf_call(QSO_PALLOC, qsf_rb);
    if (DB_FAILURE_MACRO(status))
	return((DB_STATUS) qsf_rb->qsf_error.err_code);

    da = (GCA_TD_DATA *) qsf_rb->qsf_piece;
    da->gca_result_modifier = 0;
    if (names & GCA_NAMES_MASK)
    {
        da->gca_result_modifier |= GCA_NAMES_MASK;
    }
    if (names & GCA_COMP_MASK)
    {
        da->gca_result_modifier |= GCA_COMP_MASK;
    }
    da->gca_tsize = 0;
    da->gca_l_col_desc = da_num;

    /* First build descriptor array in aligned memory */
    ulc_att_bld(&da->gca_result_modifier,
		    target_list->pst_left,
		    da->gca_col_desc,
		    &da->gca_tsize,
		    fake_big_as_small, attsize);

    if ( basesize != attsize )
    {
	/* Squash aligned array */
	for ( sp = dp = (char*)da->gca_col_desc; 
	      da_num-- > 0;
	      sp = sp + attsize )
	{
	    cpyamt = basesize + ((GCA_COL_ATT *)sp)->gca_l_attname;
	    if ( dp != sp )
		MEcopy( sp, cpyamt, dp );
	    dp += cpyamt;
	}
    }
    *descrip = da;
    return(E_DB_OK);
}

/*{
** Name: ULC_ATT_BLD - Fill in a GCA_COL_ATT array for a GCA_TD_DATA
**
** Description:
**      Opc_sqlatt is the work horse for ulc_bld_descriptor(). As such, it fills 
**      in one GCA_COL_ATT struct for each target_list that is given. If you 
**      noticed, the GCA_COL_ATT is a variable length structure which means 
**      that, since we need an array of GCA_COL_ATTs, pointer arithmetic 
**      is needed to determine where each array element is.
**
** Inputs:
**	modifier			Address of the descriptor being built's
**					gca_result_modifier field.  As input,
**					this field is inspected for the
**					GCA_NAMES_MASK bit.
**
**	target_list			One or more target_lists that describe
**					the data being returned.
**					
**	sqlatts				Pointer to a block of uninitialized
**					memory of sufficient size.
**					
**	tup_size			Pointer to an initialized nat.
**	
**	fake_big_as_small		Should blobs look like small things.
**
**	att_size			byte offset to next sqlatt.
**
** Outputs:
**	*modifier			GCA_LO_MASK is set if the descriptor
**					describes any LARGE OBJECT columns.
**					
**	sqlatts				Will contain the array of GCA_COL_ATTs,
**					one for each target_list.
**					
**	tup_size			Will be increased by the total size of
**					all the target_lists.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-apr-87 (eric)
**          written
**      13-Jul-1987 (fred)
**          Stolen and adapted for ulc usage.
**      16-jan-1990 (fredp)
**          Sqlatts may point to unaligned data, don't use direct/structure
**          assignment on BYTE_ALIGN machines, instead use MECOPY_CONST_MACRO().
**      12-Dec-1989 (fred)
**	    Modified to handle return of large datatypes,
**	    whose length is unknown.
**      04-sep-1992 (fred)
**          Added `fake_big_as_small' parameter to ulc_bld_descriptor()
**	    and ulc_att_bld().  This is set by OPC when a trace point
**	    is set to pretend in the descriptor that a long varchar datatype is
**	    a regular varchar.  Useful for testing -- especially before
**	    long varchar works in the FE's.
**	05-nov-2001 (devjo01)
**	    Simplify, by removing all byte alignment stuff, since caller
**	    will squash attibute array in one pass.  Add att_size param.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
[@history_template@]...
*/
static GCA_COL_ATT *
ulc_att_bld( i4  *modifier, PST_QNODE *target_list, GCA_COL_ATT *sqlatts,
	     i4  *tup_size, i4  fake_big_as_small, i4 att_size )
{
    i4		l_attname;
    DB_DT_ID	type;
    i4		length;

    if (target_list->pst_left != NULL &&
	target_list->pst_left->pst_sym.pst_type == PST_RESDOM
       )
    {
	sqlatts = ulc_att_bld(modifier, target_list->pst_left, 
	    		      sqlatts, tup_size, fake_big_as_small,att_size);
    }
    else
    {
	*tup_size = 0;
    }

    /* fill in it's DB_DATA_VALUE; */
    if ((target_list->pst_sym.pst_type == PST_RESDOM) && 
	(target_list->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
    {
	DB_COPY_DV_TO_ATT( &target_list->pst_sym.pst_dataval, sqlatts );

	if (*modifier & GCA_NAMES_MASK)
	{
	    MEcopy(target_list->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
					    DB_MAXNAME, sqlatts->gca_attname);
	    l_attname = STtrmnwhite(sqlatts->gca_attname,DB_MAXNAME);
	}
	else
	{
	    l_attname = 0;
	}

	sqlatts->gca_l_attname = l_attname;

	type = sqlatts->gca_attdbv.db_datatype;
	length = sqlatts->gca_attdbv.db_length;

	if (fake_big_as_small &&
		(abs(type) == DB_LVCH_TYPE))
	{
	    if (type < 0)
	    {
		type = -(DB_VCH_TYPE);
		length = ADP_TST_VLENGTH + 1;
	    }
	    else
	    {
		type = DB_VCH_TYPE;
		length = ADP_TST_VLENGTH;
	    }
	    *tup_size += length;
	    *modifier |= GCA_LO_MASK;
	}
	else if (length == ADE_LEN_UNKNOWN)
	{
	    /*
	    ** In these cases, the length of the BLOB is not included in the
	    ** tuple length shown in the descriptor.  Instead, the large object
	    ** bit is set in the descriptor's modifier mask.  The association
	    ** partner will then be on the lookout for objects with unspecified
	    ** length.
	    */
	    
	    *modifier |= GCA_LO_MASK;
	}
	else
	{
	    /* add in it's size to the tuple size; */
	    *tup_size += target_list->pst_sym.pst_dataval.db_length;
	}
	sqlatts->gca_attdbv.db_datatype = type;
	sqlatts->gca_attdbv.db_length = length;

	sqlatts = (GCA_COL_ATT *) (((char *) sqlatts) + att_size);
    }

    return(sqlatts);
}
