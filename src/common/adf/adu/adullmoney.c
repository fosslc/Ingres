/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adumoney.h>

/**
**  Name:   ADULLMONEY.C -  support ADF external interface for money datatype.
**
**  Description:
**	  Low level money routines -- these routines for the most part support
**	the external interface to ADF for money.  Some routines however can be
**	called indirectly throught a pointer to a function to perform a specific
**	action on the "money" datatype.
**
**	This file defines:
**
**          adu_6mny_cmp - compare two money data values.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:	
**	11-apr-86 (ericj)
**	    Taken from 4.0/02 code and modified for Jupiter.
**	03-feb-87 (thurston)
**	    Upgraded the adu_6mny_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for money.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Name: adu_6mny_cmp() - compare two money data values.
**
**  Inputs:
**	adf_scb				Pointer to ADF session control block.
**	dv1  				Pointer to first money argument.
**      dv2  				Pointer to second money argument.
**
**  Outputs:
**	cmp				Ptr to i4  to hold result, as follows:
**					    -1  if  dv1 <  dv2
**					     1  if  dv1 >  dv2
**					     0  if  dv1 == dv2
**
**	Returns:
**	    E_DB_OK
**
**  History:
**	11-apr-86 (ericj) - modified for Jupiter; returns i4  instead of i4 and
**		and removed the ifdef BYTE_ALIGN code as the Jupiter spec 
**		requires that it will already have been aligned.
**	03-feb-87 (thurston)
**	    Upgraded the adu_6mny_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for money.
*/

/*ARGSUSED*/
DB_STATUS
adu_6mny_cmp(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
i4		*cmp)
{
    AD_MONEYNTRNL	*m1 = (AD_MONEYNTRNL *) dv1->db_data;
    AD_MONEYNTRNL	*m2 = (AD_MONEYNTRNL *) dv2->db_data;


    if (m1->mny_cents > m2->mny_cents)
        *cmp = 1;
    else if (m1->mny_cents < m2->mny_cents)
        *cmp = -1;
    else
        *cmp = 0;

    return (E_DB_OK);
}
