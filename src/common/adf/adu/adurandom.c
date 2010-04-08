/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <mh.h>
#include    <fp.h>
#if ( defined(any_aix) || defined(axp_osf) ) && !defined(BITSPERBYTE)
/* see module edit history */
#define BITSPERBYTE 8
#endif  /* aix  || axp_osf */
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/* [@#include@]... */

/**
**
**  Name: ADURANDOM.C - Routines related to random functions. 
**
**  Description:
**      This file contains routines related to the random functions.
**
**	This file defines the following externally visible routines:
**
**          adu_random()  - Return the next random integer
**          adu_randomf() - Return the next random float
**          adu_random_range(l,h)  - Return the next random integer in range
**          adu_randomf_range(l,h) - Return the next random integer in range
**          adu_randomf_rangef(lf,hf) - Return the next random float in range
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      15-Jan-1999 (shero03)
**          Initial creation.
**	01-May-2000 (hanch04)
**	    In adu_random and adu_randomf, check to see if the range is
**	    greater than the maximum size of an int.
**      20-Nov-2003 (hanal04) Bug 111324 INGSRV 2607
**          Resolve unnecessary integer overflow errors (E_AD1121).
**      20-Sep-2004 (hweho01)
**          Due to the change in fpcl.h  (#undef BITSPERBYTE),
**          it is necessary to include bzarch.h after fp.h, avoid
**          compiler error "Undeclared identifier BITSPERBYTE" when
**          ULT_VECTOR_MACRO is expanded in adfint.h file on AIX platform.
**      29-Jun-2005 (hweho01)
**          Due to the change in fpcl.h  (#undef BITSPERBYTE),
**          avoid BITSPERBYTE undeclared, include bzarch.h after fp.h
**          for Tru64 platform.	
**	20-Jun-2009 (kschendel) SIR 122138
**	    For the above case, instead of re-including bzarch which
**	    causes redefinition warnings, just re-define bitsperbyte.
**	    On the platforms noted, it's 8.  And, remove this if/when
**	    we stop fpcl from undefining bitsperbyte!
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/*
**  Definition of static variables and forward static functions.
*/
	
/*
[@static_variable_or_function_definition@]...
*/


/*
** Name: adu_random() - ADF function instance to return the next random integer
**
** Description:
**      This function calls MHrand2 to get the next random integer
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Initial creation.
*/

DB_STATUS
adu_random(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *rdv)
{
    i4	    		d;
    DB_DATA_VALUE	exp;
    DB_STATUS		db_stat;

    /* Verify the result is an integer */
    if (rdv->db_datatype != DB_INT_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
       return(db_stat);
    }

    d = MHrand2();

    exp.db_datatype = DB_INT_TYPE;
    exp.db_length = 4;
    exp.db_data = (PTR) &d;
    if ((db_stat = adu_1int_coerce(adf_scb, &exp, rdv)) != E_DB_OK)
        return (db_stat);

    return(E_DB_OK);
}

/*
** Name: adu_random_range() - ADF function instance to return the next random
**				integer in the given range
**
** Description:
**      This function calls MHrand2 to get the next random integer
**	The random number is then put into the requested range by
**	Knuth's suggestion: 0..k-1 -> k*rand()/max
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Initial creation.
**	22-jan-2000 (stephenb)
**	    We must divide before multiplying to prevent integer overflow,
**	    which causes that returned value to never be above 256.
**      20-Nov-2003 (hanal04) Bug 111324 INGSRV 2607
**          Correct check for E_AD1121 to stop legitimate ranges
**          such as (1, 2147483647) from generating errors.
**          Also corrected assignment of result which could contain
**          values outside of the specified range because of OS
**          truncation of the result value.
*/

DB_STATUS
adu_random_range(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    i4	    		e1, e2, d;
    f8			r;
    DB_DATA_VALUE	exp;
    DB_STATUS		db_stat;
    f8			round;			

    /* Verify the result is an integer */
    if (rdv->db_datatype != DB_INT_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
       return (db_stat);
    }

    exp.db_datatype = DB_INT_TYPE;
    exp.db_length = 4;
    exp.db_data = (PTR) &e1;
    if ((db_stat = adu_1int_coerce(adf_scb, dv1, &exp)) != E_DB_OK)
       return (db_stat);

    exp.db_data = (PTR) &e2;
    if ((db_stat = adu_1int_coerce(adf_scb, dv2, &exp)) != E_DB_OK)
        return (db_stat);

    /* Verify that dv1 is less than dv2 */	
    /* Fixme Need to verify that the numbers are different */
    if (e1 > e2)
    {
    	i4 temp = e1;
	e2 = e1;
	e1 = temp;
    }

    if ( (e1 < 0 && e2 > 0) && (e2 - e1 < 0) )
    {
       db_stat = adu_error(adf_scb, E_AD1121_INTOVF_ERROR, 0);
       return (db_stat);
    }

    r = (f8)(MHrand2() / (f8)MH_MAXRAND2INT);
    round = (((e2 - e1) * r) + e1);
    d = round < 0 ? round-0.5 : round+0.5;
    exp.db_data = (PTR) &d;
    db_stat = adu_1int_coerce(adf_scb, &exp, rdv);
    
    return(db_stat);
}


/*
** Name: adu_randomf() - ADF function instance to return the next random float
**
** Description:
**      This function calls FPrand to get the next random float
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Initial creation.
*/

DB_STATUS
adu_randomf(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *rdv)
{
    f8			d;
    DB_DATA_VALUE       flt;
    DB_STATUS		db_stat;

    /* Verify the result is a float */
    if (rdv->db_datatype != DB_FLT_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
       return(db_stat);
    }
    
    d = FPrand();
			        
    flt.db_datatype = DB_FLT_TYPE;
    flt.db_length = 8;
    flt.db_data = (PTR) &d;
    db_stat = adu_1flt_coerce(adf_scb, &flt, rdv);

    return(db_stat);
}

/*
** Name: adu_randomf_range() - ADF function instance to return the next random 
**				integer in the given range
**
** Description:
**      This function calls FPrand to get the next random float
**	The random number is then put into the requested range by
**	multiplying the random by the hi range, then adding the low range value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Initial creation.
**	22-jan-2000 (stephenb)
**	    We can't assume both values in the range are positive, alter 
**	    calculation to cope with negative numbers, also add 1 before
**	    multiplying to allow the max value to be generated.
**      20-Nov-2003 (hanal04) Bug 111324 INGSRV 2607
**          Correct check for E_AD1121 to stop legitimate ranges
**          such as (1, 2147483647) from generating errors.
**          Also corrected assignment of result which could contain
**          values outside of the specified range because of OS
**          truncation of the result value.
*/

DB_STATUS
adu_randomf_range(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    i4	    		e1, e2, d;
    DB_DATA_VALUE   	exp;
    DB_STATUS       	db_stat;
    f8			round;

    /* Verify the result is an integer */
    if (rdv->db_datatype != DB_INT_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
       return(db_stat);
    }

    exp.db_datatype = DB_INT_TYPE;
    exp.db_length = 4;
    exp.db_data = (PTR) &e1;
    if ((db_stat = adu_1int_coerce(adf_scb, dv1, &exp)) != E_DB_OK)
       return (db_stat);

    exp.db_data = (PTR) &e2;
    if ((db_stat = adu_1int_coerce(adf_scb, dv2, &exp)) != E_DB_OK)
        return (db_stat);

    /* Verify that dv1 is less than dv2 */	
    /* Fixme Need to verify that the numbers are different */
    if (e1 > e2)
    {
    	i4 temp = e1;
	e2 = e1;
	e1 = temp;
    }
       
    if ( (e1 < 0 && e2 > 0) && (e2 - e1 < 0) )
    {
       db_stat = adu_error(adf_scb, E_AD1121_INTOVF_ERROR, 0);
       return (db_stat);
    }

    round = (((e2 - e1) * FPrand()) + e1);
    d = round < 0 ? round-0.5 : round+0.5;

    exp.db_data = (PTR) &d;
    db_stat = adu_1int_coerce(adf_scb, &exp, rdv);

    return(db_stat);
}

/*
** Name: adu_randomf_rangef() - ADF function instance to return the next random 
**				float in the given range
**
** Description:
**      This function calls FPrand to get the next random float
**	The random number is then put into the requested range by
**	multiplying the random by the hi range, then adding the low range value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Initial creation.
**      11-Jan-2002 (zhahu02)
**          Updated to ensure the random number to be in the right range. 
**          (b102325/ingsrv1462)
*/

DB_STATUS
adu_randomf_rangef(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    f8	    		e1, e2, d;
    DB_DATA_VALUE   	flt;
    DB_STATUS       	db_stat;

    /* Verify the result is a float */
    if (rdv->db_datatype != DB_FLT_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
       return(db_stat);
    }

    flt.db_datatype = DB_FLT_TYPE;
    flt.db_length = 8;
    flt.db_data = (PTR) &e1;
    if ((db_stat = adu_1flt_coerce(adf_scb, dv1, &flt)) != E_DB_OK)
       return (db_stat);

    flt.db_data = (PTR) &e2;
    if ((db_stat = adu_1flt_coerce(adf_scb, dv2, &flt)) != E_DB_OK)
        return (db_stat);

    /* Verify that dv1 is less than dv2 */	
    /* Fixme Need to verify that the numbers are different */
    if (e1 > e2)
    {
    	f8 temp = e1;
	e2 = e1;
	e1 = temp;
    }
       
    d = ((e2-e1)* FPrand()) + e1;

    flt.db_data = (PTR) &d;
    db_stat = adu_1flt_coerce(adf_scb, &flt, rdv);

    return(db_stat);
}


/*
[@function_definition@]...
*/
