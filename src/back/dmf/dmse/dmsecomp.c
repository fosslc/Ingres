/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <dmf.h>
#include    <cs.h>
#include    <dm.h>

/**
**
**  Name: DMSECOMP.C - Sort comparison routine
**
**  Description:
**      This routine is used to compare columns by the sort routines.
**
**          dmse_compare - Compare two records.
**
**
**  History:
**      12-jan-1986 (Derek)
**          Created.
**	8-jul-1992 (bryanp)
**	    Prototyping DMF. THis file is obsolete and unused.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: dmse_compare	- Compare two records.
**
** Description:
**      Compare two records using sorted attribute description.
**
** Inputs:
**      atts                            Pointer to DB_CMP_LIST
**	count				Count of attributes in list.
**      record1                         Pointer to first record.
**      record2                         Pointer to second record.
**
** Outputs:
**      None
**	Returns:
**	    < 0	-   record1 < record2
**	    0   -   record1 == record2
**	    > 0 -   record1 > record2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-jan-1987 (Derek)
**          Created.
**	16-dec-04 (inkdo01)
**	    Init. cmp_collID's.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
[@history_template@]...
*/
i4
dmse_compare(
DB_CMP_LIST         *att,
i4                  count,
PTR                 record1,
PTR		    record2)
{
    DB_CMP_LIST	    *a = att;
    i4	    c = count;
    ADF_CB	    adf_cb;
    DB_DATA_VALUE   adc_dv1;
    DB_DATA_VALUE   adc_dv2;
    i4	    compare;

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);

    adf_cb.adf_errcb.ad_ebuflen = 0;
    adf_cb.adf_errcb.ad_errmsgp = 0;
    adf_cb.adf_maxstring = DB_MAXSTRING;

    while (--c >= 0)
    {
        adc_dv1.db_datatype = a->cmp_type;
        adc_dv1.db_length = a->cmp_length;
        adc_dv1.db_prec = a->cmp_precision;
        adc_dv1.db_data = &record1[a->cmp_offset];
        adc_dv1.db_collID = a->cmp_collID;
        adc_dv2.db_datatype = a->cmp_type;
        adc_dv2.db_length = a->cmp_length;
        adc_dv2.db_prec = a->cmp_precision;
        adc_dv2.db_data = &record2[a->cmp_offset];
        adc_dv2.db_collID = a->cmp_collID;
        adc_compare(&adf_cb, &adc_dv1, &adc_dv2, &compare);
	if (compare)
	    return (a->cmp_direction ? -compare : compare);
	a++;
    }

    return (0);
}
