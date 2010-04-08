/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include    	<erglf.h>
#include	<gl.h>
#include	<sl.h>
#include        <cs.h>
#include	<st.h>
#include	<tr.h>
#include	<ex.h>
#include	<sp.h>
#include	<me.h>
#include	<mo.h>

#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<rdfint.h>


/**
**
**  Name: RDFMO.C - Mangement objects for RDF
**
**  Description:
**	This file contains MO management objects for RDF, providing
**  	access to the same data visible in the server shutdown "tombstone"
**
**  Functions:
**  	rdf_mo_init -- call once to export the statistics.
**
**  History:    
**      12-May-1994 (daveb)
**          created
**      20-May-1995 (canor01)
**          added <me.h>
**	23-sep-1996 (canor01)
**	    Move global data definitions to rdfdata.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* forward decls */

MO_GET_METHOD rdf_state_get;
MO_GET_METHOD rdf_avg_get;

/* this needs to be filled in when it's known, before registering */

# define RDF_STATISTICS_ADDR   NULL

GLOBALREF MO_CLASS_DEF rdf_stat_classes[];

# define RDI_SVCB_ADDR	    NULL

GLOBALREF MO_CLASS_DEF rdf_svci_classes[];


/* ---------------------------------------------------------------- */


/*{
** Name:	rdf_mo_init - set up RDF MO classes.
**
** Description:
**	Defines the MO classes.  Should only be called once
**
** Re-entrancy:
**	no, don't!
**
** Inputs:
**	rdf_statistics --   pointer to the RDF_STATISTICS block to export.
**  	rdf_svci    	-- pointer to the RDI_SVCB to export.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	12-May-94 (daveb)
**	    created.
*/

VOID
rdf_mo_init( RDI_SVCB *rdf_svci )
{
    int i;
    RDF_STATISTICS *rdf_statistics = &rdf_svci->rdvstat;

    for( i = 0; rdf_stat_classes[i].classid != RDF_STATISTICS_ADDR; i++ )
	rdf_stat_classes[i].cdata = (PTR) rdf_statistics;

    (void) MOclassdef( MAXI2, rdf_stat_classes );

    for( i = 0; rdf_svci_classes[i].classid != RDI_SVCB_ADDR; i++ )
	rdf_svci_classes[i].cdata = (PTR) rdf_svci;
    (void) MOclassdef( MAXI2, rdf_svci_classes );
}



/*{
**  Name:	rdf_avg_get - get method for float and round up .5
**
**  Description:
**
**	Convert the float at the object location to a character
**	string rounded up.  The offset is treated as a byte offset to
**  	the input object .  They are added together to get the object location.
**	
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or
**	sizeof(i4).
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS 
rdf_avg_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;
    float fval;
    i4 ival;

    fval = *(float *)((char *)object + offset) + 0.5;

    ival = (fval > MAXI4) ? MAXI4 : (i4)fval;
    
    return( MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf ) );
}



STATUS 
rdf_state_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    char buf[ 100 ];
    i4 state = *(i4 *)((char *)object + offset);

    MEfill(sizeof(buf), (u_char)0, buf);

    TRformat(NULL, 0, buf, sizeof(buf), "%v",
	     "RELCACHE,LDBCACHE,RELHASH,QTREEHASH,DEFAULTHASH",
	    state);

    return (MOstrout( MO_VALUE_TRUNCATED, buf, lsbuf, sbuf ));
}
