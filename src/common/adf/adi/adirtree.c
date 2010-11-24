/*
** Copyright (c) 1997 - 1999,  Ingres Corporation
** All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>

/**
**
**  Name: ADIRTREE.C - Common routines to obtain datatype and functions
**  necessary for R-tree operation.
**
**  Description:
**      This file contains all the routines needed to manipulate
**      R-tree indexes.
**
**      The routines defined in this file are:
**	adi_dt_rtree 	    - Initialize a value of a system_maintained datatype
**
**  History:
**      15-apr-1997 (shero03)
**          Created from dm1c_getrtree_blk
**	12-Jun-1997 (shero03)
**	    Set the result data type before calling adi_0calclen
**/


/*{
** Name: adi_dt_rtree - Obtain Data Type and Function Information for R-tree indexes
**
** Description:
**	Given a datatype, obtain the associated nbr type, dimension,
**      hilbert size and the address of:
**	NBR(obj, box)->nbr; HILBERT(nbr)->binary(); NbrONbr(nbr, nbr)->I2
**
** Inputs:
**	*adf_cb	       Pointer to ADF_Control Block
**
**	obj_dt_id      Object data type
**
**	*adi_dt_rtree  Pointer to a ADI_DT_RTREE data structure.
**
** Outputs:
**      adi_dt_rtree  Fills in the ADI_DT_RTREE structure with the addresses to
**                   the routines.
**
** Returns:
**	OK
**
** Side Effects:
**	none
** History:
**      15-apr-1997 (shero03)
**          Created for RTree from dm1c_getrtree_blk
**	12-Jun-1997 (shero03)
**	    Set the result data type before calling adi_0calclen
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
**	16-jul-03 (hayke02)
**	    Call adi_resolve() with varchar_precedence of FALSE. This change
**	    fixes bug 109134.
**      31-aug-2004 (sheco02)
**          X-integrate change 466442 to main.
**      09-mar-2010 (thich01)
**          Change how the NBR function is validated to make us of the GEOM
**          family type.  Hard code the dimesion of an NBR for now, until a
**          3d type comes along and requires a calculation.
**      02-apr-2010 (thich01)
**          Add a search for Union by name instead of just by argument types.
*/
DB_STATUS
adi_dt_rtree(
            ADF_CB	  *adf_scb,
	    DB_DT_ID	  obj_dt_id,
	    ADI_DT_RTREE  *rtree_blk)
{

    ADI_RSLV_BLK	adi_rslv_blk;
    ADI_OP_ID		nbr_op_id;
    ADI_OP_ID		hilbert_op_id;
    ADI_OP_ID		overlaps_op_id;
    ADI_OP_ID		inside_op_id;
    ADI_OP_ID		perimeter_op_id;
    ADI_OP_ID		union_op_id;
    DB_DT_ID		hilbert_dt_id, family_dt_id;
    DB_DATA_VALUE	dv1;
    DB_DATA_VALUE	dv2;
    DB_STATUS		status;

    /* Initialize the info blk */
    rtree_blk->adi_hilbertsize = 0;
    rtree_blk->adi_dimension = 0;
    rtree_blk->adi_obj_dtid = (DB_DT_ID)abs(obj_dt_id);
    rtree_blk->adi_range_dtid = DB_NODT;
    rtree_blk->adi_nbr_dtid= DB_NODT;
    rtree_blk->adi_nbr = NULL;
    rtree_blk->adi_hilbert = NULL;
    rtree_blk->adi_nbr_overlaps = NULL;
    rtree_blk->adi_nbr_inside = NULL;
    rtree_blk->adi_nbr_union = NULL;
    rtree_blk->adi_nbr_perimeter = NULL;

    /*
    ** Find the NBR(obj, range)-> nbr function
    */

    status = adi_opid(adf_scb, "nbr", &nbr_op_id);
    if (DB_FAILURE_MACRO(status))
	return(status);

    adi_rslv_blk.adi_op_id = nbr_op_id;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0] = (DB_DT_ID)abs(obj_dt_id);
    adi_rslv_blk.adi_dt[1] = DB_ALL_TYPE;

    /*
     * The function expects DB_GEOM_TYPE but all spatial types are valid.
     *Therefore, check the family type to ensure it's a spatial type.
     */ 
    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    family_dt_id = adi_dtfamily_retrieve(adi_rslv_blk.adi_dt[0]);
    if ((DB_FAILURE_MACRO(status)) ||
	((adi_rslv_blk.adi_fidesc->adi_dt[0] != adi_rslv_blk.adi_dt[0]) &&
        (family_dt_id != DB_GEOM_TYPE)))
	return(status);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_nbr);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    rtree_blk->adi_range_dtid = adi_rslv_blk.adi_fidesc->adi_dt[1];
    rtree_blk->adi_nbr_dtid = adi_rslv_blk.adi_fidesc->adi_dtresult;
    rtree_blk->adi_nbr_fiid = adi_rslv_blk.adi_fidesc->adi_finstid;

    /*
    ** Find the hilbert(nbr)-> function
    */

    status = adi_opid(adf_scb, "hilbert", &hilbert_op_id);
    if (DB_FAILURE_MACRO(status))
	return(status);

    adi_rslv_blk.adi_op_id = hilbert_op_id;
    adi_rslv_blk.adi_num_dts = 1;
    adi_rslv_blk.adi_dt[0] = abs(rtree_blk->adi_nbr_dtid);

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* If the data type requires coercion, return an error */
    if (adi_rslv_blk.adi_fidesc->adi_dt[0] != adi_rslv_blk.adi_dt[0])
	return(E_DB_ERROR);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_hilbert);

    /* Obtain the length of the hilbert */
    dv1.db_datatype = DB_BYTE_TYPE;	/* set the result data type */
    status = adi_0calclen( adf_scb, &adi_rslv_blk.adi_fidesc->adi_lenspec,
			0, (DB_DATA_VALUE **)NULL, &dv1);
    rtree_blk->adi_hilbertsize = dv1.db_length;
    /* This will need to determine 2d, 3d, 4d when applicable. */
    rtree_blk->adi_dimension = 2;
    rtree_blk->adi_hilbert_fiid = adi_rslv_blk.adi_fidesc->adi_finstid;

    /* Obtain the length of the range */
    dv1.db_length = 0;
    dv1.db_datatype = rtree_blk->adi_range_dtid;
    dv2.db_datatype = dv1.db_datatype;
    status = adc_lenchk( adf_scb, TRUE, &dv1, &dv2);
    if (dv2.db_length == sizeof(f8)*rtree_blk->adi_dimension*2)
    	rtree_blk->adi_range_type = 'F';
    else
    	rtree_blk->adi_range_type = 'I';

    /*
    ** Find the overlaps(nbr, nbr)-> 0,1 function
    */

    status = adi_opid(adf_scb, "OVERLAPS", &overlaps_op_id);
    if (DB_FAILURE_MACRO(status))
	return(status);

    adi_rslv_blk.adi_op_id = overlaps_op_id;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0] = rtree_blk->adi_nbr_dtid;
    adi_rslv_blk.adi_dt[1] = rtree_blk->adi_nbr_dtid;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_nbr_overlaps);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /*
    ** Find the inside(nbr, nbr)-> 0,1 function
    */

    status = adi_opid(adf_scb, "INSIDE", &inside_op_id);
    if (DB_FAILURE_MACRO(status))
	return(status);

    adi_rslv_blk.adi_op_id = inside_op_id;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0] = rtree_blk->adi_nbr_dtid;
    adi_rslv_blk.adi_dt[1] = rtree_blk->adi_nbr_dtid;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_nbr_inside);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /*
    ** Find the Union(nbr, nbr)-> 0,1 function
    */

    status = adi_opid(adf_scb, "UNION", &union_op_id);
    if (DB_FAILURE_MACRO(status))
	return(status);

    adi_rslv_blk.adi_op_id = union_op_id;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0] = rtree_blk->adi_nbr_dtid;
    adi_rslv_blk.adi_dt[1] = rtree_blk->adi_nbr_dtid;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_nbr_union);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /*
    ** Find the perimeter(nbr)-> float function
    */

    status = adi_opid(adf_scb, "PERIMETER", &perimeter_op_id);
    if (DB_FAILURE_MACRO(status))
        return(E_DB_OK);        /* perimeter is optional */    

    adi_rslv_blk.adi_op_id = perimeter_op_id;
    adi_rslv_blk.adi_num_dts = 1;
    adi_rslv_blk.adi_dt[0] = rtree_blk->adi_nbr_dtid;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = adi_function(adf_scb,
                          adi_rslv_blk.adi_fidesc, &rtree_blk->adi_nbr_perimeter);
    if (DB_FAILURE_MACRO(status))
	return(status);

    return(status);
}
