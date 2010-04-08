/*
**Copyright (c) 2005 Ingres Corporation
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
#define        OPN_URN      TRUE
#include    <ex.h> 
#include    <mh.h>
#include    <opxlint.h>
 
 
/**
**
**  Name: OPNURN.C	- implement simple urn model
**
**  Description:
**	Routine to calculate the number of "n" empty urns [not] touched 
**	when randomly assigning "m" balls. This is similar to "balls and
**	cells", but without the non-replacement constraint.
**
**  History:    
**      6-oct-05 (inkdo01)
**	    Written.
**      26-sep-2007 (huazh01)
**          Add opn_uhandler(). 
[@history_line@]...
**/

/*
**  Defines of other constants.
*/

/* 
** Name: opn_uhandler 
** 
** Description: 
**      Exception handler for opn_urn(), which could possibly
**      cause unexpected floating pointing exception when calling 
**      MHpow().
** Inputs:
**      args                            arguments which describe the 
**                                      exception which was generated
**
** Outputs:
**	Returns:
**          EXDECLARE                  - if exception happened.
**	Exceptions:
**	    none
**
**      History:
**          26-sep-2007 (huazh01)
**             Written.   
*/
static STATUS
opn_uhandler(
	EX_ARGS            *args)
{
    return (EXDECLARE);	  
} 

/*{
** Name: opn_urn	- implement the simple urn modelon
**
** Description:
**	Computes the number of cells (urns) touched when m balls are 
**	assigned to n cells with no concern for overflow (which seems
**	to be the difference between the urn model and the balls and 
**	cells model.
** 
**	Like balls and cells, this model has several disparate uses.
**	The first is in computing the effect on the number of distinct
**	values of a particular column, given some restriction on another
**	column that reduces the table cardinality. The original column
**	cardinality is n and the restricted table cardinality is m. 
**	The resulting number of cells touched represents the number of 
**	distinct values in the reduced set of rows.
**
**	Another use is to determine the number of null inner rows in
**	an outer join that produces m result rows and has n distinct 
**	sets of join column values in the outer rows of the join. The
**	resulting number of cells touched represents the join column
**	values with a match in the inner join source and the cells NOT
**	touched are the null rows.
**
**	The formula is very simple, not requiring iteration to converge
**	like "balls and cells". It is based on the following 
**	probabilities:
**		P(ball is put in urn i) = 1/n
**		P(ball isn't put in urn i) = 1 - 1/n
**		P(none of the m balls is put in urn i) = (1 - 1/n)**m
**		P(at least one ball is put in urn i) = 1 - (1 - 1/n)**m
**
**	So the expected number of cells touched is n * (1 - (1 - 1/n)**m)
**
** Inputs:
**      rballs                          number of "balls" to assign
**      ncells			        original number of cells
**
** Outputs:
**	Returns:
**	    number of cells which were touched
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-oct-05 (inkdo01)
**	    Written.
**      26-sep-2007 (huazh01)
**          Add opn_uhandler() so that OPF can continue the enumeration 
**          process if a floating point exception happened during MHpow().
**          This fixes bug 119197.
[@history_line@]...
*/
OPH_DOMAIN
opn_urn(
	OPO_TUPLES         rballs,
	OPH_DOMAIN	   ncells)
{
    OPH_DOMAIN          touchedcells;
    EX_CONTEXT          excontext; 
 

    if ( EXdeclare(opn_uhandler, &excontext) == EXDECLARE )
    {
       (VOID)EXdelete();
       return ncells; 
    }

    /* 
    ** handle special cases
    */
#ifdef	SUN_FINITE
    if (!finite(rballs) || !finite(ncells))
	return (1.0);
#else
    if (!MHffinite(rballs) || !MHffinite(ncells))
	return (1.0);
#endif

    if (MHisnan(ncells) || ncells <= 0.0)
	return (1.0);

    if (MHisnan(rballs))
	return (ncells/2.0);


    /* Compute it & return. */
    touchedcells = ncells * (1 - MHpow((1 - 1/ncells), rballs));
    (VOID)EXdelete();
    return(touchedcells);

}
