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
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPU_COMPARE      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPUCOMPARE.C - compare two values with the same type
**
**  Description:
**      routine to compare two values of the same type
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opu_dcompare	- compare two values with different types
**
** Description:
**      This routine will compare two values of different types.
**
** Inputs:
**      global                          ptr to global state variable
**      vp1                             ptr to first value
**      vp2                             ptr to second value
**      datatype                        ptr to datatype info on values
**
** Outputs:
**	Returns:
**	    -1 if vp1 < vp2
**          0  if vp1 = vp2
**          1  if vp1 > vp2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-jun-86 (seputis)
**          initial creation to fix constant compare problem in opvctrees.c
[@history_line@]...
*/
i4
opu_dcompare(
	OPS_STATE          *global,
	DB_DATA_VALUE	   *vp1,
	DB_DATA_VALUE	   *vp2)
{
    i4			adc_cmp_result;
    DB_STATUS		comparestatus;

    comparestatus = adc_compare(global->ops_adfcb, vp1, vp2,
	&adc_cmp_result);
# ifdef E_OP078C_ADC_COMPARE
    if (comparestatus != E_DB_OK)
	opx_verror( comparestatus, E_OP078C_ADC_COMPARE, 
	    global->ops_adfcb->adf_errcb.ad_errcode);
# endif
    return (adc_cmp_result);
}

/*{
** Name: opu_compare	- compare two values with the same type
**
** Description:
**      This routine will compare two values of the same type.
**
** Inputs:
**      global                          ptr to global state variable
**      vp1                             ptr to first value
**      vp2                             ptr to second value
**      datatype                        ptr to datatype info on values
**
** Outputs:
**	Returns:
**	    -1 if vp1 < vp2
**          0  if vp1 = vp2
**          1  if vp1 > vp2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
i4
opu_compare(
	OPS_STATE          *global,
	PTR                vp1,
	PTR                vp2,
	DB_DATA_VALUE      *datatype)
{
    i4			adc_cmp_result;
    DB_DATA_VALUE	vpdv1;
    DB_DATA_VALUE	vpdv2;
    DB_STATUS		comparestatus;

    STRUCT_ASSIGN_MACRO(*datatype, vpdv1);
    STRUCT_ASSIGN_MACRO(vpdv1, vpdv2);
    vpdv2.db_data = vp2;
    vpdv1.db_data = vp1;
    comparestatus = adc_compare(global->ops_adfcb, &vpdv1, &vpdv2,
	&adc_cmp_result);
# ifdef E_OP078C_ADC_COMPARE
    if (comparestatus != E_DB_OK)
	opx_verror( comparestatus, E_OP078C_ADC_COMPARE, 
	    global->ops_adfcb->adf_errcb.ad_errcode);
# endif
    return (adc_cmp_result);
}
