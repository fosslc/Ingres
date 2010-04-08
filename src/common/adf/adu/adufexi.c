/*
** Copyright (c) 2004 Ingres Corporation 
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/*  [@#include@]... */

/**
**
**  Name: ADUFEXI.C - Implementation of all FEXI functions
**
**  Description:
**	ADF top layer to all FEXI (Function EXternally Implemented)
**	operations.
**
**	This file defines:
**	    adu_restab()    	-	resolve_table function
**	    adu_allocated_pages -	Total allocated pages in a table.
**	    adu_overflow_pages  -	Total overflow pages in a table.
**          adu_peripheral      -       Make the fexi call for OME users.
**          adu_last_id		-	Last identifier in this session
**
**  Function prototypes defined in ADUINT.H file.  adu_peripheral() is
**  prototyped in adp.h -- it is currently referenced only in scf.
**  
**  History:
**	31-jan-90 (jrb)
**	    Created.
**	30-November-1992 (rmuth)
**	    File Extend Project: Add adu_allocated_pages and
**	    adu_overflow_pages.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**       6-Jul-1993 (fred)
**          Added adu_peripheral() routine.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**      9-sep-2009 (stephenb)
**          Add adu_last_id function to support last_identity()
**/


/*{
** Name: adu_restab 	- resolve_table function top-layer
**
** Description:
**	This routine sets up the interface for the call-back function in
**	the FEXI table.  If no entry is found for resolve_table, we return
**	an error.
**	
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Pointer to the data value to be filled:
**		.db_datatype		Must be a character type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		Length of input string.
**		.db_data    		Actual string value.
**	rdv				Result returned here.
**		.db_datatype		Will always be VARCHAR.
**		.db_prec		Ignored (should be zero).
**		.db_length		Will always be 65.
**		.db_data		Points to space to put result in.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    rdv
**		.db_data		This space filled in with result.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD2100_NULL_RESTAB_PTR	Callback to function not set
**	    E_AD2101_RESTAB_FCN_ERROR	Error returned from callback function
**					
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	31-jan-90 (jrb)
**	    Created.
**      10-jul-92 (stevet)
**          Changed this function to two input parameters.
*/
DB_STATUS
adu_restab(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *rdv)
{
    i4		    i;
    DB_STATUS	    db_stat;
    DB_STATUS	    (*func)();

    if ((func = Adf_globs->Adi_fexi[ADI_00RESTAB_FEXI].adi_fcn_fexi) == NULL)
    	return(adu_error(adf_scb, E_AD2100_NULL_RESTAB_PTR, 0));

    /* call the function in the FEXI table */
    db_stat = (*func)(dv1->db_data, dv2->db_data, rdv->db_data);

    if (db_stat != E_DB_OK)
    	return(adu_error(adf_scb, E_AD2101_RESTAB_FCN_ERROR, 0));

    return(E_DB_OK);
}

/*{
** Name: adu_allocated_pages 	- Return total allocated pages in a table.
**
** Description:
**	This routine sets up the interface for the call-back function in
**	the FEXI table. This call-back fucntion will call DMF to find
**	out the total allocated pages in the table.
**	If no entry is found for adu_allocated_pages, we return
**	an error.
**	
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		will always be 4.
**		.db_data    		Actual string value.
**      dv2				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		Will always be 4.
**		.db_data    		Actual string value.
**	rdv				Result returned here.
**		.db_datatype		Will always be integer
**		.db_prec		Ignored (should be zero).
**		.db_length		will always be 4.
**		.db_data		Points to space to put result in.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    rdv
**		.db_data		This space filled in with result.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK				Completed successfully.
**	    E_AD2102_NULL_ALLOCATED_PTR		Callback to function not set
**	    E_AD2103_ALLOCATED_FCN_ERR		Error returned from callback 
**						function
**					
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	30-November-1992 (rmuth)
**	    Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_allocated_pages(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS	    db_stat;
    DB_STATUS	    (*func)();
    i4		    reltid;
    i4		    reltidx;
    i8		    i8temp;

    if ((func = Adf_globs->Adi_fexi[ADI_03ALLOCATED_FEXI].adi_fcn_fexi) == NULL)
    	return(adu_error(adf_scb, E_AD2102_NULL_ALLOCATED_PTR, 0));

    /*
    ** setup reltid
    */
    switch ( dv1->db_length )
    {
	case 1:
	  reltid = I1_CHECK_MACRO(*(i1 *)dv1->db_data );
	  break;
	case 2:
	  reltid = *(i2 *)dv1->db_data;
	  break;
	case 4:
	  reltid = *(i4 *)dv1->db_data;
	  break;
	case 8:
	  i8temp = *(i8 *)dv1->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "allocated_pages reltid overflow"));

	  reltid = (i4) i8temp ;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2103_ALLOCATED_FCN_ERR, 0));
    }
    /*
    ** Setup reltidx
    */
    switch ( dv2->db_length )
    {
	case 1:
	  reltidx = I1_CHECK_MACRO(*(i1 *)dv2->db_data );
	  break;
	case 2:
	  reltidx = *(i2 *)dv2->db_data;
	  break;
	case 4:
	  reltidx = *(i4 *)dv2->db_data;
	  break;
	case 8:
	  i8temp = *(i8 *)dv2->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "allocated_pages reltidx overflow"));

	  reltidx = (i4) i8temp ;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2103_ALLOCATED_FCN_ERR, 0));
    }

    /* call the function in the FEXI table */
    db_stat = (*func)( &reltid, 
		       &reltidx, 
		       ADI_03ALLOCATED_FEXI, 
		       NULL,
		      (i4 *)rdv->db_data);

    if (db_stat != E_DB_OK)
        return(adu_error(adf_scb, E_AD2103_ALLOCATED_FCN_ERR, 0));

    return(E_DB_OK);
}

/*{
** Name: adu_overflow_pages 	- Return total overflow pages in a table.
**
** Description:
**	This routine sets up the interface for the call-back function in
**	the FEXI table. This call-back fucntion will call DMF to find
**	out the total overflow pages in the table.
**	If no entry is found for adu_allocated_pages, we return
**	an error.
**	
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		will always be 4.
**		.db_data    		Actual string value.
**      dv2				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		Will always be 4.
**		.db_data    		Actual string value.
**	rdv				Result returned here.
**		.db_datatype		Will always be integer
**		.db_prec		Ignored (should be zero).
**		.db_length		will always be 4.
**		.db_data		Points to space to put result in.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    rdv
**		.db_data		This space filled in with result.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK				Completed successfully.
**	    E_AD2104_NULL_OVERFLOW_PTR		Callback to function not set
**	    E_AD2105_OVERFLOW_FCN_ERR		Error returned from callback 
**						function
**					
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	30-November-1992 (rmuth)
**	    Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_overflow_pages(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS	    db_stat;
    DB_STATUS	    (*func)();
    i4		    reltid;
    i4		    reltidx;
    i8		    i8temp;

    if ((func = Adf_globs->Adi_fexi[ADI_04OVERFLOW_FEXI].adi_fcn_fexi) == NULL)
    	return(adu_error(adf_scb, E_AD2104_NULL_OVERFLOW_PTR, 0));

    /*
    ** setup reltid
    */
    switch ( dv1->db_length )
    {
	case 1:
	  reltid = I1_CHECK_MACRO(*(i1 *)dv1->db_data );
	  break;
	case 2:
	  reltid = *(i2 *)dv1->db_data;
	  break;
	case 4:
	  reltid = *(i4 *)dv1->db_data;
	  break;
	case 8:
	  i8temp = *(i8 *)dv1->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "overflow_pages reltid overflow"));

	  reltid = (i4) i8temp ;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2105_OVERFLOW_FCN_ERR, 0));
    }
    /*
    ** Setup reltidx
    */
    switch ( dv2->db_length )
    {
	case 1:
	  reltidx = I1_CHECK_MACRO(*(i1 *)dv2->db_data );
	  break;
	case 2:
	  reltidx = *(i2 *)dv2->db_data;
	  break;
	case 4:
	  reltidx = *(i4 *)dv2->db_data;
	  break;
	case 8:
	  i8temp = *(i8 *)dv2->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "overflow_pages reltidx overflow"));

	  reltidx = (i4) i8temp ;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2105_OVERFLOW_FCN_ERR, 0));
    }

    /* 
    ** call the function in the FEXI table 
    */
    db_stat = (*func)( &reltid, 
		       &reltidx, 
		       ADI_04OVERFLOW_FEXI,
		       NULL,
		      (i4 *)rdv->db_data);

    if (db_stat != E_DB_OK)
    	return(adu_error(adf_scb, E_AD2105_OVERFLOW_FCN_ERR, 0));

    return(E_DB_OK);
}

/*{
** Name:	adu_peripheral	- Invoke peripheral service fcn
**
** Description:
**	This routine is used to invoke the peripheral service
**	function.  This function is used to manipulate segments of
**	large objects.  It is provided by its host server (frontend or
**	DBMS). 
**
** Re-entrancy:
**	Yes.
**
** Inputs:
**	op_code         Operation desired.  See adp.h for details.
**	pop_cb          Pointer to controlling CB.
**
** Outputs:
**	Depends on operation.   See description in adp.h.
**
** Returns:
**	DB_STATUS.  Errors vary depending upon host.
**
** Exceptions: (if any)
**	None.
**
** Side Effects:
**	None.
**
** History:
**       6-Jul-1993 (fred)
**          Created.
*/
DB_STATUS
adu_peripheral(i4  	    op_code,
	       ADP_POP_CB   *pop_cb)
{
    DB_STATUS 	    status;

    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
    {
	status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (op_code, pop_cb);
    }
    else
    {
	status = E_DB_ERROR;
    }
    return(status);
}


/*{
** Name: adu_tableinfo 	- Return table information.
**
** Description:
**	This routine sets up the interface for the call-back function in
**	the FEXI table. This call-back fucntion will call DMF to find
**	out table information from dmf.
**	
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Pointer to the data value to be filled:
**		.db_datatype		Must be a character type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		length of tableinfo request string.
**		.db_data    		Actual string value.
**      dv2				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		Will always be 4.
**		.db_data    		Actual string value.
**      dv3				Pointer to the data value to be filled:
**		.db_datatype		Must be an integer type.
**		.db_prec		Ignored (should be zero).
**		.db_length  		Will always be 4.
**		.db_data    		Actual string value.
**	rdv				Result returned here.
**		.db_datatype		Will always be integer
**		.db_prec		Ignored (should be zero).
**		.db_length		will always be 4.
**		.db_data		Points to space to put result in.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    rdv
**		.db_data		This space filled in with result.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK				Completed successfully.
**	    E_AD2106_TABLEINFO_FCN_ERR		Error returned from callback 
**						function
**					
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-apr-2001 (stial01)
**	    Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_tableinfo(
ADF_CB		    *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE       *dv3,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS	    db_stat;
    DB_STATUS	    (*func)();
    i4		    reltid;
    i4		    reltidx;
    i8		    i8temp;
    char	    *dv1_text = (char *)((DB_TEXT_STRING *)dv1->db_data)->db_t_text;
    i4		    dv1_len = ((DB_TEXT_STRING *)dv1->db_data)->db_t_count;
    char	    request[100];
    i4		    return_count;
    DB_DATA_VALUE   tmp_dv;
    i4		    j;

    if ((func = Adf_globs->Adi_fexi[ADI_03ALLOCATED_FEXI].adi_fcn_fexi) == NULL)
    	return(adu_error(adf_scb, E_AD2106_TABLEINFO_FCN_ERR, 0));

    /* set up tableinfo request */
    if (dv1_len + 1 > sizeof(request) - 1)
	return(adu_error(adf_scb, E_AD2106_TABLEINFO_FCN_ERR, 0));

    /* dv1 is varchar, copy to request buffer and null terminate */
    j = 0;
    while (j < dv1_len && *dv1_text != 0)
    {
	CMtolower(dv1_text + j, &request[j]);
	j++;
    }
    request[j] = 0;

    /*
    ** setup reltid
    */
    switch ( dv2->db_length )
    {
	case 1:
	  reltid = I1_CHECK_MACRO(*(i1 *)dv2->db_data );
	  break;
	case 2:
	  reltid = *(i2 *)dv2->db_data;
	  break;
	case 4:
	  reltid = *(i4 *)dv2->db_data;
	  break;
	case 8:
	  i8temp = *(i8 *)dv2->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "tableinfo reltid overflow"));

	  reltid = (i4) i8temp ;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2106_TABLEINFO_FCN_ERR, 0));
    }
    /*
    ** Setup reltidx
    */
    switch ( dv3->db_length )
    {
	case 1:
	  reltidx = I1_CHECK_MACRO(*(i1 *)dv3->db_data );
	  break;
	case 2:
	  reltidx = *(i2 *)dv3->db_data;
	  break;
	case 4:
	  reltidx = *(i4 *)dv3->db_data;
	  break;
	case 8:
	  i8temp  = *(i8 *)dv3->db_data;
	  /* limit to i4 values */
	  if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "tableinfo reltidx overflow"));

	  reltidx = (i4) i8temp;
	  break;
	default:
    	    return(adu_error(adf_scb, E_AD2106_TABLEINFO_FCN_ERR, 0));
    }

    /* call the function in the FEXI table */
    db_stat = (*func)( &reltid, 
		       &reltidx, 
		       ADI_06TABLEINFO_FEXI, 
		       request,
		       &return_count);

    if (db_stat != E_DB_OK)
        return(adu_error(adf_scb, E_AD2106_TABLEINFO_FCN_ERR, 0));

    /* dmf_tbl_info returns an integer, convert it to char */
    if (DB_SUCCESS_MACRO(db_stat))
    {
	tmp_dv.db_datatype = DB_INT_TYPE;
	tmp_dv.db_length   = sizeof(return_count);
	tmp_dv.db_data     = (PTR)&return_count;
	db_stat = adu_ascii(adf_scb, &tmp_dv, rdv);
    }

    return(db_stat);
}

/*
** Name: adu_last_id- Get most recently generated identifier in this session
**
** Description:
**	Returns the most recently generated number for any identity column in 
**	this session.
**
** Inputs:
**	adf_scb session control block
**
** Outputs:
**	rdv	data value with returned sequence number
**	
** History:
**	4-sep-2009 (stephenb)
**	    Created
*/
DB_STATUS
adu_last_id(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *rdv)
{
    DB_STATUS	    (*func)();
    DB_STATUS	    db_stat;
    i8		    id;
 
    if ((func = Adf_globs->Adi_fexi[ADI_07LASTID_FEXI].adi_fcn_fexi) == NULL)
    	return(adu_error(adf_scb, E_AD2107_LASTID_FCN_ERR, 0));
    
    /* call the function */
    db_stat = (*func)(&id);
    
 
 
    if (db_stat == E_DB_OK)
    {
	/* got the value */
	if (rdv->db_datatype < 0)
	{
	    /* nullable*/
	    ADF_CLRNULL_MACRO(rdv);
	}
	*((i8 *)rdv->db_data) = id;
	
	return (E_DB_OK);
    }
    else if (db_stat == E_DB_WARN)
    {
	 /* no data value for this session */
	 *((i8 *)rdv->db_data) = (i8)0;
	 if (rdv->db_datatype < 0)
	 {
	     /* nullable */
	     ADF_SETNULL_MACRO(rdv);
	 }
	 
	 return(E_DB_OK);
    }
    else
	return(adu_error(adf_scb, E_AD2107_LASTID_FCN_ERR, 0));
}

