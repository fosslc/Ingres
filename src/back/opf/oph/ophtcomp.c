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
#define             OPH_TCOMPARE        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPHTCOMP.C - compare text histogram datatypes
**
**  Description:
**      Compare two text histogram datatypes together
**
**  History:    
**      14-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: oph_tcompare	- compare text histogram datatype
**
** Description:
**      The text histogram datatypes do not look like text types so 
**      write a special comparison routine to handle this case 
**      FIXME - this routine should be in the ADF  - almost like char type
**      but not quite!
**
** Inputs:
**      hist1ptr                        ptr to first histogram value
**      hist2ptr                        ptr to second histogram value
**      datatype                        ptr to histogram datatype
**
** Outputs:
**	Returns:
**	    0  iff *hist1ptr == *hist2ptr
**          >0 iff *hist1ptr >  *hist2ptr
**          <0 iff *hist1ptr <  *hist2ptr
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-jun-86 (seputis)
**          initial creation from hcmp_text in hinterp.c
[@history_line@]...
*/
i4
oph_tcompare(
	char               *hist1ptr,
	char		   *hist2ptr,
	DB_DATA_VALUE      *datatype)
{
    OPS_DTLENGTH        length;		    /* length of histogram type */

    length = datatype->db_length;
    while (length--)
    {
    	if (*hist1ptr != *hist2ptr)
	    return ((i4) (*hist1ptr - *hist2ptr));
	hist1ptr++;
	hist2ptr++;
    }
    return (0);
}
